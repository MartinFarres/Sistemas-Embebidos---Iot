# TP1 — Conversores AD y DA. Protocolos de comunicación serial cableados

Sistema de control de LEDs PWM y lectura de sensor LDR usando Arduino Uno, con interfaz web FastAPI + Vue 3.

## Arquitectura

- **Arduino** (`arduino.c++`): Recibe comandos para controlar 3 LEDs PWM y LED 13, envía lecturas del sensor LDR por serial.
- **FastAPI** (`main.py`): Conecta el puerto serial con el cliente web mediante endpoints REST.
- **Vue 3** (`frontend/`): Sliders para controlar brillo de LEDs PWM, toggle para LED 13, y visualización del sensor LDR.

## Protocolo Serial

### Trama de computadora -> Arduino (Comandos de los LEDs)

Necesitamos enviar 4 valores:

- el brillo de los tres LEDs PWM (que va de 0 a 255);
- el estado del LED 13 (0 o 1).

Hacemos un string de 10 caracteres:

- Bytes 0-2: Valor LED 9 (Ej: 000 a 255)

- Bytes 3-5: Valor LED 10 (Ej: 000 a 255)

- Bytes 6-8: Valor LED 11 (Ej: 000 a 255)

- Byte 9: Estado LED 13 (Ej: 0 o 1)

Ejemplo de trama: El string 2551270001 -> LED9: 255, LED10: 127, LED11: 000, LED13: 1

### Trama de Arduino -> Computadora (Datos del sensor LDR)

El Arduino envia los datos del sensor LDR.
analogRead() devuelve valores entre 0 y 1023 -> trama de longitud fija de 4 caracteres, rellenando con ceros a la izquierda si es necesario.

- Bytes 0-3: Valor LDR (Ej: 0250 o 1023)

## Compilar el Sketch de Arduino

Requiere `arduino-cli` con el core `arduino:avr`.

```bash
arduino-cli core install arduino:avr

# Compilar
arduino-cli compile --fqbn arduino:avr:uno TP1/arduino --output-dir TP1/arduino/build
```

Los archivos compilados (`.elf` y `.hex`) se guardan en `arduino/build/`, referenciados por `wokwi.toml` para el simulador Wokwi.

## Ejecutar con el Simulador Wokwi

1. Abrir `TP1/arduino/` en VS Code con la extensión Wokwi instalada.
2. Iniciar la simulación — expone un puerto serial en `rfc2217://localhost:4000`.

## Iniciar el Servidor Web

```bash
cd TP1
pip install -r requirements.txt
uvicorn main:app --reload
```

Luego, en el navegador, ingresar a: [http://localhost:8000](http://localhost:8000)
