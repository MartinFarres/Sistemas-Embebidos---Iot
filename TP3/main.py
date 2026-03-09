from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import serial
import threading
import os
import time
import ntplib 


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
    "events": [],
    "serialConnected": False,
    "lastSerialError": "",
}


# Define el servidor NTP
server = "pool.ntp.org"

# Crea un cliente NTP
client = ntplib.NTPClient()

@app.get("/updateTime")
def update_time():
    response = client.request(server)
    try:
        s = get_serial()
        if s is not None:
            # envia marcador 'T' seguido del tiempo
            ser.write(f"T{int(response.tx_time)}\n".encode("utf-8"))
            return {"status": "tiempo_enviado"}
        return {"status": "error_puerto_cerrado"}
    except Exception as e:
        return {"status": "error", "error_msg": str(e)}

@app.post("/readEEPROM")
def read_EEPROM():
    try:
        s = get_serial()
        if s is not None:
            shareMem["events"] = []
            # envia marcador 'L' 
            ser.write(b'L\n')
            return {"status": "marcador_lectura_enviado"}
        return {"status": "error_puerto_cerrado"}
    except Exception as e:
        return {"status": "error", "error_msg": str(e)}
    
@app.post("/deleteEEPROM")
def delete_EEPROM():
    try:
        s = get_serial()
        if s is not None:
            # envia marcador 'L' 
            ser.write(b'B\n')
            shareMem["events"] = []
            return {"status": "historial_eliminado"}
        return {"status": "error_puerto_cerrado"}
    except Exception as e:
        return {"status": "error", "error_msg": str(e)}


@app.get("/events")
def get_estado_sistema():
    return shareMem["events"]

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
            # ser = serial.Serial('/dev/ttyS1', 9600, timeout=1)
            ser = serial.serial_for_url('rfc2217://localhost:4001', baudrate=9600, timeout=1)
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

            if line and ',' in line:
                # puerto serial envia: evento,tiempo\n
                line_aux = line.split(",")            
                event = line_aux[0]
                timestamp_crudo = int(line_aux[1])
                if event != "255" and timestamp_crudo > 0:
                    res_time = time.ctime(int(line_aux[1]))
                    shareMem["events"].append({
                        "evento": event, 
                        "hora": res_time
                    })

        except Exception as e:
            shareMem["serialConnected"] = False
            shareMem["lastSerialError"] = str(e)
            time.sleep(0.5)


# Conectar con Vue frontend
FRONTEND_DIR = os.path.join(os.path.dirname(__file__), "frontend")
app.mount("/assets", StaticFiles(directory=os.path.join(FRONTEND_DIR, "assets")), name="assets")

# Run (dev):
#   uvicorn main:app --reload