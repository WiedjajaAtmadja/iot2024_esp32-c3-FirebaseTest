#include <Arduino.h>
#include <WiFi.h>
#include <DHTesp.h>

#define LED_GREEN 4
#define LED_RED 5
#define SWITCH_STATUS 9
#define DHT_PIN 6

DHTesp dht;

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

void onTimer1Sec()
{
  digitalWrite(LED_BUILTIN, HIGH);
  int nStatus = digitalRead(SWITCH_STATUS);
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  Serial.printf("[%7d] Temperature: %.1f C, Humidity: %.1f %%, SwitchStatus: %d \r\n",
     millis()/1000, temperature, humidity, nStatus);
  digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(SWITCH_STATUS, INPUT_PULLUP);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.begin(115200);
  WiFi_Connect(7);
}

int nCurrentMillis = 0;
void loop() {
  if (millis() - nCurrentMillis > 1000)
  {
    nCurrentMillis = millis();
    onTimer1Sec();
  }
}

