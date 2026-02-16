#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <PubSubClient.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "hadahade";
const char* password = "levion1234";

// --- KONFIGURASI MQTT ---
const char* mqtt_server = "103.197.190.235";
const int mqtt_port     = 1883;
const char* mqtt_topic  = "levion/ota/4led";

// --- KONFIGURASI GITHUB OTA ---
const String firmwareURL = "https://raw.githubusercontent.com/daeng3/OTA4LED/main/4ledOTA.ino.bin";

// VERSI 10
const int currentVersion = 10;

// Pin LED
int leds[] = {18, 19, 21, 22};

// Client Objects
WiFiClient espClient;
PubSubClient client(espClient);

// Flag untuk Trigger Update
bool updateTriggered = false;

// --- FUNGSI CALLBACK MQTT ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  
  Serial.print("Pesan MQTT Masuk [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);

  int incomingVersion = messageTemp.toInt();
  
  if (incomingVersion > currentVersion) {
    Serial.printf("Versi Baru (%d) Diterima! Menjadwalkan Update...\n", incomingVersion);
    updateTriggered = true;
  } else {
    Serial.println("Versi pesan sama/lebih rendah. Diabaikan.");
  }
}

// --- FUNGSI UPDATE OTA ---
void run_ota_update() {
  Serial.println("\n>>> UPDATE DIPICU LEWAT MQTT! Memulai Download V10... <<<");
  
  for(int i=0; i<4; i++) digitalWrite(leds[i], LOW);

  WiFiClientSecure clientSecure;
  clientSecure.setInsecure(); 
  clientSecure.setTimeout(15000); 
  
  httpUpdate.onProgress([](int cur, int total) {
      if (cur % (total / 10) == 0) Serial.printf("Download: %d%%\n", (cur * 100) / total);
  });
  
  httpUpdate.rebootOnUpdate(true); 

  t_httpUpdate_return ret = httpUpdate.update(clientSecure, firmwareURL);

  if (ret == HTTP_UPDATE_FAILED) {
    Serial.printf("Update Gagal (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    updateTriggered = false; 
  }
}

// --- FUNGSI KONEKSI ---
void setup_wifi() {
  if(WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void reconnect_mqtt() {
  if (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32_4LED_Client";
    
    if (client.connect(clientId.c_str())) {
      Serial.println(" Connected!");
      client.subscribe(mqtt_topic);
      Serial.print("Subscribed to: ");
      Serial.println(mqtt_topic);
    } else {
      Serial.print(" Failed rc="); Serial.print(client.state());
      Serial.println(" try again in 5s");
      delay(5000);
    }
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);

  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  Serial.println("\n--- BOOTING FIRMWARE V10 (INWARD-OUTWARD) ---");
  Serial.print("Current Version: ");
  Serial.println(currentVersion);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback); 
}

// --- LOOP ---
void loop() {
  if (WiFi.status() != WL_CONNECTED) setup_wifi();
  if (!client.connected()) reconnect_mqtt();
  client.loop(); 

  if (updateTriggered) {
    run_ota_update();
  }

  if (!updateTriggered) {
    run_inward_outward_animation();
  }
}

// --- ANIMASI BARU V10 (INWARD - OUTWARD) ---
void run_inward_outward_animation() {
  // Nyala bagian LUAR (LED 0 dan 3)
  digitalWrite(leds[0], HIGH);
  digitalWrite(leds[1], LOW);
  digitalWrite(leds[2], LOW);
  digitalWrite(leds[3], HIGH);
  delay(300);

  // Nyala bagian TENGAH (LED 1 dan 2)
  digitalWrite(leds[0], LOW);
  digitalWrite(leds[1], HIGH);
  digitalWrite(leds[2], HIGH);
  digitalWrite(leds[3], LOW);
  delay(300);
}