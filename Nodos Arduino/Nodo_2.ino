#include <SPI.h>      // Librería para la comunicación SPI
#include <mcp2515.h>  // Librería para el controlador CAN MCP2515

// Estructura para almacenar un mensaje CAN
struct can_frame canMsg_tx;
struct can_frame canMsg_rx;
// Instancia del objeto MCP2515 en el pin 10
MCP2515 mcp2515(10);

// Definición del pin del pulsador
int salida_rele_1 = 3;
int salida_rele_2 = 4;
int salida_rele_3 = 6;
//Tiempo de envio de mensaje de temperatura
int tiempo_temperatura = 1000;
// Variable de cuenta para temporizador
unsigned int cuenta_tiempo = 0;
int bandera_temp_rx = 0;
int bandera_limite_luz = 0;
char temperatura;
char humedad;
unsigned char luz;
int cuenta_rx_temperatura = 0;

void setup() {
  // Configurar el pin del pulsador como entrada con resistencia de pull-up
  pinMode(salida_rele_1, OUTPUT);
  pinMode(salida_rele_2, OUTPUT);
  pinMode(salida_rele_3, OUTPUT);

  // Configurar el mensaje CAN
  canMsg_tx.can_id = 0x0120;  // Identificador del nodo transmisor
  canMsg_tx.can_dlc = 2;      // Data Length Code - código de longitud de datos, 2 Bytes.
  /*
  Byte 1: Indica si se recibe temperatura. Valor 0 no se recibe. Valor 1 se recibe.
  Byte 2: Alarma de limite de temperatura alcanzado. Si se alcanza se activa salida rele 3
  */

  // Inicializar los datos del mensaje CAN a 0x00
  for (int i = 0; i < canMsg_tx.can_dlc; i++) {
    canMsg_tx.data[i] = 0x00;
  }

  // Inicializar la comunicación serial a 115200 baudios
  Serial.begin(115200);
  while (!Serial)
    ;  // Esperar hasta que la comunicación serial esté lista

  // Reiniciar el MCP2515
  mcp2515.reset();
  // Configurar la velocidad de bits a 125 Kbps
  mcp2515.setBitrate(CAN_125KBPS);
  // Configurar el MCP2515 en modo normal
  mcp2515.setNormalMode();
}

void loop() {


  // Lee mensajes CAN para activar salidas
  if (mcp2515.readMessage(&canMsg_rx) == MCP2515::ERROR_OK) {

    // Controla recepción de IDs
    if (canMsg_rx.can_id == 0x0110) {
      if (canMsg_rx.data[0] == 1) {
        digitalWrite(salida_rele_1, HIGH);
      } else {
        digitalWrite(salida_rele_1, LOW);
      }

      if (canMsg_rx.data[1] == 1) {
        digitalWrite(salida_rele_2, HIGH);
      } else {
        digitalWrite(salida_rele_2, LOW);
      }
    }

    // Controla recepción de IDs
    if (canMsg_rx.can_id == 0x0130) {

      cuenta_rx_temperatura = 0;
      bandera_temp_rx = 1;

      /*
       Byte 1: Temperatura
       Byte 2: Humedad
       Byte 3: Luz
       */
      temperatura = canMsg_rx.data[0];
      humedad = canMsg_rx.data[1];
      luz = canMsg_rx.data[2];

      // Limite de luz
      if (luz > 50) {
        bandera_limite_luz = 1;
        digitalWrite(salida_rele_3, HIGH);
      } else {
        bandera_limite_luz = 0;
        digitalWrite(salida_rele_3, LOW);
      }

    } else {
      if (cuenta_rx_temperatura == 10) {
        bandera_temp_rx = 0;
        digitalWrite(salida_rele_3, LOW);
      } else cuenta_rx_temperatura++;
    }
  }

  // Revisa estado de recepción de temperatura
  if (cuenta_tiempo == tiempo_temperatura) {

    // Actualizar bytes de teclas del mensaje CAN con sus estados actuales
    canMsg_tx.data[0] = bandera_temp_rx;
    canMsg_tx.data[1] = bandera_limite_luz;

    // Enviar el mensaje CAN
    mcp2515.sendMessage(&canMsg_tx);
    // Resetea cuenta de tiempo
    cuenta_tiempo = 0;

  } else cuenta_tiempo++;


  // Pausa antes de la siguiente iteración del loop
  delay(1);
}