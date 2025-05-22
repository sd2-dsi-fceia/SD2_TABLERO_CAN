#include <SPI.h> // Librería para la comunicación SPI
#include <mcp2515.h> // Librería para el controlador CAN MCP2515
#include "DHT.h" // Librería para el sensor DHT

#define DHTPIN 4     // Pin del sensor DHT11
#define DHTTYPE DHT11 // Tipo de sensor DHT11

// Estructura para almacenar un mensaje CAN
struct can_frame canMsg1;
// Instancia del objeto MCP2515 en el pin 10
MCP2515 mcp2515(10);


// Instancia del sensor DHT
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Inicializar el sensor DHT
  dht.begin();

  // Configurar el mensaje CAN
  canMsg1.can_id  = 0x0130; // Identificador del nodo transmisor
  canMsg1.can_dlc = 8; // Data Length Code - código de longitud de datos, 8 Bytes
  // Inicializar los datos del mensaje CAN a 0x00
  for (int i = 0; i < 8; i++) {
    canMsg1.data[i] = 0x00;
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
}

void loop() {
  // Leer la temperatura y la humedad del sensor DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Verificar si la lectura es válida
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Error al leer el sensor DHT11");
    return;
  }

  int ldr = analogRead(A1); 


  // Pausa para estabilizar las lecturas del sensor
  delay(50);
  
  // Escalar la temperatura y la humedad a enteros (manteniendo dos decimales)
  char tempInt = (char)temperature;
  char humInt = (char)humidity;
  float porcentaje = (ldr / 1023.0) * 100;
  unsigned char ldrInt = porcentaje;

  // Dividir los valores en bytes y almacenarlos en el mensaje CAN
  canMsg1.data[0] = tempInt; // Parte alta de la temperatura
  canMsg1.data[1] = humInt;        // Parte baja de la temperatura
  canMsg1.data[2] = ldrInt;  // Parte alta de la humedad

  //Serial.println(ldrInt);

  // Enviar el mensaje CAN
  mcp2515.sendMessage(&canMsg1);



  // Pausa antes de la siguiente iteración del loop
  delay(1000); // Pausa de 2 segundos para evitar lecturas demasiado frecuentes
}