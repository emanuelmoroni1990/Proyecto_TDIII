/*
 * ssp.c
 *
 *  Created on: 1 nov. 2017
 *  Author: Emanuel Moroni
 *  Proyecto_Manejo_SSP
 *
 */

#include "ssp.h"

//---------------------------------------------ARRAYS necesarios para inicializar la tarjeta------------------------------------------//
unsigned char CMD0[10] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95, 0xFF, 0xFF, 0xFF, 0xFF}; //GO IDLE STATE, IR A UN ESTADO DE AGUARDANDO
unsigned char CMD1[10] = {0x41, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //SEND OPERATION CONDITION
unsigned char CMD8[12] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char CMD16[10]= {0x50, 0x00, 0x00, 0x02, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //SET BLOCK LENGTH, BLOQUES DE 512K
//-------------------------------------------------------------------------------------------------------------------------------------//
unsigned char CMD24[10]= {0x58, 0x00, 0x00, 0x02, 0x00, 0xFF, 0xFF, 0xFF};
unsigned char CMD58[12]= {0x7A, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//-------------------------------------------------------------------------------------------------------------------------------------//
unsigned char CSD[8] = {0x49, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF}; //Este comando es para leer un codigo propio de la tarjeta
unsigned char INI[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char INI2[25] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char ini_r[10], cmd0_r[10], cmd8_r[12], cmd1_r[10], cmd16_r[10], cmd24_r[8], escritura_r[520], csd[8], data[25];
//-------------------------------------------------------------------------------------------------------------------------------------//

static unsigned int i = 0;

//--------------------------Funcion de Configuracion del SPI que recibe la frecuencia del Clock que quiero setear-----------------------//
void Configuracion_SPI (uint32_t frec)
{
	/* 8Bits en cada trama, Tramas de formato SPI, Phase y Poliradad 0. Todos estos seteos los hago en el CR0
	 * Master Mode, Look Back mode disable, y HABILITACIÓN DEL SSP. Todos estos seteos los hago tocando el CR1
	 * Páginas 431 y 432 de la datasheet.
	 * Serial clock rate = 100K por defecto. Lo puedo cambiar con el SetBitRate - VER
	 * Emanuel Moroni
	 */

	Chip_SSP_Init(LPC_SSP1); // En esta funcion configura 8Bits, Tramas de formato SPI, Phase y Poliradad 0, Master
	Chip_SSP_SetBitRate(LPC_SSP1, frec);
}

unsigned char inic_memoria_sd (void)
{
	unsigned char idle = 0, match = 0;

	//--------------------Tengo que dejar pasar 80 flancos del clock-------------------------------------//
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, 1); //SEL -> 6
	//GPIO_SetValue(0, SEL);
	envio_cmd(INI,ini_r,10);
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, 0); //SEL -> 6
	//GPIO_ClearValue(0, SEL);
	//---------------------------------------------------------------------------------------------------//

	//Comando 0, IR AL ESTADO DE ESPERA
	for(i=0;i<N_intentos;i++)
	{
		envio_cmd(CMD0,cmd0_r,10);
		if((cmd0_r[6]==0xFF)&&(cmd0_r[7]==0x01)){idle = 1; break;}
		envio_cmd(INI,ini_r,8); // Pongo esta linea aca porque tiene que tener un pequenio retardo en alto
	}
	if((cmd0_r[6]!=0xFF)&&(cmd0_r[7]!=0x01))
	{
		Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, 1);
		//GPIO_SetValue(0, SEL);}; // Si no entra en estado IDLE deselecciono la tarjeta
	}

	if (idle == 1)
	{
		//Comando 8, CHEQUEAR UNA CONDICIÃƒâ€œN DE MATCHEO PROPIA DE CADA SD
		for(i=0;i<N_intentos;i++)
		{
			envio_cmd(CMD8,cmd8_r,12);
			if((cmd8_r[10]==0X01)&&(cmd8_r[11]==0xAA)){match = 1; break;}
			envio_cmd(INI,ini_r,8); // Pongo esta linea aca porque tiene que tener un pequenio retardo en alto
		}

		if(match == 1)
		{
			//Comando 1, ENVIAMOS LA CONDICION DE TRABAJO
			//printf("MATCHED\n\n");
			for(i=0;i<N_intentos;i++)
			{
				envio_cmd(CMD1,cmd1_r,10);
				if(cmd1_r[7] != 0x01){break;}
				envio_cmd(INI,ini_r,8); // Pongo esta linea aca porque tiene que tener un pequeno retardo en alto
			}

			//Comando 16, SETEAMOS LA LONGITUD DEL BLOQUE QUE VAMOS A ENVIAR
			envio_cmd(CMD16,cmd16_r,10);
			envio_cmd(INI,ini_r,8);

			//---------Termine de inicializar deselecciono la tarjeta y aumento la velocidad del SPI-------------//
			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, 1);
			//GPIO_SetValue(0, SEL);
			Chip_SSP_Disable(LPC_SSP1);
			//SSP_Cmd(LPC_SSP1, DISABLE);
			Configuracion_SPI (25000000); //Dejo deseleccionada la tarjeta
			//GPIO_ClearValue(0, SEL);
			//---------------------------------------------------------------------------------------------------//

		}
		else
		{
			//printf("ERROR\n\n");
			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, 1);
			//GPIO_SetValue(0, SEL);
		}
	}
	else{}//printf("TARJETA NO EN IDLE\n\n");}
	return(match);

}

void envio_cmd (unsigned char* cmd, unsigned char *res, unsigned int length)
{
	Chip_SSP_DATA_SETUP_T aux;
	//SSP_DATA_SETUP_Type aux; //SPI_DATA_SETUP_Type inic;
	aux.length = length;
	aux.rx_data = res;
	aux.tx_data = cmd;
	aux.rx_cnt = 0;
	aux.tx_cnt = 0;

	//Chip_SSP_WriteFrames_Blocking(LPC_SSP1, cmd, length);
	Chip_SSP_RWFrames_Blocking(LPC_SSP1, &aux);
	//SSP_ReadWrite(LPC_SSP1,&aux,SPI_TRANSFER_POLLING);
}
