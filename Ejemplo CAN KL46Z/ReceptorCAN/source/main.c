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
#include "mcp2515.h"
#include "can.h"
/* TODO: insert other definitions and declarations here. */
#define TIEMPO_DE_LECTURA_BUS_CAN	300

uint16_t Delay200ms = TIEMPO_DE_LECTURA_BUS_CAN;

/**
 * @brief Mensaje de lectura.
 */
struct can_frame canMsgRead = { .can_dlc = 0, .can_id = 0, };

// DECLARACION DE FUNCIONES.
//---------------------------------------------------------------------------------------
/**
 * @brief Inicializa todas las configuraciones.
 */
static void init(void);
/**
 * @brief Lectura del mensaje can desde el bus.
 */
static void canmsg_readFromBus(void);
/**
 * @brief Salida por serie de datos del nodo 3.
 */
static void canmsg_procesar(void);
//---------------------------------------------------------------------------------------

// CUERPO DE FUNCIONES.
//---------------------------------------------------------------------------------------
int main(void) {
	init();

	PRINTF("\nNombre: Nodo\n\r");
	PRINTF("Descripcion: Este nodo se encarga de recibir cualquier mensaje"
			"y mostrarlo en pantalla.\n\r");

	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	while (1) {
		if (!Delay200ms)
			canmsg_readFromBus();
	}

	return 0;
}
//---------------------------------------------------------------------------------------
static void init(void) {
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
#endif

	/* Configuracion de los led y botones */
	BOARD_InitLEDs();
	BOARD_InitButtons();

	/* Inicializa el modulo mcp2515. */
	ERROR_t status = mcp2515_reset();
	assert(status == ERROR_OK);	// Detiene el programa si hubo algún error al incializar

	status = mcp2515_setBitrate(CAN_125KBPS, MCP_8MHZ);
	assert(status == ERROR_OK);

	status = mcp2515_setNormalMode();
	assert(status == ERROR_OK);

	BOARD_InitBootPeripherals();

	return;
}
//---------------------------------------------------------------------------------------
static void canmsg_readFromBus(void) {
	// Reinicia el contador
	Delay200ms = TIEMPO_DE_LECTURA_BUS_CAN;

	// Recibe el mensaje
	ERROR_t status = mcp2515_readMessage(&canMsgRead);

	// Si no hay mensajes entonces no muestra nada
	if (status != ERROR_OK)
		return;

	// Muestra por pantalla el mensaje
	canmsg_procesar();

	return;
}
//---------------------------------------------------------------------------------------
static void canmsg_procesar(void) {
	PRINTF("\nID\tDLC\tDATA\n\r");
	PRINTF("%d\t%d\t", canMsgRead.can_id, canMsgRead.can_dlc);

	for(uint8_t i = 0; i < 8; i++)
	{
		PRINTF("%d ", canMsgRead.data[i]);
	}

	PRINTF("\n\r");

	return;
}
//---------------------------------------------------------------------------------------
void SysTick_Handler(void) {
	if (Delay200ms != 0)
		Delay200ms--;

	return;
}
