#include <Arduino_FreeRTOS.h>
#include <semphr.h> // Librería específica para usar Semáforos y Mutex

void TaskReadLDR(void *pvParameters);       // Task Lectura de intensidad Luminica
void TaskSendLDR(void *pvParameters);       // Task Envio de intensidad Luminica
void TaskControlButton(void *pvParameters); // Task control de boton
void TaskStatusLed(void *pvParameters);     // Task Parpadeo led 11
void TaskAlarm800(void *pvParameters);      // Task Alarma LDR > 800
void TaskReceiveSerial(void *pvParameters); // Task Recibir toggle del servidor
void buttonISR();

// controlador del semaforo mutex del puerto serial
SemaphoreHandle_t xSerialSemaphore;

// controlador del semaforo para comunicar la ISR con la tarea
SemaphoreHandle_t xBotonSemaphore;

// variable de estado para saber si esta activa la lectura del LDR
volatile bool lecturaActivada = false;
volatile bool alarmaActivada = false;

// varible global del valorLDR
volatile int valorLDR = 0;

void setup()
{
    Serial.begin(9600);

    // Boton para frenar la lectura y envio de valor del sensor LDR
    pinMode(2, INPUT);

    if (xBotonSemaphore == NULL)
    {
        xBotonSemaphore = xSemaphoreCreateBinary(); // creamos semaforo binario
    }

    if (xSerialSemaphore == NULL)
    {
        xSerialSemaphore = xSemaphoreCreateMutex(); // creamos semaforo mutex
    }

    // seteamos las tareas ---------------------
    xTaskCreate(TaskReadLDR, "Lectura", 80, NULL, 2, NULL);
    xTaskCreate(TaskSendLDR, "Envio", 90, NULL, 2, NULL);
    xTaskCreate(TaskControlButton, "Boton", 80, NULL, 3, NULL);
    xTaskCreate(TaskStatusLed, "Led11", 70, NULL, 2, NULL);
    xTaskCreate(TaskAlarm800, "Led12", 70, NULL, 2, NULL);
    xTaskCreate(TaskReceiveSerial, "Recibir", 80, NULL, 2, NULL);

    attachInterrupt(digitalPinToInterrupt(2), buttonISR, RISING);
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
            if ((valorLDR >= 800) && (alarmaActivada == false))
            {
                alarmaActivada = true;
                // Trata de tomar el semaforo por 5 ticks
                if (xSemaphoreTake(xSerialSemaphore, (TickType_t)5) == pdTRUE)
                {
                    Serial.println("=== ALARMA ACTIVADA ===");
                    xSemaphoreGive(xSerialSemaphore); // Liberamos el semaforo
                }
            }
        }
        else
        {
            alarmaActivada = false;
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

        char bufferLDR[5];
        int tempVal = valorLDR;
        bufferLDR[0] = (tempVal / 1000) + '0';
        bufferLDR[1] = ((tempVal / 100) % 10) + '0';
        bufferLDR[2] = ((tempVal / 10) % 10) + '0';
        bufferLDR[3] = (tempVal % 10) + '0';
        bufferLDR[4] = '\0'; // Fin de cadena

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
            vTaskDelay(500 / portTICK_PERIOD_MS); // 500 ms
            digitalWrite(11, LOW);
            vTaskDelay(500 / portTICK_PERIOD_MS); // 500 ms
        }
        else
        {
            digitalWrite(11, LOW);
            vTaskDelay(50 / portTICK_PERIOD_MS); // 50 ms
        }
    }
}

void TaskAlarm800(void *pvParameters)
{
    (void)pvParameters;
    pinMode(12, OUTPUT);

    for (;;)
    {
        if (alarmaActivada == true)
        {
            digitalWrite(12, HIGH);
            vTaskDelay(50 / portTICK_PERIOD_MS);
            digitalWrite(12, LOW);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        else
        {
            digitalWrite(12, LOW);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
}
void TaskReceiveSerial(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        if (xSemaphoreTake(xSerialSemaphore, (TickType_t)10) == pdTRUE)
        {

            if (Serial.available() > 0)
            {
                char req = Serial.read();
                if (req == 'T')
                {
                    xSemaphoreGive(xBotonSemaphore);
                }
            }
            xSemaphoreGive(xSerialSemaphore);
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

    // damos el semaforo
    xSemaphoreGiveFromISR(xBotonSemaphore, &xHigherPriorityTaskWoken);

    // Si despertamos a una tarea de mayor prioridad, forzamos un cambio de contexto inmediato
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}