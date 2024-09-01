/*
 * Nodo2.c
 *
 *  Created on: 30 ago. 2024
 *      Author: agustin
 */

/**
 * @file Nodo 2
 * @author Zuliani, Agustin
 */

#include "Nodo2.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "can.h"
#include "mcp2515.h"

#include "Nodo1.h"

#include "fsl_debug_console.h"

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"

#define CAN_NODO_2_ID	20
#define CAN_NODO_2_DLC	1
#define TIEMPO_ENVIAR_DATOS_BUSCAN 500

#define __delay_ms(x)	vTaskDelay(pdMS_TO_TICKS(x))

#define LUZ_MIN	500
#define LUZ_MAX	1500
#define LED_ON	GPIO_ClearPinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN_MASK)
#define LED_OFF	GPIO_SetPinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN_MASK)

typedef union
{
	struct
	{
		unsigned LED_ROJO :1;
		unsigned PULSADOR1 :1;
		unsigned PULSADOR2 :1;
		unsigned RESERVADO :5;
	};
	uint8_t data;
} EstPerifericos_t;

/**
 * @brief Mensaje can del nodo 2 para escritura.
 */
static struct can_frame canMsg_Nodo2_write;
static struct can_frame canMsg_Nodo2_read;

TimerHandle_t Timer1;
TaskHandle_t TaskNodo2_Handle;

/**
 * @brief Tarea del nodo 2.
 */
static void taskRtos_Nodo2(void *pvParameters);
/**
 * @brief Tarea de escritura en el bus can del nodo 2.
 */
static void taskRtos_Nodo2_Write(void *pvParameters);
/**
 * @brief Funcion de callback del timer.
 */
static void timerRtos_DatosPerifericos(void *pvParameters);

extern BaseType_t receiveFromQueue(struct can_frame *dato);
extern BaseType_t receiveFromQueue_Peek(struct can_frame *dato);

extern void Nodo2_init(void)
{
	PRINTF("\nNombre: Nodo 2\n\r");

	BaseType_t status = xTaskCreate(taskRtos_Nodo2, "Task Nodo 2",
	configMINIMAL_STACK_SIZE, NULL, 2, &TaskNodo2_Handle);
	if (status == pdFALSE)
		PRINTF("Fallo al crear la tarea.\n\r");

	status = xTaskCreate(taskRtos_Nodo2_Write, "Task Nodo 2 Write",
	configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	if (status == pdFALSE)
		PRINTF("Fallo al crear la tarea.\n\r");

	Timer1 = xTimerCreate("Muestro de adc",
			pdMS_TO_TICKS(TIEMPO_ENVIAR_DATOS_BUSCAN), true, NULL,
			timerRtos_DatosPerifericos);
	if (Timer1 == NULL)
		PRINTF("Fallo al crear el timer.\n\r");

	return;
}

static void taskRtos_Nodo2(void *pvParameters)
{
	uint32_t event_notify;

	/* Configuracion de los led y botones */
	BOARD_InitLEDs();
	BOARD_InitButtons();

	canMsg_Nodo2_write.can_id = CAN_NODO_2_ID;	// Id del nodo 2.

	BaseType_t status = xTimerStart(Timer1, portMAX_DELAY);
	if (status != pdPASS)
		PRINTF("\n\rFallo al inciar el timer.\n\r");

	for (;;)
	{
		event_notify = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

		if (event_notify > 0)
		{
			/* Debe tomar los datos de la cola de datos */
			BaseType_t status = receiveFromQueue_Peek(&canMsg_Nodo2_read);
			if (status == pdFALSE)
				PRINTF("Fallo al recivir datos de la cola.\n\r");

			uint16_t adc_read;

			if (canMsg_Nodo2_read.can_id == Nodo1_id())
			{
				adc_read = canMsg_Nodo2_read.data[1];
				adc_read = (adc_read << 8) | canMsg_Nodo2_read.data[0];

				if (adc_read > LUZ_MAX)
					LED_ON;
				if (adc_read < LUZ_MIN)
					LED_OFF;
			}
		}
		__delay_ms(50);
	}

	vTaskDelete(NULL);

	return;
}

static void taskRtos_Nodo2_Write(void *pvParameters)
{
	for (;;)
	{
		EstPerifericos_t perifericos =
		{ .data = 0 };

		perifericos.LED_ROJO = ~GPIO_ReadPinInput(BOARD_LED_RED_GPIO,
		BOARD_LED_RED_PIN);
		perifericos.PULSADOR1 = ~GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_PIN);
		perifericos.PULSADOR2 = ~GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_PIN);

		canMsg_Nodo2_write.data[0] = perifericos.data;
		canMsg_Nodo2_write.can_dlc = CAN_NODO_2_DLC;

		ERROR_t estado = mcp2515_sendMessage(&canMsg_Nodo2_write);
		if (estado == ERROR_OK)
		{
					PRINTF("\nNodo 2.\n\r");
					PRINTF("Mensaje enviado\n\r");
					PRINTF("ID\tDLC\tDATA\n\r");
					PRINTF("%d\t%d\t", canMsg_Nodo2_write.can_id, canMsg_Nodo2_write.can_dlc);

					for (uint8_t i = 0; i < 8; i++)
					{
						PRINTF("%d ", canMsg_Nodo2_write.data[i]);
					}
					PRINTF("\n\r");
		}
		else
		{
			PRINTF("\nError al enviar\n\r");
		}

		__delay_ms(500);
	}

	vTaskDelete(NULL);

	return;
}

static void timerRtos_DatosPerifericos(void *pvParameters)
{
//	EstPerifericos_t perifericos =
//	{ .data = 0 };
//
//	perifericos.LED_ROJO = ~GPIO_ReadPinInput(BOARD_LED_RED_GPIO,
//	BOARD_LED_RED_PIN);
//	perifericos.PULSADOR1 = ~GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_PIN);
//	perifericos.PULSADOR2 = ~GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_PIN);
//
//	canMsg_Nodo2_write.data[0] = perifericos.data;
//	canMsg_Nodo2_write.can_dlc = CAN_NODO_2_DLC;

	return;
}

extern canid_t Nodo2_id(void)
{
	canid_t id = CAN_NODO_2_ID;

	return id;
}

extern void Nodo2_giveNotify(void)
{
	xTaskNotifyGive(TaskNodo2_Handle);

	return;
}
