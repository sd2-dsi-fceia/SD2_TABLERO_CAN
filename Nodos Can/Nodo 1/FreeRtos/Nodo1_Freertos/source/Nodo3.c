/*
 * Nodo3.c
 *
 *  Created on: 30 ago. 2024
 *      Author: agustin
 */

/**
 * @file Nodo 3
 * @author Zuliani, Agustin
 */

#include "Nodo3.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "CanApi.h"
#include "can.h"

#include "Nodo1.h"
#include "Nodo2.h"

#include "fsl_debug_console.h"

#define CAN_NODO3_ID	30

#define TIEMPO_SALIDA_SERIE	1000

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

static struct can_frame canMsg_Nodo3_read;
static volatile uint16_t adc_read;
static EstPerifericos_t perifericos;
static bool estSW1, estSW2, estLedRojo;

TaskHandle_t TaskNodo3_Handle;
TimerHandle_t TimerSerial_Handle;

/**
 * @brief Tarea del nodo 3.
 */
static void taskRtos_Nodo3(void *pvParameters);
/**
 * @brief Envia los datos por puerto serie.
 * @note Podria activarse solamente una bandera
 * dado que los datos por puerto serie llevan
 * un tiempo considerable y los timers tienen la prioridad mas
 * alta.
 */
static void timerRtos_DatosSerial(void *pvParameters);

extern BaseType_t receiveFromQueue(struct can_frame *dato);

extern void Nodo3_init(void)
{
	PRINTF("\nNombre: Nodo 3.\n\r");

	BaseType_t status = xTaskCreate(taskRtos_Nodo3, "Task Nodo 3",
	configMINIMAL_STACK_SIZE+100, NULL, 2, &TaskNodo3_Handle);
	if (status == pdFALSE)
		PRINTF("Fallo al crear la tarea.\n\r");

	TimerSerial_Handle = xTimerCreate("Muestro de adc",
			pdMS_TO_TICKS(TIEMPO_SALIDA_SERIE), true, NULL,
			timerRtos_DatosSerial);
	if (TimerSerial_Handle == NULL)
		PRINTF("Fallo al crear el timer.\n\r");

	return;
}

static void taskRtos_Nodo3(void *pvParameters)
{
	static enum
	{
		NODO1 = 0, NODO2,
	} NodoSubs;
	NodoSubscriptions_t Subscripciones[2] =
	{
	{ .IdSub = 10, .taskHandle = TaskNodo3_Handle, },
	{ .IdSub = 20, .taskHandle = TaskNodo3_Handle, }, };
	uint32_t event_notify;

	Error_Can_t statusSub = CAN_Subscribe(Subscripciones[NODO1].IdSub,
			Subscripciones[NODO1].taskHandle);
	if (statusSub != ERROR_CAN_OK)
	{
		if (statusSub == ERROR_CAN_MEMORY)
			PRINTF("\n\rError: no hay memoria suficiente.\n\r");
		else
			PRINTF("\n\rError desconocido.\n\r");
	}

	statusSub = CAN_Subscribe(Subscripciones[NODO2].IdSub,
			Subscripciones[NODO2].taskHandle);
	if (statusSub != ERROR_CAN_OK)
	{
		if (statusSub == ERROR_CAN_MEMORY)
			PRINTF("\n\rError: no hay memoria suficiente.\n\r");
		else
			PRINTF("\n\rError desconocido.\n\r");
	}

	/* Espera el evento de sincronizacion. */
	CAN_getEvent();

	BaseType_t status = xTimerStart(TimerSerial_Handle, portMAX_DELAY);
	if (status != pdPASS)
		PRINTF("Fallo al inciar el timer.\n\r");

	for (;;)
	{
		event_notify = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (event_notify > 0)
		{
			/* Debe tomar los datos de la cola de datos */
			Error_Can_t statusRx = CAN_readMsg(&canMsg_Nodo3_read,
					Subscripciones[NODO1].IdSub,
					Subscripciones[NODO1].taskHandle);
//			if (statusRx != ERROR_CAN_OK)
//			{
//				if (statusRx == ERROR_CAN_QUEUERX)
//					PRINTF("\n\rError al leer desde la cola de datos.\n\r");
//				else
//					PRINTF("\n\rError no se encontro la cola de datos.\n\r");
//			}

			if (canMsg_Nodo3_read.can_id == Subscripciones[NODO1].IdSub)
			{
				adc_read = canMsg_Nodo3_read.data[1];
				adc_read = (adc_read << 8) | canMsg_Nodo3_read.data[0];
			}

			statusRx = CAN_readMsg(&canMsg_Nodo3_read,
					Subscripciones[NODO2].IdSub,
					Subscripciones[NODO2].taskHandle);
//			if (statusRx != ERROR_CAN_OK)
//			{
//				if (statusRx == ERROR_CAN_QUEUERX)
//					PRINTF("\n\rError al leer desde la cola de datos.\n\r");
//				else
//					PRINTF("\n\rError desconocido.\n\r");
//			}

			if (canMsg_Nodo3_read.can_id == Subscripciones[NODO2].IdSub)
			{
				perifericos.data = canMsg_Nodo3_read.data[0];

				estLedRojo = perifericos.LED_ROJO;
				estSW1 = perifericos.PULSADOR1;
				estSW2 = perifericos.PULSADOR2;
			}
		}
	}

	vTaskDelete(NULL);

	return;
}

static void timerRtos_DatosSerial(void *pvParameters)
{
	PRINTF("\nNodo 3.\n\r");
	PRINTF("Parametros de medidos:\n\r");
	PRINTF("Led rojo: %d\n\r", estLedRojo);
	PRINTF("Boton 1: %d\n\r", estSW1);
	PRINTF("Boton 2: %d\n\r", estSW2);
	PRINTF("Sensor de luz: %d\n\r", adc_read);

	return;
}

extern canid_t Nodo3_id(void)
{
	canid_t id = CAN_NODO3_ID;

	return id;
}

extern void Nodo3_giveNotify(void)
{
	xTaskNotifyGive(TaskNodo3_Handle);

	return;
}
