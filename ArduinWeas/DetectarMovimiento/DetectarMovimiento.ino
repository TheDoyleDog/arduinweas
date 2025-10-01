// Pin D3 es GPIO14
// Pin D7 es GPIO13
const int ledPin = 14;      // D3
const int inputPin = 13;   // D7
int pirState = LOW;
int val = 0;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(inputPin, INPUT);
  Serial.begin(9600);
  
  // Mensaje inicial
  Serial.println("Iniciando sensor PIR...");
  delay(2000); // Dar tiempo al sensor para estabilizarse
}

void loop() {
  val = digitalRead(inputPin);
  
  if (val == HIGH) {            // Corregido = a ==
    digitalWrite(ledPin, HIGH);
    if (pirState == LOW) {
      Serial.println("Movimiento Detectado");
      pirState = HIGH;
    }
  }
  else {
    digitalWrite(ledPin, LOW);
    if (pirState == HIGH) {
      Serial.println("Movimiento Terminado");
      pirState = LOW;
    }
  }
  
  delay(100); // Pequeño delay para estabilidad
}
