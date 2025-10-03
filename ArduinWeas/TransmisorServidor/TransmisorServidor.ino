// WEMOS D1 #1 - SERVIDOR CON SENSOR DE MOVIMIENTO Y LED
// Tiene sensor PIR local, LED controlable, y responde a comandos WiFi

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Configuración WiFi
const char* ssid = "DoyleDog";         
const char* password = "cris02894"; 

// Servidor web en puerto 80
ESP8266WebServer server(80);

// Pines
const int ledPin = 14;        // D3 (GPIO14)
const int pirPin = 13;        // D7 (GPIO13)
const int buzzerPin = 12;     // D6 (GPIO12) - Agregamos el pin del buzzer

// Variables
bool ledState = false;       // Estado del LED (ON/OFF)
bool buzzerState = false;    // Estado del BUZZER (ON/OFF)
bool motionDetected = false; // Estado del sensor de movimiento
unsigned long lastMotionCheck = 0;
const unsigned long motionCheckInterval = 1000; // Verificar movimiento cada segundo
String lastClientIP = "";
unsigned long lastCommandReceived = 0;
int val = 0;                // Variable para leer el estado del sensor PIR

// Estadísticas
int commandsReceived = 0;
int motionRequests = 0;

// Variables del watchdog
unsigned long watchdogTimer = 0;
const unsigned long WATCHDOG_TIMEOUT = 8000;
int resetCount = 0;

#define MAX_LOGS 50  // Máximo número de registros

struct MovementLog {
  unsigned long timestamp;
  bool isMotionStart;  // true = inicio de movimiento, false = fin de movimiento
};

// Array para almacenar los registros
MovementLog movementLogs[MAX_LOGS];
int logCount = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Aumentamos el delay inicial para estabilización

  Serial.println("\n\n=== INICIANDO SISTEMA ===");
  
  // Configurar pines
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT); // Agregamos configuración del buzzer
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
  pinMode(pirPin, INPUT);
  
  Serial.println("- Pines configurados");
  Serial.println("- LED en pin D3 (GPIO14)");
  Serial.println("- PIR en pin D7 (GPIO13)");
  Serial.println("- Buzzer en pin D6 (GPIO12)");
  
  // Configuración WiFi mejorada
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);  // Desconectar cualquier conexión previa
  delay(1000);           // Esperar a que se limpie

  Serial.println("\nEscaneando redes WiFi disponibles...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No se encontraron redes WiFi!");
  } else {
    Serial.print(n);
    Serial.println(" redes encontradas:");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println("dBm)");
      delay(10);
    }
  }
  
  Serial.println("\nIntentando conexión a WiFi:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password length: ");
  Serial.println(strlen(password));
  
  WiFi.begin(ssid, password);
  
  // Aumentamos el tiempo de espera y mostramos más información
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { // Aumentado a 30 intentos
    delay(1000);  // Aumentado a 1 segundo entre intentos
    Serial.print(".");
    
    // Mostrar estado de la conexión
    switch(WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("\nSSID no encontrado!");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("\nPassword incorrecto!");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("\nConexión perdida!");
        break;
    }
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nError de conexión WiFi - Reiniciando...");
    ESP.restart();
    return;
  }
  
  Serial.println("\nConexión WiFi establecida!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurar servidor
  setupServerRoutes();
  server.begin();
  Serial.println("Servidor web iniciado");
  Serial.println("- Rutas configuradas:");
  Serial.println("  * /        -> Página principal");
  Serial.println("  * /led     -> Control LED");
  Serial.println("  * /sensor  -> Estado sensor");
  Serial.println("  * /status  -> Estado sistema");
  
  // Test inicial del LED
  Serial.println("Realizando test del LED...");
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  
  Serial.println("Sistema listo!");
  Serial.println("================");
  
  watchdogTimer = millis();
}

void loop() {
  // Watchdog check
  if (millis() - watchdogTimer > WATCHDOG_TIMEOUT) {
    Serial.println("Watchdog timeout - Reiniciando...");
    ESP.restart();
    return;
  }
  
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conexión WiFi perdida - Reconectando...");
    WiFi.reconnect();
    delay(500);
    return;
  }
  
  // Añadir checkMotion aquí
  checkMotion();
  
  // Manejar peticiones del servidor web
  server.handleClient();
  
  // Reset watchdog timer
  watchdogTimer = millis();
  
  checkTimeout(); // Agregar para ver estadísticas periódicas
  
  // Dar tiempo al ESP8266 para tareas del sistema
  yield();
  delay(10);
}

void checkMotion() {
  val = digitalRead(pirPin);
  
  if (val == HIGH) {
    if (!motionDetected) {
      Serial.println("=== SENSOR PIR ===");
      Serial.println("Estado: MOVIMIENTO DETECTADO");
      Serial.println("================");
      motionDetected = true;
      
      // Encender LED y activar buzzer al detectar movimiento
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
      delay(100);  // Beep corto
      digitalWrite(buzzerPin, LOW);
      ledState = true;
      
      // Agregar registro de inicio de movimiento
      if (logCount < MAX_LOGS) {
        movementLogs[logCount].timestamp = millis();
        movementLogs[logCount].isMotionStart = true;
        logCount++;
      }
    }
  } else {
    if (motionDetected) {
      Serial.println("=== SENSOR PIR ===");
      Serial.println("Estado: MOVIMIENTO TERMINADO");
      Serial.println("================");
      motionDetected = false;
      
      // Apagar LED y hacer doble beep cuando termina el movimiento
      digitalWrite(ledPin, LOW);
      ledState = false;
      
      // Doble beep
      digitalWrite(buzzerPin, HIGH);
      delay(50);
      digitalWrite(buzzerPin, LOW);
      delay(50);
      digitalWrite(buzzerPin, HIGH);
      delay(50);
      digitalWrite(buzzerPin, LOW);
      
      // Agregar registro de fin de movimiento
      if (logCount < MAX_LOGS) {
        movementLogs[logCount].timestamp = millis();
        movementLogs[logCount].isMotionStart = false;
        logCount++;
      }
    }
  }
}

void setupServerRoutes() {
  // Ruta para controlar el LED
  server.on("/led", HTTP_GET, handleLedControl);
  
  // Ruta para obtener datos del sensor PIR
  server.on("/sensor", HTTP_GET, handleSensorRequest);
  
  // Ruta para ver el estado del sistema
  server.on("/status", HTTP_GET, handleStatus);
  
  // Ruta raíz con información básica
  server.on("/", HTTP_GET, handleRoot);
  
  // Ruta para controlar el BUZZER
  server.on("/buzzer", HTTP_GET, handleBuzzerControl);
  
  // Manejar rutas no encontradas
  server.onNotFound(handleNotFound);
}

void handleLedControl() {
  Serial.println("=== COMANDO LED RECIBIDO ===");
  
  if (server.hasArg("state")) {
    String stateStr = server.arg("state");
    bool newState = (stateStr == "1" || stateStr.equalsIgnoreCase("true") || stateStr.equalsIgnoreCase("on"));
    
    ledState = newState;
    digitalWrite(ledPin, ledState);
    
    // Actualizar información
    lastCommandReceived = millis();
    lastClientIP = server.client().remoteIP().toString();
    commandsReceived++;
    
    Serial.print("LED: ");
    Serial.println(ledState ? "ENCENDIDO" : "APAGADO");
    Serial.print("Cliente: ");
    Serial.println(lastClientIP);
    
    // Respuesta JSON
    String response = "{";
    response += "\"status\":\"ok\",";
    response += "\"led_state\":" + String(ledState ? "true" : "false") + ",";
    response += "\"command_count\":" + String(commandsReceived) + ",";
    response += "\"timestamp\":" + String(millis());
    response += "}";
    
    server.send(200, "application/json", response);
    
  } else {
    Serial.println("✗ Error: falta parámetro 'state'");
    server.send(400, "application/json", "{\"error\":\"missing state parameter (0 or 1)\"}");
  }
  
  Serial.println();
}

void handleSensorRequest() {
  Serial.println("=== SOLICITUD SENSOR PIR ===");
  
  checkMotion();
  motionRequests++;
  
  lastClientIP = server.client().remoteIP().toString();
  
  Serial.print("Estado movimiento: ");
  Serial.println(motionDetected ? "DETECTADO" : "NO DETECTADO");
  Serial.print("Cliente: ");
  Serial.println(lastClientIP);
  
  // Respuesta JSON con datos del sensor PIR
  String response = "{";
  response += "\"status\":\"ok\",";
  response += "\"motion_detected\":" + String(motionDetected ? "true" : "false") + ",";
  response += "\"led_state\":" + String(ledState ? "true" : "false") + ",";
  response += "\"request_count\":" + String(motionRequests) + ",";
  response += "\"timestamp\":" + String(millis());
  response += "}";
  
  server.send(200, "application/json", response);
  
  Serial.println();
}

void handleStatus() {
  Serial.println("Consultando estado del sistema...");
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Estado Servidor PIR+LED</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".card{background:white;padding:20px;border-radius:8px;margin:10px 0;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += ".value{font-size:2em;font-weight:bold;color:#007bff;}";
  html += ".label{color:#666;font-size:0.9em;}";
  html += ".button{display:inline-block;padding:10px 20px;margin:5px;border-radius:5px;cursor:pointer;text-decoration:none;}";
  html += ".on{background:#28a745;color:white;}";
  html += ".off{background:#dc3545;color:white;}";
  html += "</style>";
  // Agregar el mismo JavaScript que en handleRoot()
  html += "<script>";
  html += "function toggleDevice(device, state) {";
  html += "  fetch('/led?state=' + state)";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      location.reload();";
  html += "    });";
  html += "}";
  html += "function updateStatus() {";
  html += "  fetch('/sensor')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('motion').innerText = data.motion_detected ? 'DETECTADO' : 'NO DETECTADO';";
  html += "      document.getElementById('led').innerText = data.led_state ? 'ENCENDIDO' : 'APAGADO';";
  html += "    });";
  html += "}";
  html += "setInterval(updateStatus, 2000);";
  html += "</script>";
  html += "</head><body>";
  
  html += "<h1>🔧 Estado del Sistema</h1>";
  
  // Control del LED
  html += "<div class='card'>";
  html += "<div class='label'>Estado LED</div>";
  html += "<div class='value' id='led'>" + String(ledState ? "ENCENDIDO" : "APAGADO") + "</div>";
  html += "<div style='margin-top:10px'>";
  html += "<a href='#' class='button on' onclick='toggleDevice(\"led\", 1)'>Encender</a>";
  html += "<a href='#' class='button off' onclick='toggleDevice(\"led\", 0)'>Apagar</a>";
  html += "</div></div>";
  
  // Estado del sensor
  html += "<div class='card'>";
  html += "<div class='label'>Estado Movimiento</div>";
  html += "<div class='value' id='motion'>" + String(motionDetected ? "DETECTADO" : "NO DETECTADO") + "</div>";
  html += "</div>";
  
  // Estado del buzzer - NUEVO
  html += "<div class='card'>";
  html += "<div class='label'>Estado BUZZER</div>";
  html += "<div class='value' id='buzzer'>" + String(buzzerState ? "ENCENDIDO" : "APAGADO") + "</div>";
  html += "<div style='margin-top:10px'>";
  html += "<a href='#' class='button on' onclick='toggleDevice(\"buzzer\", 1)'>Encender</a>";
  html += "<a href='#' class='button off' onclick='toggleDevice(\"buzzer\", 0)'>Apagar</a>";
  html += "</div></div>";
  
  // Información adicional
  html += "<div class='card'>";
  html += "<div class='label'>Último Cliente</div>";
  html += "<div class='value'>" + lastClientIP + "</div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<div class='label'>Comandos Recibidos</div>";
  html += "<div class='value'>" + String(commandsReceived) + "</div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<div class='label'>Consultas Sensor</div>";
  html += "<div class='value'>" + String(motionRequests) + "</div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<div class='label'>IP Servidor</div>";
  html += "<div class='value'>" + WiFi.localIP().toString() + "</div>";
  html += "</div>";
  
  // Agregar sección de registros de movimiento
  html += "<div class='card'>";
  html += "<h2>Registros de Movimiento</h2>";
  html += "<table style='width:100%;border-collapse:collapse;'>";
  html += "<tr style='background:#f8f9fa'><th>Hora</th><th>Evento</th></tr>";
  
  for (int i = logCount - 1; i >= 0; i--) {
    unsigned long timestamp = movementLogs[i].timestamp;
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    minutes %= 60;
    seconds %= 60;
    
    html += "<tr style='border-bottom:1px solid #ddd'>";
    html += "<td style='padding:8px'>";
    html += String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
    html += "</td>";
    html += "<td style='padding:8px'>";
    html += movementLogs[i].isMotionStart ? "Inicio movimiento" : "Fin movimiento";
    html += "</td></tr>";
  }
  
  html += "</table>";
  html += "</div>";
  
  html += "<p><a href='/'>← Volver</a> | <a href='/status'>🔄 Actualizar</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Servidor PIR+LED Wemos D1</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".button{display:inline-block;padding:10px 20px;margin:5px;border-radius:5px;cursor:pointer;text-decoration:none;}";
  html += ".on{background:#28a745;color:white;}";
  html += ".off{background:#dc3545;color:white;}";
  html += ".card{background:white;padding:20px;border-radius:8px;margin:10px 0;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
  html += "</style>";
  html += "<script>";
  html += "function toggleDevice(device, state) {";
  html += "  fetch('/led?state=' + state)";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      location.reload();";
  html += "    });";
  html += "}";
  html += "function updateStatus() {";
  html += "  fetch('/sensor')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('motion').innerText = data.motion_detected ? 'DETECTADO' : 'NO DETECTADO';";
  html += "      document.getElementById('led').innerText = data.led_state ? 'ENCENDIDO' : 'APAGADO';";
  html += "    });";
  html += "}";
  html += "setInterval(updateStatus, 2000);"; // Actualizar cada 2 segundos
  html += "</script>";
  html += "</head><body>";
  
  html += "<h1>🔧 Servidor PIR+LED Wemos D1</h1>";
  
  // Card para control del LED
  html += "<div class='card'>";
  html += "<h2>Control de LED</h2>";
  html += "<p>Estado actual: <strong id='led'>" + String(ledState ? "ENCENDIDO" : "APAGADO") + "</strong></p>";
  html += "<a href='#' class='button on' onclick='toggleDevice(\"led\", 1)'>Encender</a>";
  html += "<a href='#' class='button off' onclick='toggleDevice(\"led\", 0)'>Apagar</a>";
  html += "</div>";
  
  // Card para sensor PIR
  html += "<div class='card'>";
  html += "<h2>Sensor de Movimiento</h2>";
  html += "<p>Estado actual: <strong id='motion'>" + String(motionDetected ? "DETECTADO" : "NO DETECTADO") + "</strong></p>";
  html += "</div>";
  
  // Información adicional
  html += "<div class='card'>";
  html += "<h2>Información del Sistema</h2>";
  html += "<p>IP del servidor: <strong>" + WiFi.localIP().toString() + "</strong></p>";
  html += "<p>Comandos recibidos: <strong>" + String(commandsReceived) + "</strong></p>";
  html += "<p>Consultas al sensor: <strong>" + String(motionRequests) + "</strong></p>";
  html += "</div>";
  
  // Enlaces
  html += "<p><a href='/status'>Ver Estado Detallado</a></p>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleNotFound() {
  String message = "Ruta no encontrada\n";
  message += "URI: " + server.uri() + "\n";
  message += "Método: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
  message += "Argumentos: " + String(server.args()) + "\n";
  
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/plain", message);
}

void checkTimeout() {
  static unsigned long lastCheck = 0;
  
  if (millis() - lastCheck > 10000) { // Verificar cada 10 segundos
    // Mostrar estadísticas periódicas
    if (commandsReceived > 0 || motionRequests > 0) {
      Serial.println("\n=== ESTADÍSTICAS ===");
      Serial.print("Comandos LED: ");
      Serial.println(commandsReceived);
      Serial.print("Consultas sensor: ");
      Serial.println(motionRequests);
      Serial.print("Movimiento actual: ");
      Serial.println(motionDetected ? "DETECTADO" : "NO DETECTADO");
      Serial.print("LED: ");
      Serial.println(ledState ? "ON" : "OFF");
      Serial.print("Memoria libre: ");
      Serial.println(ESP.getFreeHeap());
      Serial.println("===================\n");
    }
    
    lastCheck = millis();
  }
}

void handleBuzzerControl() {
    Serial.println("=== COMANDO BUZZER RECIBIDO ===");
    
    if (server.hasArg("state")) {
        String stateStr = server.arg("state");
        bool newState = (stateStr == "1" || stateStr.equalsIgnoreCase("true") || stateStr.equalsIgnoreCase("on"));
        
        buzzerState = newState;
        digitalWrite(buzzerPin, buzzerState);
        
        // Actualizar información
        lastCommandReceived = millis();
        lastClientIP = server.client().remoteIP().toString();
        commandsReceived++;
        
        Serial.print("BUZZER: ");
        Serial.println(buzzerState ? "ENCENDIDO" : "APAGADO");
        Serial.print("Cliente: ");
        Serial.println(lastClientIP);
        
        // Respuesta JSON
        String response = "{";
        response += "\"status\":\"ok\",";
        response += "\"buzzer_state\":" + String(buzzerState ? "true" : "false") + ",";
        response += "\"command_count\":" + String(commandsReceived) + ",";
        response += "\"timestamp\":" + String(millis());
        response += "}";
        
        server.send(200, "application/json", response);
        
    } else {
        Serial.println("✗ Error: falta parámetro 'state'");
        server.send(400, "application/json", "{\"error\":\"missing state parameter (0 or 1)\"}");
    }
    
    Serial.println();
}
