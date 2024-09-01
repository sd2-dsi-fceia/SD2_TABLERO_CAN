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

#define __delay_ms(x)	vTaskDelay(pdMS_TO_TICKS(x))

struct can_frame canMsgRead;
/**
 * @brief Manipulador de tarea de lectura.
 */
TaskHandle_t TaskRxCan_Handle = NULL;
xQueueHandle queue_RxCan;

// Tamaño de la cola y del dato que va a manejar
#define QUEUE_LENGTH 4
#define QUEUE_ITEM_SIZE sizeof(canMsgRead)

/**
 * @brief Inicializacion de perifericos.
 */
static void perifericos_init(void);
/**
 * @brief Inicializacion de interrupciones.
 */
static void interrupt_init(void);
/**
 * @brief Recepcion de los datos y procesamiento de la informacion.
 */
static void canmsg_receive(void);
/**
 * @brief Procesa la interrupcion de la recepcion.
 */
static void canmsg_interrupt(void);
/**
 * @brief Tarea de lectura de datos can.
 */
static void taskRtos_CanInterrupt(void *pvParameters);
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

	BaseType_t status = xTaskCreate(taskRtos_CanInterrupt, "Task read can",
	configMINIMAL_STACK_SIZE + 50, NULL, configMAX_PRIORITIES,
			&TaskRxCan_Handle);
	if (status != pdTRUE)
		PRINTF("Fallo al crear la tarea.\n\r");

	queue_RxCan = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
	if (queue_RxCan == NULL)
		PRINTF("Fallo al crear la cola de datos.\n\r");

	Nodo1_init();
	Nodo2_init();
//	Nodo3_init();

	vTaskStartScheduler();

	while (1)
	{
	}

	return 0;
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

static void taskRtos_CanInterrupt(void *pvParameters)
{
	uint32_t event_notify;

	PRINTF("\n\rTarea: lectura de bus can.\n\r");

	/* Configura de can */
	perifericos_init();
	__delay_ms(5);

	interrupt_init();
	__delay_ms(5);

	for (;;)
	{
//		event_notify = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
		xTaskNotifyWait(0, 0, &event_notify, portMAX_DELAY);

		if (event_notify > 0)
		{
			__delay_ms(3);
			canmsg_interrupt();	// Procesa la interrupcion

			event_notify--;
		}
	}

	vTaskDelete(NULL);

	return;
}

static void canmsg_receive(void)
{
	ERROR_t estado;

	estado = mcp2515_readMessage(&canMsgRead);

	if (estado != ERROR_NOMSG)
	{
		PRINTF("\n\r----------------------");
		PRINTF("\n\rMensaje de recepcion\n\r");
		PRINTF("ID\tDLC\tDATA\n\r");
		PRINTF("%d\t%d\t", canMsgRead.can_id, canMsgRead.can_dlc);

		for (uint8_t i = 0; i < 8; i++)
		{
			PRINTF("%d ", canMsgRead.data[i]);
		}

		PRINTF("\n\r----------------------");

		if (queue_RxCan != NULL)
		{
			UBaseType_t status = uxQueueSpacesAvailable(queue_RxCan);
			if (status > 0)
				xQueueSend(queue_RxCan, &canMsgRead, portMAX_DELAY);
			else
			{
				struct can_frame aux;

				BaseType_t status = receiveFromQueue(&aux);  // Elimina el dato más antiguo
				if (status == pdFALSE)
					PRINTF("\n\rFallo al leer la cola de datos.\n\r");

				status = xQueueSendToBack(queue_RxCan, &canMsgRead, pdMS_TO_TICKS(200));
				if (status != pdPASS)
					PRINTF("\n\rFallo al sobreescribir datos en la cola.\n\r");
			}
//			PRINTF("\n\rFallo: cola de datos llena.\n\r");
		}
		else
			PRINTF("\n\rFallo: puntero de cola nulo.\n\r");
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
		//				Nodo3_giveNotify();
	}
	else if (canMsgRead.can_id == Nodo2_id())
	{
		//				Nodo3_giveNotify();
	}

	return;
}

static void canmsg_interrupt(void)
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
		canmsg_receive();	// Procesa la informacion

		// Limpia banderas de interrupcion
	}

	return;
}

extern BaseType_t receiveFromQueue_Peek(struct can_frame *dato)
{
	if (queue_RxCan != NULL)
	{
		return xQueuePeek(queue_RxCan, dato, portMAX_DELAY);
	}
	return pdFALSE;  // Indica que la recepción falló
}

extern BaseType_t receiveFromQueue(struct can_frame *dato)
{
	if (queue_RxCan != NULL)
	{
		/* Recibe la cola de datos */
		return xQueueReceive(queue_RxCan, dato, portMAX_DELAY);
	}
	return pdFALSE;
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
	NVIC_SetPriority(PORTA_IRQn, 1);

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
#if USE_FREERTOS
		xTaskNotifyFromISR(TaskRxCan_Handle, 0, eIncrement,
				&xHigherPriorityTaskWoken);
#else
			Rx_flag_mcp2515 = true;
#endif
		GPIO_ClearPinsInterruptFlags(GPIOA, 1U << PIN_NUMBER);
	}

#if USE_FREERTOS
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

	return;
}

void vApplicationMallocFailedHook(void)
{
	PRINTF("\n\rError: Fallo de memoria dinamica.\n\r");
	for (;;)
		;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	PRINTF("\n\rError: StackOverFlow.\n\r");
	PRINTF("Tarea: %s", pcTaskName);

	/* También puedes detener el sistema, quedándote en un loop infinito */
	for (;;)
		;
}
