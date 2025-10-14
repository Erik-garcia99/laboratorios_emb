#ifndef I2C_LIB_H
#define I2C_LIB_H
#include<driver/i2c_master.h>

/**
 * @author erik garcia chavez 
 * @author erik lerma 
 * 
 * @brief general 
 * 
 * este modulo sera para un maestro y esclavo el cual sea otro ESP32 o pudee ser genreal??hmmmm, veremos que pasa 
 * 
 * 
 * @attention la libreria solo permite direcciones de 7 bits
 * 
 * < en este momento no acepta de 10 bits > 
 */



//macros 


//varibales

//funciones


/**
 * 
 * @brief inicalizacion del I2C
 * 
 * @param num_i2c -> numero del I2C a utulizar (I2C_NUM_0 - I2C_NUM_1)
 * @param pin_sda -> pin GPIO para los datos 
 * @param pin_scl -> pin GPIO para el reloj 
 * @param slave_addr -> direccion del dispositovo esclavo 
 * 
 * 
 * 
 */
void i2c_master_init(i2c_port_num_t num_i2c,gpio_num_t pin_sda, gpio_num_t pin_scl, uint8_t slave_addr, uint32_t speed);

//tareas 





#endif