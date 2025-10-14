#include "modulos/UART/uart_lib.h"
#include"modulos/I2C/i2c_lib.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include<driver/i2c.h>



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



void app_main(void)
{
    init_uart(0,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE, UART_DATA_8_BITS,UART_PARITY_DISABLE,UART_STOP_BITS_1);

    i2c_master_init(I2C_NUM_0,21,22, 0x40,400000);

    


}