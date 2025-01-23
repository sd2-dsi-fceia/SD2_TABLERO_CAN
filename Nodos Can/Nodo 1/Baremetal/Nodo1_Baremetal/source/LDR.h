/*
 * LDR.h
 *
 *  Created on: 29 ago. 2024
 *      Author: agustin
 */

/**
 * @file LDR.h
 * @author Zuliani, Agustin
 * @brief Archivo de lectura y configuracion del ldr de la KL46Z
 * @details Dentro de este archivo se configuran los pines necesarios
 * para trabajar con el convertidor de analogico a digital correspondiente
 * al canal donde se encuentra el ldr de la placa. Tambien se realiza
 * la lectura de dicho valor cada un tiempo definido por el usuario.
 */

#ifndef LDR_H_
#define LDR_H_

#include <stdint.h>
#include <stdbool.h>

#define TIEMPO_DE_MUESTREO_LDR	500	// En milisegundos

#define ADC16_IRQ_HANDLER_FUNC ADC0_IRQHandler

typedef enum {
	ERROR_LDR_OK = 0,
	ERROR_LDR_INIT,
	ERROR_LDR_READ,
} Error_LDR_t;

/**
 * @brief Inicializa los perifericos correspondientes.
 * @return Estado de la inicializacion.
 */
extern Error_LDR_t LDR_init(void);
/**
 * @brief Lee el valor cargado en el registro una vez finalizada la
 * conversion.
 */
extern void LDR_read(void);
/**
 * @brief Lee el valor de la ultima conversion del adc.
 * @return Devuelve el valor de la conversion del adc.
 */
extern uint16_t LDR_UltimaConversion(void);
/**
 * @brief Lanza la conversion.
 *
 * Esta funcion debe ser llamada con el correspondiente tiempo
 * de muestreo.
 * @return Verifica si pudo ser lanzada la conversion o hubo un fallo.
 */
extern Error_LDR_t LDR_convertir(void);
/**
 * @brief Obtiene el estado de la conversion.
 *
 * @return Estado de la conversion.
 */
extern bool LDR_getConvComplete(void);
/**
 * @brief Setea el fin de la conversion.
 */
extern void LDR_setConvComplete(void);
/**
 * @brief Limpia la bandera de conversion completa.
 */
extern void LDR_clearConvComplete(void);

#endif /* LDR_H_ */
