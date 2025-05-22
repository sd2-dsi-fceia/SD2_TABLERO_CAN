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
#define TIEMPO_DE_SALIDA	500
#define TIMEOUT	20000

uint16_t Delay1s = TIEMPO_DE_SALIDA;
uint16_t DelayTimeOut = TIMEOUT;

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

typedef union {
	struct {
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
struct can_frame canMsgRead = { .can_dlc = 0, .can_id = 0, };

volatile uint16_t adc_read;
EstPerifericos_t perifericos;

bool estSW1, estSW2, estLedRojo;

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
 * @brief Procesa la interrupcion del MCP2515.
 */
static void canmsg_interrupt(void);
/**
 * @brief Salida por serie de datos del nodo 3.
 */
static void canmsg_procesar(void);
/**
 * @brief Función de callback cuando se termina el tiempo.
 */
static void callbackTimeout(void);
//---------------------------------------------------------------------------------------

// CUERPO DE FUNCIONES.
//---------------------------------------------------------------------------------------
int main(void) {
	init();

	PRINTF("\nNombre: Nodo 3\n\r");
	PRINTF("Descripcion: Este nodo se encarga de recibir"
			"datos desde el modulo can y luego msotrar"
			"en un puerto serie dicho resultados.\n\r");

	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	while (1) {
		if (Delay1s == 0)
			canmsg_procesar();
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

#define MASK0_ID10		0x00F	// Para el ID=20 es lo mismo
#define FILTER0_ID10	0x00A
#define FILTER1_ID20	20

	mcp2515_setFilterMask(MASK0, false, MASK0_ID10);
	mcp2515_setFilterMask(MASK1, false, MASK0_ID10);

	mcp2515_setFilter(RXF0, false, FILTER0_ID10);
	mcp2515_setFilter(RXF2, false, FILTER1_ID20);

	status = mcp2515_setNormalMode();
	assert(status == ERROR_OK);

	mcp2515_clearInterrupts();

	BOARD_InitBootPeripherals();

	return;
}
//---------------------------------------------------------------------------------------
static void canmsg_readFromBus(void) {
#define CAN_NODO1_ID	10
#define CAN_NODO2_ID	20

	// Recibe el mensaje
	ERROR_t status = mcp2515_readMessage(&canMsgRead);
	if (status != ERROR_OK)
		PRINTF("Error al recibir el mensaje.\n\r");

	if (canMsgRead.can_id == CAN_NODO1_ID) {
		adc_read = canMsgRead.data[1];
		adc_read = (adc_read << 8) | canMsgRead.data[0];
	}

	else if (canMsgRead.can_id == CAN_NODO2_ID) {
		perifericos.data = canMsgRead.data[0];

		estLedRojo = perifericos.LED_ROJO;
		estSW1 = perifericos.PULSADOR1;
		estSW2 = perifericos.PULSADOR2;
	}

	/* Reiniciamos el contador de tiempo */
	DelayTimeOut = TIMEOUT;

	return;
}
//---------------------------------------------------------------------------------------
static void canmsg_procesar(void) {
//	PRINTF("\n\r> Salida por consola\n\r");
	PRINTF("\n----------------------------------\n\r");
	PRINTF("Led rojo: %d\r\n", estLedRojo);
	PRINTF("Boton 1: %d\r\n", estSW1);
	PRINTF("Boton 2: %d\r\n", estSW2);
	PRINTF("Sensor de luz: %d\r\n", adc_read);
	PRINTF("----------------------------------\n\r");

	return;
}
//---------------------------------------------------------------------------------------
static void callbackTimeout(void) {
	PRINTF("\n\nTiempo agotado!!!\n\r");

	LED_RED_ON();

	NVIC_DisableIRQ(GPIOA_IRQN);

	assert(false);

	return;
}
//---------------------------------------------------------------------------------------
void SysTick_Handler(void) {
	if (Delay1s != 0)
		Delay1s--;
	else
		Delay1s = TIEMPO_DE_SALIDA;

	if (DelayTimeOut != 0)
		DelayTimeOut--;
	else
		callbackTimeout();

	return;
}
//---------------------------------------------------------------------------------------
static void canmsg_interrupt(void) {
	// Leemos las interrupciones generadas
	ERROR_t error = mcp2515_getInterrupts();

	if (error != ERROR_OK) {
		PRINTF("Fallo al leer la interrupcion\n\r");
		return;
	}

	// Detectamos las interrupciones relevantes
	if (mcp2515_getIntERRIF()) {
		PRINTF("Error interrupt flag\n\r");
		mcp2515_clearERRIF();
		return;
	}

	if (mcp2515_getIntMERRF()) {
		PRINTF("Message error interrupt flag\n\r");
		mcp2515_clearMERR();
		return;
	}

	if (mcp2515_getIntRX0IF() || mcp2515_getIntRX1IF()) {
		canmsg_readFromBus();
	}

	return;
}
//---------------------------------------------------------------------------------------
/* PORTA_IRQn interrupt handler */
void GPIOA_IRQHANDLER(void) {
	/* Get pin flags */
	uint32_t pin_flags = GPIO_PortGetInterruptFlags(GPIOA);

	/* Place your interrupt code here */
	canmsg_interrupt();

	/* Clear pin flags */
	GPIO_PortClearInterruptFlags(GPIOA, pin_flags);

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
	 Store immediate overlapping exception return operation might vector to incorrect interrupt. */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
  #endif
}
