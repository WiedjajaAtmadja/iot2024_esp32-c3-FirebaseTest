#include <Arduino.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>

#define LED_GREEN 4
#define LED_RED 5
#define SWITCH_STATUS 9
#define DHT_PIN 6

DHTesp dht;

// Firebase data
#define FIREBASE_HOST "https://iot2024-a1310-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "X4NqPkKfDEArr6ig77ZwVFJmJN1ZptOS1eDYemqg"
FirebaseData fbdo;
FirebaseConfig fbConfig;
FirebaseData fbdoStream;

bool WiFi_Connect(int nRetry)
{
  WiFi.begin("Wokwi-GUEST", "");
  Serial.print("Connecting to WiFi.");

  for (int i=0; i<nRetry && (WiFi.status() != WL_CONNECTED); i++)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return (WiFi.status() == WL_CONNECTED);
}

void onTimerSendData()
{
  digitalWrite(LED_BUILTIN, HIGH);
  int nStatus = digitalRead(SWITCH_STATUS);
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  Firebase.RTDB.setFloat(&fbdo, "/data/temperature", temperature);
  Firebase.RTDB.setFloat(&fbdo, "/data/humidity", humidity);
  Firebase.RTDB.setInt(&fbdo, "/data/switchStatus", nStatus);
  Firebase.RTDB.setInt(&fbdo, "/data/currentMillis", millis());

  Serial.printf("[%7d] Temperature: %.1f C, Humidity: %.1f %%, SwitchStatus: %d \r\n",
     millis()/1000, temperature, humidity, nStatus);
  digitalWrite(LED_BUILTIN, LOW);
}

void onFirebaseStream(FirebaseStream data)
{
  Serial.printf("onFirebaseStream: %s %s %s %s\r\n", data.streamPath().c_str(),
              data.dataPath().c_str(), data.dataType().c_str(),
              data.stringData().c_str());

  if (data.dataPath() == "/" && data.dataType() == "json")
  {
    StaticJsonDocument<256> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc,  data.stringData());
    if (!error)
    {
      int nLedRed = jsonDoc["ledRed"];
      int nLedGreen = jsonDoc["ledGreen"];
      digitalWrite(LED_RED, nLedRed);
      digitalWrite(LED_GREEN, nLedGreen);
      Serial.printf("Received JSON data: ledRed=%d, ledGreen=%d\r\n", nLedRed, nLedGreen);
    }
    else
    {
      Serial.println("Failed to parse JSON data");
    }
  }
  else if (data.dataType() == "int")
  {
    if (data.dataPath() == "/ledRed")
      digitalWrite(LED_RED, data.intData());
    
    if (data.dataPath() == "/ledGreen")
      digitalWrite(LED_GREEN, data.intData());
  }
}

void Firebase_Init(const String& streamPath, int nMaxRetry=10)
{
  FirebaseAuth fbAuth;
  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;  
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(1024);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "small");
  for (int i=0; i<nMaxRetry && !Firebase.ready(); i++)
  {
    Serial.println("Connecting to Firebase...");
    delay(1000);
  }
  if (Firebase.ready())
  {
    Serial.println("Connected to Firebase");
    if (Firebase.RTDB.beginStream(&fbdoStream, streamPath))
    {
      Serial.println("Stream started");
      Firebase.RTDB.setStreamCallback(&fbdoStream, onFirebaseStream, 0);
    }
    else
    {
      Serial.println("Failed to start stream: " + fbdoStream.errorReason());
    }
  }
  else
  {
    Serial.println("Failed to connect to Firebase");
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(SWITCH_STATUS, INPUT_PULLUP);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.begin(115200);
  WiFi_Connect(7);
  Firebase_Init("/cmd");
}

int nCurrentMillis = 0;
void loop() {
  if (millis() - nCurrentMillis > 3000)
  {
    nCurrentMillis = millis();
    onTimerSendData();
  }
}

