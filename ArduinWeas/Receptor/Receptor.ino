#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Configuración WiFi
const char* ssid = "DoyleDog";
const char* password = "cris02894";

// IP del servidor (Arduino transmisor)
const char* serverIP = "192.168.1.XXX"; // Reemplaza con la IP correcta de tu servidor

// Variables de control
unsigned long lastTime = 0;
const long interval = 2000;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== RECEPTOR DE COMANDOS ===");
  Serial.println("Comandos disponibles:");
  Serial.println("1: Encender LED");
  Serial.println("0: Apagar LED");
  Serial.println("S: Consultar estado del sensor");
  Serial.println("T: Ver último registro de movimiento");
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if ((millis() - lastTime) > interval) {
    if (WiFi.status() == WL_CONNECTED) {
      if (Serial.available() > 0) {
        char command = Serial.read();
        WiFiClient client;
        HTTPClient http;
        String url = "http://" + String(serverIP);
        
        switch(command) {
          case '1':
            // Encender LED
            url += "/led?state=1";
            http.begin(client, url);
            if (http.GET() > 0) {
              Serial.println("Comando: LED ENCENDIDO");
            }
            http.end();
            break;
            
          case '0':
            // Apagar LED
            url += "/led?state=0";
            http.begin(client, url);
            if (http.GET() > 0) {
              Serial.println("Comando: LED APAGADO");
            }
            http.end();
            break;
            
          case 'S':
          case 's':
            // Consultar estado del sensor
            url += "/sensor";
            http.begin(client, url);
            if (int httpCode = http.GET() > 0) {
              String payload = http.getString();
              Serial.println("Estado del sensor:");
              Serial.println(payload);
            }
            http.end();
            break;
            
          case 'T':
          case 't':
            // Ver último registro de movimiento
            url += "/status";
            http.begin(client, url);
            if (int httpCode = http.GET() > 0) {
              String payload = http.getString();
              Serial.println("Últimos registros de movimiento:");
              Serial.println(payload);
            }
            http.end();
            break;
            
          default:
            if (command != '\n' && command != '\r') {
              Serial.println("\nComando no válido");
              Serial.println("Comandos disponibles:");
              Serial.println("1: Encender LED");
              Serial.println("0: Apagar LED");
              Serial.println("S: Consultar estado del sensor");
              Serial.println("T: Ver último registro de movimiento");
            }
            break;
        }
      }
    }
    lastTime = millis();
  }
}