
#include"modulos/I2C/i2c_lib.h"
// #include<driver/i2c.h>
// #include<driver/i2c_master.h>
// #include<driver/i2c_slave.h>
#include<esp_log.h>
#include<driver/i2c.h>
#include <driver/i2c_types.h>

static const char *TAG ="I2C SLAVE";

// i2c_master_bus_handle_t bus_handle;
// i2c_master_dev_handle_t dev_handle; 

#define I2C_SLAVE_RX_BUF_LEN 256
#define I2C_SLAVE_TX_BUF_LEN 256

/*
void i2c_master_init(i2c_port_num_t num_i2c,gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t slave_addr, uint32_t speed, i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle){


    i2c_master_bus_config_t i2c_mst_config ={
        .clk_source = I2C_CLK_SRC_DEFAULT,//fuente del reloj
        .i2c_port = num_i2c,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .glitch_ignore_cnt = 7, //por default es 7
        .flags.enable_internal_pullup = true,
    };


    //crear el bus maestro 

    

    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, bus_handle);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "error creando el bus: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Bus I2C maestro inicializado correctamente");

    //agregndo el dispoisivo esclavo 
    //
    i2c_device_config_t slave_config ={
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address= slave_addr,
        .scl_speed_hz =speed,
    };

    

    ret = i2c_master_bus_add_device(*bus_handle, &slave_config, dev_handle);

    if(ret != ESP_OK){
        ESP_LOGE(TAG,"error al agregar el dispositivo slave %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG,"sensor LM75ab iniciado");
}
*/

void i2c_master_init(i2c_port_num_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint32_t speed) {

    i2c_config_t conf_master = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = speed,
    };

    esp_err_t ret = i2c_param_config(num_i2c, &conf_master);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master param config failed: %s", esp_err_to_name(ret));
    }

    ret = i2c_driver_install(num_i2c, conf_master.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master driver install failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Bus I2C maestro para sensor inicializado correctamente");
}

//configurar el I2C slave
/**
 * en este momneto el inicar le I2C no sabe si ya se quiero registrar el I2C o los pines de 
 * master como slave, por lo que como yo lo estoy hacinedo no hay pedo porque no estoy pendejo 
 * pero deberia de tener una forma de mandar a llamar a algo para verificar que x I2C o x pines
 * no se esten usando 
 */

 /*
void i2C_slave_init(i2c_port_num_t num_i2c,gpio_num_t pin_sda, gpio_num_t pin_scl,uint8_t slave_addr, i2c_slave_dev_handle_t *slave_handle ,uint32_t buff_tx, uint32_t buff_rx){

    i2c_slave_config_t i2c_slv_config ={
        .i2c_port = num_i2c,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .scl_io_num = pin_scl,
        .sda_io_num= pin_sda,
        .slave_addr = slave_addr,
        .send_buf_depth= buff_tx,
        // .receive_buf_depth = buff_rx,
        // .rx_buf_depth = buff_rx,
        //.receive_buf_depth = buff_rx,
        .addr_bit_len = I2C_ADDR_BIT_7,
    };

    esp_err_t ret = i2c_new_slave_device(&i2c_slv_config, slave_handle);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "error al crear el bus : %s\n", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "I2C esclavo inicializado en dirección 0x%02X", slave_addr);
}
*/

// REEMPLAZA la función i2C_slave_init en i2c_lib_slave.c con ESTA:

#include "driver/i2c.h" // Asegúrate que este include esté al principio del archivo

void i2C_slave_init(i2c_port_num_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t slave_addr){
    
    i2c_config_t conf_slave = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = pin_sda,
        .scl_io_num = pin_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = slave_addr,
    };

    esp_err_t ret = i2c_param_config(num_i2c, &conf_slave);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C slave param config failed: %s", esp_err_to_name(ret));
    }

    ret = i2c_driver_install(num_i2c, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C slave driver install failed: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "I2C esclavo inicializado en dirección 0x%02X", slave_addr);
}


