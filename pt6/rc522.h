#ifndef RC522_H
#define RC522_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>


// DEFINICIONES DE PINES spi


#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_RST  4


// CONFIGURACIÓN 


#define MAX_HISTORY 10      //  10 lecturas
#define SCAN_INTERVAL 2000  // 2 segundos entre lecturas

extern const char *TAG;  //  TAG


// ESTRUCTURAS DE DATOS


/**
 * @brief Estructura para almacenar datos del UID con información adicional
 */
typedef struct {
    uint8_t size;           ///  Tamaño del UID en bytes
    uint8_t uidByte[10];    ///  Bytes del UID
    uint32_t sumBytes;      ///  Suma de todos los bytes del UID
    char uidStr[25];        ///  UID en formato string
} UidData;

/**
 * @brief Estructura para gestionar el historial de lecturas
 */
typedef struct {
    UidData history[MAX_HISTORY];  ///  Buffer circular de últimas lecturas
    int currentIndex;              ///  indice actual en el buffer circular
    int totalReads;                ///  Total de lecturas realizadas max10
} HistoryManager;


// VARIABLES GLOBALES EXTERNAS

extern spi_device_handle_t spi_handle; //handler spi
extern UidData current_uid;   //uid leido al instante
extern HistoryManager uid_history; //arreglo de 10 uid
extern bool rfid_flag;  //bandera al leer rfid


// PROTOTIPOS DE FUNCIONES

// Funciones básicas SPI para rc522
esp_err_t rc522_write(uint8_t reg, uint8_t value);
uint8_t rc522_read(uint8_t reg);

// Inicialización
void rc522_init(void);

// Detección y lectura RFID
bool rc522_detect_card(void);
bool rc522_read_uid_with_history(void);

// Gestión del historial
void init_history(void);
uint32_t calculate_uid_sum(uint8_t *uidBytes, uint8_t size);
void uid_to_string(UidData *uid);
void add_to_history(UidData *uid);

// Visualización y estadísticas
void display_uid_with_sum(void);
void display_history(void);

// Tarea principal
void rfid_reader_task(void *pvParameters);

void start_rfid_reader(void);
#endif /* RC522_H */