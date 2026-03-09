#include <Arduino_FreeRTOS.h>
#include <stdlib.h>
#include <semphr.h> // Librería específica para usar Semáforos y Mutex
#include <EEPROM.h>

void TaskReceiveSerial(void *pvParameters); // Task Recibir del servidor
void TaskInterpreter(void *pvParameters);   // Task Parsear buffer de tiempo a long int
void TaskClock(void *pvParameters);         // Task Clock suma 1 por cada seg
void TaskSaveTime(void *pvParameters);      // Task guarda el tiempo en EEPROM
void TaskReadEEPROM(void *pvParameters);    // Task lee el tiempo en EEPROM
void TaskDeleteEEPROM(void *pvParameters);  // Task elimina el tiempo en EEPROM
void buttonISR();

// controlador del semaforo mutex del puerto serial
SemaphoreHandle_t xSerialSemaphore;
// controlador del semaforo binario que habilita al interprete
SemaphoreHandle_t xInterpreterSemaphore;
// controlador del semaforo para comunicar la ISR con la tarea
SemaphoreHandle_t xBotonSemaphore;
// controlador del semaforo para leer el EEPROM
SemaphoreHandle_t xReadSemaphore;
// controlador del semaforo para borrar el EEPROM
SemaphoreHandle_t xDeleteSemaphore;

// Global vars
char bufferRx[15];
int indice = -1;
long int tiempoRecibido = 0;
int posMemDisponible = 0;
volatile byte botonPresionado = 0;

void setup()
{
    Serial.begin(9600);

    // Iteramos por la EEPROM hasta encontrar memoria vacia == 0xFF
    bool posGuardada = false;
    int index = 0;
    while ((index < EEPROM.length()) && (posGuardada == false))
    {
        if (EEPROM[index] == 0xFF)
        {
            posMemDisponible = index;
            posGuardada = true;
        }
        else
        {
            index += 5; // escanea por registro
        }
    }

    pinMode(2, INPUT);
    pinMode(3, INPUT);

    if (xSerialSemaphore == NULL)
    {
        xSerialSemaphore = xSemaphoreCreateMutex(); // creamos semaforo mutex
    }

    if (xInterpreterSemaphore == NULL)
    {
        xInterpreterSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario
    }

    if (xBotonSemaphore == NULL)
    {
        xBotonSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario boton
    }

    if (xReadSemaphore == NULL)
    {
        xReadSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario
    }

    if (xDeleteSemaphore == NULL)
    {
        xDeleteSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario
    }

    xTaskCreate(TaskReceiveSerial, "Recibir", 80, NULL, 2, NULL);
    xTaskCreate(TaskInterpreter, "InterpretarTiempo", 80, NULL, 2, NULL);
    xTaskCreate(TaskClock, "Clock", 80, NULL, 4, NULL); // con alta prioridad
    xTaskCreate(TaskSaveTime, "GuardarTiempo", 80, NULL, 2, NULL);
    xTaskCreate(TaskReadEEPROM, "LeeTiempo", 80, NULL, 2, NULL);
    xTaskCreate(TaskDeleteEEPROM, "EliminaTiempo", 80, NULL, 2, NULL);

    attachInterrupt(digitalPinToInterrupt(2), buttonISR, RISING);
    attachInterrupt(digitalPinToInterrupt(3), buttonISR, RISING);
}

void loop()
{
}

/*---------------------- Tasks ---------------------*/
void TaskReceiveSerial(void *pvParameters)
{
    (void)pvParameters;

    bool guardando = false;

    for (;;)
    {
        if (xSemaphoreTake(xSerialSemaphore, (TickType_t)10) == pdTRUE)
        {
            if (Serial.available() > 0)
            {
                char req = Serial.read();
                if (guardando == true)
                {
                    if (req == '\n')
                    {
                        bufferRx[indice] = '\0';
                        guardando = false;
                        // llamar a la task interpreter
                        xSemaphoreGive(xInterpreterSemaphore);
                    }
                    else
                    {
                        bufferRx[indice] = req;
                        indice += 1;
                    }
                }
                if (req == 'T') // viene el tiempo
                {
                    indice = 0;
                    guardando = true;
                }
                else if (req == 'L')
                { // leer tiempo guardado
                    xSemaphoreGive(xReadSemaphore);
                }
                else if (req == 'B')
                { // borrar EEPROM
                    xSemaphoreGive(xDeleteSemaphore);
                }
            }
            xSemaphoreGive(xSerialSemaphore);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void TaskInterpreter(void *pvParameters)
{
    for (;;)
    {

        if (xSemaphoreTake(xInterpreterSemaphore, (portMAX_DELAY)) == pdTRUE)
        {
            tiempoRecibido = atol(bufferRx);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void TaskClock(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        tiempoRecibido += 1;

        vTaskDelayUntil(&xLastWakeTime, (1000 / portTICK_PERIOD_MS));
    }
}

void TaskSaveTime(void *pvParameters)
{

    for (;;)
    {
        // Intenta tomar el semaforo y espera infinitamente
        if (xSemaphoreTake(xBotonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Chequeamos que no tenemos la memoria llena
            if (posMemDisponible + 5 <= EEPROM.length())
            {
                EEPROM.put(posMemDisponible, tiempoRecibido);      // ocupa los casilleros pos, pos+1, pos+2, pos+3
                EEPROM.put(posMemDisponible + 4, botonPresionado); // ocupa el casillero pos+4
                posMemDisponible += 5;                             // movemos el puntero al proximo bloque vacío
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void TaskReadEEPROM(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(xReadSemaphore, (portMAX_DELAY)) == pdTRUE)
        {
            long int readTime;
            byte eventoGuardado;

            for (i = 0; i <= EEPROM.length(); i += 5)
            {

                EEPROM.get(i, readTime);
                EEPROM.get(i + 4, eventoGuardado);

                // enviar por serial
                if (xSemaphoreTake(xSerialSemaphore, (TickType_t)10) == pdTRUE)
                {
                    Serial.print(eventoGuardado);
                    Serial.print(",");
                    Serial.println(tiempoGuardado);
                    xSemaphoreGive(xSerialSemaphore);
                }
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void TaskDeleteEEPROM(void *pvParameters)
{
    for (;;)
    {

        if (xSemaphoreTake(xDeleteSemaphore, (portMAX_DELAY)) == pdTRUE)
        {
            for (int i = 0; i < EEPROM.length(); i++)
            {
                EEPROM.write(i, 0xFF);
            }
            posMemDisponible = 0;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

/*---------------------- ISRs ---------------------*/
void buttonISR()
{
    // para saber si al dar el semáforo
    // despertamos a una tarea mAs importante que la que estaba corriendo.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (digitalRead(2) == HIGH)
    {
        botonPresionado = 2;
    }
    else if (digitalRead(3) == HIGH)
    {
        botonPresionado = 3;
    }
    // damos el semaforo
    xSemaphoreGiveFromISR(xBotonSemaphore, &xHigherPriorityTaskWoken);

    // Si despertamos a una tarea de mayor prioridad, forzamos un cambio de contexto inmediato
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}