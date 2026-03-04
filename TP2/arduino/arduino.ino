#include <Arduino_FreeRTOS.h>
#include <semphr.h> // Librería específica para usar Semáforos y Mutex

void TaskReadLDR(void *pvParameters);       // Task Lectura de intensidad Luminica
void TaskSendLDR(void *pvParameters);       // Task Envio de intensidad Luminica
void TaskControlButton(void *pvParameters); // Task control de boton
void TaskStatusLed(void *pvParameters);     // Task Parpadeo led 11
void buttonISR();

// controlador del semaforo mutex del puerto serial
SemaphoreHandle_t xSerialSemaphore;

// controlador del semaforo para comunicar la ISR con la tarea
SemaphoreHandle_t xBotonSemaphore;

// variable de estado para saber si esta activa la lectura del LDR
volatile bool lecturaActivada = false;

// varible global del valorLDR
volatile int valorLDR = 0;

void setup()
{
    Serial.begin(9600);

    // Boton para frenar la lectura y envio de valor del sensor LDR
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), buttonISR, RISING);

    if (xBotonSemaphore == NULL)
    {
        xBotonSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario
    }

    if (xSerialSemaphore == NULL)
    {
        xSerialSemaphore = xSemaphoreCreateMutex(); // creamos semaforo mutex
    }

    // seteamos las tareas ---------------------
    xTaskCreate(
        TaskReadLDR,
        "Lectura_LDR",
        128,
        NULL,
        2,
        NULL);

    xTaskCreate(
        TaskSendLDR,
        "Envio_LDR",
        128,
        NULL,
        2,
        NULL);

    xTaskCreate(
        TaskControlButton,
        "Control_Boton",
        128,
        NULL,
        3,
        NULL);

    xTaskCreate(
        TaskStatusLed,
        "Led11_Parpadeo",
        128,
        NULL,
        2,
        NULL);
}

void loop()
{
}

/*---------------------- Tasks ---------------------*/
void TaskReadLDR(void *pvParameters)
{
    (void)pvParameters;

    // Init pines
    pinMode(A3, INPUT);

    // for loop
    for (;;)
    {

        if (lecturaActivada == true)
        {
            const int valorLDR_now = analogRead(A3);
            valorLDR = valorLDR_now;
        }

        vTaskDelay(50 / portTICK_PERIOD_MS); // 50 ms
    }
}

void TaskSendLDR(void *pvParameters)
{
    (void)pvParameters;

    // for loop
    for (;;)
    {

        char bufferLDR[5];                    // longitud del buffer de 4 chars
        sprintf(bufferLDR, "%04d", valorLDR); // rellena con ceros a la izquierda

        // Trata de tomar el semaforo por 5 ticks
        if (xSemaphoreTake(xSerialSemaphore, (TickType_t)5) == pdTRUE)
        {
            Serial.println(bufferLDR);
            xSemaphoreGive(xSerialSemaphore); // Liberamos el semaforo
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS); // 3 segundos
    }
}

void TaskControlButton(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        // Intenta tomar el semaforo y espera infinitamente
        if (xSemaphoreTake(xBotonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Alguien apreto el boton -> Recibe el ISR
            lecturaActivada = !lecturaActivada;

            if (xSemaphoreTake(xSerialSemaphore, (TickType_t)10) == pdTRUE)
            {
                if (lecturaActivada)
                {
                    Serial.println("=== LECTURA INICIADA ===");
                }
                else
                {
                    Serial.println("=== LECTURA DETENIDA ===");
                }
                xSemaphoreGive(xSerialSemaphore);
            }

            vTaskDelay(200 / portTICK_PERIOD_MS); // 50 ms
        }
    }
}

void TaskStatusLed(void *pvParameters)
{
    (void)pvParameters;

    // Init pines
    pinMode(11, OUTPUT);

    // for loop
    for (;;)
    {

        if (lecturaActivada == true)
        {
            digitalWrite(11, HIGH);
            vTaskDelay(200 / portTICK_PERIOD_MS); // 200 ms
            digitalWrite(11, LOW);
            vTaskDelay(200 / portTICK_PERIOD_MS); // 200 ms
        }
        digitalWrite(11, LOW);
        vTaskDelay(50 / portTICK_PERIOD_MS); // 50 ms
    }
}

/*---------------------- ISRs ---------------------*/
void buttonISR()
{

    // para saber si al dar el semáforo
    // despertamos a una tarea mAs importante que la que estaba corriendo.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // damos el semaforo
    xSemaphoreGiveFromISR(xBotonSemaphore, &xHigherPriorityTaskWoken);

    // Si despertamos a una tarea de mayor prioridad, forzamos un cambio de contexto inmediato
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}