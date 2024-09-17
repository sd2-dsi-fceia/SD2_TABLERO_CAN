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
 * @file    Nodo3_Baremetal.c
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
#include "can.h"
#include "CanApi.h"
/* TODO: insert other definitions and declarations here. */
#define TIEMPO_DE_SALIDA	500

uint16_t Delay1s = TIEMPO_DE_SALIDA;
/**
 * @brief Interrupcion por recepcion de datos del modulo can.
 *
 * Cuando se genera una interrupcion por recepcion se activa
 * la bandera y luego dicha bandera se detecta por polling en
 * el programa principal.
 *
 * @note Debe ser de tipo volatile para que no sea optimizada
 * por el compilador, dado que es una variable que se modifica
 * en una interrupcion.
 */
volatile static bool Rx_flag_mcp2515 = false;

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
 * @brief Mensaje de lectura.
 */
struct can_frame canMsgRead =
{ .can_dlc = 0, .can_id = 0, };

volatile uint16_t adc_read;
EstPerifericos_t perifericos;

bool estSW1, estSW2, estLedRojo;

static void init_Can(void);
/**
 * @brief Salida por serie de datos del nodo 3.
 */
static void canmsg_procesar(void);
/**
 * @brief Funcion callback para nodo 1.
 */
static void Callback_Nodo1(canid_t SubcriberId, canid_t nodeId);
/**
 * @brief Funcion callback para nodo 2.
 */
static void Callback_Nodo2(canid_t SubcriberId, canid_t nodeId);
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

	PRINTF("\nNombre: Nodo 3\n\r");
	PRINTF("Descripcion: Este nodo se encarga de recibir"
			"datos desde el modulo can y luego msotrar"
			"en un puerto serie dicho resultados.\n\r");
	PRINTF("Materia: Sistemas digitales 2\n\r");

	/* Incializamos todos los parametros de can. */
	init_Can();

	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	while (1)
	{
		if (Delay1s == 0)
			canmsg_procesar();
	}
	return 0;
}
//---------------------------------------------------------------------------------------
static void init_Can(void)
{
	/* Inicializamos el can. */
	CAN_init();

	/* Configuro los filtros y mascaras para la recepcion. */
	Error_Can_t result = CAN_setMask(MASK0, false, 0x7FF); // Máscara 0x7FF, no extendido (ID estándar)
	if (result != ERROR_CAN_OK) {
		PRINTF("\n\rError: al configurar la mascara.\n\r");
	}

	result = CAN_setFilter(RXF0, false, 10); // Filtro para ID 10, no extendido (ID estándar)
	if (result != ERROR_CAN_OK) {
		PRINTF("\n\rError: al configurar el filtro.\n\r");
	}

	result = CAN_setFilter(RXF1, false, 20); // Filtro para ID 20, no extendido (ID estándar)
	if (result != ERROR_CAN_OK) {
		PRINTF("\n\rError: al configurar el filtro.\n\r");
	}

	/* Creamos las subscriones a los distintos nodos. */
	Error_Can_t error = CAN_Subscribe(10, 30, Callback_Nodo1);
	assert(error != ERROR_CAN_MEMORY);

	error = CAN_Subscribe(20, 30, Callback_Nodo2);
	assert(error != ERROR_CAN_MEMORY);

	return;
}
//---------------------------------------------------------------------------------------
static void Callback_Nodo1(canid_t SubcriberId, canid_t nodeId)
{
	CAN_readMsg(&canMsgRead, nodeId, SubcriberId);

	adc_read = canMsgRead.data[1];
	adc_read = (adc_read << 8) | canMsgRead.data[0];

	return;
}
//---------------------------------------------------------------------------------------
static void Callback_Nodo2(canid_t SubcriberId, canid_t nodeId)
{
	CAN_readMsg(&canMsgRead, nodeId, SubcriberId);

	perifericos.data = canMsgRead.data[0];

	estLedRojo = perifericos.LED_ROJO;
	estSW1 = perifericos.PULSADOR1;
	estSW2 = perifericos.PULSADOR2;

	return;
}
//---------------------------------------------------------------------------------------
static void canmsg_procesar(void)
{
//	PRINTF("\n\r> Salida por consola\n\r");
	PRINTF("Led rojo: %d\r\n", estLedRojo);
	PRINTF("Boton 1: %d\r\n", estSW1);
	PRINTF("Boton 2: %d\r\n", estSW2);
	PRINTF("Sensor de luz: %d\r\n", adc_read);

	return;
}
//---------------------------------------------------------------------------------------
extern void callbackTimeout(void)
{
	init_Can();

	return;
}
//---------------------------------------------------------------------------------------
void SysTick_Handler(void)
{
	if (Delay1s != 0)
		Delay1s--;
	else
		Delay1s = TIEMPO_DE_SALIDA;

	if (CAN_getTimer())
	{
		CAN_eventTx();
		CAN_eventRx();
	}

	return;
}
