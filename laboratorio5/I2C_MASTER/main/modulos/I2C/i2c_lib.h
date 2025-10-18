#ifndef I2C_LIB_H
#define I2C_LIB_H

#include<driver/i2c.h>




/**
 * @author erik garcia chavez 
 * @author erik lerma 
 * 
 */


//funciones

/**
 * 
 * @brief inicalizacion del I2C
 * 
 * @param num_i2c -> numero del I2C a utulizar (I2C_NUM_0 - I2C_NUM_1)
 * @param pin_sda -> pin GPIO para los datos 
 * @param pin_scl -> pin GPIO para el reloj 
 * @param speed -> velocidad de la comunicacion
 * 
 */


void i2c_master_init(i2c_port_t num_i2c, gpio_num_t pin_sda, gpio_num_t pin_scl, uint32_t speed);


//tareas 





#endif