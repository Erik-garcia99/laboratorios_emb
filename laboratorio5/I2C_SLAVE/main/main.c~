#include <stdio.h>
#include "modulos/I2C/i2c_lib.h"
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<esp_log.h>
#include<driver/i2c.h>
#include <driver/i2c_types.h>


//macros 

//configuracion como ESP slave

#define I2C_PORT_SLAVE I2C_NUM_0
#define I2C_SLAVE_SDA_IO 21
#define I2C_SLAVE_SCL_IO 22
#define I2C_SLAVE_ADDR 0x40
#define I2C_SLAVE_RX_BUF_LEN 256
#define I2C_SLAVE_TX_BUF_LEN 256

//configruacion ESP MASTER - SENSOR LM75ab 

#define I2C_SEN_PORT I2C_NUM_1
#define I2C_SEN_SDA_IO 19
#define I2C_SEN_SCL_IO 18
#define SPEED 100000
#define LM75_SEN_ADDR 0x48 //esta direccion esta por default es lo que dice la documnetacion 

//protocolo de comunicacion 
#define I2C_REQUEST_HEADER 0x1F
#define I2C_REQUEST_CMD 0x28
#define I2C_RESPONSE_HEADER 0x2F
#define I2C_RESPONSE_CMD 0x28

//varibales 

// i2c_slave_dev_handle_t slave_handle;
// i2c_master_bus_handle_t sensor_bus_handle;
// i2c_master_dev_handle_t sensor_handle;

static const char *TAG ="MAIN - SLAVE";


//funciones 

bool read_sens_tmp(int16_t *tmp);

//tareas 

void slave_task(void *parms);


void app_main(void)
{

    //ESTE SE COMUNICA CON EL SENSOR 
    // i2c_master_init(I2C_SEN_PORT, I2C_SEN_SDA_IO, I2C_SEN_SCL_IO, LM75_SEN_ADDR, SPEED, &sensor_bus_handle, &sensor_handle);

    i2c_master_init(I2C_SEN_PORT, I2C_SEN_SDA_IO, I2C_SEN_SCL_IO, SPEED);


    //ESTE SE COMUNICA CON EL OTRO ESP
    //i2C_slave_init(I2C_PORT_SLAVE, I2C_SLAVE_SDA_IO, I2C_SLAVE_SCL_IO, I2C_SLAVE_ADDR, &slave_handle, I2C_SLAVE_TX_BUF_LEN, I2C_SLAVE_RX_BUF_LEN);

    // LÍNEA NUEVA:
    i2C_slave_init(I2C_PORT_SLAVE, I2C_SLAVE_SDA_IO, I2C_SLAVE_SCL_IO, I2C_SLAVE_ADDR);

    xTaskCreate(slave_task,"slave_task", 4098, NULL, 10, NULL);

}

/*
bool read_sens_tmp(int16_t *tmp){
    
    uint8_t reg_addr = 0x00;
    uint8_t data[2]= {0};

    esp_err_t ret = i2c_master_transmit(sensor_handle, &reg_addr, 1, 1000/portMAX_DELAY);

    if(ret != ESP_OK){
         ESP_LOGE(TAG, "error al seleccionar registro: %s", esp_err_to_name(ret));
        return false;
    }

    ret = i2c_master_receive(sensor_handle, data, 2, 1000/portTICK_PERIOD_MS);
    
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "error en la lectura al sensor: %s", esp_err_to_name(ret));
        return false;
    }

    *tmp = (data[0] << 8) | data[1];

    return true;
}*/

// REEMPLAZA tu función read_sens_tmp completa con esta:
bool read_sens_tmp(int16_t *tmp) {
    uint8_t reg_addr = 0x00;
    uint8_t data[2] = {0};

    // El driver antiguo usa "command links"
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // Escribir la dirección del sensor y el registro que queremos leer
    i2c_master_write_byte(cmd, (LM75_SEN_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    // Reiniciar para leer
    i2c_master_start(cmd);
    // Escribir la dirección del sensor y solicitar lectura
    i2c_master_write_byte(cmd, (LM75_SEN_ADDR << 1) | I2C_MASTER_READ, true);
    // Leer los 2 bytes de datos
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_SEN_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "error en la lectura al sensor: %s", esp_err_to_name(ret));
        return false;
    }

    *tmp = (data[0] << 8) | data[1];
    return true;
}


void slave_task(void *parms){

    ESP_LOGI(TAG , "TASK SLAVE INIT");

    uint8_t request[2] = {0};
    uint8_t response[4] = {0};
    
    while(1){

        int size_read = i2c_slave_read_buffer(I2C_PORT_SLAVE, request, 2, portMAX_DELAY);

        if (size_read != 2) {
            ESP_LOGE(TAG, "Error al leer el buffer del master");
            continue;
        }
        else{
            if(request[0] == I2C_REQUEST_HEADER && request[1] == I2C_REQUEST_CMD){
                ESP_LOGI(TAG, "solcitud recibida correctamente");

                int16_t raw_tmp;

                if(read_sens_tmp(&raw_tmp)){
                    response[0] = I2C_RESPONSE_HEADER;
                    response[1] = I2C_RESPONSE_CMD;
                    response[2] = (raw_tmp >> 8) & 0xFF;
                    response[3] = raw_tmp & 0xFF;

                    //enviamos al ESP 

                    i2c_slave_write_buffer(I2C_PORT_SLAVE, response, 4, portMAX_DELAY);
                }else{
                    ESP_LOGE(TAG, "error al leer el sensor");
                }
            }
            else{
                ESP_LOGE(TAG,"error en la recepcion");
            }
        }
    }
}
