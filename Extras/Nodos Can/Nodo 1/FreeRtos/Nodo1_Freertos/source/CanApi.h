/*
 * CanApi.h
 *
 *  Created on: 1 sep. 2024
 *      Author: agustin
 */

#ifndef CANAPI_H_
#define CANAPI_H_

#include "FreeRTOS.h"
#include "task.h"

#include "can.h"

#define INIT_COMPLETE_EVENT (1 << 0)  // Bit del evento que representa la inicialización completa

typedef enum
{
	ERROR_CAN_OK = 0,
	ERROR_CAN_FAILTX,
	ERROR_CAN_QUEUETX,
	ERROR_CAN_FAILRX,
	ERROR_CAN_QUEUERX,
	ERROR_CAN_MEMORY,
	ERROR_CAN_NOT_FOUND,
} Error_Can_t;

typedef struct
{
	/**
	 * @brief Id del nodo al que se subscribe.
	 */
	uint32_t IdSub;
	/**
	 * @brief Handle de la tarea o nodo que espera el evento.
	 */
	TaskHandle_t taskHandle;
} NodoSubscriptions_t;

/**
 * @brief Inicializa la api.
 */
extern void CAN_init(void);
/**
 * @brief Envia informacion al buffer de transmision.
 * @param[in] *dato Puntero al dato de tipo can_frame.
 * @prara[in] xTicksToWait Tiempo de espera maximo.
 * @return Indica si el dato pudo ser cargado en la cola de datos.
 */
extern Error_Can_t CAN_sendMsg(struct can_frame *dato, TickType_t xTicksToWait);
/**
 * @brief Recive la informacion desde la cola de datos del propio nodo.
 * @param[out] *dato Puntero al dato donde cargar la informacion.
 * @param[in] nodeId Id del nodo que se quiere leer.
 * @param[in] taskHandle Indentifica el nodo del que viene.
 */
extern Error_Can_t CAN_readMsg(struct can_frame *dato, uint16_t nodeId,
		TaskHandle_t taskHandle);
/**
 * @brief Crea una subscripcion al nodo con el id especificado.
 * @param[in] nodeId Id del nodo al que se subscribe.
 * @param[in] taskHandle Handle de la tarea que se subscribe.
 */
extern Error_Can_t CAN_Subscribe(uint16_t nodeId, TaskHandle_t taskHandle);
/**
 * @brief Borrar una subscripcion al nodo con el id especificado.
 * @param[in] nodeId Id del nodo al que se desubscribe.
 * @param[in] taskHandle Handle de la tarea que se desubscribe.
 */
extern Error_Can_t CAN_Unsubscribe(uint16_t nodeId, TaskHandle_t taskHandle);
/**
 * @brief Espera hasta que suceda el evento de sincronización.
 */
extern void CAN_getEvent(void);

#endif /* CANAPI_H_ */
