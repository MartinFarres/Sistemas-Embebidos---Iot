from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import serial
import threading
import os
import time

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

shareMem = {
    "ldrSensor": 0,
    "alarma": False,
    "lecturaActiva": False,
    "serialConnected": False,
    "lastSerialError": "",
}


@app.post("/toggleLectura")
def toggle_lectura():
    try:
        s = get_serial()
        if s is not None:
            # envia marcador 'T\n'
            s.write(b"T\n") 
            return {"status": "comando_enviado"}
        return {"status": "error_puerto_cerrado"}
    except Exception as e:
        return {"status": "error", "error_msg": str(e)}

@app.get("/ldrSensor")
def get_estado_sistema():
    return shareMem

@app.get("/")
def serve_frontend():
    return FileResponse(os.path.join(FRONTEND_DIR, "index.html"))

@app.on_event("startup")
def startup_event():
    thread_serial_port = threading.Thread(target=listening_serial_port, name="Thread-SP", daemon=True)
    thread_serial_port.start()


def get_serial():
    global ser
    if ser is None or not ser.is_open:
        try:
            ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
            # ser = serial.serial_for_url('rfc2217://localhost:4001', baudrate=9600, timeout=1)
            # ser = serial.serial_for_url('socket://127.0.0.1:4001', timeout=1)
            shareMem["serialConnected"] = True
            shareMem["lastSerialError"] = ""
        except Exception as e:
            shareMem["serialConnected"] = False
            shareMem["lastSerialError"] = str(e)
            raise
    return ser

def listening_serial_port():
    while True:
        try:
            s = get_serial()
            line = s.readline().decode().strip()
            if line:
                if line == "=== LECTURA INICIADA ===":
                    shareMem["lecturaActiva"] = True
                elif line == "=== LECTURA DETENIDA ===":
                    shareMem["lecturaActiva"] = False
                    shareMem["alarma"] = False 
                elif line == "=== ALARMA ACTIVADA ===":
                    shareMem["alarma"] = True
                elif line.isdigit(): 
                    shareMem["ldrSensor"] = int(line)
        except Exception as e:
            shareMem["serialConnected"] = False
            shareMem["lastSerialError"] = str(e)
            time.sleep(0.5)


# Conectar con Vue frontend
FRONTEND_DIR = os.path.join(os.path.dirname(__file__), "frontend")
app.mount("/assets", StaticFiles(directory=os.path.join(FRONTEND_DIR, "assets")), name="assets")

# Run (dev):
#   uvicorn main:app --reload