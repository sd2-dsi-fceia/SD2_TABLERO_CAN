# Proyecto Nodos SD2

## Indice
- [Introducción](#Introducción)
- [Información detallada de cada Nodo](#información-detallada-de-cada-nodo)


## Introducción
El objetivo de este proyecto es mostrar el uso de los nodos CAN, enfocando fuertemente el concepto de productor y consumidor. Detallaremos a continuación en una tabla comparativa cada nodo en particular para poder entender a grandes rasgos que hace cada uno. Luego en la siguiente sección se tendrá una descripción mucho mas detallada de la trama de transimisión y de recepción.

| Nodo | ID recibido   | ID enviado | Consumidor | Productor | Descripción | Dispositivo |
|:----:|:-------------:|:----------:|:----------:|:---------:|:-----------:|:-----------:|
| 1    | 0x120 y 0x130 | 0x110      | X          | X         | Muestra en Display: Temperatura, Humedad, Luz, si se pierde comunicación con Nodo 3, y bandera de alarma de luz | Arduino Uno |
| 2    | 0x110 y 0x130 | 0x120      | X          | X         | Activa una alarma ante un determinado nivel de Luz | Arduino Uno |
| 3    | -             | 0x130      | -          | X         | Envía cada 1seg el valor de Temperatura, Humedad y Luz | Arduino Uno |
| 4    | 0x130         | -          | X          | -         | Reporta por WiFi en una página HTML para ver en el navegador el dato de temperatura y humedad del Nodo 3 | ESP32 |
| 5    | -             | 0x150      | -          | X         | Envía los estados de sus pulsadores SW1 y SW3 | KL46Z |

## Información detallada de cada Nodo
A continuación detallamos cada nodo en particular, describiendo el mensaje que envía y recibe.

---

### Nodo 1 - Arduino Uno

#### Mensaje que envía

| ID    | Trama del mensaje                                                                 | Período de transmisión |
|:-----:|:----------------------------------------------------------------------------------|:----------------------:|
| 0x110 | Byte1: 0 = Tecla off / 1 = Tecla On <br> Byte2: 0 = Tecla Off / 1 = Tecla On      | 200 ms                |

#### Mensaje que recibe

| ID    | Trama del mensaje                                                              | Período de recepción |
|:-----:|:-------------------------------------------------------------------------------|:--------------------:|
| 0x120 | Byte1: Flag Rx de Temperatura <br> Byte2: Flag alarma de luz                   | -                   |
| 0x130 | Byte1: Temperatura <br> Byte2: Humedad <br> Byte3: Luz                         | -                   |

#### Descripción

Muestra en el display LCD el valor de la Temperatura, Humedad, Luz, estado de la conexión con el nodo 3 y bandera de alarma de luz.

---

### Nodo 2

#### Mensaje que recibe

| ID    | Trama del mensaje                                                                                        | Período de recepción |
|:-----:|:---------------------------------------------------------------------------------------------------------|:--------------------:|
| 0x110 | Byte1: 0 = relé1 desactivado / 1 = relé1 activado <br> Byte2: 0 = relé2 desactivado / 1 = relé2 activado | 200 ms               |
| 0x130 | Byte1: Temperatura <br> Byte2: Humedad <br> Byte3: Luz                                                   | 1 s                  |

#### Mensaje que envía

| ID    | Trama del mensaje                                                                      | Período de transmisión |
|:-----:|:---------------------------------------------------------------------------------------|:----------------------:|
| 0x120 | Byte1: Bandera Rx Temp 1/0 <br> Byte2: 0 = Bandera alarma luz / 1 = Bandera activa     | 1 s                    |

#### Descripción

Activa una alarma ante un determinado nivel de Luz. Envía cada 1 segundo el estado de recepción de temperatura y estado de la alarma de luz.

---

### Nodo 3

#### Mensaje que envía

| ID    | Trama del mensaje                                           | Período de transmisión |
|:-----:|:------------------------------------------------------------|:----------------------:|
| 0x130 | Byte1: Temperatura <br> Byte2: Humedad <br> Byte3: Luz      | 1 s                    |

#### Descripción

Envía cada 1 segundo el valor de Temperatura, Humedad y Luz.

---

### Nodo 4

#### Mensaje que recibe

| ID    | Trama del mensaje                                           | Período de transmisión |
|:-----:|:------------------------------------------------------------|:----------------------:|
| 0x130 | Byte1: Temperatura <br> Byte2: Humedad <br> Byte3: Luz      | 1 s                    |

#### Descripción

Trabaja como servidor web. Reporta el valor de Temperatura y Humedad mediante HTML para acceder desde un navegador.

---

### Nodo 5

#### Mensaje que envía

| ID    | Trama del mensaje                                                                                            | Período de transmisión |
|:-----:|:-------------------------------------------------------------------------------------------------------------|:----------------------:|
| 0x150 | Byte1: 0 = switch1 desactivado / 1 = tecla activado <br> Byte2: 0 = switch3 desactivado / 1 = tecla activado | 500 ms                 |

---

## Diagrama de conexión

![Diagrama de conexionado del Bus Can con sus Nodos](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Diagrama_CANBUS.jpeg)

## Organización de carpetas

### Documentación
Dentro de esta carpeta se encuentra el datasheet del módulo MCP2515 y la presentación del módulo.

### Extras
Se encuentra ejemplos de tres Nodos CAN, todos realizados con la placa de desarrollo KL46Z. Se encuentra una mejor explicación y mas detallada dentro del README de dicha carpeta.

### Nodo KL46Z
Se encuentra un Nodo realizado con dicha placa de desarrollo que corresponde al proyecto de Nodos CAN. Dicho Nodo se encarga de mandar cada 500 ms el estado de los pulsadores SW1 y SW3. Dentro de este programa se utiliza la libreria del MCP2515 para la KL46Z.

### Nodos Arduino
Se encuentran todos los programas .ino correspondientes a los Nodos 1,2,3.
