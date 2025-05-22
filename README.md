# Proyecto Nodos SD2

## Introducción
El objetivo de este proyecto es mostrar el uso de los nodos CAN, enfocando fuertemente el concepto de productor y consumidor. Como veremos en la siguiente imagen se presentan cinco nodos que detallaremos a continuación en una tabla comparativa.

| Nodo | ID recibido | ID enviado | Consumidor | Productor | Descripción | Dispositivo |
|:----:|:-----------:|:----------:|:----------:|:---------:|:-----------:|:-----------:|
| 1    | 0x150       | 0x110      | X          | X         | Envía datos de sus pulsadores. Recibe datos de **Falta** y los muestra por el display LCD | Arduino Uno |
| 2    | -           | 0x120      |            | X         | Descripción Nodo 2 | Arduino Uno |
| 3    | -           | 0x130      |            | X         | Descripción Nodo 3 | Arduino Uno |
| 4    | 0x130       | -          | X          |           | Reporta por WiFi en una página HTML para ver en el navegador el dato de temperatura y humedad del Nodo 3 | ESP32 |
| 5    | -           | 0x150      |            | X         | Envía los estados de sus pulsadores SW1 y SW3 | KL46Z |

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
