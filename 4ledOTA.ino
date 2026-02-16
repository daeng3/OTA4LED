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

// VERSI 11 (Update versi)
const int currentVersion = 11;

// Pin LED
int leds[] = {18, 19, 21, 22};

// Client Objects
WiFiClient espClient;
PubSubClient client(espClient);

// Flags Trigger
bool updateTriggered = false;
bool restartTriggered = false; // Flag baru untuk restart

// --- FUNGSI CALLBACK MQTT ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  
  Serial.print("Pesan MQTT: ");
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  
  // Bersihkan spasi/enter dan ubah ke huruf kecil semua agar tidak sensitif case
  messageTemp.trim();
  messageTemp.toLowerCase();
  
  Serial.println(messageTemp);

  // LOGIKA 1: CEK APAKAH PERINTAH RESTART?
  if (messageTemp == "restart" || messageTemp == "reboot") {
    Serial.println(">> Perintah RESTART diterima!");
    restartTriggered = true; // Aktifkan flag restart
  }
  // LOGIKA 2: CEK APAKAH ANGKA VERSI BARU?
  else {
    int incomingVersion = messageTemp.toInt();
    if (incomingVersion > currentVersion) {
      Serial.printf(">> Versi Baru (%d) Diterima! OTW Update...\n", incomingVersion);
      updateTriggered = true;
    } else {
      Serial.println(">> Pesan bukan perintah restart & versi tidak lebih baru. Diabaikan.");
    }
  }
}

// --- FUNGSI UPDATE OTA ---
void run_ota_update() {
  Serial.println("Memulai Download OTA...");
  for(int i=0; i<4; i++) digitalWrite(leds[i], LOW); // Matikan LED

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

// --- FUNGSI RESTART ---
void perform_restart() {
  Serial.println("Melakukan Restart dalam 3 detik...");
  
  // Beri tanda visual (Kedip cepat 5x) sebelum mati
  for(int i=0; i<5; i++) {
    for(int j=0; j<4; j++) digitalWrite(leds[j], HIGH);
    delay(100);
    for(int j=0; j<4; j++) digitalWrite(leds[j], LOW);
    delay(100);
  }
  
  ESP.restart();
}

// --- FUNGSI KONEKSI ---
void setup_wifi() {
  if(WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" OK!");
}

void reconnect_mqtt() {
  if (!client.connected()) {
    String clientId = "ESP32_4LED_Client";
    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT Connected!");
      client.subscribe(mqtt_topic);
    } else {
      delay(5000);
    }
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);

  for(int i=0; i<4; i++) pinMode(leds[i], OUTPUT);

  Serial.println("\n--- BOOTING FIRMWARE V11 (MQTT RESTART) ---");
  Serial.printf("Versi: %d\n", currentVersion);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback); 
}

// --- LOOP ---
void loop() {
  if (WiFi.status() != WL_CONNECTED) setup_wifi();
  if (!client.connected()) reconnect_mqtt();
  client.loop(); 

  // Prioritas 1: Cek Restart
  if (restartTriggered) {
    perform_restart();
  }

  // Prioritas 2: Cek Update
  if (updateTriggered) {
    run_ota_update();
  }

  // Prioritas 3: Animasi Normal (Inward-Outward)
  if (!updateTriggered && !restartTriggered) {
    run_inward_outward_animation();
  }
}

void run_inward_outward_animation() {
  digitalWrite(leds[0], HIGH); digitalWrite(leds[3], HIGH);
  digitalWrite(leds[1], LOW); digitalWrite(leds[2], LOW);
  delay(300);
  digitalWrite(leds[0], LOW); digitalWrite(leds[3], LOW);
  digitalWrite(leds[1], HIGH); digitalWrite(leds[2], HIGH);
  delay(300);
}