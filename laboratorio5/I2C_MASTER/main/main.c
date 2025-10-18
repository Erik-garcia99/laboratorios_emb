#include "modulos/UART/uart_lib.h"
#include"modulos/I2C/i2c_lib.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include<driver/i2c.h>
#include<driver/i2c_types.h>
#include<esp_log.h>



/**
 * aspectos de este MAIN
 * 
 * este main de lo que se encargara es de recibir los datos del ESP esclavo. 
 * 
 * 
 * 
 * 
 * 
 */


//macros 
//formato de solicitud y respuesta 

#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_REQUEST_HEADER 0x1F
#define I2C_REQUEST_CMD 0x28
#define I2C_RESPONSE_HEADER 0x2F
#define I2C_RESPONSE_CMD 0x28
#define MAX_ATTEMPTS 3
#define TIME_MS 1000
#define I2C_ESP_SLAVE 0x40
#define SPEED 100000
#define ACK_CHECK_EN 0x1
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22


//UART

#define UART_USE UART_NUM_0


//variables 

static const char* TAG = "MAIN - MASTER";

/*
    como una medicion de temepratura esta conformada por una parte entera y una parte decimal, una operacion con numeros flotantes suele ser algo complicado para un microcontrolador, lo ideal seria usar alguna notacion etc. en este caso usare una estrucutrua para rpresetnador el valor 

    el sentor LM75AB solo tiene una resolucion de 0.5 C, por lo que va de 0 o 5
*/

typedef struct{
    int8_t integer; //porque la temperatura puede ser negativa o positiva 
    uint8_t decimal; // 0 o 5 para repesentar los valores, estara escalador
}temperature_t;



//declariacion de funciones 

/**
 * @brief pide al ESP slave que quiere leer algo de el 
 */
bool request_master();

/**
 * 
 * @brief recibe la lectura y compurbea el protoclo sea correcto y inserta el valor dentro de la estrucutura
 */

bool requets_temperature(temperature_t *temperature);


//tareas 

void master_task(void *parms);






void app_main(void)
{
    init_uart(UART_USE,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE, UART_DATA_8_BITS,UART_PARITY_DISABLE,UART_STOP_BITS_1);

    i2c_master_init(I2C_MASTER_PORT,I2C_SDA_PIN, I2C_SCL_PIN, SPEED);

    xTaskCreate(master_task, "master_task", 4098,NULL, 10,NULL);

}

//solo debemos  de mostrar por pantalla lo que recibimos pero devemos de convertir el numero ingresado en su represetacion ASCII
//pero recordando que puede ser un numero negativo el resultado de la temperatura
void master_task(void *parms){
    ESP_LOGI(TAG, "I2C MASTER inicializado");


    while(1){

        bool communication_ok = false;
        temperature_t received_temp;
        for(int i = 0 ; i< MAX_ATTEMPTS; i++){
            
            ESP_LOGI(TAG, "intento %d de comunicacion ", i+1);

            
            

            //si la comunicacion del master con el slave es correcta entonces 
            if(request_master()){
                //digamos que si logo realizar la lectrua 

                if(requets_temperature(&received_temp)){

                    //si recibio la temperatura en el formato correcto
                    char buff_int[5]; //´para alamcenar hasta 5 bytes de informacion, los justos para repsetar -128\0 hasta 127\0

                    sprintf(buff_int, "%d",received_temp.integer);
                    uart_write_bytes(UART_USE,buff_int, strlen(buff_int));

                    uart_write_bytes(UART_USE,".", 1);

                    char buff_dec[5];
                    sprintf(buff_dec, "%d",received_temp.decimal);
                    uart_write_bytes(UART_USE, buff_dec, strlen(buff_dec));
                    communication_ok = true;
                    uart_write_bytes(UART_USE,"\n", 1);
                    break;
                }
                else{
                    ESP_LOGE(TAG, "error en la recepcion");
                }

            }else{

                ESP_LOGE(TAG, "intento de comunicacion # <%d> fallido...", i+1);

                continue;
            }
        }


        // Si después de 3 intentos no hubo éxito, terminamos
        if (!communication_ok) {
            const char *mess = "Comunicacion terminada, el periferico no responde\n";
            uart_write_bytes(UART_USE, mess, strlen(mess));
            break; 
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    //en el caso de falla se queda aqui infinitamente
    while(1){
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


/**
 * 
 * @brief estbalece la comunicacion enstre el slave y el master
 * 
 * le pide al slave que mande lo que leyo del sensor 
 * 
 */

bool request_master(){

    esp_err_t ret; 

    uint8_t request[2]= {I2C_REQUEST_HEADER, I2C_REQUEST_CMD};
   


    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();

    i2c_master_start(cmd_handle);

    i2c_master_write_byte(cmd_handle,(I2C_ESP_SLAVE << 1) | (I2C_MASTER_WRITE),  ACK_CHECK_EN);
    i2c_master_write(cmd_handle, request, sizeof(request), ACK_CHECK_EN);
    i2c_master_stop(cmd_handle);


    ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd_handle, pdMS_TO_TICKS(TIME_MS));
    i2c_cmd_link_delete(cmd_handle);
    
    if(ret !=ESP_OK){

        ESP_LOGE(TAG,"Fallo al enviar la solicitud: %s", esp_err_to_name(ret));
        return false;
    }
    else{
        return true;
    }

    return false;
}


bool requets_temperature(temperature_t *temperature){


    uint8_t response[4]={0};

    esp_err_t ret;

    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (I2C_ESP_SLAVE << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read(cmd_handle, response, sizeof(response), I2C_MASTER_LAST_NACK);

    i2c_master_stop(cmd_handle);

    ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd_handle, pdMS_TO_TICKS(TIME_MS));
    i2c_cmd_link_delete(cmd_handle);

    if(ret != ESP_OK){
        ESP_LOGE(TAG,"Fallo al enviar la solicitud: %s", esp_err_to_name(ret));
        return false;
    }

    //valiadamos la respuesta 

    if(response[0] != I2C_RESPONSE_HEADER || response[1] != I2C_RESPONSE_CMD){
        const char *mess = "formato de respuesta invalido\n";
         uart_write_bytes(UART_USE,mess, strlen(mess));
        return false;
    }


    int8_t temp_integer = (int8_t)response[2]; //parte entera con signo 
    uint8_t temp_decimal = (response[3] & 0x80) >>7;

    temperature->integer = temp_integer;
    temperature->decimal = temp_decimal * 5;

    return true;
}

