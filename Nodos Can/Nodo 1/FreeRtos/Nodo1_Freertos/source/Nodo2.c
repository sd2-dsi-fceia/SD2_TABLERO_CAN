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

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "CanApi.h"
#include "can.h"

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

#define RELE_MIN	500
#define RELE_MAX	3000
#define RELE_ON		GPIO_SetPinsOutput(BOARD_RELE1_GPIO, BOARD_RELE1_GPIO_PIN_MASK)
#define RELE_OFF	GPIO_ClearPinsOutput(BOARD_RELE1_GPIO, BOARD_RELE1_GPIO_PIN_MASK)

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
/**
 * @brief Mensaje can del nodo 2 para lectura.
 */
static struct can_frame canMsg_Nodo2_read;

/**
 * @brief Handle del temporizador 1.
 */
TimerHandle_t Timer1;
/**
 * @brief Handle de la tarea del nodo 2.
 */
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
	NodoSubscriptions_t Subscripciones =
	{ .IdSub = 10, .taskHandle = TaskNodo2_Handle, };
	uint32_t event_notify;

	/* Configuracion de los led y botones */
	BOARD_InitLEDs();
	BOARD_InitButtons();

	canMsg_Nodo2_write.can_id = CAN_NODO_2_ID;	// Id del nodo 2.

	BaseType_t status = xTimerStart(Timer1, portMAX_DELAY);
	if (status != pdPASS)
		PRINTF("\n\rFallo al inciar el timer.\n\r");

	Error_Can_t statusSub = CAN_Subscribe(Subscripciones.IdSub,
			Subscripciones.taskHandle);
	if (statusSub != ERROR_CAN_OK)
	{
		if (statusSub == ERROR_CAN_MEMORY)
			PRINTF("\n\rError: no hay memoria suficiente.\n\r");
		else
			PRINTF("\n\rError desconocido.\n\r");
	}

	RELE_OFF;
	LED_OFF;

	/* Espera el evento de sincronizacion. */
	CAN_getEvent();

	for (;;)
	{
		event_notify = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

		if (event_notify > 0)
		{
			/* Debe tomar los datos de la cola de datos */
			Error_Can_t statusRx = CAN_readMsg(&canMsg_Nodo2_read,
					Subscripciones.IdSub, Subscripciones.taskHandle);
			if (statusRx != ERROR_CAN_OK)
				PRINTF("\n\r");

			uint16_t adc_read;

			if (canMsg_Nodo2_read.can_id == Subscripciones.IdSub)
			{
				adc_read = canMsg_Nodo2_read.data[1];
				adc_read = (adc_read << 8) | canMsg_Nodo2_read.data[0];

				if (adc_read > LUZ_MAX)
					LED_ON;
				else if (adc_read < LUZ_MIN)
					LED_OFF;

				/* Estado del rele. */
				if (adc_read > RELE_MAX)
					RELE_ON;
				else if (adc_read < RELE_MIN)
					RELE_OFF;
			}
		}
		__delay_ms(50);
	}

	vTaskDelete(NULL);

	return;
}

static void taskRtos_Nodo2_Write(void *pvParameters)
{
	/* Espera el evento de sincronizacion. */
	CAN_getEvent();

	for (;;)
	{
		Error_Can_t statusTx = CAN_sendMsg(&canMsg_Nodo2_write,
				pdMS_TO_TICKS(200));
		if (statusTx != ERROR_CAN_OK)
		{
			if (statusTx == ERROR_CAN_QUEUETX)
				PRINTF("\n\rError al enviar datos a la cola.\n\r");
			else
				PRINTF("\n\rError desconocido.\n\r");
		}

		__delay_ms(1000);

		GPIO_TogglePinsOutput(BOARD_LED_GREEN_GPIO,
		BOARD_LED_GREEN_GPIO_PIN_MASK);
	}

	vTaskDelete(NULL);

	return;
}

static void timerRtos_DatosPerifericos(void *pvParameters)
{
	EstPerifericos_t perifericos =
	{ .data = 0 };

	perifericos.LED_ROJO = ~GPIO_ReadPinInput(BOARD_LED_RED_GPIO,
	BOARD_LED_RED_PIN);
	perifericos.PULSADOR1 = ~GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_PIN);
	perifericos.PULSADOR2 = ~GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_PIN);

	canMsg_Nodo2_write.data[0] = perifericos.data;
	canMsg_Nodo2_write.can_dlc = CAN_NODO_2_DLC;

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
