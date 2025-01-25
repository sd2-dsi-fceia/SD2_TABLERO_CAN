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
#include "mcp2515.h"
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
 * @brief Mensaje 1 de can.
 */
struct can_frame canMsg1;
/**
 * @brief Mensaje de lectura.
 */
struct can_frame canMsgRead = { .can_dlc = 0, .can_id = 0, };

// DECLARACION DE FUNCIONES.
//..........................................................................................
/**
 * @brief Inicializacion de todos los perifericos.
 */
static void init(void);
/**
 * @brief Escritura del modulo can.
 */
static void canmsg_writeToBus(void);
/**
 * @brief Lectura de los mensajes can.
 */
static void canmsg_readFromBus(void);
/**
 * @brief Procesa la interrupcion del MCP2515.
 */
static void canmsg_interrupt(void);
/**
 * @brief Lectura del modulo can.
 */
static void serialPort(void);
/**
 * @brief Procesa la informacion leida.
 */
static void canmsg_sensorLuz(void);
//..........................................................................................

// CUERPO DE FUNCIONES.
//..........................................................................................
int main(void) {
	/* Inicializacion */
	init();

	/* Inicializacion en el puerto serie. */
	PRINTF("\nNombre: Nodo 2\n\r");
	PRINTF("Descripcion: Este nodo se encarga de producir un "
			"valor de los estados de sus perifericos y ademas"
			"obtener datos del ldr proveniente de otro modulo"
			"can. Ademas cuenta con un Id=20.\n\r");

	/* Velocidad del SysTick en 1ms */
	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	LED_ON;

	while (1) {
//		if (Rx_msgFlag) {
//			canmsg_readFromBus();
//		}

		if (Delay1s == 0)
			canmsg_writeToBus();
	}

	return 0;
}
//..........................................................................................
static void init(void) {
	/* Init board hardware. */
	BOARD_InitBootPins();
	CLOCK_EnableClock(kCLOCK_PortB);
	CLOCK_EnableClock(kCLOCK_PortC);
	CLOCK_EnableClock(kCLOCK_PortD);
	BOARD_InitBootClocks();
//	BOARD_InitBootPeripherals();
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

	/* Configurar las mascaras y filtros para una ID=10 */
	/*
	 * Nota: Dentro de las siguientes dos funciones de configuracion
	 * de filtro y mascara se configura en el mcp el modo de configuracion
	 * por ende es necesario volver a ponerlo en modo de operacion normal
	 * para que pueda enviar y recibir datos. En conclusión conviene
	 * primero configurar el filtro y mascara y luego ponerlo en modo
	 * de operacion normal.
	 * */
#define MASK0_ID10		0x00F
#define FILTER0_ID10	0x00A

	mcp2515_setFilterMask(MASK0, false, MASK0_ID10);
	mcp2515_setFilterMask(MASK1, false, MASK0_ID10);

	mcp2515_setFilter(RXF0, false, FILTER0_ID10);
	mcp2515_setFilter(RXF1, false, FILTER0_ID10);

	status = mcp2515_setNormalMode();
	assert(status == ERROR_OK);

	mcp2515_clearInterrupts();

	canMsg1.can_id = CAN_NODO_2_ID;
	canMsg1.can_dlc = 1;

	/*
	 * Lo ponemos al final debido a que habillita la interrupcion
	 * del pin gpio.
	 */
	BOARD_InitBootPeripherals();

	return;
}
//..........................................................................................
static void canmsg_writeToBus(void) {
	ERROR_t estado;
	EstPerifericos_t perifericos = { .data = 0 };

	// Lectura de los estados de los perifericos
	perifericos.LED_ROJO = ~GPIO_ReadPinInput(BOARD_LED_RED_GPIO,
	BOARD_LED_RED_PIN);
	perifericos.PULSADOR1 = ~GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_PIN);
	perifericos.PULSADOR2 = ~GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_PIN);

	// Armamos el mensaje
	canMsg1.data[0] = perifericos.data;
	canMsg1.can_dlc = 1;

	// Envio de mensaje al bus can
	estado = mcp2515_sendMessage(&canMsg1);

	// Verificacion
	if (estado == ERROR_OK) {
		PRINTF("\n\rMensaje enviado\n\r");
		PRINTF("ID\tDLC\tDATA\n\r");
		PRINTF("%d\t%d\t", canMsg1.can_id, canMsg1.can_dlc);

		for (uint8_t i = 0; i < 8; i++) {
			PRINTF("%d ", canMsg1.data[i]);
		}
		PRINTF("\n\r");
	} else {
		PRINTF("Error al enviar el mensaje.\n\r");
	}

	return;
}
//..........................................................................................
static void canmsg_readFromBus(void) {
	// Recibe el mensaje
	ERROR_t status = mcp2515_readMessage(&canMsgRead);
	if (status != ERROR_OK)
		PRINTF("Error al recibir el mensaje.\n\r");

	canmsg_sensorLuz();
	serialPort();

	Rx_msgFlag = false;

	return;
}
//..........................................................................................
static void serialPort(void) {
	PRINTF("\n\rMensaje de recepcion\n\r");
	PRINTF("ID\tDLC\tDATA\n\r");
	PRINTF("%d\t%d\t", canMsgRead.can_id, canMsgRead.can_dlc);

	for (uint8_t i = 0; i < 8; i++) {
		PRINTF("%d ", canMsgRead.data[i]);
	}

	PRINTF("\n\r");

	return;
}
//..........................................................................................
static void canmsg_sensorLuz(void) {
	uint16_t adc_read;

	if (canMsgRead.can_id == 10) {
		adc_read = canMsgRead.data[1];
		adc_read = (adc_read << 8) | canMsgRead.data[0];

		if (adc_read > LUZ_MAX)
			LED_ON;
		if (adc_read < LUZ_MIN)
			LED_OFF;
	}

	return;
}
//..........................................................................................
void SysTick_Handler(void) {
	if (Delay1s != 0)
		Delay1s--;
	else
		Delay1s = TIEMPO_DE_MUESTRA_ESTADOS;

	return;
}
//..........................................................................................
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
		Rx_msgFlag = true;	// Leemos en el while
		// La bandera de interrupcion debe limpiarse luego de leer
	}

	return;
}
//..........................................................................................
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

//..........................................................................................
//void PORTA_IRQHandler(void) {
//	// Obtiene el estado de las banderas de interrupción del puerto A
//	uint32_t interruptFlags = GPIO_GetPinsInterruptFlags(GPIOA);
//
//	if (interruptFlags & BOARD_INT_CAN_PIN_MASK) {
//		canmsg_interrupt();
//		GPIO_ClearPinsInterruptFlags(GPIOA, BOARD_INT_CAN_PIN_MASK);
//	}
//
//	return;
//}
