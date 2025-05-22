/*
 * Nodo2.h
 *
 *  Created on: 30 ago. 2024
 *      Author: agustin
 */

#ifndef NODO2_H_
#define NODO2_H_

#include "can.h"

/**
 * @brief Inicializa el nodo 2.
 */
extern void Nodo2_init(void);
/**
 * @brief Obtiene el id del nodo 2.
 */
extern canid_t Nodo2_id(void);
/**
 * @brief Genera una notificacion al nodo 2.
 */
extern void Nodo2_giveNotify(void);

#endif /* NODO2_H_ */
