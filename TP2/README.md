# TP2 — Sistema de Alarma LDR con FreeRTOS

Sistema de monitoreo de sensor de luz en tiempo real usando Arduino Uno, tareas FreeRTOS, y una interfaz web con FastAPI + Vue 3.

## Arquitectura

- **Arduino** (FreeRTOS): Lee el sensor LDR, activa la alarma cuando el valor > 800, se comunica por puerto serial.
- **FastAPI** (`main.py`): Conecta el puerto serial con el cliente web mediante endpoints REST.
- **Vue 3** (`frontend/`): Muestra el valor del LDR, estado de lectura/alarma, y un botón de toggle.

## Compilar el Sketch de Arduino

Requiere `arduino-cli` con el core `arduino:avr` y la librería `FreeRTOS`.

```bash
arduino-cli core install arduino:avr
arduino-cli lib install FreeRTOS

# Compilar
arduino-cli compile --fqbn arduino:avr:uno TP2/arduino --output-dir TP2/arduino/build
```

Los archivos compilados (`.elf` y `.hex`) se guardan en `arduino/build/`, referenciados por `wokwi.toml` para el simulador Wokwi.

## Ejecutar con el Simulador Wokwi

1. Abrir `TP2/arduino/` en VS Code con la extensión Wokwi instalada.
2. Iniciar la simulación — expone un puerto serial en `rfc2217://localhost:4001`.

## Iniciar el Servidor Web

```bash
cd TP2
pip install -r requirements.txt
uvicorn main:app --reload
```

Luego abrir [http://localhost:8000](http://localhost:8000) en el navegador.
