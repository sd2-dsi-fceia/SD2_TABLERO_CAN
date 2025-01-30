# Blink Quantum Leaps

En este ejemplo se muestra un programa para la KL46Z que utiliza el framework de QP para realizar una máquina de estados de un blink.

## Objetivos
A continuación se detallan los objetivos y conceptos que se utilizan en esta aplicación.

- Creación de un objeto activo.
- Manejo de eventos de tiempo.
- Manejo de señales de eventos.

## Funciones
En esta sección nos centraremos en las funciones pricipales que se utilizan para configurar parámetros, a decir eventos de tiempo.

### Eventos de tiempo
Mostramos las funciones utilizadas para los eventos de tiempo y un ejemplo de uso.

- `QTimeEvt_ctorX`

```c
QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
```
Inicializa un objeto de evento de tiempo `(QTimeEvt)`.


- **Armado de la función:** `QTimeEvt_armX()`

**Ejemplo de uso**

```c
QTimeEvt_armX(&me->timeEvt,
BSP_TICKS_PER_SEC/2, BSP_TICKS_PER_SEC/2);
```

**Descripción**

Esta función se utiliza para habilitar el evento de tiempo. Se pasan tres parámetros, el primero corresponde al atributo timeEvt. El segundo corresponde al tiempo que tarda en iniciarse y el último cada cuanto se repite.
