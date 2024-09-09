/*
 * CanApi.c
 *
 *  Created on: 1 sep. 2024
 *      Author: agustin
 */

/**
 * @file CanAPI.c
 * @author Zuliani, Agustin
 * @date 03/09/24
 * @brief Esta libreria se encarga de manejar a la libreria del modulo mcp2515. De esta forma
 * los nodos interactuan directamente con esta capa y no con el mcp2515. Esto permite que se haga
 * mejor uso del recuros y evite colisiones en los mensajes de los nodos.
 * Para esto cada nodo debe generar una subcripcion a una ide determinada y enviar su task handle.
 */

#include "CanApi.h"

#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_debug_console.h"

#include <stdio.h>

#include "can.h"
#include "mcp2515.h"

#define MAX_RETRY_COUNT 3  // Número máximo de reintentos
#define INTERRUPT_RETRY_DELAY_MS 1  // Tiempo de espera entre reintentos en ms

/* Symbols to be used with GPIO driver */
#define BOARD_INT_CAN_FGPIO FGPIOA              /*!<@brief FGPIO peripheral base pointer */
#define BOARD_INT_CAN_GPIO GPIOA                /*!<@brief GPIO peripheral base pointer */
#define BOARD_INT_CAN_GPIO_PIN_MASK (1U << 17U) /*!<@brief GPIO pin mask */

/* Symbols to be used with PORT driver */
#define BOARD_INT_CAN_PORT PORTA                /*!<@brief PORT peripheral base pointer */
#define BOARD_INT_CAN_PIN 17U                   /*!<@brief PORT pin number */
#define BOARD_INT_CAN_PIN_MASK (1U << 17U)      /*!<@brief PORT pin mask */

#define delay_ms(x)	delay_ms(x)

#define CAN_PERIFERICOS_INIT	perifericos_init
#define CAN_INTERRUPT_INIT		interrupt_init

#define CAN_INTERRUPT			canmsg_interrupt
#define CAN_PROCESS_RECEIVE 	canmsg_receive

#if	TIMEOUT_ENABLE
#define CAN_callbackTIMEOUT		callbackTimeout

extern void callbackTimeout(void);
#endif

// Recepcion
#define QUEUE_RECEIVE_LENGTH	5
#define QUEUE_RECEIVE_SIZE		sizeof(canMsg_Receive)
// Transmision
#define QUEUE_TRANSMISION_LENGTH	10
#define QUEUE_TRANSMISION_SIZE		sizeof(canMsg_Transmision)

/**
 * @brief Tipo de subcripcion a los nodos.
 */
typedef struct CANSubscription
{
	/**
	 * @brief ID del nodo que envía el mensaje.
	 */
	canid_t nodeId;
	/**
	 * @brief ID del nodo que está suscrito a los mensajes.
	 */
	canid_t subscriberId;
	/**
	 * @brief Cola de datos de recepcion.
	 */
	struct can_frame bufferRx[QUEUE_RECEIVE_LENGTH];
	/**
	 * @brief Indice de lectura del buffer.
	 */
	uint8_t readIndex;
	/**
	 * @brief Funcion de callback.
	 */
	can_callback_t callback;
	/**
	 * @brief Puntero al siguiente nodo de la lista.
	 */
	struct CANSubscription *next;
} CANSubscription_t;

/**
 * @brief Lista de subscripciones de can.
 */
static CANSubscription_t *subscriptionList = NULL; // Inicio de la lista de suscripciones

/**
 * @brief Buffer de transmision.
 */
struct can_frame bufferTx[QUEUE_TRANSMISION_LENGTH];
/**
 * @brief Indice del buffer de escritura.
 */
static uint8_t writeIndex = 0;

/**
 * @brief Mensaje de recepcion de tipo can.
 */
static struct can_frame canMsg_Receive;

/**
 * @brief Contador de eventos de recepcion.
 */
static uint8_t EventRx = 0;
/**
 * @brief Contador de eventos de transmision.
 */
static uint8_t EventTx = 0;

/**
 * @brief Tiempo de transmision de mensajes can.
 */
#define TIMER_PERIOD_MS	100
#define TIMER_TIMEOUT	5000

static uint16_t timerXtransfer = TIMER_PERIOD_MS;
static uint16_t timerTimeout = TIMER_TIMEOUT;

/**
 * @brief Funcion de procesamiento de interrupcion.
 */
static void canmsg_interrupt(void);
static Error_Can_t canmsg_receive(void);
/**
 * @brief Notificación de tareas.
 * @param[in] nodeId Id del mensaje recivido.
 */
static void NotifySubscribedNodes(void);
/**
 * @brief Inicializacion de perifericos.
 */
static void perifericos_init(void);
/**
 * @brief Inicializacion de interrupcion por gpio.
 */
static void interrupt_init(void);
/**
 * @brief Tiempo de bloqueo.
 */
static void delay_ms(uint16_t ms);

/*
 * ===========================================
 * =			PUBLIC FUNCTIONS			 =
 * ===========================================
 */

extern void CAN_init(void)
{
	/* Configura de can */
	CAN_PERIFERICOS_INIT();	// Inicializacion de los perifericos
	delay_ms(5);

	CAN_INTERRUPT_INIT();	// Inicializacion de las interrupciones
	delay_ms(5);

	return;
}

extern Error_Can_t CAN_Subscribe(canid_t nodeId, canid_t subscriberId, can_callback_t callback)
{
	CANSubscription_t *current = subscriptionList;

	// Verificar si ya existe una suscripción para este nodeId y callback
	while (current != NULL)
	{
		if (current->nodeId == nodeId && current->callback == callback
					&& current->subscriberId == subscriberId)
		{
			// Si ya existe una suscripción con el mismo nodeId y callback, retornamos un error
			return ERROR_CAN_ALREADY_SUBSCRIBED;
		}
		current = current->next;
	}

	// Crear una nueva suscripción si no existe
	CANSubscription_t *newSubscription = (CANSubscription_t*) malloc(sizeof(CANSubscription_t));
	if (newSubscription == NULL)
	{
		return ERROR_CAN_MEMORY;
	}

	// Carga la informacion en la nueva lista
	newSubscription->readIndex = 0;
	newSubscription->nodeId = nodeId;
	newSubscription->callback = callback;
	newSubscription->subscriberId = subscriberId;

	// Insertar al inicio de la lista enlazada
	newSubscription->next = subscriptionList;
	subscriptionList = newSubscription;

	return ERROR_CAN_OK;
}

extern Error_Can_t CAN_Unsubscribe(canid_t nodeId, canid_t subscriberId)
{
	CANSubscription_t **current = &subscriptionList;

	while (*current != NULL)
	{
		if ((*current)->nodeId == nodeId && (*current)->subscriberId == subscriberId)
		{
			CANSubscription_t *toDelete = *current;
			*current = (*current)->next;

			// Liberar la memoria de la suscripción
			free(toDelete);

			return ERROR_CAN_OK;
		}

		current = &(*current)->next;
	}

	return ERROR_CAN_NOT_FOUND;
}

extern Error_Can_t CAN_sendMsg(struct can_frame *dato)
{
	// Verificar si el buffer de transmisión está lleno
	if (writeIndex >= QUEUE_TRANSMISION_LENGTH)
	{
		return ERROR_CAN_QUEUETX_FULL; // Error si está lleno
	}

	// Copiar el mensaje al buffer de transmisión
	memcpy(&bufferTx[writeIndex], dato, sizeof(struct can_frame));
	writeIndex++; // Avanzar el índice de escritura
	EventTx++;		// Genera un evento de transmision

	return ERROR_CAN_OK;
}

extern Error_Can_t CAN_readMsg(struct can_frame *dato, canid_t nodeId, canid_t subscriberId)
{
	CANSubscription_t *current = subscriptionList;

	// Buscar la suscripción correspondiente al nodo y suscriptor
	while (current != NULL)
	{
		if (current->nodeId == nodeId && current->subscriberId == subscriberId)
		{
			// Verificar si hay mensajes en el buffer de recepción
			if (current->readIndex == 0) return ERROR_CAN_QUEUERX_EMPTY;

			// Leer el mensaje desde el buffer de recepción
			memcpy(dato, &current->bufferRx[current->readIndex - 1], sizeof(struct can_frame));

			// Actualizar el índice de lectura
			current->readIndex--;

			return ERROR_CAN_OK;
		}
		current = current->next;
	}

	return ERROR_CAN_NOT_FOUND;
}

extern Error_Can_t CAN_eventTx(void)
{
	if (EventTx == 0) return ERROR_CAN_NO_EVENT_TX;

	ERROR_t status = mcp2515_sendMessage(&bufferTx[writeIndex - 1]);
	if (status != ERROR_OK) return ERROR_CAN_FAILTX;	// Fallo al transmitir

	writeIndex--;
	EventTx--;

	return ERROR_CAN_OK;
}

extern Error_Can_t CAN_eventRx(void)
{
	if (EventRx == 0) return ERROR_CAN_NO_EVENT_RX;

	// Notificar a los nodos suscritos
	NotifySubscribedNodes();

	EventRx--;

	return ERROR_CAN_OK;
}

extern bool CAN_getTimer(void)
{
	if (timerXtransfer != 0) timerXtransfer--;
	else
	{
		timerXtransfer = TIMER_PERIOD_MS;
		return true;
	}

#if TIMEOUT_ENABLE
	if (EventRx)
	{
		timerTimeout = TIMER_TIMEOUT;
	}
	else
	{
		if (timerTimeout != 0)
			timerTimeout--;

		else
		{
			CAN_callbackTIMEOUT();	// Ejecuta el callback si se termina el timeout
			return false;
		}
	}
#endif

	return false;
}

/*
 * ===========================================
 * =			PRIVATE FUNCTIONS			 =
 * ===========================================
 */

static Error_Can_t canmsg_receive(void)
{
	CANSubscription_t *current = subscriptionList;

	ERROR_t estado = mcp2515_readMessage(&canMsg_Receive);
	if (estado != ERROR_OK)
	{
		if (estado == ERROR_SPI_READ)
		PRINTF("\n\rFallo al leer el spi.\n\r");
		else if (estado == ERROR_NOMSG)
		PRINTF("\n\rFallo no hubo mensajes.\n\r");
		else
		PRINTF("\n\rFallo al leer el modulo can.\n\r");

		return ERROR_CAN_OK;
	}

	// Buscar la suscripción correspondiente al nodo y suscriptor
	while (current != NULL)
	{
		if (current->nodeId == canMsg_Receive.can_id)
		{
			// Verifica si hay espacio
			if (current->readIndex < QUEUE_RECEIVE_LENGTH)
			{
				// Leer el mensaje desde el buffer de recepción
				memcpy(&current->bufferRx[current->readIndex], &canMsg_Receive,
							sizeof(struct can_frame));

				// Actualizar el índice de lectura
				current->readIndex++;
			}
		}
		current = current->next;
	}

	EventRx++;

	return ERROR_CAN_OK;
}

static void canmsg_interrupt(void)
{
	// Leemos las interrupciones generadas
	ERROR_t error = mcp2515_getInterrupts();

	if (error != ERROR_OK)
	{
		PRINTF("Fallo al leer la interrupcion\n\r");
		return;
	}

	// Detectamos las interrupciones relevantes
	if (mcp2515_getIntERRIF())
	{
		PRINTF("Error interrupt flag\n\r");
		mcp2515_clearERRIF();
		return;
	}
	else if (mcp2515_getIntMERRF())
	{
		PRINTF("Message error interrupt flag\n\r");
		mcp2515_clearMERR();
		return;
	}

	// Interrupciones por recepción de datos (RX0 y RX1)
	int retryCount = 0;

	while (retryCount < MAX_RETRY_COUNT)
	{
		if (mcp2515_getIntRX0IF() || mcp2515_getIntRX1IF())
		{
			Error_Can_t error = CAN_PROCESS_RECEIVE();
			if (error != ERROR_CAN_OK)
			{
				if (error == ERROR_CAN_NOT_FOUND)
				PRINTF("\n\rError: no se encontro el id"
							"con los existentes.\n\r");
			}

			return;
		}

		retryCount++;
		delay_ms(INTERRUPT_RETRY_DELAY_MS);

		// Volvemos a leer las interrupciones para intentar nuevamente
		error = mcp2515_getInterrupts();
		if (error != ERROR_OK)
		{
			PRINTF("Fallo al leer la interrupcion en el intento %d\n\r", retryCount);
		}
	}

	if (retryCount == MAX_RETRY_COUNT)
	{
		// Si alcanzamos el límite de reintentos, reportamos el fallo
		PRINTF("Error: RX0 y RX1 no procesadas después de %d intentos\n\r", MAX_RETRY_COUNT);
	}

	return;
}

static void NotifySubscribedNodes(void)
{
	CANSubscription_t *current = subscriptionList;

	while (current != NULL)
	{
		if (current->readIndex > 0)
		{
			// Ejecutar el callback para el nodo suscrito
			current->callback(current->subscriberId, current->nodeId);
		}
		current = current->next;
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
//	PRINTF("Fallo al setear el loopback mode\n\r");

	mcp2515_clearInterrupts();

	return;
}

static void interrupt_init(void)
{
	/* Port A Clock Gate Control: Clock enabled */
	CLOCK_EnableClock(kCLOCK_PortA);

	gpio_pin_config_t INT_CAN_config =
	{ .pinDirection = kGPIO_DigitalInput, .outputLogic = 0U };
	/* Initialize GPIO functionality on pin PTA17 (pin 47)  */
	GPIO_PinInit(BOARD_INT_CAN_GPIO, BOARD_INT_CAN_PIN, &INT_CAN_config);

	/* PORTA17 (pin 47) is configured as PTA17 */
	PORT_SetPinMux(BOARD_INT_CAN_PORT, BOARD_INT_CAN_PIN, kPORT_MuxAsGpio);

	/* Interrupt configuration on PORTA17 (pin 47): Interrupt on falling edge */
	PORT_SetPinInterruptConfig(BOARD_INT_CAN_PORT, BOARD_INT_CAN_PIN, kPORT_InterruptFallingEdge);

	PORTA->PCR[17] = ((PORTA->PCR[17] &
	/* Mask bits to zero which are setting */
	(~(PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_ISF_MASK)))

	/* Pull Select: Internal pullup resistor is enabled on the corresponding pin, if the
	 * corresponding PE field is set. */
	| (uint32_t) (kPORT_PullUp));

	NVIC_EnableIRQ(PORTA_IRQn); // Habilita la interrupción para el puerto A
	NVIC_SetPriority(PORTA_IRQn, 0);

	return;
}

void PORTA_IRQHandler(void)
{
#if USE_FREERTOS
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif

	// Obtiene el estado de las banderas de interrupción del puerto A
	uint32_t interruptFlags = GPIO_GetPinsInterruptFlags(GPIOA);

	if (interruptFlags & BOARD_INT_CAN_PIN_MASK)
	{
		CAN_INTERRUPT();
#if USE_FREERTOS
		xTaskNotifyFromISR(task_Receive_Handle, 0, eIncrement,
				&xHigherPriorityTaskWoken);
#else

#endif
		GPIO_ClearPinsInterruptFlags(GPIOA, BOARD_INT_CAN_PIN_MASK);
	}

#if USE_FREERTOS
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif

	return;
}

static void delay_ms(uint16_t ms)
{
	// Calcula el número de ciclos necesarios
	uint32_t cycles = (CLOCK_GetCoreSysClkFreq() / 1000) * ms / 4;

	// Realiza el bucle para generar el retardo
	for (uint32_t i = 0; i < cycles; i++)
	{
		__NOP(); // No Operation (1 ciclo de instrucción)
	}

	return;
}
