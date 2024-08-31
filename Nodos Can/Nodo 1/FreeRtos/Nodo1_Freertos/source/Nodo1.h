/*
 * Nodo1.h
 *
 *  Created on: 30 ago. 2024
 *      Author: agustin
 */

/**
 * @file Nodo 1
 * @author Zuliani, Agustin
 * @date 30/08/24
 * @brief El nodo 1 se encarga de enviar informacion al bus
 * can sobre el estado del sensor de luz.
 */
#ifndef NODO1_H_
#define NODO1_H_

#include "can.h"

/**
 * @brief Inicializa las tareas y demas parametros
 * del nodo 1.
 */
extern void Nodo1_init(void);
/**
 * @brief Obtiene el id del nodo 1.
 */
extern canid_t Nodo1_id(void);

#endif /* NODO1_H_ */
