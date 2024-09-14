/*
 * CanApi.h
 *
 *  Created on: 1 sep. 2024
 *      Author: agustin
 */

#ifndef CANAPI_H_
#define CANAPI_H_

#include "can.h"
#include "mcp2515.h"

#include <stdbool.h>

#define TIMEOUT_ENABLE	1

/**
 * @brief Tipo de funcion callback.
 */
typedef void (*can_callback_t)(canid_t subscriberId, canid_t nodoId);

typedef enum
{
	ERROR_CAN_OK = 0,
	ERROR_CAN_FAILTX,
	ERROR_CAN_QUEUETX_FULL,
	ERROR_CAN_FAILRX,
	ERROR_CAN_QUEUERX_EMPTY,
	ERROR_CAN_MEMORY,
	ERROR_CAN_NOT_FOUND,
	ERROR_CAN_ALREADY_SUBSCRIBED,
	ERROR_CAN_NO_EVENT_TX,
	ERROR_CAN_NO_EVENT_RX,
} Error_Can_t;

/**
 * @brief Inicializa la api.
 */
extern void CAN_init(void);
/**
 * @brief Envia informacion al buffer de transmision.
 * @param[in] *dato Puntero al dato de tipo can_frame.
 * @return Indica si el dato pudo ser cargado en la cola de datos.
 */
extern Error_Can_t CAN_sendMsg(struct can_frame *dato);
/**
 * @brief Recive la informacion desde la cola de datos del propio nodo.
 * @param[out] *dato Puntero al dato donde cargar la informacion.
 * @param[in] nodeId Id del nodo que se quiere leer.
 * @param[in] taskHandle Indentifica el nodo del que viene.
 */
extern Error_Can_t CAN_readMsg(struct can_frame *dato, canid_t nodeId, canid_t subscriberId);
/**
 * @brief Crea una subscripcion al nodo con el id especificado.
 * @param[in] nodeId Id del nodo al que se subscribe.
 * @param[in] subscriberId Id del nodo que se subscribe.
 * @param[in] callback Funcion de callback cuando se genera el evento.
 */
extern Error_Can_t CAN_Subscribe(canid_t nodeId,  canid_t subscriberId, can_callback_t callback);
/**
 * @brief Borrar una subscripcion al nodo con el id especificado.
 * @param[in] nodeId Id del nodo al que se desubscribe.
 * @param[in] subscriberId Id del nodo que se desubscribe.
 */
extern Error_Can_t CAN_Unsubscribe(canid_t nodeId, canid_t subscriberId);
/**
 * @brief Procesa los eventos de transmision.
 *
 * Puede llamarse dentro del systick handler pero afecta
 * dependiendo de la velocidad del spi.
 */
extern Error_Can_t CAN_eventTx(void);
/**
 * @brief Procesa los eventos de recepcion.
 *
 * Esta funcion puede llamarse dentro del systick handler
 * siempre y cuando las funciones de callback sean cortas.
 */
extern Error_Can_t CAN_eventRx(void);
/**
 * @brief Setea el filtro del modulo.
 */
extern Error_Can_t CAN_setFilter(const RXF num, const bool ext,
								 const uint32_t ulData);
/**
 * @brief Setea la mascara del modulo.
 */
extern Error_Can_t CAN_setMask(const MASK mask, const bool ext,
									 const uint32_t ulData);
/**
 * @brief Procesa las unidades de tiempo de can.
 */
extern bool CAN_getTimer(void);

#endif /* CANAPI_H_ */
