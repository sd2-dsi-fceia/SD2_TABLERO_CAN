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

#include "fsl_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "event_groups.h"

#include <stdio.h>

#include "can.h"
#include "mcp2515.h"

#include "Nodo1.h"
#include "Nodo2.h"
#include "Nodo3.h"

#define PIN_NUMBER 17

#define __delay_ms(x)	vTaskDelay(pdMS_TO_TICKS(x))

#define CAN_PERIFERICOS_INIT	perifericos_init
#define CAN_INTERRUPT_INIT		interrupt_init

static void CAN_PERIFERICOS_INIT(void);
extern void CAN_INTERRUPT_INIT(void);

/**
 * @brief Tipo de subcripcion a los nodos.
 */
typedef struct CANSubscription
{
	/**
	 * @brief Id del nodo que envia el mensaje.
	 */
	uint16_t nodeId;  // ID del nodo que envía el mensaje

	/**
	 * @brief Tarea que maneja la recepción del mensaje.
	 */
	TaskHandle_t taskHandle;  // Tarea que maneja la recepción para ese nodo

	/**
	 * @brief Cola específica para el nodo suscrito.
	 */
	QueueHandle_t queueHandle;  // Cola específica del nodo

	/**
	 * @brief Puntero al siguiente nodo de la lista enlazada.
	 */
	struct CANSubscription *next;  // Apuntador al siguiente nodo en la lista
} CANSubscription_t;

/**
 * @brief Lista de subscripciones de can.
 */
static CANSubscription_t *subscriptionList = NULL; // Inicio de la lista de suscripciones

static struct can_frame canMsg_Receive;
static struct can_frame canMsg_Transmision;

#define QUEUE_RECEIVE_LENGTH	5
#define QUEUE_RECEIVE_SIZE		sizeof(canMsg_Receive)
#define QUEUE_TRANSMISION_LENGTH	10
#define QUEUE_TRANSMISION_SIZE	sizeof(canMsg_Transmision)

/**
 * @brief Cola de recepcion de datos.
 */
//QueueHandle_t queue_Receive;
/**
 * @brief Cola de transmision de datos.
 */
QueueHandle_t queue_Transmision;

/**
 * @brief Temporizador de transmision.
 */
TimerHandle_t timer_Transimision;
/**
 * @brief Handle de la tarea de recepcion.
 */
TaskHandle_t task_Receive_Handle;

#define TIMER_TRANSMISION_PERIOD_MS	pdMS_TO_TICKS(200)

/**
 * @brief Evento de inicialización de perifericos e interrupcion.
 */
EventGroupHandle_t xInitEventGroup;

/**
 * @brief Callback de el temporizador por software.
 */
static void timerRtos_Transmision(TimerHandle_t xTimer);
/**
 * @brief Tarea de recepcion de datos por el bus can.
 */
static void taskRtos_Receive(void *pvParameters);
/**
 * @brief Funcion de procesamiento de interrupcion.
 */
static void canmsg_interrupt(void);
/**
 * @brief Notificación de tareas.
 * @param[in] nodeId Id del mensaje recivido.
 */
static void NotifySubscribedNodes(uint16_t nodeId);
/**
 * @brief Inicializacion de perifericos.
 */
static void perifericos_init(void);

/*
 * ===========================================
 * =			PUBLIC FUNCTIONS			 =
 * ===========================================
 */

extern void CAN_init(void)
{
	/* Inicializacion de queue de transmision. */
	queue_Transmision = xQueueCreate(QUEUE_TRANSMISION_LENGTH,
			QUEUE_TRANSMISION_SIZE);
	if (queue_Transmision == NULL)
		PRINTF("\n\rFallo al crear la cola de datos.\n\r");

	/* Inicializacion de tarea de recepcion. */
	BaseType_t status = xTaskCreate(taskRtos_Receive, "Task Read can",
	configMINIMAL_STACK_SIZE + 50, NULL, configMAX_PRIORITIES,
			&task_Receive_Handle);
	if (status != pdTRUE)
		PRINTF("Fallo al crear la tarea.\n\r");

	/* Inicializacion de timer. */
	timer_Transimision = xTimerCreate("Timer transmision",
	TIMER_TRANSMISION_PERIOD_MS, pdTRUE, NULL, timerRtos_Transmision);
	if (timer_Transimision == NULL)
		PRINTF("\n\rFallo al crear el timer.\n\r");

	/* Creamos el evento de sincronizacion. */
	xInitEventGroup = xEventGroupCreate();

	return;
}

extern Error_Can_t CAN_Subscribe(uint16_t nodeId, TaskHandle_t taskHandle)
{
	CANSubscription_t *newSubscription = pvPortMalloc(
			sizeof(CANSubscription_t));
	if (newSubscription == NULL)
	{
		return ERROR_CAN_MEMORY;
	}

	newSubscription->nodeId = nodeId;
	newSubscription->taskHandle = taskHandle;

	// Crear una cola específica para este nodo
	newSubscription->queueHandle = xQueueCreate(QUEUE_RECEIVE_LENGTH,
			QUEUE_RECEIVE_SIZE);
	if (newSubscription->queueHandle == NULL)
	{
		vPortFree(newSubscription);
		return ERROR_CAN_MEMORY;
	}

	newSubscription->next = subscriptionList;
	subscriptionList = newSubscription;

	return ERROR_CAN_OK;
}

extern Error_Can_t CAN_Unsubscribe(uint16_t nodeId, TaskHandle_t taskHandle)
{
	CANSubscription_t **current = &subscriptionList;

	while (*current != NULL)
	{
		if ((*current)->nodeId == nodeId
				&& (*current)->taskHandle == taskHandle)
		{
			CANSubscription_t *toDelete = *current;
			*current = (*current)->next;

			// Eliminar la cola específica del nodo
			vQueueDelete(toDelete->queueHandle);

			vPortFree(toDelete);

			return ERROR_CAN_OK;
		}

		current = &(*current)->next;
	}

	return ERROR_CAN_NOT_FOUND;
}

extern Error_Can_t CAN_sendMsg(struct can_frame *dato, TickType_t xTicksToWait)
{
	configASSERT(queue_Transmision != NULL);

	BaseType_t status = xQueueSendToBack(queue_Transmision, dato,
			xTicksToWait);
	if (status != pdPASS)
	{
		PRINTF("\n\rFallo al cargar datos en la cola.\n\r");
		return ERROR_CAN_QUEUETX;
	}

	return ERROR_CAN_OK;
}

extern Error_Can_t CAN_readMsg(struct can_frame *dato, uint16_t nodeId,
		TaskHandle_t taskHandle)
{
	CANSubscription_t *current = subscriptionList;

	// Buscar la cola correspondiente al nodo
	while (current != NULL)
	{
		if (current->nodeId == nodeId && current->taskHandle == taskHandle)
		{
			// Leer el mensaje de la cola específica del nodo
			BaseType_t status = xQueueReceive(current->queueHandle, dato,
					pdMS_TO_TICKS(200));
			if (status != pdPASS)
			{
//				PRINTF("\n\rFallo al recibir datos de la cola.\n\r");
				return ERROR_CAN_QUEUERX;
			}
			return ERROR_CAN_OK;
		}
		current = current->next;
	}

	return ERROR_CAN_NOT_FOUND;
}

extern void CAN_getEvent(void)
{
	// Esperar a que se complete la inicialización (bloqueante)
	xEventGroupWaitBits(xInitEventGroup, INIT_COMPLETE_EVENT, pdFALSE, pdTRUE,
	portMAX_DELAY);

	return;
}
/*
 * ===========================================
 * =			PRIVATE FUNCTIONS			 =
 * ===========================================
 */

static void taskRtos_Receive(void *pvParameters)
{
	uint32_t event_notify;

	/* Configura de can */
	CAN_PERIFERICOS_INIT();	// Inicializacion de los perifericos
	__delay_ms(5);

	CAN_INTERRUPT_INIT();	// Inicializacion de las interrupciones
	__delay_ms(5);

	BaseType_t status = xTimerStart(timer_Transimision, portMAX_DELAY);
	if (status != pdPASS)
		PRINTF("Fallo al inciar el timer.\n\r");

	// Establecer el evento de inicialización completa
	xEventGroupSetBits(xInitEventGroup, INIT_COMPLETE_EVENT);

	for (;;)
	{
		xTaskNotifyWait(0, 0, &event_notify, portMAX_DELAY);

		if (event_notify > 0)
		{
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

	estado = mcp2515_readMessage(&canMsg_Receive);
	if (estado != ERROR_OK)
	{
		if (estado == ERROR_SPI_READ)
			PRINTF("\n\rFallo al leer el spi.\n\r");
		else if (estado == ERROR_NOMSG)
			PRINTF("\n\rFallo no hubo mensajes.\n\r");
		else
			PRINTF("\n\rFallo al leer el modulo can.\n\r");

		return;
	}

	// Notificar a los nodos suscritos
	NotifySubscribedNodes(canMsg_Receive.can_id);

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
	__delay_ms(10);

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
	else if (mcp2515_getIntMERRF())
	{
		// Acciones ...
		PRINTF("Message error interrupt flag\n\r");

		// Limpiamos la bandera
		mcp2515_clearMERR();
	}
	else if (mcp2515_getIntRX0IF() || mcp2515_getIntRX1IF())
	{
		// Acciones ...
		canmsg_receive();	// Procesa la informacion

		// Limpia banderas de interrupcion
	}
	else
	{
		PRINTF("\n\rFallo al detectar la interrupcion.\n\r");
	}

	return;
}

static void timerRtos_Transmision(TimerHandle_t xTimer)
{
	configASSERT(queue_Transmision != NULL);

	if (uxQueueMessagesWaiting(queue_Transmision) > 0)
	{
		BaseType_t status = xQueueReceive(queue_Transmision,
				&canMsg_Transmision, pdMS_TO_TICKS(200));
		if (status != pdPASS)
			PRINTF("\n\rFallo al recivir dato de la cola.\n\r");

		ERROR_t estado;

		estado = mcp2515_sendMessage(&canMsg_Transmision);

		if (estado != ERROR_OK)
		{
			if (estado == ERROR_ALLTXBUSY)
				PRINTF("\n\rError: buffers de transmision llenos.\n\r");
			else if (estado == ERROR_FAILTX)
				PRINTF("\n\rError: fallo al transmitir.\n\r");
			else if (estado == ERROR_SPI_WRITE)
				PRINTF("\n\rError: fallo en escritura de spi.\n\r");
			else
				PRINTF("\nError al enviar\n\r");

			/* Vuelve a cargar el mensaje en la cola si es posible. */
			BaseType_t status = xQueueSendToFront(queue_Transmision,
					&canMsg_Transmision, pdMS_TO_TICKS(200));
			if (status != pdPASS)
			{
				PRINTF("\n\rFallo al cargar datos en la cola.\n\r");
				return;
			}
		}
	}
	else
	{
		// Acciones si no hay mensajes en el buffer
	}

	return;
}

static void NotifySubscribedNodes(uint16_t nodeId)
{
	CANSubscription_t *current = subscriptionList;
	struct can_frame messageCopy;

	while (current != NULL)
	{
		if (current->nodeId == nodeId)
		{
			// Hacer una copia del mensaje para cada nodo
			memcpy(&messageCopy, &canMsg_Receive, sizeof(struct can_frame));

			// Enviar el mensaje a la cola específica del nodo
			xQueueSendToBack(current->queueHandle, &messageCopy,
					pdMS_TO_TICKS(200));

			// Notificar al nodo
			xTaskNotify(current->taskHandle, 0, eIncrement);
		}
		current = current->next;
	}
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
		xTaskNotifyFromISR(task_Receive_Handle, 0, eIncrement,
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
