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
#include "spi.h"
#include "mcp2515.h"
/* TODO: insert other definitions and declarations here. */
#define PIN_NUMBER 17

#define CAN_NODO_2_ID	20

#define TIEMPO_DE_MUESTRA_ESTADOS	1000

#define BOARD_LED_RED_GPIO_PIN_MASK (1U << 29U)

#define LUZ_MIN	500
#define LUZ_MAX	1500
#define LED_ON	GPIO_ClearPinsOutput(BOARD_LED_RED_GPIO,BOARD_LED_RED_GPIO_PIN_MASK)
#define LED_OFF	GPIO_SetPinsOutput(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN_MASK)

uint16_t Delay1s = TIEMPO_DE_MUESTRA_ESTADOS;

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
static void canmsg_escritura(void);
/**
 * @brief Lectura del modulo can.
 */
static void canmsg_lectura(void);
static void canmsg_procesar(void);
/**
 * @brief Inicializacion de perifericos.
 */
static void perifericos_init(void);
/**
 * @brief Configuraciones de interrupcion.
 */
static void interrupt_init(void);

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

	PRINTF("\nNombre: Nodo 2\n\r");
	PRINTF("Descripcion: Este nodo se encarga de producir un "
			"valor de los estados de sus perifericos y ademas"
			"obtener datos del ldr proveniente de otro modulo"
			"can. Ademas cuenta con un Id=20.\n\r");
	PRINTF("Materia: Sistemas digitales 2\n\r");

	SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U);

	/* Configura de can */
	perifericos_init();
	interrupt_init();

	canMsg1.can_id = CAN_NODO_2_ID;
	canMsg1.can_dlc = 1;

	LED_ON;

	while (1)
	{
		if (Rx_flag_mcp2515)
		{
			canmsg_lectura();
			canmsg_procesar();
			Rx_flag_mcp2515 = false;
		}

		if (Delay1s == 0)
			canmsg_escritura();
	}
	return 0;
}

static void canmsg_escritura(void)
{
	ERROR_t estado;

	EstPerifericos_t perifericos = {.data = 0};

	perifericos.LED_ROJO = ~GPIO_ReadPinInput(BOARD_LED_RED_GPIO,
	BOARD_LED_RED_PIN);
	perifericos.PULSADOR1 = ~GPIO_ReadPinInput(BOARD_SW1_GPIO, BOARD_SW1_PIN);
	perifericos.PULSADOR2 = ~GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_PIN);

	canMsg1.data[0] = perifericos.data;
	canMsg1.can_dlc = 1;

	estado = mcp2515_sendMessage(&canMsg1);

	if (estado == ERROR_OK)
	{
		PRINTF("\nMensaje enviado\n\r");
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
		if (estado == ERROR_ALLTXBUSY)
			PRINTF("\n\rError: buffers de transmision llenos.\n\r");
		else if (estado == ERROR_FAILTX)
			PRINTF("\n\rError: fallo al transmitir.\n\r");
		else if (estado == ERROR_SPI_WRITE)
			PRINTF("\n\rError: fallo en escritura de spi.\n\r");
		else
			PRINTF("\nError al enviar\n\r");
	}

	return;
}

static void canmsg_lectura(void)
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

	return;
}

static void canmsg_procesar(void)
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

	error = mcp2515_setNormalMode();
	if (error != ERROR_OK)
		PRINTF("Fallo al setear el modo normal\n\r");
//	error = mcp2515_setLoopbackMode();
//	if (error != ERROR_OK)
//		PRINTF("Fallo al setear el loopback mode\n\r");

	/* Configuracion de los led y botones */
	BOARD_InitLEDs();
	BOARD_InitButtons();

	return;
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

void SysTick_Handler(void)
{
	if (Delay1s != 0)
		Delay1s--;
	else
		Delay1s = TIEMPO_DE_MUESTRA_ESTADOS;

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
			vTaskNotifyGiveFromISR(TaskRxCan_Handle, &xHigherPriorityTaskWoken);
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
