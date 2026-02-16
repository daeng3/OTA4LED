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
const int currentVersion = 12;

// Pin LED
// Index 0=18, 1=19, 2=21, 3=22
int leds[] = {18, 19, 21, 22};

// Client Objects
WiFiClient espClient;
PubSubClient client(espClient);

// Flags Trigger
bool updateTriggered = false;
bool restartTriggered = false;

// --- VARIABEL TIMING UNTUK ANIMASI (MILLIS) ---
unsigned long previousMillisFlipFlop = 0;
unsigned long previousMillisStrobe = 0;
bool flipFlopState = false;
int strobeState = 0;

// --- FUNGSI CALLBACK MQTT ---
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  
  Serial.print("Pesan MQTT: ");
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  
  messageTemp.trim();
  messageTemp.toLowerCase();
  
  Serial.println(messageTemp);

  if (messageTemp == "restart" || messageTemp == "reboot") {
    Serial.println(">> Perintah RESTART diterima!");
    restartTriggered = true;
  }
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

// --- FUNGSI RESTART ---
void perform_restart() {
  Serial.println("Melakukan Restart dalam 3 detik...");
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

// --- FUNGSI ANIMASI BARU (STROBO & FLIPFLOP) ---
void run_custom_animation() {
  unsigned long currentMillis = millis();

  // --- BAGIAN 1: FLIP-FLOP (Pin 18 & 19) ---
  // Kecepatan Flip-Flop: 500ms (0.5 detik)
  if (currentMillis - previousMillisFlipFlop >= 500) {
    previousMillisFlipFlop = currentMillis;
    
    flipFlopState = !flipFlopState; // Tukar status

    if (flipFlopState) {
      digitalWrite(leds[0], HIGH); // Pin 18 ON
      digitalWrite(leds[1], LOW);  // Pin 19 OFF
    } else {
      digitalWrite(leds[0], LOW);  // Pin 18 OFF
      digitalWrite(leds[1], HIGH); // Pin 19 ON
    }
  }

  // --- BAGIAN 2: STROBO POLISI (Pin 21 & 22) ---
  // Kecepatan Strobo: 40ms (Sangat Cepat)
  if (currentMillis - previousMillisStrobe >= 40) {
    previousMillisStrobe = currentMillis;
    strobeState++;
    
    // Reset cycle setelah 12 step (biar ada jeda antar kedipan kiri/kanan)
    if (strobeState > 11) strobeState = 0;

    // Logika Kedip Polisi (3x Pin 21, lalu 3x Pin 22)
    switch (strobeState) {
      // Kedip Pin 21 (ON-OFF-ON-OFF-ON-OFF)
      case 0: digitalWrite(leds[2], HIGH); digitalWrite(leds[3], LOW); break;
      case 1: digitalWrite(leds[2], LOW);  digitalWrite(leds[3], LOW); break;
      case 2: digitalWrite(leds[2], HIGH); digitalWrite(leds[3], LOW); break;
      case 3: digitalWrite(leds[2], LOW);  digitalWrite(leds[3], LOW); break;
      case 4: digitalWrite(leds[2], HIGH); digitalWrite(leds[3], LOW); break;
      case 5: digitalWrite(leds[2], LOW);  digitalWrite(leds[3], LOW); break;

      // Kedip Pin 22 (ON-OFF-ON-OFF-ON-OFF)
      case 6: digitalWrite(leds[2], LOW); digitalWrite(leds[3], HIGH); break;
      case 7: digitalWrite(leds[2], LOW); digitalWrite(leds[3], LOW);  break;
      case 8: digitalWrite(leds[2], LOW); digitalWrite(leds[3], HIGH); break;
      case 9: digitalWrite(leds[2], LOW); digitalWrite(leds[3], LOW);  break;
      case 10: digitalWrite(leds[2],LOW); digitalWrite(leds[3], HIGH); break;
      case 11: digitalWrite(leds[2],LOW); digitalWrite(leds[3], LOW);  break;
    }
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);

  for(int i=0; i<4; i++) pinMode(leds[i], OUTPUT);

  Serial.println("\n--- BOOTING FIRMWARE CUSTOM STROBO/FLIPFLOP ---");
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
  else if (updateTriggered) {
    run_ota_update();
  }

  // Prioritas 3: Animasi Custom (Strobo & FlipFlop)
  else {
    run_custom_animation();
  }
}