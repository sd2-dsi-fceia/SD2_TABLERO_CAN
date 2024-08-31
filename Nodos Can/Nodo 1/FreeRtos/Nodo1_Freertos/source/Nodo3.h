/*
 * Nodo3.h
 *
 *  Created on: 30 ago. 2024
 *      Author: agustin
 */
/**
 * @file Nodo 3
 * @author Zuliani, Agustin
 * @date 30/08/24
 * @brief El nodo 3 se encarga de mostrar por pantalla
 * la informacion recopilada desde los nodos anteriores.
 */

#ifndef NODO3_H_
#define NODO3_H_

#include "can.h"

/**
 * @brief Inicializa el nodo 3.
 */
extern void Nodo3_init(void);
/**
 * @brief Obtiene el id del nodo 3.
 */
extern canid_t Nodo3_id(void);
/**
 * @brief Genera una notificacion al nodo 3.
 */
extern void Nodo3_giveNotify(void);

#endif /* NODO3_H_ */
