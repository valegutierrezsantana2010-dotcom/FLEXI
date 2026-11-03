# FLEXI – Sistema Inteligente de Corrección Postural

## 1. Descripción del proyecto

FLEXI es un prototipo tecnológico diseñado para identificar cambios en la postura corporal mediante un sensor de movimiento MPU6050. El sistema utiliza un microcontrolador ESP32-S3 para procesar la información y activar una alerta sonora cuando detecta una inclinación inadecuada durante un tiempo determinado.

## 2. Objetivo general

Desarrollar un sistema electrónico que ayude a reconocer posturas inadecuadas mediante sensores y genere alertas para fomentar una mejor postura corporal.

## 3. Componentes principales

- ESP32-S3: procesa la información del sistema.
- MPU6050: sensor que detecta inclinación y movimiento.
- Buzzer pasivo: genera las alertas sonoras.
- Protoboard y cables: permiten realizar las conexiones.

## 4. Funcionamiento

El sensor MPU6050 registra los movimientos y la inclinación. El ESP32-S3 analiza los datos y los compara con la posición de referencia establecida durante la calibración. Cuando detecta una inclinación que supera el límite configurado durante el tiempo establecido, activa el buzzer como advertencia.

## 5. Tecnologías utilizadas

- Arduino IDE.
- Lenguaje de programación C++.
- Microcontrolador ESP32-S3.
- Comunicación I2C.

## 6. Estructura del repositorio

- `codigo/`: contiene el programa del proyecto.
- `documentacion/`: contiene el documento técnico.
- `imagenes/`: contiene fotografías del prototipo.
- `diagrama/`: contiene el diagrama y la explicación de las conexiones.

## 7. Integrantes

Proyecto desarrollado por el equipo de trabajo escolar.

## 8. Estado del proyecto

Prototipo funcional.

## 9. Propósito

FLEXI busca promover hábitos posturales saludables mediante el uso de tecnología y alertas que permitan reconocer posiciones inadecuadas.
