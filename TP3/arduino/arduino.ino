#include <Arduino_FreeRTOS.h>
#include <stdlib.h>
#include <semphr.h> // Librería específica para usar Semáforos y Mutex

void TaskReceiveSerial(void *pvParameters); // Task Recibir del servidor
void TaskInterpreter(void *pvParameters);   // Task Parsear buffer de tiempo a long int

// controlador del semaforo mutex del puerto serial
SemaphoreHandle_t xSerialSemaphore;
// controlador del semaforo binario que habilita al interprete
SemaphoreHandle_t xInterpreterSemaphore;

// Global vars
char bufferRx[15];
int indice = -1;
long int tiempoRecibido = 0;

void setup()
{
    Serial.begin(9600);

    if (xSerialSemaphore == NULL)
    {
        xSerialSemaphore = xSemaphoreCreateMutex(); // creamos semaforo mutex
    }

    if (xInterpreterSemaphore == NULL)
    {
        xInterpreterSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario
    }

    xTaskCreate(TaskReceiveSerial, "Recibir", 80, NULL, 2, NULL);
    xTaskCreate(TaskInterpreter, "InterpretarTiempo", 80, NULL, 2, NULL);
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
                if (req == 'T')
                {
                    indice = 0;
                    guardando = true;
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

/*---------------------- ISRs ---------------------*/
