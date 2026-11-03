/*
  CORRECTOR DE POSTURA - VERSION FINAL
  ESP32-S3 + MPU6050 + Buzzer pasivo (sonido fuerte)

  CONEXIONES:
    MPU6050 VCC -> 3V3      MPU6050 GND -> GND
    MPU6050 SDA -> GPIO 8   MPU6050 SCL -> GPIO 9
    Buzzer (+)  -> GPIO 4   Buzzer (-)  -> GND

  COMO FUNCIONA:
    - LA PRIMERA VEZ se calibra solo: suenan 3 pitidos, te sientas DERECHO
      y quieto unos segundos. La calibracion se GUARDA en la memoria.
    - Las siguientes veces que lo enciendas ya esta calibrado:
      solo deja el sensor quieto 2 segundos (suena 1 pitido largo = listo).
    - Si te inclinas mas de UMBRAL_GRADOS durante TOLERANCIA_MS -> SUENA EL BUZZER.
    - Para volver a calibrar: mantén presionado el boton BOOT.

  Monitor Serie: 115200 baud.
  Placa: ESP32S3 Dev Module | USB CDC On Boot: Enabled
*/

#include <Wire.h>
#include <Preferences.h>
#include "driver/gpio.h"

// ---------- PINES ----------
#define PIN_SDA     8
#define PIN_SCL     9
#define PIN_BUZZER  4
#define PIN_BOTON   0       // boton BOOT

// ---------- BUZZER ----------
#define FREC_BUZZER  2700   // frecuencia donde tu buzzer suena MAS FUERTE (prueba 2000-4000)
#define MODO_BARRIDO false  // pon true para encontrar la frecuencia mas fuerte

// ---------- AJUSTES ----------
float UMBRAL_GRADOS = 12.0;                  // inclinacion maxima permitida (menos = mas estricto)
unsigned long TOLERANCIA_MS = 3000;          // segundos de mala postura antes de sonar
float MOVIMIENTO_MAX = 30.0;                 // grados/seg: si te mueves rapido no te regaña
unsigned long SENTADO_MAX_MS = 30UL * 60UL * 1000UL;  // recordatorio de levantarte (30 min)

const uint8_t MPU = 0x68;
Preferences prefs;

float gxOff = 0, gyOff = 0, gzOff = 0;
float refPitch = 0, refRoll = 0;
float pitch = 0, roll = 0;
unsigned long tPrev = 0;
unsigned long inicioMala = 0, ultimoPitido = 0, ultimoMovimiento = 0, ultimoPrint = 0;
unsigned long tiempoBueno = 0, tiempoMalo = 0;
bool avisando = false;

// ---------- BUZZER ----------
void sonido(int freq) {
  ledcWriteTone(PIN_BUZZER, freq);   // 0 = silencio
}

void pitar(int freq, int ms) {
  sonido(freq);
  delay(ms);
  sonido(0);
}

void melodiaFeliz() {
  pitar(FREC_BUZZER, 100); delay(80);
  pitar(FREC_BUZZER, 100); delay(80);
  pitar(FREC_BUZZER, 300);
}

void melodiaParate() {
  for (int i = 0; i < 3; i++) { pitar(FREC_BUZZER, 400); delay(250); }
}

void barrido() {
  Serial.println("BARRIDO: escucha cual frecuencia suena MAS FUERTE");
  for (int f = 1000; f <= 5000; f += 250) {
    Serial.printf("Frecuencia: %d Hz\n", f);
    pitar(f, 800);
    delay(400);
  }
  Serial.println("Fin. Pon esa frecuencia en FREC_BUZZER.");
}

// ---------- MPU6050 ----------
bool leerMPU(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(MPU, (uint8_t)14, (uint8_t)true);
  if (Wire.available() < 14) return false;

  int16_t v[7];
  for (int i = 0; i < 7; i++) {
    uint8_t alto = Wire.read();
    uint8_t bajo = Wire.read();
    v[i] = (int16_t)((alto << 8) | bajo);
  }
  ax = v[0] / 16384.0;  ay = v[1] / 16384.0;  az = v[2] / 16384.0;
  gx = v[4] / 131.0 - gxOff;
  gy = v[5] / 131.0 - gyOff;
  gz = v[6] / 131.0 - gzOff;
  return true;
}

float anguloPitch(float ax, float ay, float az) { return atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI; }
float anguloRoll(float ay, float az)            { return atan2(ay, az) * 180.0 / PI; }

float difAngulo(float a, float b) {
  float d = a - b;
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d;
}

// Mide quieto durante "ms": saca el promedio de angulos y el error del giroscopio
bool medirReposo(int ms, float &pMed, float &rMed) {
  int n = ms / 20;
  double sP = 0, sR = 0, sgx = 0, sgy = 0, sgz = 0;
  int buenas = 0;
  gxOff = gyOff = gzOff = 0;
  for (int i = 0; i < n; i++) {
    float ax, ay, az, gx, gy, gz;
    if (leerMPU(ax, ay, az, gx, gy, gz)) {
      sP += anguloPitch(ax, ay, az);
      sR += anguloRoll(ay, az);
      sgx += gx; sgy += gy; sgz += gz;
      buenas++;
    }
    delay(20);
  }
  if (buenas == 0) return false;
  pMed = sP / buenas;  rMed = sR / buenas;
  gxOff = sgx / buenas; gyOff = sgy / buenas; gzOff = sgz / buenas;
  return true;
}

void reiniciarEstado() {
  pitch = refPitch; roll = refRoll;
  tPrev = micros();
  inicioMala = 0; avisando = false;
  ultimoMovimiento = millis();
  tiempoBueno = tiempoMalo = 0;
}

// Calibracion completa de postura (y se guarda en memoria)
void calibrarPostura() {
  Serial.println("CALIBRANDO: sientate DERECHO y quieto...");
  for (int i = 0; i < 3; i++) { pitar(FREC_BUZZER, 150); delay(850); }
  delay(2000);

  float p, r;
  if (!medirReposo(3000, p, r)) { Serial.println("ERROR leyendo el sensor"); return; }
  refPitch = p; refRoll = r;

  prefs.begin("postura", false);
  prefs.putFloat("p", refPitch);
  prefs.putFloat("r", refRoll);
  prefs.putBool("ok", true);
  prefs.end();

  Serial.printf("Calibrado y guardado. pitch=%.1f roll=%.1f\n", refPitch, refRoll);
  reiniciarEstado();
  pitar(FREC_BUZZER, 600);   // pitido largo = listo
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PIN_BOTON, INPUT_PULLUP);

  ledcAttach(PIN_BUZZER, FREC_BUZZER, 8);
  gpio_set_drive_capability((gpio_num_t)PIN_BUZZER, GPIO_DRIVE_CAP_3);   // maxima fuerza
  sonido(0);

  pitar(FREC_BUZZER, 300);   // prueba de volumen
  delay(200);
  if (MODO_BARRIDO) { barrido(); while (true) delay(1000); }

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  if (Wire.endTransmission() != 0) {
    Serial.println("NO encuentro el MPU6050. Revisa cables SDA/SCL/VCC/GND.");
    while (true) { pitar(FREC_BUZZER, 500); delay(500); }
  }
  Wire.beginTransmission(MPU);
  Wire.write(0x1A);
  Wire.write(3);
  Wire.endTransmission();

  // ¿Hay una calibracion guardada?
  prefs.begin("postura", true);
  bool guardada = prefs.getBool("ok", false);
  float pGuardado = prefs.getFloat("p", 0);
  float rGuardado = prefs.getFloat("r", 0);
  prefs.end();

  bool forzar = (digitalRead(PIN_BOTON) == LOW);

  if (guardada && !forzar) {
    Serial.println("Calibracion guardada encontrada. Deja el sensor quieto 2 s...");
    delay(500);
    float p, r;
    medirReposo(2000, p, r);       // solo ajusta el giroscopio
    refPitch = pGuardado; refRoll = rGuardado;
    reiniciarEstado();
    pitar(FREC_BUZZER, 500);       // listo
    Serial.println("LISTO. Corrigiendo tu postura.");
  } else {
    calibrarPostura();
  }
}

// ---------- LOOP ----------
void loop() {
  if (digitalRead(PIN_BOTON) == LOW) { calibrarPostura(); return; }

  float ax, ay, az, gx, gy, gz;
  if (!leerMPU(ax, ay, az, gx, gy, gz)) { delay(20); return; }

  unsigned long ahoraUs = micros();
  float dt = (ahoraUs - tPrev) / 1000000.0;
  tPrev = ahoraUs;
  if (dt <= 0 || dt > 0.5) dt = 0.02;

  // filtro complementario (giroscopio + acelerometro)
  pitch = 0.96 * (pitch + gy * dt) + 0.04 * anguloPitch(ax, ay, az);
  roll  = 0.96 * (roll  + gx * dt) + 0.04 * anguloRoll(ay, az);

  float dp = difAngulo(pitch, refPitch);
  float dr = difAngulo(roll, refRoll);
  float desv = sqrt(dp * dp + dr * dr);

  float mov = sqrt(gx * gx + gy * gy + gz * gz);
  bool moviendo = mov > MOVIMIENTO_MAX;
  unsigned long ahora = millis();

  if (moviendo) {
    ultimoMovimiento = ahora;
    inicioMala = 0;
  } else {
    if (desv > UMBRAL_GRADOS) tiempoMalo += (unsigned long)(dt * 1000);
    else tiempoBueno += (unsigned long)(dt * 1000);

    if (desv > UMBRAL_GRADOS) {
      if (inicioMala == 0) inicioMala = ahora;
    } else {
      if (avisando) { melodiaFeliz(); avisando = false; }
      inicioMala = 0;
    }
  }

  // MALA POSTURA -> SUENA EL BUZZER (cada vez mas seguido si no corriges)
  if (inicioMala != 0 && ahora - inicioMala > TOLERANCIA_MS) {
    avisando = true;
    unsigned long extra = ahora - inicioMala - TOLERANCIA_MS;
    int nivel = (extra < 10000) ? 1 : (extra < 25000) ? 2 : 3;
    unsigned long intervalo = (nivel == 1) ? 2000 : (nivel == 2) ? 1000 : 500;

    if (ahora - ultimoPitido > intervalo) {
      ultimoPitido = ahora;
      if (fabs(dp) >= fabs(dr)) {            // encorvado: un pitido largo
        pitar(FREC_BUZZER, 200 + nivel * 150);
      } else {                                // de lado: dos pitidos cortos
        pitar(FREC_BUZZER, 120); delay(80); pitar(FREC_BUZZER, 120);
      }
    }
  }

  // recordatorio de levantarte
  if (ahora - ultimoMovimiento > SENTADO_MAX_MS) {
    melodiaParate();
    ultimoMovimiento = ahora;
  }

  // informe cada segundo
  if (ahora - ultimoPrint > 1000) {
    ultimoPrint = ahora;
    float total = tiempoBueno + tiempoMalo;
    int puntaje = (total > 0) ? (int)(100.0 * tiempoBueno / total) : 100;
    Serial.printf("Inclinacion: %.1f grados | Limite: %.0f grados | Adelante/atras: %.1f | Lado: %.1f | %s | Buena postura: %d%%\n",
                  desv, UMBRAL_GRADOS, dp, dr, (desv > UMBRAL_GRADOS) ? "MALA POSTURA" : "BIEN", puntaje);
  }

  delay(20);
}
