// Nombramos los pines
// Control brillo ------
const int pinLed9 = 9;
const int pinLed10 = 10;
const int pinLed11 = 11;
// ON/OFF (Output D)
const int pinLed13 = 13;
// Sensor (Input A)
const int pinLDR = A3;

void setup()
{
    pinMode(pinLed9, OUTPUT);
    pinMode(pinLed10, OUTPUT);
    pinMode(pinLed11, OUTPUT);
    pinMode(pinLed13, OUTPUT);

    // pinMode(pinLDR, INPUT); A3 NO HACE FALTA CONFIG

    Serial.begin(9600);
}

void loop()
{
    // Enviar al Servidor
    int valorLDR = analogRead(pinLDR);    // leemos datos del sensor
    char bufferLDR[5];                    // longitud del buffer de 4 chars
    sprintf(bufferLDR, "%04d", valorLDR); // rellena con ceros a la izquierda
    Serial.println(bufferLDR);

    // Recibir valores del Servidor
    if (Serial.available() >= 10)
    {

        // leemos la trama hasta el enter
        String tramaRecibida = Serial.readStringUntil('\n');

        if (tramaRecibida.length() >= 10)
        {
            int pwmLed9 = tramaRecibida.substring(0, 3).toInt();
            int pwmLed10 = tramaRecibida.substring(3, 6).toInt();
            int pwmLed11 = tramaRecibida.substring(6, 9).toInt();
            int estadoLed13 = tramaRecibida.substring(9, 10).toInt();

            analogWrite(pinLed9, pwmLed9);
            analogWrite(pinLed10, pwmLed10);
            analogWrite(pinLed11, pwmLed11);

            if (estadoLed13 == 1)
            {
                digitalWrite(pinLed13, HIGH);
            }
            else
            {
                digitalWrite(pinLed13, LOW);
            }
        }
    }

    delay(1000); // wait for a second
}
