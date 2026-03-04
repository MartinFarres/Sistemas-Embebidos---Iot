from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from typing import Optional
from pydantic import BaseModel
import serial
import threading
import time
import os

app = FastAPI(
    title="FastAPI Template",
    version="0.1.0",
    description="Simple FastAPI starter template.",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

ser = None

class LedValues(BaseModel):
    led9: int = 0
    led10: int = 0
    led11: int = 0
    led13: int = 0

@app.post("/ledValues")
def led_slider(led_values:LedValues):
    # Actualizar memoria compartida
    shareMem["ledValues"] = led_values
    # Armar la trama con el buffer
    trama = f"{led_values.led9:03d}{led_values.led10:03d}{led_values.led11:03d}{led_values.led13}"
    # Enviar la trama al arduino
    try:
        s = get_serial()
        s.write((trama + "\n").encode())
    except Exception as e:
        return {"status":"error", "error_msg": str(e)}
    
    return {"status": "ok"}

@app.get("/ldrSensor")
def get_ldr_sensor():
    return {"ldrSensor": shareMem["ldrSensor"]}

shareMem = {
    "ldrSensor": 0,
    "ledValues": LedValues()
}

def get_serial():
    global ser
    if ser is None or not ser.is_open:
        # ser = serial.Serial('/dev/ttyS1', 9600, timeout=1)
        ser = serial.serial_for_url('rfc2217://localhost:4000', baudrate=9600, timeout=1)
        # ser = serial.serial_for_url('socket://127.0.0.1:4000', timeout=1)
    return ser

def listening_serial_port():
    while True:
        try:
            s = get_serial()
            line = s.readline().decode().strip()
            if line:
                shareMem["ldrSensor"] = int(line)
        except Exception:
            pass
        time.sleep(0.5)

@app.on_event("startup")
def startup_event():
    thread_serial_port = threading.Thread(target=listening_serial_port, name="Thread-SP", daemon=True)
    thread_serial_port.start()

# Conectar con Vue frontend
FRONTEND_DIR = os.path.join(os.path.dirname(__file__), "frontend")
app.mount("/assets", StaticFiles(directory=os.path.join(FRONTEND_DIR, "assets")), name="assets")

@app.get("/")
def serve_frontend():
    return FileResponse(os.path.join(FRONTEND_DIR, "index.html"))

# Run (dev):
#   uvicorn main:app --reload