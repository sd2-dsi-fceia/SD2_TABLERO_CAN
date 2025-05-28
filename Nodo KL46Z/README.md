# Ejemplo-de-comunicacion-CAN

## Introducción

Dentro de esta carpeta podemos encontrar dos proyectos para la KL46Z. El proyecto con el nombre MensajeCAN se encarga de enviar un mensaje donde se informen
los estados de los pulsadores SW1 y SW3 cada 500 ms. Esto se realiza cada 500 ms. Luego se encuentra el otro proyecto ReceptorCAN que se encarga de tomar
cualquier mensaje CAN e imprimirlo en el puerto serie. El período de recepción de mensajes es cada 300 ms (se puede cambiar en la macro correspondiente).

## Conexión del Nodo a la KL46Z

* MISO: PTE19
* MOSI: PTE18
* SCK: PTE17
* CS: PTE16
* INT: PTA17

![KL46Z Headers](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Nodos%20can/frdm-kl46z_headers.png)
> Pinout de la KL46Z

## Documentacion de la libreria MCP2515
[Ver index.html](https://agustin586.github.io/Ejemplos-SD2/)
