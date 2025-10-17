
#include"modulos/I2C/i2c_lib.h"
// #include<driver/i2c.h>
// #include<driver/i2c_master.h>
#include<driver/i2c.h>
#include<esp_log.h>
#include <driver/i2c_types.h>


static const char *TAG ="I2C MASTER";

// i2c_master_bus_handle_t bus_handle;
// i2c_master_dev_handle_t dev_handle; 


// void i2c_master_init(i2c_port_num_t num_i2c,gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t slave_addr, uint32_t speed, i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle){


//     i2c_master_bus_config_t i2c_mst_config ={
//         .clk_source = I2C_CLK_SRC_DEFAULT,//fuente del reloj
//         .i2c_port = num_i2c,
//         .sda_io_num = pin_sda,
//         .scl_io_num = pin_scl,
//         .glitch_ignore_cnt = 7, //por default es 7
//         .flags.enable_internal_pullup = true,
//     };


//     //crear el bus maestro 

    

//     esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, bus_handle);

//     if(ret != ESP_OK){
//         ESP_LOGE(TAG, "error creando el bus: %s", esp_err_to_name(ret));
//         return;
//     }

//     ESP_LOGI(TAG, "Bus I2C maestro inicializado correctamente");

//     //agregndo el dispoisivo esclavo 
//     //
//     i2c_device_config_t slave_config ={
//         .dev_addr_length = I2C_ADDR_BIT_LEN_7,
//         .device_address= slave_addr,
//         .scl_speed_hz =speed,
//     };

    

//     ret = i2c_master_bus_add_device(*bus_handle, &slave_config, dev_handle);

//     if(ret != ESP_OK){
//         ESP_LOGE(TAG,"error al agregar el dispositivo slave %s", esp_err_to_name(ret));
//         return;
//     }

//     ESP_LOGI(TAG,"salve agregado correctamente");
// }

//el proceso de lectura solo servira aqui no sera un proceso general 



void i2c_master_init(i2c_port_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint32_t speed){

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
        ESP_LOGE(TAG, "I2C master config fallido: %s", esp_err_to_name(ret));
    }

    ret = i2c_driver_install(num_i2c, conf_master.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master-driver fallido: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Bus I2C maestro para sensor inicializado correctamente");
}


