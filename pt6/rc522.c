#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <inttypes.h>

// Pines SPI
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_RST  4

// Configuración del sistema
#define MAX_HISTORY 10      // Últimas 10 lecturas
#define SCAN_INTERVAL 2000  // 2 segundos entre lecturas

static const char *TAG = "RC522";

//bufer para comunicacion con rc522
static uint8_t tx_buffer[2] = {0};
static uint8_t rx_buffer[2] = {0};
static spi_device_handle_t spi_handle;

// Estructura mejorada para UID con suma y timestamp
typedef struct {
    uint8_t size;
    uint8_t uidByte[10];
    uint32_t sumBytes;      // Suma de todos los bytes del UId
    char uidStr[25];        // UID en formato string
} UidData;

bool rfid_flag=false;

// Estructura para el historial 
typedef struct {
    UidData history[MAX_HISTORY];  // Últimas 10 lecturas
    int currentIndex;              // Índice actual del historial
    int totalReads;                // Total de lecturas realizadas
} HistoryManager;

UidData current_uid;
HistoryManager uid_history;


// FUNCIONES BÁSICAS SPI
// ============================================================================

// Función para escribir registros
esp_err_t rc522_write(uint8_t reg, uint8_t value) {
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer
    };
    
    tx_buffer[0] = (reg << 1) & 0x7E;
    tx_buffer[1] = value;
    
    return spi_device_transmit(spi_handle, &t);
}

// Función para leer registros
uint8_t rc522_read(uint8_t reg) {
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer
    };
    
    tx_buffer[0] = ((reg << 1) & 0x7E) | 0x80;
    tx_buffer[1] = 0x00;
    
    spi_device_transmit(spi_handle, &t);
    return rx_buffer[1];
}


// INICIALIZACIÓN RC522
// ============================================================================

// Inicializar RC522
void rc522_init(void) {
    ESP_LOGI(TAG, "Inicializando RC522...");
    
    // Configurar pin RST
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_RST),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Reset
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Soft reset
    rc522_write(0x01, 0x0F);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Configuración
    rc522_write(0x2A, 0x8D); // TModeReg
    rc522_write(0x2B, 0x3E); // TPrescalerReg
    rc522_write(0x2C, 30);   // TReloadRegL
    rc522_write(0x2D, 0);    // TReloadRegH
    rc522_write(0x15, 0x40); // TxASKReg
    rc522_write(0x11, 0x3D); // ModeReg
    
    // Activar antena
    rc522_write(0x14, 0x83); // TxControlReg
    
    ESP_LOGI(TAG, "RC522 inicializado - Versión: 0x%02X", rc522_read(0x37));
}


// DETECCIÓN Y LECTURA RFID
// ============================================================================

// Detectar tarjeta
bool rc522_detect_card(void) {
    // Limpiar registros
    rc522_write(0x0E, 0x80); // CollReg
    rc522_write(0x04, 0x7F); // ComIrqReg
    
    // Configurar para envío
    rc522_write(0x0D, 0x07); // BitFramingReg
    rc522_write(0x01, 0x00); // CommandReg - Idle
    
    // Limpiar FIFO
    rc522_write(0x0A, 0x80); // FIFOLevelReg - FlushBuffer
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Enviar comando REQA
    rc522_write(0x09, 0x26); // FIFODataReg - REQA
    rc522_write(0x01, 0x0C); // CommandReg - Transceive
    rc522_write(0x0D, 0x87); // BitFramingReg - StartSend
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Verificar respuesta
    uint8_t com_irq = rc522_read(0x04);
    uint8_t fifo_level = rc522_read(0x0A);
    
    if ((com_irq & 0x30) && fifo_level > 0) {
        uint8_t response = rc522_read(0x09);
        return (response == 0x04 || response == 0x44);
    }
    
    return false;
}


// GESTIÓN DEL HISTORIAL
// ============================================================================

// Inicializar el historial (bufer circular)
void init_history(void) {
    memset(&uid_history, 0, sizeof(uid_history));
    uid_history.currentIndex = 0;
    uid_history.totalReads = 0;
}

// Calcular suma de bytes del UID
uint32_t calculate_uid_sum(uint8_t *uidBytes, uint8_t size) {
    uint32_t sum = 0;
    for (int i = 0; i < size; i++) {
        sum += uidBytes[i];
    }
    return sum;
}

// Convertir UID a string
void uid_to_string(UidData *uid) {
    memset(uid->uidStr, 0, sizeof(uid->uidStr));
    char temp[5];
    
    for (int i = 0; i < uid->size; i++) {
        if (i > 0) strcat(uid->uidStr, ":");
        sprintf(temp, "%02X", uid->uidByte[i]);
        strcat(uid->uidStr, temp);
    }
}

// Agregar lectura al historial (bufer circular)
void add_to_history(UidData *uid) {
    
    // Calcular suma de bytes
    uid->sumBytes = calculate_uid_sum(uid->uidByte, uid->size);
    
    // Convertir a string
    uid_to_string(uid);
    
    // Agregar al historial (buffer circular)
    uid_history.history[uid_history.currentIndex] = *uid;
    uid_history.currentIndex = (uid_history.currentIndex + 1) % MAX_HISTORY;
    uid_history.totalReads++;
    
    ESP_LOGI(TAG, "Lectura %d agregada al historial", uid_history.totalReads);
}

// Leer UID y agregar al historial
bool rc522_read_uid_with_history(void) {
    memset(&current_uid, 0, sizeof(current_uid));
    
    // Limpiar todo
    rc522_write(0x0E, 0x80);
    rc522_write(0x04, 0x7F);
    rc522_write(0x0A, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Enviar comando anticolisión
    uint8_t cmd[] = {0x93, 0x20};  // SEL_CL1 + NVB
    for (int i = 0; i < 2; i++) {
        rc522_write(0x09, cmd[i]);
    }
    
    // Ejecutar transceive
    rc522_write(0x01, 0x0C);  // CommandReg - Transceive
    rc522_write(0x0D, 0x80);  // BitFramingReg - StartSend
    
    // Esperar respuesta UID
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Leer UID
    uint8_t fifo_level = rc522_read(0x0A);
    if (fifo_level >= 5) {
        for (int i = 0; i < 4; i++) {
            current_uid.uidByte[i] = rc522_read(0x09);
        }
        current_uid.size = 4;
        
        // Agregar al historial
        add_to_history(&current_uid);
        return true;
    }
    return false;
}


// MOSTRAR ESTADISTICAS
// ============================================================================

// Mostrar UID actual con suma (no se usa ,prueba con printf)
void display_uid_with_sum(void) {
    printf("\n");
    printf("┌─────────────────────────────┐\n");
    printf("│ 🎫 TARJETA RFID DETECTADA   │\n");
    printf("├─────────────────────────────┤\n");
    printf("│ UID: %-19s    │\n", current_uid.uidStr);
    printf("│ Suma bytes: %-14" PRIu32 "  │\n", current_uid.sumBytes);
    printf("└─────────────────────────────┘\n");
    printf("\n");
}

// Mostrar historial completo (nose usa , prueba con printf)
void display_history(void) {
    printf("\n");
    printf("┌──────────────────────────────────┐\n");   
    int historial_count = (uid_history.totalReads < MAX_HISTORY) ? uid_history.totalReads : MAX_HISTORY;
    printf("│ 📊 HISTORIAL DE LECTURAS RFID    │\n" );
    printf("├──────────────────────────────────┤\n");
    if (uid_history.totalReads == 0) {
        printf("│ No hay lecturas en el historial  │\n");
        printf("└───────────────────────────────┘\n");
        return;
    }
    
    // Calcular índice de inicio (para buffer circular)
    int startIndex = 0;
    if (uid_history.totalReads > MAX_HISTORY) {
        startIndex = uid_history.currentIndex;
    }
    
    for (int i = 0; i < historial_count; i++) {
        int idx = (startIndex + i) % MAX_HISTORY;
        UidData *uid = &uid_history.history[idx];
        
        printf("│ %2d. UID: %-12s Suma: %-4" PRIu32 " │\n", 
               i + 1, uid->uidStr, uid->sumBytes);
    }
    
    printf("└──────────────────────────────────┘\n");
    printf("\n");
}


// TAREA PRINCIPAL (LEE UID CADA 2 SEG)
// ============================================================================

void rfid_reader_task(void *pvParameters) {
    bool last_card_state = false;
    uint32_t last_scan_time = 0;
    int successful_reads = 0;
    int total_scans = 0;
    
    ESP_LOGI(TAG, "🔍 Lector RFID iniciado - Escaneo cada %d ms", SCAN_INTERVAL);
    
    while (1) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Escanear cada 2 segundos
        if (current_time - last_scan_time >= SCAN_INTERVAL) {
            bool card_present = rc522_detect_card();
            
            if (card_present ) {
                ESP_LOGI(TAG, "🎯 Tarjeta detectada (Scan #%d) - Intentando lectura...", total_scans);
                
                if (rc522_read_uid_with_history()) 
                    rfid_flag=true;
                else 
                    ESP_LOGI(TAG, "❌ Tarjeta detectada pero no se pudo leer UID");
                

                while(rc522_detect_card())
                {
                    vTaskDelay(pdMS_TO_TICKS(10)); 
                }

            } else if (!card_present) {
                ESP_LOGI(TAG, "📭 No hubo escaneo");
            }
            
            last_scan_time = current_time;
            
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Pequeño delay para no saturar CPU
    }
}

void start_rfid_reader(void) {
    // Inicializar SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle));
    
    // Inicializar RC522
    rc522_init();
    
    // Inicializar historial
    init_history();
    
    // Iniciar tarea
    xTaskCreate(rfid_reader_task, "rfid_reader", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "=== LECTOR RFID INICIADO ===");
}

