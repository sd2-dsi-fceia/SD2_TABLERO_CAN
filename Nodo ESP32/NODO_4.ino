#include <SPI.h>                 // Librería para la comunicación SPI
#include <mcp2515.h>             // Librería para el controlador CAN MCP2515
#include "WiFi.h"                // Librería WiFi
#include "ESPAsyncWebServer.h"   // Librería para el servidor web asíncrono

// Credenciales Wi-Fi
const char* ssid = " ";          // Reemplaza con tu red Wi-Fi
const char* password = " "; // Reemplaza con tu contraseña

// Crear objeto AsyncWebServer en el puerto 80
AsyncWebServer server(80);

// Estructura para almacenar un mensaje CAN
struct can_frame canMsg_rx;
MCP2515 mcp2515(5); // Instancia del MCP2515 en el pin 5

// Variables para almacenar los datos recibidos
unsigned char temperatura = 0;
unsigned char humedad = 0;

// Funciones para obtener los datos como cadenas
String getTemperatura() {
  return String(temperatura);
}

String getHumedad() {
  return String(humedad);
}

// Página HTML almacenada en PROGMEM
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="UTF-8"> <!-- Asegura la codificación UTF-8 -->
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 CAN Bus Data</title>
  <!-- Bootstrap CSS CDN -->
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css">
  <!-- Font Awesome para íconos -->
  <script src="https://kit.fontawesome.com/a076d05399.js" crossorigin="anonymous"></script>
  <style>
    .display-4 {
      font-size: 2.5rem;
    }
    .unit {
      font-size: 1.2rem;
      color: #666;
    }
    .card-title {
      font-size: 1.5rem;
      font-weight: bold;
    }
    .footer {
      margin-top: 20px;
    }
    /* Estilo para el mensaje "Datos del Bus CAN" en azul */
    .titulo-datos-can {
      color: blue;
    }
    /* Estilo para el texto pequeño debajo del título */
    .subtitulo {
      font-size: 1rem;
      color: #555;
      margin-bottom: 20px;
    }
  </style>
</head>
<body>
  <div class="container text-center my-5">
    <h1 class="mb-3">Facultad de Ciencias Exactas, Ingeniería y Agrimensura</h1>
    <h3 class="mb-4">Escuela de Posgrado y Educación Continua</h3>
    <h2 class="mb-2 titulo-datos-can">Datos del Bus CAN</h2>
    <p class="subtitulo">Lectura de los primeros dos bytes del nodo 1</p>
    <div class="row justify-content-center">
      <!-- Tarjeta de Temperatura -->
      <div class="col-md-4">
        <div class="card border-danger mb-4 shadow-sm">
          <div class="card-body">
            <div class="mb-3">
              <i class="fas fa-temperature-high fa-3x text-danger"></i>
            </div>
            <h5 class="card-title">Temperatura</h5>
            <p class="display-4">
              <span id="temperatura">%TEMPERATURA%</span><span class="unit">°C</span>
            </p>
          </div>
        </div>
      </div>
      <!-- Tarjeta de Humedad -->
      <div class="col-md-4">
        <div class="card border-primary mb-4 shadow-sm">
          <div class="card-body">
            <div class="mb-3">
              <i class="fas fa-tint fa-3x text-primary"></i>
            </div>
            <h5 class="card-title">Humedad</h5>
            <p class="display-4">
              <span id="humedad">%HUMEDAD%</span><span class="unit">%</span>
            </p>
          </div>
        </div>
      </div>
    </div>
    <!-- Pie de página con los autores -->
    <footer class="footer mt-auto py-3">
      <div class="container">
        <span class="text-muted">by Daniel Márquez y Walter Wagner</span>
      </div>
    </footer>
  </div>
  <!-- Bootstrap JS y dependencias -->
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.bundle.min.js"></script>
  <!-- Actualizar los datos cada segundo -->
  <script>
    setInterval(function() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('temperatura').innerText = data.temperatura;
          document.getElementById('humedad').innerText = data.humedad;
        });
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

// Función para reemplazar los placeholders en el HTML
String processor(const String& var){
  if(var == "TEMPERATURA"){
    return getTemperatura();
  } else if(var == "HUMEDAD"){
    return getHumedad();
  }
  return String();
}

void setup() {
  // Inicializar comunicación serial para depuración
  Serial.begin(115200);

  // Inicializar MCP2515
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();

  // Conectar a la red Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a la red Wi-Fi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Configurar rutas del servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Ruta para obtener los datos en formato JSON
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"temperatura\":" + String(temperatura) + ",";
    json += "\"humedad\":" + String(humedad);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Iniciar el servidor
  server.begin();
}

void loop() {
  // Leer mensajes CAN
  if (mcp2515.readMessage(&canMsg_rx) == MCP2515::ERROR_OK) {
    // Procesar mensajes del nodo 1 (ID puede variar según configuración)
    if (canMsg_rx.can_id == 0x0130) {
      temperatura = canMsg_rx.data[0]; // Primer byte
      humedad = canMsg_rx.data[1];     // Segundo byte
      // Ya no se utiliza el byte de luz
    }
  }
}


