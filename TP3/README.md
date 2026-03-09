# TP3 — Logger de Eventos con EEPROM y NTP

Sistema de registro de eventos en EEPROM con sincronización de hora NTP, usando Arduino Uno, tareas FreeRTOS, y una interfaz web con FastAPI + Vue 3.

## Arquitectura

- **Arduino** (FreeRTOS): Registra eventos de botones en EEPROM con timestamp, responde comandos de lectura (`L`) y borrado (`B`) por serial.
- **FastAPI** (`main.py`): Obtiene la hora desde un servidor NTP, la envía al Arduino, y expone los eventos leídos de la EEPROM mediante endpoints REST.
- **Vue 3** (`frontend/`): Panel de control con 3 acciones (actualizar hora, leer historial, borrar memoria) y una tabla de eventos con auto-refresh.

## Compilar el Sketch de Arduino

Requiere `arduino-cli` con el core `arduino:avr` y la librería `FreeRTOS`.

```bash
arduino-cli core install arduino:avr
arduino-cli lib install FreeRTOS

# Compilar
arduino-cli compile --fqbn arduino:avr:uno TP3/arduino --output-dir TP3/arduino/build
```

Los archivos compilados (`.elf` y `.hex`) se guardan en `arduino/build/`, referenciados por `wokwi.toml` para el simulador Wokwi.

## Ejecutar con el Simulador Wokwi

1. Abrir `TP3/arduino/` en VS Code con la extensión Wokwi instalada.
2. Iniciar la simulación — expone un puerto serial en `rfc2217://localhost:4001`.

## Iniciar el Servidor Web

```bash
cd TP3
pip install -r requirements.txt
uvicorn main:app --reload
```

Luego abrir [http://localhost:8000](http://localhost:8000) en el navegador.

## Endpoints de la API

| Método | Ruta            | Descripción                                              |
| ------ | --------------- | -------------------------------------------------------- |
| GET    | `/updateTime`   | Obtiene la hora NTP y la envía al Arduino                |
| POST   | `/readEEPROM`   | Envía comando `L` al Arduino para leer eventos de EEPROM |
| POST   | `/deleteEEPROM` | Envía comando `B` al Arduino para borrar la EEPROM       |
| GET    | `/events`       | Devuelve la lista de eventos `[{evento, hora}]`          |
