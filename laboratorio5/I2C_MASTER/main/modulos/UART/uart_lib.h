#ifndef UART_LIB_H
#define UART_LIB_H

//macros
#define BUFF 1024
//varibales

//definiciones de funciones 


/**
 * 
 * @brief define el UART, todos sus parametros 
 * 
 * @param num_uart el numero del uart 
 * @param data el tamanio de bits del frame
 * @param bits_parity habilitado o deshabitiadp el bit de paridad
 * @param stop_bits_f la cantidad de stop bits que delimietan el final del frame
 * 
 * @return
 *  ESP_OK -> si la configuracion se realizo con exito 
 *  ESP_FAIL -> si ocurrio un error que algun parametro no esta bien 
 * 
 */
esp_err_t init_uart(uart_port_t num_uart,int pin_tx, int pin_rx, uart_word_length_t data,uart_parity_t bits_parity,uart_stop_bits_t stop_bits_f);



#endif