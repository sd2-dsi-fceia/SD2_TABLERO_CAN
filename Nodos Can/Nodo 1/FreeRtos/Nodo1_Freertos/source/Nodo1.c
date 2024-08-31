/*
 * Nodo1.c
 *
 *  Created on: 30 ago. 2024
 *      Author: agustin
 */

/**
 * @file Nodo 1
 * @author Zuliani, Agustin
 */

#include "Nodo1.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "mcp2515.h"
#include "can.h"
#include "LDR.h"
#include "fsl_debug_console.h"

#define CAN_NODO1_ID	10
#define CAN_NODO1_DLC	2

#define TASK_NODO1_STACK 	configMINIMAL_STACK_SIZE+20

TimerHandle_t TimerMuestreo;

/**
 * @brief Mensaje de tipo can.
 */
struct can_frame canMsg_Nodo1;

/**
 * @brief Inicializacion de perifericos.
 */
static void perifericos_init(void);
/**
 * @brief Tarea del Nodo 1.
 */
static void taskRtos_Nodo1(void *pvParameters);
/**
 * @brief Timer por software.
 */
static void timerRtos_LdrConversion(TimerHandle_t xTimer);

extern void Nodo1_init(void)
{
	PRINTF("\nNombre: Nodo 1\n\r");

	BaseType_t status = xTaskCreate(taskRtos_Nodo1, "Task Nodo 1",
	configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	if (status == pdFALSE)
		PRINTF("Fallo al crear la tarea.\n\r");

	TimerMuestreo = xTimerCreate("Muestro de adc",
			pdMS_TO_TICKS(TIEMPO_DE_MUESTREO_LDR), true, NULL, timerRtos_LdrConversion);
	if (TimerMuestreo == NULL)
		PRINTF("Fallo al crear el timer.\n\r");

	return;
}

static void taskRtos_Nodo1(void *pvParameters)
{
	Error_LDR_t error = LDR_init();

	if (error != ERROR_LDR_OK)
		PRINTF("Fallo al inicializar el adc.\n\r");

	/* Configura de can */
	perifericos_init();

	canMsg_Nodo1.can_id = CAN_NODO1_ID;
	canMsg_Nodo1.can_dlc = CAN_NODO1_DLC;

	BaseType_t status = xTimerStart(TimerMuestreo, portMAX_DELAY);
	if (status != pdPASS)
		PRINTF("Fallo al inciar el timer.\n\r");

	for (;;)
	{
		if (LDR_getConvComplete())
		{
			PRINTF("Valor del Adc: %d\n\r", LDR_UltimaConversion());

			LDR_clearConvComplete();

			canMsg_Nodo1.data[1] = (LDR_UltimaConversion() & 0xff00) >> 8;
			canMsg_Nodo1.data[0] = (LDR_UltimaConversion() & 0x00ff);

			ERROR_t estado;

			estado = mcp2515_sendMessage(&canMsg_Nodo1);

			if (estado == ERROR_OK)
			{
				PRINTF("\nNodo 1.\n\r");
				PRINTF("Mensaje enviado\n\r");
				PRINTF("ID\tDLC\tDATA\n\r");
				PRINTF("%d\t%d\t", canMsg_Nodo1.can_id, canMsg_Nodo1.can_dlc);

				for (uint8_t i = 0; i < 8; i++)
				{
					PRINTF("%d ", canMsg_Nodo1.data[i]);
				}

				PRINTF("\n\r");
			}
			else
			{
				PRINTF("\nError al enviar\n\r");
			}
		}
	}

	vTaskDelete(NULL);

	return;
}

extern canid_t Nodo1_id(void)
{
	canid_t id = CAN_NODO1_ID;

	return id;
}

static void perifericos_init(void)
{
	ERROR_t error;

	mcp2515_init();

	error = mcp2515_reset();

	if (error != ERROR_OK)
		PRINTF("Fallo al resetear el modulo\n\r");

	error = mcp2515_setBitrate(CAN_125KBPS, MCP_8MHZ);

	if (error != ERROR_OK)
		PRINTF("Fallo al setear el bit rate\n\r");

//	error = mcp2515_setNormalMode();
//	if (error != ERROR_OK)
//		PRINTF("Fallo al setear el modo normal\n\r");
	error = mcp2515_setLoopbackMode();
	if (error != ERROR_OK)
		PRINTF("Fallo al setear el loopback mode\n\r");

	return;
}

static void timerRtos_LdrConversion(TimerHandle_t xTimer)
{
	LDR_convertir();

	return;
}

void ADC16_IRQ_HANDLER_FUNC(void)
{
	LDR_setConvComplete(); /* Setea la bandera en 1 */
	LDR_read(); /* Lee el registro */

	return;
}
