#ifndef UART_LIB_C
#define UART_LIB_C
#include<uart_lib.h>
#include<driver/uart.h>
#include<esp_log.h>



//una macro que detectar si el uart que se envio esta dentro del rengo acordado 
#define MAX_UART UART_NUM_MAX

static const char* TAG = "uart";




esp_err_t init_uart(uart_port_t num_uart,int pin_tx, int pin_rx, uart_word_length_t data,uart_parity_t bits_parity,uart_stop_bits_t stop_bits_f){

    ESP_LOGI(TAG,"CONFIGURANDO UART");


    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = data,
        .parity = bits_parity,
        .stop_bits = stop_bits_f,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk= UART_SCLK_DEFAULT,
    };

    if(num_uart > MAX_UART){
        //error al querer configurar un UART fura del rango 
        return ESP_FAIL;
    }

    else{
        //el numero del uart entra bein 
        ESP_ERROR_CHECK(uart_param_config(num_uart, &uart_config));
        // ESP_ERROR_CHECK(uart_driver_install(num_uart,BUFF*2, BUFF*2, ))
    }
    


}


#endif




