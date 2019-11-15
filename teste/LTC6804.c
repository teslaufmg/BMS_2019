/****************************************************************/
/**					Team Formula Tesla UFMG - 2019				*/
/** Microcontroller: STM32F103XXXX								*/
/** Compiler: AC6 - STM worbench								*/
/** Author: Henrique, Rodolfo, Heuller, Wellington				*/
/** License: Free - Open Source									*/
/** 															*/
/****************************************************************/

#include <LTC6804.h>
#include <stdarg.h>
#include <math.h>
#include "dwt_stm32_delay.h"
#include "defines.h"

extern SPI_HandleTypeDef hspi1;

#define BYTESWAP(word) ((word >> 8) + (word << 8))

uint16_t PEC_TABLE[256] = {
		0x0000, 0xC599, 0xCEAB, 0x0B32, 0xD8CF, 0x1D56, 0x1664, 0xD3FD, 0xF407, 0x319E, 0x3AAC, 0xFF35, 0x2CC8, 0xE951, 0xE263, 0x27FA, 0xAD97, 0x680E, 0x633C, 0xA6A5, 0x7558, 0xB0C1, 0xBBF3, 0x7E6A, 0x5990, 0x9C09, 0x973B, 0x52A2, 0x815F, 0x44C6, 0x4FF4, 0x8A6D, 0x5B2E, 0x9EB7, 0x9585, 0x501C, 0x83E1, 0x4678, 0x4D4A, 0x88D3, 0xAF29, 0x6AB0, 0x6182, 0xA41B, 0x77E6, 0xB27F, 0xB94D, 0x7CD4, 0xF6B9, 0x3320, 0x3812, 0xFD8B, 0x2E76, 0xEBEF, 0xE0DD, 0x2544, 0x02BE, 0xC727, 0xCC15, 0x098C, 0xDA71, 0x1FE8, 0x14DA, 0xD143, 0xF3C5, 0x365C, 0x3D6E, 0xF8F7, 0x2B0A, 0xEE93, 0xE5A1, 0x2038, 0x07C2, 0xC25B, 0xC969, 0x0CF0, 0xDF0D, 0x1A94, 0x11A6, 0xD43F, 0x5E52, 0x9BCB, 0x90F9, 0x5560, 0x869D, 0x4304, 0x4836, 0x8DAF, 0xAA55, 0x6FCC, 0x64FE, 0xA167, 0x729A, 0xB703, 0xBC31, 0x79A8, 0xA8EB, 0x6D72, 0x6640, 0xA3D9, 0x7024, 0xB5BD, 0xBE8F, 0x7B16, 0x5CEC, 0x9975, 0x9247, 0x57DE, 0x8423, 0x41BA, 0x4A88, 0x8F11, 0x057C, 0xC0E5, 0xCBD7, 0x0E4E, 0xDDB3, 0x182A, 0x1318, 0xD681, 0xF17B, 0x34E2, 0x3FD0, 0xFA49, 0x29B4, 0xEC2D, 0xE71F, 0x2286, 0xA213, 0x678A, 0x6CB8, 0xA921, 0x7ADC, 0xBF45, 0xB477, 0x71EE, 0x5614, 0x938D, 0x98BF, 0x5D26, 0x8EDB, 0x4B42, 0x4070, 0x85E9, 0x0F84, 0xCA1D, 0xC12F, 0x04B6, 0xD74B, 0x12D2, 0x19E0, 0xDC79, 0xFB83, 0x3E1A, 0x3528, 0xF0B1, 0x234C, 0xE6D5, 0xEDE7, 0x287E, 0xF93D, 0x3CA4, 0x3796, 0xF20F, 0x21F2, 0xE46B, 0xEF59, 0x2AC0, 0x0D3A, 0xC8A3, 0xC391, 0x0608, 0xD5F5, 0x106C, 0x1B5E, 0xDEC7, 0x54AA, 0x9133, 0x9A01, 0x5F98, 0x8C65, 0x49FC, 0x42CE, 0x8757, 0xA0AD, 0x6534, 0x6E06, 0xAB9F, 0x7862, 0xBDFB, 0xB6C9, 0x7350, 0x51D6, 0x944F, 0x9F7D, 0x5AE4, 0x8919, 0x4C80, 0x47B2, 0x822B, 0xA5D1, 0x6048, 0x6B7A, 0xAEE3, 0x7D1E, 0xB887, 0xB3B5, 0x762C, 0xFC41, 0x39D8, 0x32EA, 0xF773, 0x248E, 0xE117, 0xEA25, 0x2FBC, 0x0846, 0xCDDF, 0xC6ED, 0x0374, 0xD089, 0x1510, 0x1E22, 0xDBBB, 0x0AF8, 0xCF61, 0xC453, 0x01CA, 0xD237, 0x17AE, 0x1C9C, 0xD905, 0xFEFF, 0x3B66, 0x3054, 0xF5CD, 0x2630, 0xE3A9, 0xE89B, 0x2D02, 0xA76F, 0x62F6, 0x69C4, 0xAC5D, 0x7FA0, 0xBA39, 0xB10B, 0x7492, 0x5368, 0x96F1, 0x9DC3, 0x585A, 0x8BA7, 0x4E3E, 0x450C, 0x8095
};

short CRC15_POLY = 0x4599;

uint16_t LTC_pec (uint16_t *data , uint8_t len)
{
	int32_t remainder, address;
	remainder = 16;  //PEC seed

	for (uint8_t i = 0; i < len; i++){

		address   = ((remainder >> 7) ^ ((data[i] >> 8) & 0xFF)) & 0xFF;    //calculate PEC table address
		remainder = (remainder << 8 ) ^ PEC_TABLE[address];
		address   = ((remainder >> 7) ^ (data[i] & 0xFF)) & 0xFF;    //calculate PEC table address
		remainder = (remainder << 8 ) ^ PEC_TABLE[address];

	}
	return (remainder * 2); //The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
}

uint16_t LTC_pec2 (uint16_t* data, uint8_t size, uint8_t option){

	uint16_t aux, in0, pec = 16;
	int i, j;

	for(i = 0; i < size; i++){

		if (option == 0)
			data[i] = BYTESWAP(data[i]);

		for(j = 15; j >= 0; j--){
			aux = 0x00;
			in0 =  ((data[i] >> j) & 0x01) ^ ((pec >> 14) & 0x01);
			aux =  in0;
			aux =  aux << 4;
			aux += in0;
			aux =  aux << 2;
			aux += in0;
			aux =  aux << 1;
			aux += in0;
			aux =  aux << 3;
			aux += in0;
			aux =  aux << 1;
			aux += in0;
			aux =  aux << 3;
			aux += in0;
			pec =  aux ^ (pec << 1);
		}
		if (option == 0)
			data[i] = BYTESWAP(data[i]);
	}
	pec = pec << 1;

	if (option == 0)
		pec = BYTESWAP(pec);

	return (pec);
}

//static uint16_t LTC_TransmitRecieve (uint16_t Tx_data){
//
//	uint16_t Rx_data = 0;
//	HAL_SPI_TransmitReceive(&hspi1,(uint8_t *) &Tx_data, (uint8_t *) &Rx_data, 1, 50);
//
//	return(BYTESWAP(Rx_data));
//
//}

void LTC_CS(uint8_t level){
	HAL_GPIO_WritePin(ISOSPI_CS_GPIO_Port, ISOSPI_CS_Pin , level);
	DWT_Delay_us(10);
}

uint16_t LTC6804_spi(uint16_t Tx_data){

	uint16_t Rx_data = 0;

	HAL_SPI_TransmitReceive(&hspi1,(uint8_t *) &Tx_data, (uint8_t *) &Rx_data, 1, 50);

	return(BYTESWAP(Rx_data));

}

void LTC_transmit_recieve (uint16_t command, uint16_t* tx_data, uint16_t* rx_data){

	uint16_t pec = LTC_pec(&command, 1),
			buffer[4] = {command, pec, 0, 0}; //tx buffer

	//WAKE UP ROUTINE
	LTC_CS(0);
	LTC6804_spi(0);
	LTC_CS(1);

	//SEND COMMAND ROUTINE:
	LTC_CS(0);
	LTC6804_spi(buffer[0]);
	LTC6804_spi(buffer[1]);

	//TRANSMIT/RECIEVE DATA ROUTINE:
	for (uint8_t i = 0; i < 4; ++i)
		buffer[i] = tx_data[i];

	if((command & 0x07FF) == LTC_COMMAND_WRCFG){
		pec = LTC_pec2(tx_data, 3, 1);
		buffer[3] = pec;
	}

	if((buffer[0] & 0x07FF) < LTC_COMMAND_ADCV){
		for (uint8_t i = 0; i < 4; ++i)
			rx_data[i] = LTC6804_spi(buffer[i]);
	}
	LTC_CS(1);

}


//
//void LTC_wakeup(){
//
//	HAL_GPIO_WritePin(ISOSPI_CS_GPIO_Port, ISOSPI_CS_Pin, 0);
//	HAL_Delay(1);
//	LTC_TransmitRecieve(0);
//	HAL_GPIO_WritePin(ISOSPI_CS_GPIO_Port, ISOSPI_CS_Pin, 1);
//	HAL_Delay(1);
//
//}
//
//void transmit_command(uint16_t command){
//
//	LTC_TransmitRecieve(command);
//	LTC_TransmitRecieve(LTC_pec(&command, 1, 1));
//
//}
//
//void transmit_receive_data(uint16_t* tx_data, uint16_t* rx_data){
//
//	uint16_t aux[4];
//	aux[0] = tx_data[0];
//	aux[1] = tx_data[1];
//	aux[2] = tx_data[2];
//	aux[3] = LTC_pec(tx_data, 3, 0);
//
//	HAL_SPI_TransmitReceive(&hspi1,(uint8_t *) &aux, (uint8_t *) rx_data, 4, 50);
//
//}

uint16_t LTC_make_command(LTC_command *command){

	switch(command->NAME){

	case LTC_COMMAND_ADCV:

		return command->NAME | command->MD | command->DCP | command->CH;

		break;

	case LTC_COMMAND_ADOW:

		return command->NAME | command->MD | command->PUP | command->DCP | command->CH;

		break;

	case LTC_COMMAND_CVST:
	case LTC_COMMAND_AXST:
	case LTC_COMMAND_STATST:

		return command->NAME | command->MD | command->ST;

		break;

	case LTC_COMMAND_ADAX	:

		return command->NAME | command->MD | command->CHG;

		break;

	case LTC_COMMAND_ADSTAT	:

		return command->NAME | command->MD | command->CHST;

		break;

	case LTC_COMMAND_ADCVAX	:

		return command->NAME | command->MD | command->CHG;

		break;

	default:
		return command->NAME;
		break;
	}
}

void LTC_send_command(LTC_config *config, ...){

	uint16_t tx_data[3] = { 0, 0, 0},
			rx_data[3] = { 0, 0, 0};

	LTC_sensor *sensor;

	if(!config->command->BROADCAST){

		va_list list;
		va_start(list, 1);

		sensor = va_arg(list, LTC_sensor*);
		config->command->NAME  |= ((sensor->ADDR & (0x1111 * ~config->command->BROADCAST)) | ~config->command->BROADCAST << 4) << 11;

		tx_data[2] |= ((sensor->DCC & 0xff) << 8) | ((sensor->DCC & 0xf00) >> 8);

		va_end(list);
	}

	if((config->command->NAME & 0x07FF) == LTC_COMMAND_WRCFG){

		tx_data[0] = (config->ADCOPT) | (config->SWTRD << 1) | (config->REFON << 2) | (config->GPIO << 3) | (config->VUV << 8);
		tx_data[1] = (config->VUV >> 8) | (config->VOV << 4);
		tx_data[2] |= ((config->DCTO & 0xf) << 4);
		//		tx_data[2] = (config->DCTO << 12) | (config->DCC);

	}

	LTC_transmit_recieve(LTC_make_command(config->command), tx_data, rx_data);


	switch(config->command->NAME & 0x07FF){

	case LTC_COMMAND_RDCFG 	:

		//		config->ADCOPT = rx_data[0] & 0x1;
		//		config->SWTRD = (rx_data[0] >> 1) & 0x1;
		//		config->REFON = (rx_data[0] >> 2) & 0x1;
		//		config->GPIO =  (rx_data[0] >> 3) & 0x1F;
		//		config->VUV =   (rx_data[0] >> 8) | (rx_data[1] & 0x000F);
		//		config->VOV =   (rx_data[1] >> 4);
		//		config->DCC =   (rx_data[2] & 0x0FFF);
		//		config->DCTO =  (rx_data[2] >> 12);

		break;

	case LTC_COMMAND_RDCVA 	:

		sensor->CxV[0] = rx_data[0];
		sensor->CxV[1] = rx_data[1];
		sensor->CxV[2] = rx_data[2];

		break;

	case LTC_COMMAND_RDCVB 	:

		sensor->CxV[3] = rx_data[0];
		sensor->CxV[4] = rx_data[1];
		sensor->CxV[5] = rx_data[2];

		break;

	case LTC_COMMAND_RDCVC 	:

		sensor->CxV[6] = rx_data[0];
		sensor->CxV[7] = rx_data[1];
		sensor->CxV[8] = rx_data[2];

		break;

	case LTC_COMMAND_RDCVD 	:

		sensor->CxV[9]  = rx_data[0];
		sensor->CxV[10] = rx_data[1];
		sensor->CxV[11] = rx_data[2];

		break;

	case LTC_COMMAND_RDAUXA	:

		sensor->GxV[0] = rx_data[0];
		sensor->GxV[1] = rx_data[1];
		sensor->GxV[2] = rx_data[2];

		break;

	case LTC_COMMAND_RDAUXB	:

		sensor->GxV[3] = rx_data[0];
		sensor->GxV[4] = rx_data[1];
		sensor->REF =    rx_data[2];

		break;

	case LTC_COMMAND_RDSTATA:

		sensor->SOC =  rx_data[0] * 0.2;
		sensor->ITMP = rx_data[1] * 7.5;
		sensor->VA =   rx_data[2];

		break;

	case LTC_COMMAND_RDSTATB:

		sensor->VD = rx_data[0];

		uint32_t flag = rx_data[1] | (uint32_t)rx_data[2] << 16;
		for (int j = 0; j < 12; j++)
			sensor->V_ERROR[j] = (flag >> (j * 2)) & 0x3;

		break;

	case LTC_COMMAND_PLADC	:

		//while (transmit_command(command) < 0xFFFF);

		break;

	case LTC_COMMAND_DIAGN	:

		//transmit_command(config->command);

		break;

	case LTC_COMMAND_WRCOMM	:
		break;
	case LTC_COMMAND_RDCOMM	:
		break;
	case LTC_COMMAND_STCOMM	:
		break;
	default:
		break;
	}

	//	HAL_GPIO_WritePin(ISOSPI_CS_GPIO_Port, ISOSPI_CS_Pin, 1);
	//	HAL_Delay(50);
}

void LTC_init(LTC_config *config){

	config->GPIO = 0;
	config->REFON = 0;
	config->SWTRD = 0;
	config->ADCOPT = 0;
	config->VUV = MAX_CELL_V_CHARGE;
	config->VOV = MIN_CELL_V;
	config->DCTO = 0;
	config->command->MD = MD_NORMAL;

	config->command->BROADCAST = TRUE;
	config->command->NAME = LTC_COMMAND_WRCOMM;
	LTC_send_command(config);

}

void LTC_balance_test(LTC_config *config, LTC_sensor *sensor){

	//TODO implement software balance check
	sensor->DCC = 1;

	while (sensor->DCC != 0){

		config->command->BROADCAST = FALSE;
		config->command->NAME = LTC_COMMAND_WRCFG;
		LTC_send_command(config, sensor);
		sensor->DCC = sensor->DCC << 1;
		HAL_Delay(500);

	}

	sensor->DCC = 0;
	LTC_send_command(config, sensor);

}

static void LTC_T_convert(LTC_sensor* sensor){
	float t0, B, r, r0;
	t0 = 25 + 273;
	r0 = 10000;
	B = 3380;

	for (int i = 0; i < 5; ++i) {
		r = (float)(sensor->GxV[i]*10000) / (float)(sensor->REF - sensor->GxV[i]);
		sensor->GxV[i] = ((t0 * B) / (t0 * log( r / r0) + B) - 273)*10;
	}
}

void LTC_read(uint8_t LTC_READ, LTC_config *config, LTC_sensor *sensor){

	config->command->BROADCAST = FALSE;

	if (LTC_READ&LTC_READ_CELL) {

		config->command->NAME = LTC_COMMAND_RDCVA;
		LTC_send_command(config, sensor);
		config->command->NAME = LTC_COMMAND_RDCVB;
		LTC_send_command(config, sensor);
		config->command->NAME = LTC_COMMAND_RDCVC;
		LTC_send_command(config, sensor);
		config->command->NAME = LTC_COMMAND_RDCVD;
		LTC_send_command(config, sensor);

		sensor->V_MIN = 36000;
		sensor->V_MAX = 28000;

				if(sensor->ADDR == 5){
					sensor->CxV[1] = (sensor->CxV[1] + sensor->CxV[2])/2;
					sensor->CxV[2] = sensor->CxV[1];
				}


		for(uint8_t i = 0; i < N_OF_CELLS; i++){
			if(sensor->CxV[i] < sensor->V_MIN)
				sensor->V_MIN = sensor->CxV[i];
			if(sensor->CxV[i] > sensor->V_MAX)
				sensor->V_MAX = sensor->CxV[i];
		}

		sensor->V_DELTA = sensor->V_MAX - sensor->V_MIN;

	}
	if (LTC_READ&LTC_READ_GPIO) {

		config->command->NAME = LTC_COMMAND_RDAUXA;
		LTC_send_command(config, sensor);
		config->command->NAME = LTC_COMMAND_RDAUXB;
		LTC_send_command(config, sensor);

		//		sensor->T_MIN = 60000;
		//		sensor->T_MAX = 28000;
		LTC_T_convert(sensor);

	}
	if (LTC_READ&LTC_READ_STATUS) {

		config->command->NAME = LTC_COMMAND_RDSTATA;
		LTC_send_command(config, sensor);
		config->command->NAME = LTC_COMMAND_RDSTATB;
		LTC_send_command(config, sensor);

	}
	if (LTC_READ&LTC_READ_CONFIG){

		config->command->NAME = LTC_COMMAND_RDCFG;
		LTC_send_command(config, sensor);

	}

}

void LTC_set_balance_flag(LTC_config *config, LTC_sensor *sensor){

	sensor->DCC = 0;

	if(sensor->V_DELTA > 10)
		for (uint8_t i = 0; i < N_OF_CELLS; ++i) {
			if((sensor->CxV[i] - sensor->V_MIN) > sensor->V_DELTA * 0.8){
				sensor->DCC |= 1 << i;
			}
		}

}

void LTC_reset_balance_flag(LTC_config *config, LTC_sensor *sensor){

	sensor->DCC = 0;

	config->command->BROADCAST = FALSE;
	config->command->NAME = LTC_COMMAND_WRCFG;
	LTC_send_command(config, sensor);

}


void LTC_balance(LTC_config *config, LTC_sensor *sensor){

	config->command->BROADCAST = FALSE;
	config->command->NAME = LTC_COMMAND_WRCFG;
	LTC_send_command(config, sensor);

}

//void LTC_balance_test(LTC_config *config, LTC_sensor *sensor){
//
//	uint16_t cell_balance[12], cell_not_balance[12];
//	config->command->DCP = DCP_NOT_PERMITED;
//	config->command->MD = MD_FILTRED;
//	config->command->CH = CH_ALL;
//	LTC_read(LTC_READ_CELL, config, sensor);
//	for(uint8_t i = 0; i<12; i++)
//		cell_not_balance[i] = sensor->CxV[i];
//
//	for(uint8_t i=0 ;i<6; i++){
//		config->DCC[i] = 1;
//		config->DCC[i+6] = 1;
//		config->command->BROADCAST = FALSE;
//		config->command->NAME = LTC_COMMAND_WRCFG;
//		LTC_send_command(config, sensor);
//
//		config->command->CH = i+1;
//		LTC_read(LTC_READ_CELL, config, sensor);
//
//
//	}
//}

void LTC_balance_rubber(LTC_config *config, LTC_sensor *sensor){

	uint16_t volt_old[12], volt_now[12];
	config->command->DCP = DCP_NOT_PERMITED;
	config->command->MD = MD_FILTRED;
	config->command->CH = CH_ALL;
	LTC_read(LTC_READ_CELL, config, sensor);
	for(uint8_t i = 0; i<12; i++){
		volt_old[i] = sensor->CxV[i];
	}
	config->command->BROADCAST = FALSE;
	config->command->NAME = LTC_COMMAND_WRCFG;

}

void LTC_open_wire(uint8_t LTC_READ, LTC_config *config, LTC_sensor *sensor){

	//	LTC_send_command(LTC_COMMAND_ADOW, FALSE, config, sensor);

}
