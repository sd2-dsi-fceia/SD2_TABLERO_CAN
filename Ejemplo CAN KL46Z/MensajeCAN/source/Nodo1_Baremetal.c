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
#include "spi.h"
#include "mcp2515.h"

/* TODO: insert other definitions and declarations here. */
#define PIN_NUMBER 17

#define ENVIAR_MENSAJE_PERIODO	1000


#define SW1_GPIO GPIOC
#define SW1_PIN	3
#define SW3_GPIO GPIOC
#define SW3_PIN	12

#define CAN_ID	10

uint16_t Delay1s = ENVIAR_MENSAJE_PERIODO;

/**
 * @brief Mensaje de tipo can.
 */
struct can_frame canMsg1;

typedef union {
	struct {
		unsigned SW1:1;
		unsigned SW3:1;
		unsigned SIN_USO:6;
	};
	uint8_t dato;
} EstPerifericos_t;

EstPerifericos_t perifericos;

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
	PRINTF("\nNombre: Nodo KL46Z\n\r");
	PRINTF("Descripcion: Este nodo se encarga mandar los valores "
			"digitales de los pulsadores "
			"con un Id=10.\n\r");

	// Utilizado para el SysTick
	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	/* Inicializacion de can */
	perifericos_init();

	canMsg1.can_id = CAN_ID;
	canMsg1.can_dlc = 1;

	while (1) {
		if (!Delay1s)
		{
			Delay1s = ENVIAR_MENSAJE_PERIODO;

			canmsg_escritura();
		}
	}
	return 0;
}
//..................................................................................
static void canmsg_escritura(void) {
	ERROR_t estado;

	// Estado de los pines
	perifericos.SW1 = !GPIO_PinRead(SW1_GPIO, SW1_PIN);
	perifericos.SW3 = !GPIO_PinRead(SW3_GPIO, SW3_PIN);

	// Mensaje
	canMsg1.data[0] = perifericos.dato;

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

	error = mcp2515_reset();	// Configura el modulo

	if (error != ERROR_OK)
		PRINTF("Fallo al resetear el modulo\n\r");

	error = mcp2515_setBitrate(CAN_125KBPS, MCP_8MHZ);

	if (error != ERROR_OK)
		PRINTF("Fallo al setear el bit rate\n\r");

	error = mcp2515_setNormalMode();
	if (error != ERROR_OK)
		PRINTF("Fallo al setear el modo normal\n\r");

	// Configura los botones
	CLOCK_EnableClock(kCLOCK_PortC); // Habilitar reloj para el puerto C

	port_pin_config_t config_switch = {
		.pullSelect = kPORT_PullUp, // Activar Pull-Up interno
		.mux = kPORT_MuxAsGpio      // Configurar como GPIO
	};

	PORT_SetPinConfig(PORTC, SW1_PIN, &config_switch);
	PORT_SetPinConfig(PORTC, SW3_PIN, &config_switch);

	gpio_pin_config_t gpio_config = {
		.pinDirection = kGPIO_DigitalInput,
		.outputLogic = 0
	};

	GPIO_PinInit(SW1_GPIO, SW1_PIN, &gpio_config);
	GPIO_PinInit(SW3_GPIO, SW3_PIN, &gpio_config);

	return;
}
//..................................................................................
void SysTick_Handler(void) {
	if (Delay1s != 0)
		Delay1s--;

	return;
}
