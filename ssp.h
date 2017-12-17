/*
 * ssp.h
 *
 *  Created on: 1 nov. 2017
 *  Author: Emanuel Moroni
 *  Proyecto_Manejo_SSP
 *
 */

#ifndef SSP_H_
#define SSP_H_

#include "board.h"
#include "chip_lpc175x_6x.h"

#define N_intentos 100

void Configuracion_SPI (uint32_t);
unsigned char inic_memoria_sd (void);
void envio_cmd (unsigned char*, unsigned char*, unsigned int);

#endif /* SSP_H_ */
