## Proyecto Nodos SD2

# Introducción
El objetivo de este proyecto es mostrar el uso de los nodos CAN, enfocando fuertemente el concepto de productor y consumidor. Como veremos en la siguiente imagen se presentan cinco nodos que detallaremos a continuación en una tabla comparativa.

| Nodo | ID del mensaje | Consumidor | Productor | Descripción | Dispositivo |
|:----:|:------------:|:----------:|:---------:|:-----------:|:-----------:|
| 1    | 0x110        | X          | X         | Envía datos de sus pulsadores. Recibe datos de **Falta** y los muestra por el display LCD | Arduino Uno |
| 2    | 0x120        |            | X         | Descripción Nodo 2 | Arduino Uno |
| 3    | 0x130        |            | X         | Descripción Nodo 3 | Arduino Uno |
| 4    | -            | X          |           | Reporta por wifi en una página HTML para ver en el navegador, el dato de temperatura y humedad del nodo 3 | ESP32 |
| 5    | 0x150        |            | X         | Envía los estados de sus pulsadores SW1 y SW3 | KL46Z |

# Diagrama de conexión

![Diagrama de conexionado del Bus Can con sus Nodos](https://github.com/Agustin586/Ejemplos-SD2/blob/main/image/Diagrama_CANBUS.jpeg)
