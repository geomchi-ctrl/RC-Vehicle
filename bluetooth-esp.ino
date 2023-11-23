  #include <NimBLEDevice.h>
  #include <NimBLEServer.h>
  #include <NimBLEUtils.h>
  #include <NimBLEService.h>
  #include <NimBLECharacteristic.h>
  #include <WiFi.h>
  #include <Losant.h>

  #define motor1PWM 13 // PWM pin for Motor 1 (ENA) TROCAR PELO 26
  #define motor1Pin1 12 // Motor 1 Control Pin 1 (IN1)
  #define motor1Pin2 14 // Motor 1 Control Pin 2 (IN2)
  #define motor2PWM 25 // PWM pin for Motor 2 (ENB)
  #define motor2Pin1 27 // Motor 2 Control Pin 1 (IN3)
  #define motor2Pin2 26 // Motor 2 Control Pin 2 (IN4) TROCAR PELO 13

  BLEServer *pServer;
  BLECharacteristic *pCharacteristic;
  
  void performAction(char command);

  class MyCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 1) {
        Serial.println("Dados recebidos:");
        for (int i = 0; i < value.length(); i++) {
          Serial.print((int)value[i]);
          Serial.print(" ");
        }
        Serial.println();

        int command = (int)value[value.length() - 2];

        performAction(command);
      }
    }
  };

  const char *WIFI_SSID = "ary";
  const char *WIFI_PASS = "teste123";

  const char *LOSANT_DEVICE_ID = "6517216db8d4c26c1c51198a";
  const char *LOSANT_ACCESS_KEY = "44b37987-075d-4fa7-a682-eedec907f74b";
  const char *LOSANT_ACCESS_SECRET = "c055000dca531455c2678fcf962d2dec57b320064a5bde4f3a29caf492c9a6c2";

  WiFiClient wifiClient;
  LosantDevice device(LOSANT_DEVICE_ID);

  void setup() {
    Serial.begin(115200);

    NimBLEDevice::init("VoltRacers");
    pServer = NimBLEDevice::createServer();

    NimBLEService *pService = pServer->createService("19B10000-E8F2-537E-4F6C-D104768A1214");

    pCharacteristic = pService->createCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

    MyCallbacks *pCallbacks = new MyCallbacks();
    pCharacteristic->setCallbacks(pCallbacks);

    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->start();

    Serial.println("Dispositivo BLE iniciado e anunciando!");
    pinMode(motor1PWM, OUTPUT);
    pinMode(motor1Pin1, OUTPUT);
    pinMode(motor1Pin2, OUTPUT);
    pinMode(motor2PWM, OUTPUT);
    pinMode(motor2Pin1, OUTPUT);
    pinMode(motor2Pin2, OUTPUT);

    // Inicializar a conexão Wi-Fi e Losant
    connect();
  }

  void loop() {
    bool toReconnect = false;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Disconnected from WiFi");
      toReconnect = true;
    }

    if (!device.connected()) {
      Serial.println("Disconnected from MQTT");
      Serial.println(device.mqttClient.connected());
      toReconnect = true;
    }

    if (toReconnect) {
      connect();
    }

    device.loop();

    delay(1000); // Aguarda 1 segundo entre as iterações
  }

  void moveForward() {
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);
    analogWrite(motor1PWM, 140); // Velocidade máxima
    Serial.print("p frente");
    digitalWrite(motor2Pin1, HIGH);
    digitalWrite(motor2Pin2, LOW);
    analogWrite(motor2PWM, 140); // Velocidade máxima
  }

  void moveBackward() {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    analogWrite(motor1PWM, 255); // Velocidade máxima
    Serial.print("p tras");
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, HIGH);
    analogWrite(motor2PWM, 255); // Velocidade máxima
  }

  void moveLeft() {
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);
    analogWrite(motor1PWM, 150); // Velocidade reduzida para curva à esquerda
    Serial.print("p esquerda");
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, HIGH);
    analogWrite(motor2PWM, 150); // Velocidade reduzida para curva à esquerda
  }

  void moveRight() {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    analogWrite(motor1PWM, 150); // Velocidade reduzida para curva à direita
    Serial.print("p direita");
    digitalWrite(motor2Pin1, HIGH);
    digitalWrite(motor2Pin2, LOW);
    analogWrite(motor2PWM, 150); // Velocidade reduzida para curva à direita
  }

  void stopMotors() {
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    digitalWrite(motor2Pin1, LOW);
    digitalWrite(motor2Pin2, LOW);
    analogWrite(motor1PWM, 0);
    analogWrite(motor2PWM, 0);
  }

  void performAction(char command) {
    switch (command) {
      case 1: // Mover para frente
        moveForward();
        break;
      case 2: // Mover para trás
        moveBackward();
        break;
      case 4: // Mover para a esquerda
        moveLeft();
        break;
      case 8: // Mover para a direita
        moveRight();
        break;
      default:
        // Parar motores para qualquer outro comando
        stopMotors();
        break;
    }
  }

  void sendData(int data) {
    StaticJsonDocument<64> jsonDocument;
    jsonDocument["data"] = data;
    JsonObject jsonObject = jsonDocument.as<JsonObject>();
    device.sendState(jsonObject);
  }

  void connect() {
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println();
    Serial.println("Connecting to LOSANT...");
    device.connect(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

    while (!device.connected()) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("Connected!");
    Serial.println("This device is now ready for use!");
  }
