#include <SPI.h> // Librería para la comunicación SPI
#include <mcp2515.h> // Librería para el controlador CAN MCP2515
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

//Crear el objeto lcd  dirección  0x27 y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27,16,2);  //

// Estructura para almacenar un mensaje CAN
struct can_frame canMsg_tx;
struct can_frame canMsg_rx;
// Instancia del objeto MCP2515 en el pin 10
MCP2515 mcp2515(10);

// Definición del pin del pulsador
int switch_1 = 6;
int switch_2 = 7;
int tiempo_lectura_teclas = 200; // Tiempo de lectura de teclas y envío de mensajes de estado
// Variable para almacenar el estado del pulsador
int valor_switch_1 = 1; // Valor inicial del pulsador (no presionado)
int valor_switch_2 = 1; // Valor inicial del pulsador (no presionado)
// Variable de cuenta para temporizador
unsigned int cuenta_tiempo = 0;

void setup() {

  // Inicializar el LCD
  lcd.init();
  
  //Encender la luz de fondo.
  lcd.backlight();
  
  // Configurar el pin del pulsador como entrada con resistencia de pull-up
  pinMode(switch_1, INPUT_PULLUP);
  pinMode(switch_2, INPUT_PULLUP);

  // Configurar el mensaje CAN
  canMsg_tx.can_id  = 0x0110; // Identificador del nodo transmisor
  canMsg_tx.can_dlc = 2; // Data Length Code - código de longitud de datos, 2 Bytes
  
  // Inicializar los datos del mensaje CAN a 0x00
  for (int i = 0; i < canMsg_tx.can_dlc; i++) {
    canMsg_tx.data[i] = 0x00;
  }

  // Inicializar la comunicación serial a 115200 baudios
  Serial.begin(115200);
  while (!Serial); // Esperar hasta que la comunicación serial esté lista

  // Reiniciar el MCP2515
  mcp2515.reset();
  // Configurar la velocidad de bits a 125 Kbps
  mcp2515.setBitrate(CAN_125KBPS);
  // Configurar el MCP2515 en modo normal
  mcp2515.setNormalMode();

  lcd.setCursor(0,0);
  lcd.print("RxTem:  Limite: ");
  lcd.setCursor(0,1);
  lcd.print("T:-- H:-- L:--  ");
}

void loop() {
  
  // Revisa estado de teclas cada 200 mseg aproximadamente
  if(cuenta_tiempo == tiempo_lectura_teclas)
  {
    // Leer el estado del pulsador
    valor_switch_1 = digitalRead(switch_1);
    valor_switch_2 = digitalRead(switch_2);

    // Actualizar bytes de teclas del mensaje CAN con sus estados actuales (invertido)
    canMsg_tx.data[0] = !valor_switch_1;
    canMsg_tx.data[1] = !valor_switch_2;

    // Enviar el mensaje CAN
    mcp2515.sendMessage(&canMsg_tx);
    // Resetea cuenta de tiempo
    cuenta_tiempo = 0;

  }
  else cuenta_tiempo++;
   
  if (mcp2515.readMessage(&canMsg_rx) == MCP2515::ERROR_OK) {
    
    // Controla recepción de IDs
    if (canMsg_rx.can_id == 0x0120)  
    {
      int bandera_temp_rx = canMsg_rx.data[0];
      int bandera_limite_luz = canMsg_rx.data[1];
      Serial.print("Bandera Rx Temp: ");
      Serial.print(bandera_temp_rx);
      Serial.print(" Limite Luz: ");
      Serial.print(bandera_limite_luz);
      Serial.println();

      lcd.setCursor(0,0);
      lcd.print("RxTem:");
      lcd.print(bandera_temp_rx);
      lcd.print(" Limite:");
      lcd.print(bandera_limite_luz);
      //lcd.setCursor(0,0);

      if(bandera_temp_rx == 0){
        lcd.setCursor(0,1);
        lcd.print("T:-- H:-- L:--  ");
      }

    }
    // Controla recepción de IDs
    if (canMsg_rx.can_id == 0x0130)  
    {
      unsigned char temperatura = canMsg_rx.data[0];
      unsigned char humedad = canMsg_rx.data[1];
      unsigned char luz = canMsg_rx.data[2];

      Serial.print("Temp: ");
      Serial.print(temperatura);
      Serial.print(" Hum: ");
      Serial.print(humedad);
      Serial.print(" Luz: ");
      Serial.print(luz);
      Serial.println();
      
      lcd.setCursor(0,1);
      lcd.print("T:");
      lcd.print(temperatura);
      lcd.print(" H:");
      lcd.print(humedad);
      lcd.print(" L:");
      lcd.print(luz);

    }

  }
  

  /*
  // Imprimir un mensaje en el monitor serial indicando que el mensaje fue enviado
  Serial.println("Mensaje enviado");
  // Imprimir los datos del mensaje CAN en el monitor serial
  for (int i = 0; i < 8; i++) {
    Serial.print(canMsg1.data[i]);
    if (i < 7) Serial.print(",");
  }
  Serial.println();
  */

  // Pausa antes de la siguiente iteración del loop
  delay(1);
}