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
 * @file    Nodo2_Baremetal.c
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
#include "CanApi.h"
#include "can.h"
/* TODO: insert other definitions and declarations here. */
#define CAN_NODO_2_ID	20

#define TIEMPO_DE_MUESTRA_ESTADOS	1000

#define BOARD_LED_RED_GPIO_PIN_MASK (1U << 29U)

#define LUZ_MIN	500
#define LUZ_MAX	1500
#define LED_ON	GPIO_ClearPinsOutput(BOARD_LED_RED_GPIO,BOARD_LED_RED_GPIO_PIN_MASK)
#define LED_OFF	GPIO_SetPinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN_MASK)

static uint16_t Delay1s = TIEMPO_DE_MUESTRA_ESTADOS;
static bool Rx_msgFlag = false;

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
 * @brief Mensaje 1 de can.
 */
struct can_frame canMsg1;
/**
 * @brief Mensaje de lectura.
 */
struct can_frame canMsgRead =
{ .can_dlc = 0, .can_id = 0, };

/**
 * @brief Escritura del modulo can.
 */
static void canmsg_writeToBus(void);
/**
 * @brief Lectura del modulo can.
 */
static void serialPort(void);
/**
 * @brief Procesa la informacion leida.
 */
static void canmsg_sensorLuz(void);
/**
 * @brief Funcion de callback para nodo 1 (id = 10).
 * @param[in] SubcriberId Id del nodo origen.
 * @param[in] nodeId Id del nodo que se quiere recibir la informacion.
 */
static void Callback_Nodo1(canid_t SubcriberId, canid_t nodeId);

/**
 * @brief Funcion de callback si termina el timeout.
 */
extern void callbackTimeout(void);

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

	/* Configuracion de los led y botones */
	BOARD_InitLEDs();
	BOARD_InitButtons();

	PRINTF("\nNombre: Nodo 2\n\r");
	PRINTF("Descripcion: Este nodo se encarga de producir un "
				"valor de los estados de sus perifericos y ademas"
				"obtener datos del ldr proveniente de otro modulo"
				"can. Ademas cuenta con un Id=20.\n\r");
	PRINTF("Materia: Sistemas digitales 2\n\r");

	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	/* Configura de can */
	CAN_init();

	canMsg1.can_id = CAN_NODO_2_ID;
	canMsg1.can_dlc = 1;

	LED_ON;

	Error_Can_t error = CAN_Subscribe(54, canMsg1.can_id, Callback_Nodo1);
	assert(error != ERROR_CAN_MEMORY);

	while (1)
	{
		if (Rx_msgFlag) serialPort(), Rx_msgFlag = false;
		if (Delay1s == 0) canmsg_writeToBus();
	}

	return 0;
}

static void canmsg_writeToBus(void)
{
	Error_Can_t estado;

	EstPerifericos_t perifericos =
	{ .data = 0 };

	perifericos.LED_ROJO = ~GPIO_ReadPinInput(BOARD_LED_RED_GPIO,
	BOARD_LED_RED_PIN);
	perifericos.PULSADOR1 = ~GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_PIN);
	perifericos.PULSADOR2 = ~GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_PIN);

	canMsg1.data[0] = perifericos.data;
	canMsg1.can_dlc = 1;

	estado = CAN_sendMsg(&canMsg1);

	if (estado == ERROR_CAN_OK)
	{
		PRINTF("\n\rMensaje enviado\n\r");
		PRINTF("ID\tDLC\tDATA\n\r");
		PRINTF("%d\t%d\t", canMsg1.can_id, canMsg1.can_dlc);

		for (uint8_t i = 0; i < 8; i++)
		{
			PRINTF("%d ", canMsg1.data[i]);
		}
		PRINTF("\n\r");
	}
	else
	{
		if (estado == ERROR_CAN_QUEUETX_FULL)
		PRINTF("\n\rError: buffers de transmision llenos.\n\r");
	}

	return;
}

static void Callback_Nodo1(canid_t SubcriberId, canid_t nodeId)
{
	/*
	 * RECOMENDACIONES DE USO DE CALLBACKS: Importante mantener
	 * las funciones de callback lo mas cortas posible y evitar
	 * el uso de rutinas que demoren tiempo como PRINTF.
	 */
	Error_Can_t error = CAN_readMsg(&canMsgRead, nodeId, SubcriberId);
	if (error != ERROR_CAN_OK)
	{
		if (error == ERROR_CAN_NOT_FOUND)
		PRINTF("\n\rError: no se encuentra el id del nodo destino.\n\r");
		if (error == ERROR_CAN_QUEUERX_EMPTY)
		PRINTF("\n\rError: no hay datos en el buffer.\n\r");
	}

	canmsg_sensorLuz();

	Rx_msgFlag = true;	// Solo utilizado para printear el mensaje en el loop infinito y no
						// dentro de la callback.

	return;
}

static void serialPort(void)
{
	PRINTF("\n\rMensaje de recepcion\n\r");
	PRINTF("ID\tDLC\tDATA\n\r");
	PRINTF("%d\t%d\t", canMsgRead.can_id, canMsgRead.can_dlc);

	for (uint8_t i = 0; i < 8; i++)
	{
		PRINTF("%d ", canMsgRead.data[i]);
	}

	PRINTF("\n\r");

	return;
}

static void canmsg_sensorLuz(void)
{
	uint16_t adc_read;

	if (canMsgRead.can_id == 10)
	{
		adc_read = canMsgRead.data[1];
		adc_read = (adc_read << 8) | canMsgRead.data[0];

		if (adc_read > LUZ_MAX)
		LED_ON;
		if (adc_read < LUZ_MIN)
		LED_OFF;
	}

	return;
}

void SysTick_Handler(void)
{
	if (Delay1s != 0) Delay1s--;
	else Delay1s = TIEMPO_DE_MUESTRA_ESTADOS;

	/*
	 * Si el spi es lento esto puede demorar por lo que se recomienda
	 * quitar CAN_eventTx de la rutina del systick. Se esta utilizando
	 * solo para prueba.
	 *
	 * En entorno de depuracion ambas funciones generan una señal de error
	 * si no hay eventos disponibles por lo que pueden utilizarse para verificar.
	 * En este caso no vamos a utilizarlas.
	 */
	if (CAN_getTimer())
	{
		CAN_eventTx();
		CAN_eventRx();
	}

	return;
}

extern void callbackTimeout(void)
{
	// Acciones si sucede esto
	CAN_init();	// Reinicio el modulo si fuese necesario

	return;
}
