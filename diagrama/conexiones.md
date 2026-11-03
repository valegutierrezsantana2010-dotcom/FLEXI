# Diagrama de conexiones de FLEXI

## Componentes utilizados
- ESP32-S3
- Sensor MPU6050
- Buzzer pasivo

## Conexiones electrónicas

| Componente | Pin ESP32-S3 |
|---|---|
| MPU6050 VCC | 3V3 |
| MPU6050 GND | GND |
| MPU6050 SDA | GPIO 8 |
| MPU6050 SCL | GPIO 9 |
| Buzzer positivo (+) | GPIO 4 |
| Buzzer negativo (-) | GND |
| Botón BOOT | GPIO 0 |

## Funcionamiento

El MPU6050 registra los cambios de inclinación y movimiento. El ESP32-S3 procesa los datos y compara la inclinación con la posición de referencia guardada durante la calibración.

Cuando se supera el umbral configurado durante el tiempo de tolerancia, el buzzer emite una alerta sonora.

## Configuración del programa
- Umbral de inclinación: 12 grados.
- Tiempo de tolerancia: 3 segundos.
- Recordatorio de movimiento: 30 minutos.
- Programación: Arduino IDE.
