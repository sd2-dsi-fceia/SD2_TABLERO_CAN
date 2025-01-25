# Ejemplos-SD2

## Objetivos

En este repositorio se encuentran todos los ejemplos brindados para trabajar en sistemas digitales 2. El objetivo de este repositorio es ayudar al alumno a la hora de ver el correcto funcionamiento del programa o módulo. Algunos de los ejemplos brindados constan de:

- Comunicación CAN con tres nodos.
- Manejo de Quantum Leaps.

**Tabla de Contenidos**

[TOCM]

[TOC]

# Nodos Can Baremetal

## Descripción

En este ejemplo se tienen tres nodos en donde se evaluarán como se interconectan entre si. Tenemos dos nodos que funcionan como productor y otros dos nodos como consumidor.  A continuación se comenta en profundidad la función cada nodo:

|  Nodo | ID  | Consumidor  | Productor  | Función  |
| ------------ | :------------: | :------------: | :------------: | ------------ |
| 1 | 10  |   | X  | Enviar datos del Luz cada 1 segundo  |
| 2 | 20  |  X | X  | Enviar datos de sus periféricos cada 500 ms y tomar datos de luz del Nodo 1 |
| 3 |   |  X |   |  Tomar datos de ambos Nodos y mostrarlos por pantalla |

## Esquema de nodos

```seq
Nodo 1->Nodo 2: Intensidad de luz
Nodo 1->Nodo 3: Intensidad de luz
Nodo 2->Nodo 3: Estado periféricos
Note right of Nodo 3: Salida de datos \npor pantalla
```
