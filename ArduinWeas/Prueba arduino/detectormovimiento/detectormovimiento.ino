#define PIR_PIN D2      // Pin del sensor HC-SR501
#define BUZZER_PIN D8  // Pin del buzzer

void setup() {
    Serial.begin(9600);  // Inicializa la comunicación serial
    pinMode(PIR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Sistema de detección de movimiento iniciado");
}

void loop() {
    int movimiento = digitalRead(PIR_PIN);
    if (movimiento == HIGH) {
        Serial.println("¡Movimiento detectado!");
        digitalWrite(BUZZER_PIN, HIGH); // Enciende el buzzer
        delay(500);                     // Suena por 0.5 segundos
        digitalWrite(BUZZER_PIN, LOW);  // Apaga el buzzer
        Serial.println("Esperando siguiente detección...");
        delay(500);                     // Espera antes de volver a detectar
    } else {
        digitalWrite(BUZZER_PIN, LOW);  // Asegura que el buzzer esté apagado
    }
}
