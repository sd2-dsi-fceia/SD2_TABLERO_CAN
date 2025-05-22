# Nodos CAN
Para realizar dicha comunicación can nos basamos en el módulo MCP2515 junto con el transceiver TJA1050. A continuación se muestra una imagen de la shield.

![MCP2515](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Nodos%20can/mcp2515.png)

> Módulo MCP2515 shield Arduino

**Tabla de Contenidos**
- [Esquema eléctrico del módulo](#esquema-eléctrico-del-módulo)
- [Conexión a la KL46Z](#Conexión-a-la-KL46Z)
- [Contenido](#Contenido)

## Esquema eléctrico del módulo

![Esquema Electrico](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Nodos%20can/esquematico.png)

> Esquema eléctrico del módulo CAN

## Conexión a la KL46Z
Destacamos los pines que se conectan al módulo.

* MISO: PTE19
* MOSI: PTE18
* SCK: PTE17
* CS: PTE16
* INT: PTA17

**Diagrama de la KL46Z**

![KL46Z Headers](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Nodos%20can/frdm-kl46z_headers.png)

> Pinout de la KL46Z

## Contenido
Dentro de cada carpeta de nodos se tienen dos programas, por un lado se tiene el programa para 
baremetal y por otro lado se tiene el programa para baremetal-quantum leaps.

- Baremetal.
- Baremetal y Quantum Leaps.
