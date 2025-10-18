#include <stdio.h>             //libreria I/O
#include <string.h>             //libreria comandos string
#include "freertos/FreeRTOS.h"   //libreria so
#include "freertos/task.h"      //libreria taras
#include "driver/spi_master.h" //libreria spi
#include "driver/gpio.h"        //libreria gpio
#include "driver/uart.h"       //libreria uart
#include "esp_timer.h"         //libreria timer
#include "rc522.h"

//GPIO 4 PINS
#define BUTTON_PIN 12

//UART 0 PINS
#define UART_TX 1   // TX0
#define UART_RX 3   // RX0
#define BUF_SIZE 1024  //BUFFER UART
#define UART_NUM UART_NUM_0 //UART PUERTO 0

//COLAS
QueueHandle_t button_queue; //cola boton
QueueHandle_t button_time_queue; //cola tipo de presion

//DEBOUNCE FUNCTION
bool debounce(uint32_t debounce_ms)
{
    static int64_t lastTime_us = 0;  
    int64_t actualTime_us = esp_timer_get_time();  //toma tiempo actual en us

    if ((actualTime_us - lastTime_us) >= ((int64_t)debounce_ms * 1000)) //ms a us
    {
        lastTime_us = actualTime_us;  //tiempo actual es al anterior
        return true;           
    }
    return false;              
}

// ISR + DEBOUNCE
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint8_t val = 1;
    int pinNumber=(int)arg;
    if (debounce(50)) {
    xQueueSendFromISR(button_queue, &val, NULL);
    }
}

//MOSTRAR MEDICIONES
void mostrarMediciones()
{
    char msg[128];
    int len;
  
    len = snprintf(msg, sizeof(msg), "\n");
    uart_write_bytes(UART_NUM_0, msg, len);

    len = snprintf(msg, sizeof(msg), "┌──────────────────────────────────┐\n");
    uart_write_bytes(UART_NUM_0, msg, len);

    len = snprintf(msg, sizeof(msg), "│ 📊 HISTORIAL DE LECTURAS RFID    │\n");
    uart_write_bytes(UART_NUM_0, msg, len);    

    len = snprintf(msg, sizeof(msg), "├──────────────────────────────────┤\n");
    uart_write_bytes(UART_NUM_0, msg, len);    
  
    if (uid_history.totalReads == 0) {

        len = snprintf(msg, sizeof(msg), "│ No hay lecturas en el historial  │\n");
        uart_write_bytes(UART_NUM_0, msg, len); 

        len = snprintf(msg, sizeof(msg), "├──────────────────────────────────┤\n");
        uart_write_bytes(UART_NUM_0, msg, len); 
        return;
    }
    
     int historial_count = (uid_history.totalReads < MAX_HISTORY) ? uid_history.totalReads : MAX_HISTORY;
    // Calcular índice de inicio (para buffer circular)
    int startIndex = 0;
    if (uid_history.totalReads > MAX_HISTORY) {
        startIndex = uid_history.currentIndex;
    }
    
    for (int i = 0; i < historial_count; i++) {
        int idx = (startIndex + i) % MAX_HISTORY;
        UidData *uid = &uid_history.history[idx];
        
        len = snprintf(msg, sizeof(msg), "│ %2d. UID: %-12s Suma: %-4" PRIu32 " │\n", 
                i + 1, uid->uidStr, uid->sumBytes);
        uart_write_bytes(UART_NUM_0, msg, len);               
    }
    

    len = snprintf(msg, sizeof(msg), "└──────────────────────────────────┘\n");
    uart_write_bytes(UART_NUM_0, msg, len); 

    len = snprintf(msg, sizeof(msg), "\n");
    uart_write_bytes(UART_NUM_0, msg, len);    

}

//IMPRIMIR RFID
void task_rfid_print()
{
    int len;
    char msg[128];

    while(1)
    {
        if(rfid_flag)
        {

            len = snprintf(msg, sizeof(msg), "\n");
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "┌─────────────────────────────┐\n");
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "│ 🎫 TARJETA RFID DETECTADA   │\n");
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "├─────────────────────────────┤\n");
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "│ UID: %-19s    │\n", current_uid.uidStr);
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "│ Suma bytes: %-14" PRIu32 "  │\n", current_uid.sumBytes);
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "└─────────────────────────────┘\n");
            uart_write_bytes(UART_NUM_0, msg, len);

            len = snprintf(msg, sizeof(msg), "\n");
            uart_write_bytes(UART_NUM_0, msg, len); 

            rfid_flag=false; 

                                             
        }
         vTaskDelay(pdMS_TO_TICKS(50));
    }
}

//MOSTRAR MEDIA Y MEDIANA
void mostrar_media_mediana() 
{
    char msg[64];
    int suma = 0;

    // --- Calcular media ---
    float media ;
    float mediana;

    int historial_count = (uid_history.totalReads < MAX_HISTORY) ? uid_history.totalReads : MAX_HISTORY;
    // Calcular índice de inicio (para buffer circular)
    int startIndex = 0;
    if (uid_history.totalReads > MAX_HISTORY) {
        startIndex = uid_history.currentIndex;
    }
    
    suma=0;
    int datos2[historial_count];
    for (int i = 0; i < historial_count; i++) {
        int idx = (startIndex + i) % MAX_HISTORY;
        UidData *uid = &uid_history.history[idx];

        suma+=uid->sumBytes;
        datos2[i]=uid->sumBytes;
    }

    
    // Ordenamiento simple (burbuja)
    for (int i = 0; i < historial_count - 1; i++) {
        for (int j = i + 1; j < historial_count; j++) {
            if (datos2[j] < datos2[i]) {
                int temp = datos2[i];
                datos2[i] = datos2[j];
                datos2[j] = temp;
            }
        }
    }

    media = (float)suma / historial_count;
    int n=historial_count;

    if (n % 2 == 0)
        mediana = (datos2[n / 2 - 1] + datos2[n / 2]) / 2.0;
    else
        mediana = datos2[n / 2];

    // --- Desplegar resultados ---
    int len = snprintf(msg, sizeof(msg),"\r\nMedia: %.2f\r\nMediana: %.2f\r\n", media, mediana);
    uart_write_bytes(UART_NUM, msg, len);
    
}

// Tarea que mide duración de pulsación
void task_button(void* arg) {
    uint8_t val;
    int64_t press_start = 0;
    uint8_t dobleClick=0;
    int64_t press_end;
    int64_t duration_ms;
    int64_t last_time;
    int64_t now;

    while (1) {
        duration_ms=0;
        vTaskDelay(pdMS_TO_TICKS(50));
        if (xQueueReceive(button_queue, &val, portMAX_DELAY)) {
            // botón presionado, guardar tiempo de inicio
            vTaskDelay(pdMS_TO_TICKS(50));
            if(gpio_get_level(BUTTON_PIN))
            {

                press_start = esp_timer_get_time();

                // esperar a que se suelte
                while (gpio_get_level(BUTTON_PIN) == 1) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                now=esp_timer_get_time();
                while(duration_ms<1500)
                {
                    if(gpio_get_level(BUTTON_PIN) == 1)
                    {
                        vTaskDelay(pdMS_TO_TICKS(50));
                        if(gpio_get_level(BUTTON_PIN) == 1)
                        {
                            dobleClick=1;
                        }
                    
                    }
                    last_time =esp_timer_get_time();
                    duration_ms = (last_time - now) / 1000;
                }
            

                
                if(dobleClick)
                {
                    val=1;
                    xQueueSend(button_time_queue, &val, portMAX_DELAY);
                }
                else
                {
                    val=0;
                    xQueueSend(button_time_queue, &val, portMAX_DELAY);
                }  

                dobleClick=0;

                // imprimir 1 o 0 y duración
                char buffer[64];
                int len = snprintf(buffer, sizeof(buffer),"Botón presionado, duración: %lld ms\r\n",duration_ms);
                uart_write_bytes(UART_NUM, buffer, len);


            }

        }
    }
}

// --- Inicialización de UART0 ---
void uart0_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_driver_install(UART_NUM_0, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// --- Inicialización del botón en GPIO0 ---
void gpio_button_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,    // desactivamos pull-up
        .pull_down_en = GPIO_PULLDOWN_ENABLE, // activamos pull-down
        .intr_type = GPIO_INTR_POSEDGE  // detecta flanco descendente
    };
    gpio_config(&io_conf);

    // Instalar ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void*)BUTTON_PIN);
}

// --- Tarea que imprime 
void task_print(void *pvParameters) {
    int counter = 0;
    char buffer[32];
    uint8_t val;
    while (1) {
        if(xQueueReceive(button_time_queue, &val,pdMS_TO_TICKS(100)))
        {
            if(val==0)
            {
            int len = snprintf(buffer, sizeof(buffer), "Click");
            uart_write_bytes(UART_NUM_0, buffer, len);
            vTaskDelay(pdMS_TO_TICKS(50));
            mostrarMediciones();
            }
            else if(val==1)
            {
            int len = snprintf(buffer, sizeof(buffer), "Doble Click");
            uart_write_bytes(UART_NUM_0, buffer, len);
            mostrar_media_mediana();
            }
        }

    }
}

// --- MAIN ---
void app_main(void) {
    uart0_init();         // Inicializa UART0
    gpio_button_init();   // Inicializa botón en GPIO0
    button_queue = xQueueCreate(10, sizeof(uint8_t));
    button_time_queue = xQueueCreate(10, sizeof(uint8_t));

    xTaskCreate(task_print, "PrintTask", 2048, NULL, 5, NULL);  //imrpime dependiendo boton
    xTaskCreate(task_button, "ButtonTask", 2048, NULL, 5, NULL); //manejador de boton
    xTaskCreate(task_rfid_print, "RFIDPrint",2048,NULL,5,NULL); //imprimr rfid cuando detecta

    start_rfid_reader(); //iniciacion spi rc522 y tarea de lectura
}
