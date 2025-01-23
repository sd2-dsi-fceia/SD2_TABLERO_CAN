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
 * @file    Nodo1_Baremetal.c
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
#include "LDR.h"
#include "spi.h"
#include "mcp2515.h"

/* TODO: insert other definitions and declarations here. */
#define PIN_NUMBER 17

#define CAN_LDR_ID	10

uint16_t Delay1s = TIEMPO_DE_MUESTREO_LDR;

/**
 * @brief Mensaje de tipo can.
 */
struct can_frame canMsg1;

// DECLARACION DE FUNCIONES.
//..................................................................................
/**
 * @brief Inicializacion de perifericos.
 */
static void perifericos_init(void);
/**
 * @brief Funcion de escritura del mensaje can.
 */
static void canmsg_escritura(void);
//..................................................................................

// CUERPO DE FUNCIONES
//..................................................................................
int main(void) {
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
#endif

	/* Mensaje de Inicializacion en puerto serie. */
	PRINTF("\nNombre: Nodo 1\n\r");
	PRINTF("Descripcion: Este nodo se encarga de producir un "
			"valor analogico durante cierto periodo de tiempo"
			"con un Id=10.\n\r");

	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	/* Inicializacion de LDR. */
	Error_LDR_t error = LDR_init();

	if (error != ERROR_LDR_OK)
		PRINTF("Fallo al inicializar el adc.\n\r");

	/* Inicializacion de can */
	perifericos_init();

	canMsg1.can_id = CAN_LDR_ID;
	canMsg1.can_dlc = 2;

	while (1) {
		if (LDR_getConvComplete()) {
			PRINTF("Valor del Adc: %d\n\r", LDR_UltimaConversion());
			LDR_clearConvComplete();

			canMsg1.data[1] = (LDR_UltimaConversion() & 0xff00) >> 8;
			canMsg1.data[0] = (LDR_UltimaConversion() & 0x00ff);

			canmsg_escritura();
		}

	}
	return 0;
}
//..................................................................................
static void canmsg_escritura(void) {
	ERROR_t estado;

	estado = mcp2515_sendMessage(&canMsg1);

	if (estado == ERROR_OK) {
		PRINTF("\nMensaje enviado\n\r");
		PRINTF("ID\tDLC\tDATA\n\r");
		PRINTF("%d\t%d\t", canMsg1.can_id, canMsg1.can_dlc);

		for (uint8_t i = 0; i < 8; i++) {
			PRINTF("%d ", canMsg1.data[i]);
		}
		PRINTF("\n\r");
	} else {
		PRINTF("\nError al enviar\n\r");
	}

	return;
}
//..................................................................................
static void perifericos_init(void) {
	ERROR_t error;

	mcp2515_init();	// Configura los pines del spi

	error = mcp2515_reset();	// Configura el modulo

	if (error != ERROR_OK)
		PRINTF("Fallo al resetear el modulo\n\r");

	error = mcp2515_setBitrate(CAN_125KBPS, MCP_8MHZ);

	if (error != ERROR_OK)
		PRINTF("Fallo al setear el bit rate\n\r");

	error = mcp2515_setNormalMode();
	if (error != ERROR_OK)
		PRINTF("Fallo al setear el modo normal\n\r");

	return;
}
//..................................................................................
void SysTick_Handler(void) {
	if (Delay1s != 0)
		Delay1s--;
	else
		Delay1s = TIEMPO_DE_MUESTREO_LDR, LDR_convertir();

	return;
}
//..................................................................................
void ADC16_IRQ_HANDLER_FUNC(void) {
	LDR_setConvComplete(); /* Setea la bandera en 1 */
	LDR_read(); /* Lee el registro */

	return;
}
