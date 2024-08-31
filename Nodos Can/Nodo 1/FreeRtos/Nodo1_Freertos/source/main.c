/*
 * Copyright 2016-2024 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    Nodo1_Freertos.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
/* TODO: insert other include files here. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mcp2515.h"
#include "can.h"

#include "Nodo1.h"
#include "Nodo2.h"
#include "Nodo3.h"
/* TODO: insert other definitions and declarations here. */
#define PIN_NUMBER 17

struct can_frame canMsgRead;
/**
 * @brief Manipulador de tarea de lectura.
 */
TaskHandle_t TaskRx_handler;
xQueueHandle queue_RxCan;

// Tamaño de la cola y del dato que va a manejar
#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE sizeof(canMsgRead)

/**
 * @brief Inicializacion de interrupciones.
 */
static void interrupt_init(void);
/**
 * @brief Tarea de lectura de datos can.
 */
static void taskRtos_Read(void *pvParameters);
/**
 * @brief Carga la cola de datos a otra de un nodo.
 * @return Estado de la recepcion.
 */
extern BaseType_t receiveFromQueue(struct can_frame *dato);
/**
 * @brief Obtiene los datos de la cola de datos sin eliminarlos.
 * @return Estado de la recepcion.
 */
extern BaseType_t receiveFromQueue_Peek(struct can_frame *dato);

/*
 * @brief   Application entry point.
 */
int main(void)
{
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
#endif

	interrupt_init();

	Nodo1_init();
//	Nodo2_init();
//	Nodo3_init();

	BaseType_t status = xTaskCreate(taskRtos_Read, "Task read can",
	configMINIMAL_STACK_SIZE, NULL, 2, &TaskRx_handler);
	if (status == pdFALSE)
		PRINTF("Fallo al crear la tarea.\n\r");

	queue_RxCan = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
	if (queue_RxCan == NULL)
	{
		PRINTF("Fallo al crear la cola de datos.\n\r");
	}

	vTaskStartScheduler();

	while (1)
	{
	}

	return 0;
}

static void taskRtos_Read(void *pvParameters)
{
	uint32_t event_notify;

	for (;;)
	{
		event_notify = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (event_notify != 0)
		{
			ERROR_t estado;

			estado = mcp2515_readMessage(&canMsgRead);

			if (estado != ERROR_NOMSG)
			{
				PRINTF("\nMensaje de recepcion\n\r");
				PRINTF("ID\tDLC\tDATA\n\r");
				PRINTF("%d\t%d\t", canMsgRead.can_id, canMsgRead.can_dlc);

				for (uint8_t i = 0; i < 8; i++)
				{
					PRINTF("%d ", canMsgRead.data[i]);
				}
			}
			else
			{
				PRINTF("No hubo mensajes");
			}

			PRINTF("\n\r");

			/* Notifica a los nodos subscriptos */
			if (canMsgRead.can_id == Nodo1_id())
			{
				Nodo2_giveNotify();
				Nodo3_giveNotify();
			}
			else if (canMsgRead.can_id == Nodo2_id())
			{
				Nodo3_giveNotify();
			}
		}
	}

	vTaskDelete(NULL);

	return;
}

extern BaseType_t receiveFromQueue_Peek(struct can_frame *dato)
{
    return xQueuePeek(queue_RxCan, dato, portMAX_DELAY);
}

extern BaseType_t receiveFromQueue(struct can_frame *dato)
{
	/* Recive la cola de datos */
	return xQueueReceive(queue_RxCan, dato, portMAX_DELAY);
}

static void interrupt_init(void)
{
	CLOCK_EnableClock(kCLOCK_PortA); // Por ejemplo, para el puerto A

	port_pin_config_t config =
	{ .pullSelect = kPORT_PullUp, .slewRate = kPORT_FastSlewRate,
			.passiveFilterEnable = kPORT_PassiveFilterDisable, .driveStrength =
					kPORT_LowDriveStrength, .mux = kPORT_MuxAsGpio };

	PORT_SetPinConfig(PORTA, PIN_NUMBER, &config);
	PORT_SetPinInterruptConfig(PORTA, PIN_NUMBER, kPORT_InterruptFallingEdge); // Configura interrupción por flanco descendente

	NVIC_EnableIRQ(PORTA_IRQn); // Habilita la interrupción para el puerto A
	NVIC_SetPriority(PORTA_IRQn, 2);

	gpio_pin_config_t gpioConfig =
	{ .pinDirection = kGPIO_DigitalInput, .outputLogic = 0U };

	GPIO_PinInit(GPIOA, PIN_NUMBER, &gpioConfig);

	return;
}

void PORTA_IRQHandler(void)
{
#if USE_FREERTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif

	// Obtiene el estado de las banderas de interrupción del puerto A
	uint32_t interruptFlags = GPIO_GetPinsInterruptFlags(GPIOA);

	if (interruptFlags & (1U << PIN_NUMBER))
	{
		// Código que se ejecutará cuando ocurra la interrupción

		/*
		 * @note
		 * Se configuraron por defecto interrupciones para rx0, rx1, err, merr. En la
		 * funcion de mcp2515_reset() se pueden configurar algunas mas.
		 * */

		/* Leemos las interrupciones generadas */
		ERROR_t error = mcp2515_getInterrupts();
		if (error != ERROR_OK)
			PRINTF("Fallo al leer la interrupcion\n\r");

		/* Detectamos las que nos sirvan */
		if (mcp2515_getIntERRIF())
		{
			// Acciones ...
			PRINTF("Error interrupt flag\n\r");

			// Limpiamos la bandera
			mcp2515_clearERRIF();
		}

		if (mcp2515_getIntMERRF())
		{
			// Acciones ...
			PRINTF("Message error interrupt flag\n\r");

			// Limpiamos la bandera
			mcp2515_clearMERR();
		}

		if (mcp2515_getIntRX0IF() || mcp2515_getIntRX1IF())
		{
			// Acciones ...
#if USE_FREERTOS
			vTaskNotifyGiveFromISR(TaskRx_handler, &xHigherPriorityTaskWoken);
#else
			Rx_flag_mcp2515 = true;
#endif

			// La bandera se limpia en la funcion de recepcion
			// mcp2515_readMessage().
		}

		/*
		 * Descomentar si es necesario tener en cuenta dicha interrupcion.
		 * Ademas debe habilitarse en la funcion de reset del mcp2515.
		 * */
		//    	if (mcp2515_getIntTX0IF() ||
		//    		mcp2515_getIntTX1IF() ||
		//			mcp2515_getIntTX2IF())
		//    	{
		//    		// Acciones ...
		////    		PRINTF("Mensaje enviado\n\r");
		//
		//    		// Limpiamos la bandera
		//    		mcp2515_clearTXInterrupts();
		//    	}
		// Limpia la bandera de interrupción
		GPIO_ClearPinsInterruptFlags(GPIOA, 1U << PIN_NUMBER);
	}

#if USE_FREERTOS
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

	return;
}
