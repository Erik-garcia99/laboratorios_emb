#include "modulos/UART/uart_lib.h"
#include"modulos/I2C/i2c_lib.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include<driver/i2c_master.h>
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

#define I2C_REQUEST_HEADER 0x1F
#define I2C_REQUEST_CMD 0x28
#define I2C_RESPONSE_HEADER 0x2F
#define I2C_RESPONSE_CMD 0x28
#define MAX_ATTEMPTS 3
#define TIME_MS 500
#define I2C_ESP_SLAVE 0x40
#define SPEED 400000


//UART

#define UART_USE UART_NUM_0


//variables 
i2c_master_bus_handle_t bus_handle= NULL;
i2c_master_dev_handle_t dev_handle = NULL; 
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

bool requets_temperature(temperature_t *temperature);

//tareas 

void master_task(void *parms);






void app_main(void)
{
    init_uart(UART_USE,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE, UART_DATA_8_BITS,UART_PARITY_DISABLE,UART_STOP_BITS_1);

    i2c_master_init(I2C_NUM_0,21,22, I2C_ESP_SLAVE,SPEED, &bus_handle, &dev_handle);


    xTaskCreate(master_task, "master_task", 4098,NULL, 10,NULL);

}

//solo debemos  de mostrar por pantalla lo que recibimos pero devemos de convertir el numero ingresado en su represetacion ASCII
//pero recordando que puede ser un numero negativo el resultado de la temperatura
void master_task(void *parms){

    //veriticicamos que este inciados el I2C
    if(bus_handle == NULL || dev_handle == NULL){
        ESP_LOGE(TAG, "error en la incialziacion de I2C");
        while(1) vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "I2C MASTER inicializado");

    while(1){

        temperature_t tmp;
        if(requets_temperature(&tmp)){
            //ahora lo sigueinte sera desplegar por UART 
            //la funcion que nos ayudara a converir en numero en su representacion ASCII respetando el signo es sprintf, lo que hace es imprimir el numero dentro de un buffer 

            char buff_int[5]; //´para alamcenar hasta 5 bytes de informacion, los justos para repsetar -128\0 hasta 127\0

            sprintf(buff_int, "%d",tmp.integer);
            uart_write_bytes(UART_USE,buff_int, strlen(buff_int));

            uart_write_bytes(UART_USE, (const char*)'.', 1);

            char buff_dec[5];
            sprintf(buff_dec, "%d",tmp.decimal);
            uart_write_bytes(UART_USE, buff_dec, strlen(buff_dec));

            uart_write_bytes(UART_USE,(const char*) '\n', 1);
        }
        else{

            const char *mess = "Comunicacion terminada, el periferico no responde\n";
            uart_write_bytes(UART_USE, mess, strlen(mess));
            break;
        }

    }

    //en el caso de falla se queda aqui infinitamente
    while(1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


//funcion que recibe la lectura 

bool requets_temperature(temperature_t *temperature){


    uint8_t request[2]= {I2C_REQUEST_HEADER, I2C_REQUEST_CMD};
    uint8_t response[4]={0};

    for(int i=0; i< MAX_ATTEMPTS; i++){

        ESP_LOGI(TAG, "intento %d de comunicacion ", i+1);

        //enviando solicitud de lectura 

        /**
         * 
         * el primre parametro 
         * @param dev_handle -> es el tiene la direccion a la cual me quiero comunicar
         * despues lo que se va a escribir en el bus, 
         * el tamanio de este  
         * 
         */
        esp_err_t ret= i2c_master_transmit(dev_handle, request,sizeof(request),TIME_MS/portTICK_PERIOD_MS);

        if(ret != ESP_OK){
            ESP_LOGE(TAG, "error al evniar la solicitud: %s", esp_err_to_name(ret));
            //el continue porque este va a intentar 3 veces cada 500 minusegundos
            continue;
        }

        //recibir la respuesta 

        // ret i2c_master_receive(dev_handle, response, sizeof(response), TIME_MS / portTICK_PERIOD_MS);
        ret = i2c_master_receive(dev_handle, response, sizeof(response), TIME_MS / portTICK_PERIOD_MS);

        if(ret != ESP_OK){
             ESP_LOGE(TAG, "error al recibir la respuesta: %s", esp_err_to_name(ret));
            //el continue porque este va a intentar 3 veces cada 500 minusegundos
            continue;
        }


        //verificando el fromato de la respuesta 

        if(response[0] != I2C_RESPONSE_HEADER || response[1] != I2C_RESPONSE_CMD){
            const char *mess = "formato de respuesta invalido\n";
            uart_write_bytes(UART_USE,mess, sizeof(mess));
            
            continue;
        }


        int16_t temp = (response[2] << 8) | response[3];

        int8_t temp_integer = (int8_t)response[2]; //parte entera con signo 
        uint8_t temp_decimal = (response[3] & 0x80) >>7;

        temperature->integer = temp_integer;
        temperature->decimal = temp_decimal * 5;

        return true;
    }

    return false; //no se logro la comunicacion 

}
