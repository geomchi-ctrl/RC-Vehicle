#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <BluetoothSerial.h>

// Define as informações da rede Wi-Fi
const char* ssid = "rede";
const char* password = "senha";

// Define as informações do servidor MQTT do Losant
#define MQTT_SERVER      "broker.losant.com"
#define MQTT_PORT        1883 // Porta padrão para MQTT
#define MQTT_USERNAME    "access-key-from-losant"
#define MQTT_PASSWORD    "access-secret-from-losant"
#define MQTT_CLIENT_ID   "my-device-id"
// Pino do servo e ângulo inicial
const int servoPin = 2;
int servoInitialAngle = 90;

// Pino do motor
const int motorPin = 4;

// Crie um cliente MQTT
WiFiClient espClient;
Adafruit_MQTT_Client mqtt(&espClient, MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);

BluetoothSerial SerialBT;

bool isReconnecting = false;

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(servoPin, OUTPUT);

  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Conectando ao WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Conexão WiFi estabelecida");

  // Inicializa o Bluetooth Serial
  SerialBT.begin("VoltRacers");
  Serial.println("Bluetooth Serial iniciado");

  // Define o ângulo inicial do servo
  digitalWrite(servoPin, LOW);
  delay(1000);
  digitalWrite(servoPin, HIGH);
  delayMicroseconds(servoInitialAngle);
  digitalWrite(servoPin, LOW);
  Serial.println("Servo inicializado");
}

void loop() {
  // Controle do veículo
  if (SerialBT.available()) {
    char command = SerialBT.read();

    if (command == 'F') {
      digitalWrite(motorPin, HIGH);
      analogWrite(motorPin, 255);
    } else if (command == 'B') {
      digitalWrite(motorPin, LOW);
      analogWrite(motorPin, 255);
    } else if (command == 'S') {
      digitalWrite(motorPin, LOW);
      analogWrite(motorPin, 0);
    } else if (command == 'A') {
      if (servoInitialAngle < 180) {
        servoInitialAngle += 10;
        digitalWrite(servoPin, HIGH);
        delayMicroseconds(servoInitialAngle);
        digitalWrite(servoPin, LOW);
      }
    } else if (command == 'D') {
      if (servoInitialAngle > 0) {
        servoInitialAngle -= 10;
        digitalWrite(servoPin, HIGH);
        delayMicroseconds(servoInitialAngle);
        digitalWrite(servoPin, LOW);
      }
    }
  }

  // Verifica a conexão com o servidor MQTT do Losant
  if (!mqtt.connected()) {
    if (!isReconnecting) {
      Serial.println("Tentando reconectar ao MQTT do Losant...");
      isReconnecting = true;
    }
    reconnect();
  } else {
    isReconnecting = false;
  }

  // Verifica se há mensagens MQTT recebidas
  mqtt.processPackets(10000);
  mqtt.ping();

  // Aguarda um curto período de tempo
  delay(5000); // Intervalo de publicação em milissegundos
}

void reconnect() {
  // Reconecta ao servidor MQTT do Losant
  while (!mqtt.connected()) {
    Serial.println("Tentando reconectar ao MQTT do Losant...");

    if (mqtt.connect()) {
      Serial.println("Conectado ao servidor MQTT do Losant");
      // Assine os tópicos MQTT do Losant aqui, se necessário
    } else {
      Serial.print("Falha na conexão com o MQTT do Losant");
      Serial.println("Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}
