# Nodo de KL46Z

## Introducción

Dentro de esta carpeta podemos encontrar dos proyectos para la KL46Z. El proyecto con el nombre MensajeCAN se encarga de enviar un mensaje donde se informen
los estados de los pulsadores SW1 y SW3 cada 500 ms. Esto se realiza cada 500 ms. Luego se encuentra el otro proyecto ReceptorCAN que se encarga de tomar
cualquier mensaje CAN e imprimirlo en el puerto serie. El período de recepción de mensajes es cada 300 ms (se puede cambiar en la macro correspondiente).

## Conexión del Nodo a la KL46Z

Se debe conectar con el módulo CAN MCP2515 de la siguiente manera:
* MISO: PTE19
* MOSI: PTE18
* SCK: PTE17
* CS: PTE16
* INT: PTA17

![KL46Z Headers](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Nodos%20can/frdm-kl46z_headers.png)
> Pinout de la KL46Z

## Documentacion de la libreria MCP2515
[Ver index.html](https://agustin586.github.io/Ejemplos-SD2/)

## Ejemplo de uso de la liberia

> Para utilizar correctamente la libreria es necesario que se tenga la libreria correspondiente al manejo del periférico SPI de la KL46Z
> y además un archivo can.h que permite definir una estructura para el mensaje a enviar.

### Inicialización CAN

```c
// Reseteo del módulo
ERROR_t status = mcp2515_reset();                
assert(status == ERROR_OK);                            // Detiene la ejecución del programa

// Configuración de Bits por Segundos
status = mcp2515_setBitrate(CAN_125KBPS, MCP_8MHZ);
assert(status == ERROR_OK);

// Modo de operación del módulo
status = mcp2515_setNormalMode();
assert(status == ERROR_OK);
```

### Enviar mensaje CAN
```c
ERROR_t error;
struct can_frame canMsg1;

error = mcp2515_sendMessage(&canMsg1);

if (error != ERROR_OK)
  PRINTF("Error al enviar el mensaje");
```

### Recibir mensaje CAN (polling)
```c
ERROR_t error;
struct can_frame canMsgRead;

error = mcp2515_readMessage(&canMsgRead);

if (error != ERROR_OK)
  PRINTF("No hubo mensaje recibidos o hubo error");
```
### Configurar filtro y máscaras
```c
#define MASK0_ID10 0x00F
#define FILTER0_ID10 0x00A
#define FILTER1_ID20 20

mcp2515_setFilterMask(MASK0, false, MASK0_ID10);
mcp2515_setFilterMask(MASK1, false, MASK0_ID10);

mcp2515_setFilter(RXF0, false, FILTER0_ID10);
mcp2515_setFilter(RXF2, false, FILTER1_ID20);
```

### Configurar Modo de funcionamiento
```c
ERROR_t error;

error = mcp2515_setListenOnlyMode();
assert (error == ERROR_OK)

error = mcp2515_setSleepMode();
assert (error == ERROR_OK)

error = mcp2515_setLoopbackMode();
assert (error == ERROR_OK)

error = mcp2515_setNormalMode();
assert (error == ERROR_OK)
```

