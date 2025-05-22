## Proyecto Nodos SD2

# Introducción
El objetivo de este proyecto es mostrar el uso de los nodos CAN, enfocando fuertemente el concepto de productor y consumidor. Como veremos en la siguiente imagen se presentan cinco nodos que detallaremos a continuación en una tabla comparativa.

| Nodo | ID que envía | Consumidor | Productor | Descripción |
|:----:|:------------:|:----------:|:---------:|:-----------:|
| 1    | 0x110        | X          | X         | Envía datos de sus pulsadores. Recibe datos de **Nodo 5** y los muestra por el display LCD |
| 2    | 0x120        |            | X         | Descripción Nodo 2 |
| 3    | 0x130        |            | X         | Descripción Nodo 3 |
| 4    |              | X          |           | Reporta por wifi en una página HTML para ver en el navegador, el dato de temperatura y humedad del nodo 3 |
| 5    | 0x150        |            | X         | Envía los estados de sus pulsadores SW1 y SW3 |

