/****************************************************************/
/**					Team Formula Tesla UFMG - 2018				*/
/** Microcontroller: STM32F103XXXX								*/
/** Compiler: AC6 - STM worbench								*/
/** Author: Henrique, Eric, Virginia							*/
/** License: Free - Open Source									*/
/** 															*/
/****************************************************************/

#include "BMS.h"
#include "stm32f1xx_hal.h"
#include "usart.h"
#include "dwt_stm32_delay.h"
#include "can.h"
#include "eeprom.h"
#include <stdlib.h>
#include "nextion.h"

static int8_t UV_retries, OV_retries, OT_retries;

//uint8_t balance_enable = 1;


//extern uint8_t mode_button;
//extern I2C_HandleTypeDef hi2c2;





static const uint16_t CAN_ID_TABLE[8][5] = {
		{
				250,		//PACK 0 V_GROUP 0 		(cells 0, 1, 2, 3)
				251,		//PACK 0 V_GROUP 1		(cells 4, 5, 6, 7)
				252,		//PACK 0 V_GROUP 2      (cells 8, 9, 10, 11)
				253,		//PACK 0 T_GROUP        (sensor 0, 1, 2, 3)
		},

		{
				255,		//PACK 1 V_GROUP 0 		(cells 0, 1, 2, 3)
				256,		//PACK 1 V_GROUP 1		(cells 4, 5, 6, 7)
				257,		//PACK 1 V_GROUP 2      (cells 8, 9, 10, 11)
				258,		//PACK 1 T_GROUP        (sensor 0, 1, 2, 3)
		},

		{
				260,		//PACK 2 V_GROUP 0 		(cells 0, 1, 2, 3)
				261,		//PACK 2 V_GROUP 1		(cells 4, 5, 6, 7)
				262,		//PACK 2 V_GROUP 2      (cells 8, 9, 10, 11)
				263,		//PACK 2 T_GROUP        (sensor 0, 1, 2, 3)
		},
		{
				265,		//PACK 3 V_GROUP 0 		(cells 0, 1, 2, 3)
				266,		//PACK 3 V_GROUP 1      (cells 4, 5, 6, 7)
				267,		//PACK 3 V_GROUP 2      (cells 8, 9, 10, 11)
				268,		//PACK 3 T_GROUP        (sensor 0, 1, 2, 3)
		},
		{
				270,		//PACK 4 V_GROUP 0 		(cells 0, 1, 2, 3)
				271,		//PACK 4 V_GROUP 1      (cells 4, 5, 6, 7)
				272,		//PACK 4 V_GROUP 2      (cells 8, 9, 10, 11)
				273,		//PACK 4 T_GROUP        (sensor 0, 1, 2, 3)
		},
		{
				275,		//PACK 5 V_GROUP 0      (cells 0, 1, 2, 3)
				276,		//PACK 5 V_GROUP 1      (cells 4, 5, 6, 7)
				277,		//PACK 5 V_GROUP 2      (cells 8, 9, 10, 11)
				278,		//PACK 5 T_GROUP        (sensor 0, 1, 2, 3)
		},
		{
				51,		//CHARGING CURRENT (bytes 0, 1, 2, 3) AND DISCHARGING CURRENT(bytes 4, 5, 6, 7)
		},
		{
				52,		//BMS MODE(bytes 0),COMM MODE(bytes 1), AIR STATUS(bytes 2 & 3), BATTERY %(bytes 4 & 5), GLOBAL ERROR FLAG(byte 6)
				53,
				54
		},
};

//extern uint8_t c_zero_flag;

void BMS_init(BMS_struct *BMS){


	//comm_time = HAL_GetTick();

	CAN_Config_Filter();
	CAN_Config_Frames();
	CAN_Receive_IT();

	BMS->config = (LTC_config*) calloc(1 ,sizeof(LTC_config));
	BMS->config->command = (LTC_command*) calloc(1 ,sizeof(LTC_command));

	for(int i = 0; i < N_OF_PACKS; i++){
		BMS->sensor[i] = (LTC_sensor*) calloc(1, sizeof(LTC_sensor));
		BMS->sensor[i]->ADDR = i;
		LTC_init(BMS->config);
	}

	BMS->error = ERR_NO_ERROR;
	//BMS->opperating_packs = 0;
	BMS->v_min = 50000;
	BMS->v_max = 0;

	BMS->mode = 0;

	LTC_init(BMS->config);

	//	LTC_set_thermistor_zeros();
	//
	//	c_zero_flag = 1;

	//	if(BMS->opperation_mode == OPP_CHARGING)
	//		HAL_GPIO_WritePin(CHARGE_ENABLE_GPIO_Port, CHARGE_ENABLE_Pin, SET);


	//	EE_WriteVariable(0x0, (uint16_t) 0);
	//	EE_WriteVariable(0x1, (uint16_t) 0);
	//
	//	uint16_t aux;
	//	EE_ReadVariable(0x0, &aux);
	//	BMS->charge = ((uint32_t)aux) << 16; 	//load upper bytes
	//	EE_ReadVariable(0x1, &aux);
	//	BMS->charge += aux;		//load lower bytes

}
void BMS_monitoring(BMS_struct *BMS){


	BMS->v_min = BMS->sensor[0]->V_MIN;
	BMS->v_max = BMS->sensor[0]->V_MAX;
	BMS->v_TS = 0;

	BMS->v_min = 50000;
	BMS->v_max = 0;

	BMS->config->ORDER = 1;

	BMS->config->command->NAME = LTC_COMMAND_ADCV;
	BMS->config->command->BROADCAST = TRUE;
	LTC_send_command(BMS->config);

	HAL_Delay(10);

	BMS->config->command->NAME = LTC_COMMAND_ADAX;
	BMS->config->command->BROADCAST = TRUE;
	LTC_send_command(BMS->config);

	HAL_Delay(10);

	BMS->config->command->NAME = LTC_COMMAND_ADSTAT;
	BMS->config->command->BROADCAST = TRUE;
	LTC_send_command(BMS->config);

	HAL_Delay(10);

	for(uint8_t i = 0; i < N_OF_PACKS; i++){

		LTC_read(LTC_READ_CELL | LTC_READ_STATUS | LTC_READ_STATUS | LTC_READ_GPIO, BMS->config, BMS->sensor[i]);

		HAL_Delay(100);

		if(BMS->sensor[i]->V_MIN < BMS->v_min)
			BMS->v_min = BMS->sensor[i]->V_MIN;
		if(BMS->sensor[i]->V_MAX > BMS->v_max)
			BMS->v_max = BMS->sensor[i]->V_MAX;

		BMS->v_TS += BMS->sensor[i]->SOC;

		//if(BMS->mode == 1)
			//LTC_balance(BMS->config, BMS->sensor[i]);

	}

	BMS->v_TS /= N_OF_PACKS/2;



	//	if (BMS->error_flag != ERR_NO_ERROR) {
	//		LTC_convert_fuses();
	//
	//		for(i = 0; i < N_OF_PACKS; i++){
	//
	//			if (BMS->sensor[i]->status == STAT_OPPERATING) {
	//
	//				if(LTC_check_fuses(BMS->sensor[i]) == ERR_OPEN_FUSES)
	//					BMS->error_flag |= ERR_OPEN_FUSES;
	//				//			else
	//				//				BMS->error_flag &= ~ERR_OPEN_FUSES;
	//			}
	//
	//		}
	//	}

	//BMS->opperating_packs = opperating_packs;

	//BMS->AIR = BMS_AIR_status(BMS);
}
//
//uint8_t BMS_AIR_status(BMS_struct *BMS){
////	HAL_GPIO_WritePin(AIR_AUX_PLUS_GPIO_Port, AIR_AUX_PLUS_Pin, RESET);
//	if (HAL_GPIO_ReadPin(AIR_AUX_PLUS_GPIO_Port, AIR_AUX_PLUS_Pin) == 1)
//		return AIR_CLOSED;
//
////	HAL_GPIO_WritePin(AIR_AUX_PLUS_GPIO_Port, AIR_AUX_PLUS_Pin, SET);
//	else if (HAL_GPIO_ReadPin(AIR_AUX_MINUS_GPIO_Port, AIR_AUX_MINUS_Pin) == 1)
//		return AIR_CLOSED;
//	else
//		return AIR_OPEN;
//}
//
//int BMS_charging(BMS_struct BMS){
//	//EM CONSTRU��O...
//	return 0;
//}
//
//int BMS_discharging(BMS_struct BMS){
//	//EM CONSTRU��O...
//	return 0;
//}
//
//int BMS_balance(BMS_struct *BMS){
//	for(uint8_t i = 0; i < N_OF_PACKS; i++){
//		if(LTC6804_check_status(BMS->sensor[i])){
//			LTC6804_balance(BMS->sensor[i]);
//		}
//	}
//	//EM CONSTRU��O...
//	return 0;
//}
//
//int BMS_communication(BMS_struct *BMS){
//	//EM CONSTRU��O...
//
//
//	//UART_print("\n\n%d\n\n", BMS->charge);
//	EE_WriteVariable(0x0, (uint16_t) (BMS->charge >> 16));
//	EE_WriteVariable(0x1, (uint16_t) BMS->charge);
//	nextLoop(BMS);
//
//	switch(BMS->communication_mode){
//	case COMM_TC_ONLY:
//		BMS_can(BMS);
//		break;
//
//	case COMM_FULL:
//
//		BMS_can(BMS);
//		if ((HAL_GetTick() - comm_time) > UART_RATE){
//			BMS_uart(BMS);
//			comm_time = HAL_GetTick();
//		}
//		break;
//
//	case COMM_UART_DEBUG:
//
//		if ((HAL_GetTick() - comm_time) > UART_RATE){
//			BMS_uart(BMS);
//			comm_time = HAL_GetTick();
//		}
//
//		break;
//	}
//	return 0;
//}
//
uint16_t flag = 0;

void BMS_error(BMS_struct *BMS){


	if(BMS->v_min <= 28000)
		flag |= ERR_UNDER_VOLTAGE;
	else if(BMS->v_min >= 30000)
		flag &= ~ERR_UNDER_VOLTAGE;
	if(BMS->v_max >= 36000)
		flag |= ERR_OVER_VOLTAGE;
	else if(BMS->v_max <= 34000)
		flag &= ~ERR_OVER_VOLTAGE;


	//	for(int i = 0; i < N_OF_PACKS; i++){
	//		//if (BMS->sensor[i]->status == STAT_OPPERATING) {
	//
	//
	//
	//
	//			//BMS->error_flag &= ~(ERR_UNDER_VOLTAGE | ERR_OVER_VOLTAGE | ERR_OVER_TEMPERATURE);
	//
	//		//}
	//	}


	if((flag & ERR_UNDER_VOLTAGE) == ERR_UNDER_VOLTAGE)
		UV_retries++;
	else
		UV_retries--;
	if((flag & ERR_OVER_VOLTAGE) == ERR_OVER_VOLTAGE)
		OV_retries++;
	else
		OV_retries--;
	if((flag & ERR_OVER_TEMPERATURE) == ERR_OVER_TEMPERATURE)
		OT_retries++;
	else
		OT_retries--;

	if(UV_retries > 5) UV_retries = 5;
	if(OV_retries > 5) OV_retries = 5;
	if(OT_retries > 5) OT_retries = 5;
	if(UV_retries < 0) UV_retries = 0;
	if(OV_retries < 0) OV_retries = 0;
	if(OT_retries < 0) OT_retries = 0;


	if(UV_retries == 5){
		NextError[0] = 1;
		BMS->error |= ERR_UNDER_VOLTAGE;
	}
	else if(UV_retries == 0){
		NextError[0] = 0;
		BMS->error &= ~ERR_UNDER_VOLTAGE;
	}
	if(OV_retries == 5){
		NextError[1] = 1;
		BMS->error |= ERR_OVER_VOLTAGE;
	}
	else if(OV_retries == 0){
		NextError[1] = 0;
		BMS->error &= ~ERR_OVER_VOLTAGE;
	}
	//	if(OT_retries == 5){
	//		NextError[2] = 1;
	//		BMS->error_flag |= ERR_OVER_TEMPERATURE;}
	//	else if(OT_retries == 0){
	//		NextError[2] = 0;
	//		BMS->error_flag &= ~ERR_OVER_TEMPERATURE;}

	//	if(BMS->opperating_packs < N_OF_PACKS){
	//		NextError[3] = 1;
	//		BMS->error |= ERR_COMM_ERROR;}  //SET FLAG
	//	else{
	//		NextError[3] = 0;
	//		BMS->error &= ~ERR_COMM_ERROR; //RESET FLAG
	//}
	//	switch(BMS->opperation_mode){
	//	case OPP_DEFAULT:
	//		if(BMS->current[0] > 1800 || BMS->current[2] > 1800)
	//			BMS->error_flag |= ERR_OVER_CURRENT;
	//		break;
	//
	//	case OPP_CHARGING:
	//		if(BMS->current[1] > 2000 || BMS->current[3] > 2000)
	//			BMS->error_flag |= ERR_OVER_CURRENT;
	//		break;
	//	}


	if(BMS->v_GLV < 13500){
		//BMS->error |= ERR_GLV_VOLTAGE;
		NextError[4] = 1;
	}


	HAL_GPIO_WritePin(AIR_ENABLE_GPIO_Port, AIR_ENABLE_Pin, SET);

	if(BMS->error != ERR_NO_ERROR){
		//		if(BMS->opperation_mode == OPP_CHARGING){
		//			HAL_GPIO_WritePin(CHARGE_ENABLE_GPIO_Port, CHARGE_ENABLE_Pin, RESET);
		//		}
		//		else{
		HAL_GPIO_WritePin(AIR_ENABLE_GPIO_Port, AIR_ENABLE_Pin, RESET);
		//if (BMS_AIR_status(BMS) ==  AIR_CLOSED){
		//BMS->error = ERR_AIR_ERROR;
		//			}
		//}

		//Envia por can pro ECU e espera ele desligar os motores;
		//abre air}

	}else{
		HAL_GPIO_WritePin(AIR_ENABLE_GPIO_Port, AIR_ENABLE_Pin, SET);
	}
}

void can_buf(uint8_t buffer[8], uint16_t word1, uint16_t word2, uint16_t word3, uint16_t word4){

	buffer[0] = word1;
	buffer[1] = word1 >> 8;
	buffer[2] = word2;
	buffer[3] = word2 >> 8;
	buffer[4] = word3;
	buffer[5] = word3 >> 8;
	buffer[6] = word4;
	buffer[7] = word4 >> 8;

}

void BMS_can(BMS_struct *BMS){
	uint8_t can_buffer[8];

	for(uint8_t i = 0; i < N_OF_PACKS; i++){
		//if (BMS->sensor[i]->status == STAT_OPPERATING) {
		for (uint8_t j = 0; j < 3; j++) {
			can_buf(can_buffer, BMS->sensor[i]->CxV[j + 0],
					BMS->sensor[i]->CxV[j + 1],
					BMS->sensor[i]->CxV[j + 2],
					BMS->sensor[i]->CxV[j + 3]);

			CAN_Transmit(can_buffer, CAN_ID_TABLE[i][j]);

			sendString(CAN_ID_TABLE[i][j], 	BMS->sensor[i]->CxV[(j * 4)],
					BMS->sensor[i]->CxV[(j * 4) +1],
					BMS->sensor[i]->CxV[(j * 4) +2],
					BMS->sensor[i]->CxV[(j * 4) +3]);

		}

		//			can_buffer[0] = BMS->sensor[i]->t_cell[0];
		//			can_buffer[1] = BMS->sensor[i]->t_cell[0] >> 8;
		//			can_buffer[2] = BMS->sensor[i]->t_cell[1];
		//			can_buffer[3] = BMS->sensor[i]->t_cell[1] >> 8;
		//			can_buffer[4] = BMS->sensor[i]->t_cell[2];
		//			can_buffer[5] = BMS->sensor[i]->t_cell[2] >> 8;
		//			can_buffer[6] = BMS->sensor[i]->t_cell[3];
		//			can_buffer[7] = BMS->sensor[i]->t_cell[3] >> 8;
		//
		//			CAN_Transmit(can_buffer, CAN_ID_TABLE[i][CAN_TEMPERATURE_ID]);

		//			sendString(CAN_ID_TABLE[i][CAN_TEMPERATURE_ID],
		//					   BMS->sensor[i]->t_cell[0],
		//					   BMS->sensor[i]->t_cell[1],
		//					   BMS->sensor[i]->t_cell[2],
		//					   BMS->sensor[i]->t_cell[3]);

		//		can_buf(can_buffer, 0,
		//							0,
		//							BMS->sensor[i]->status | (BMS->sensor[i]->status << 8),
		//							0)
		//
		//		CAN_Transmit(can_buffer, CAN_ID_TABLE[i][CAN_ERROR_FLAG_ID]);
		//
		//		sendString(CAN_ID_TABLE[i][CAN_ERROR_FLAG_ID],
		//				BMS->sensor[i]->status,
		//				BMS->sensor[i]->address,
		//				0,
		//				0);

		//}
	}

	can_buf(can_buffer, BMS->current[0],
			BMS->current[1],
			BMS->current[2],
			BMS->current[3]);

	CAN_Transmit(can_buffer, CAN_ID_TABLE[CAN_CURRENT_ID][0]);

	sendString(CAN_ID_TABLE[CAN_CURRENT_ID][0],
			BMS->current[0],
			BMS->current[1],
			BMS->current[2],
			BMS->current[3]);

	uint32_t percent = 10000 - ((BMS->charge + 50000000)/14400);

	can_buf(can_buffer, BMS->v_GLV,
			(int16_t)(percent),
			0,
			BMS->AIR);

	CAN_Transmit(can_buffer, 52);

	sendString(CAN_ID_TABLE[CAN_GENERAL_ID][0],
			BMS->v_GLV,
			percent,
			BMS->AIR,
			BMS->error);


	//	can_buffer[0] = c_mean;
	//	can_buffer[1] = c_mean >> 8;
	//	can_buffer[2] = v_bank;
	//	can_buffer[3] = v_bank >> 8;
	//	can_buffer[4] = t_mean;
	//	can_buffer[5] = t_mean >> 8;
	//	can_buffer[6] = t_max;
	//	can_buffer[7] = t_max >> 8;

	CAN_Transmit(can_buffer, CAN_ID_TABLE[CAN_GENERAL_ID][1]);

	//	sendString(CAN_ID_TABLE[CAN_GENERAL_ID][1],
	//			c_mean,
	//			v_bank,
	//			t_mean,
	//			t_max);

	can_buf(can_buffer, BMS->v_min,
			BMS->v_max,
			0,
			0);

	CAN_Transmit(can_buffer, CAN_ID_TABLE[CAN_GENERAL_ID][2]);

	sendString(CAN_ID_TABLE[CAN_GENERAL_ID][2], BMS->v_min,0,0,0);

	can_buf(can_buffer, BMS->AIR,
			0,
			0,
			0);

	can_buffer[7] = 0;

	CAN_Transmit(can_buffer, 0);

	sendString(0, BMS->AIR,0,0);

}

//extern uint8_t max_retries;
//extern uint16_t ADC_BUF[5];
//
//
//void BMS_uart(BMS_struct *BMS){
//
//	HAL_GPIO_WritePin(DEBUG_GPIO_Port,DEBUG_Pin, 1);
//
//
//	UART_print("\n\n\n\n"  );//HEADER
//	UART_print("\n\t\tONLINE PACKS: %d\t\tGLV: %d\t\tCHARGE: %d\t\tERROR:", BMS->opperating_packs, BMS->GLV_voltage , BMS->charge);//HEADER
//	UART_print_error_flag(BMS->error_flag);
//	UART_print("\t\tRETRIES:%d", max_retries);
//	UART_print("\n"  );//HEADER
//
//	for(int i = 0; i < N_OF_PACKS; i++){
//
//		UART_print("\n");
//		UART_print("\t\t\t\t\t\t\t\t   -- PACK %d STATUS: %x --\n", i, BMS->sensor[i]->status);
//		UART_print("\n\t");
//
//		if (BMS->sensor[i]->status == STAT_OPPERATING) {
//			for(int j = 0; j < N_OF_CELLS; j++){
//				UART_print("CELL %2d: ", j + 1);
//				UART_print("%d", BMS->sensor[i]->CxV[j]);
//				//UART_print_float((float)BMS->sensor[i]->CxV[j]/10000);
//				//UART_print("%x", BMS->sensor[i]->v_error[j]);
//				UART_print_error_flag(BMS->sensor[i]->v_error[j]);
//				UART_print("\t");
//
//				if ((j + 1) % 6 == 0){
//					UART_print("\n\t");
//				}
//			}
//
//			UART_print("\n\t");
//
////			for(int j = 0; j < N_OF_THERMISTORS; j++){
////				UART_print("TEMP %d: ", j);
////				UART_print_float((float)BMS->sensor[i]->t_cell[j]/1000);
////				UART_print_error_flag(BMS->sensor[i]->t_error[j]);
////				UART_print("\t");
////			}
//
//			UART_print("V PACK %d: ", i);
//			UART_print("%d", BMS->sensor[i]->v_sum);
//			UART_print("\t");
//			UART_print("VAR MAX %d: ", i);
//			UART_print("%d", BMS->sensor[i]->v_var);
//
//			UART_print("\n");
//		}
//	}
//
////	UART_print("\n\tCURRENT 1(1): ");
////	UART_print_float((float)BMS->current[0]/100);
////	UART_print("\tCURRENT 1(2): ");
////	UART_print_float((float)BMS->current[1]/1000);
////	UART_print("\tCURRENT 2(1): ");
////	UART_print_float((float)BMS->current[2]/100);
////	UART_print("\tCURRENT 2(2): ");
////	UART_print_float((float)BMS->current[3]/1000);
////	UART_print("\n\tCGLV: ");
////	UART_print_float((float)BMS->GLV_voltage/1000);
////	UART_print("\n\t%d \t%d \t%d \t%d", ADC_BUF[0], ADC_BUF[1], ADC_BUF[2], ADC_BUF[3]);
//
//	UART_print("\n\t[UV] times: %d", UV_retries);
//
//	UART_print("\t[OV] times: %d", OV_retries);
//
//	UART_print("\t[OT] times: %d", OT_retries);
//
//	HAL_GPIO_WritePin(DEBUG_GPIO_Port,DEBUG_Pin, 0);
//}
//
//
////uint8_t BMS_mode_comm_navigate(uint8_t mode){
////	switch(mode){
////	case COMM_FULL:
////		return COMM_TC_ONLY;
////		break;
////	case COMM_TC_ONLY:
////		return COMM_UART_DEBUG;
////		break;
////	case COMM_UART_DEBUG:
////		return COMM_FULL;
////		break;
////	default:
////		return OPP_DEFAULT;
////	}
////}
//
////uint8_t mode_display(uint8_t mode){
////	switch(mode){
////	case OPP_BALANCING:
////		return b;
////		break;
////	case OPP_CHARGING:
////		return c;
////		break;
////	case OPP_DEFAULT:
////		return d;
////		break;
////	case OPP_TESTING:
////		return t;
////		break;
////	default:
////		return d;
////	}
////}
//
////extern uint8_t mode, accept_time;
//
//
////void BMS_mode_selector(BMS_struct *BMS){
////	if(!HAL_GPIO_ReadPin(MODE_SELECT_GPIO_Port, MODE_SELECT_Pin)){
////		int i = 0, op = 0;
////
////		i2c_transmit(D);
////
////		while (!HAL_GPIO_ReadPin(MODE_SELECT_GPIO_Port, MODE_SELECT_Pin));
////
////		while(1){
////			//---MODE SELECTION ROUTINE---
////
////			i2c_transmit(mode_display(a));
////
////			if(mode_button == 1){
////				//---MODE BLINKING ROUTINE---
////
////				if(i >= 12) op = 1;
////				else if (i <= 0) op = 0;
////
////				i2c_transmit(mode_display(mode));
////				HAL_Delay(i);
////				i2c_transmit(0);
////				HAL_Delay(12 - i);
////
////				if (op == 0) i++;
////				else i--;
////
////				if(accept_time > 1000){
////					//---MODE CHANGING ROUTINE---
////
////					mode_confirm_animation();
////					return;
////
////					//---MODE CHANGING ROUTINE--- END
////				}
////
////				//---MODE BLINKING ROUTINE--- END
////			}
////
////			//---MODE SELECTION ROUTINE--- END
////		}
////	}
////}
//
//
