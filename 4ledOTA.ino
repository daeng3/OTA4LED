#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "hadahade";
const char* password = "levion1234";

// --- KONFIGURASI GITHUB OTA ---
const String repoRaw = "https://raw.githubusercontent.com/daeng3/OTA4LED/main/";
const String firmwareURL = repoRaw + "4ledOTA.ino.bin";
const String versionURL  = repoRaw + "version.txt";

// VERSI 6 (Pastikan update version.txt di GitHub jadi 6 juga)
const int currentVersion = 7;

// Pin LED
int leds[] = {18, 19, 21, 22};

void setup() {
  Serial.begin(115200);
  delay(1000); 

  // Inisialisasi LED
  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  Serial.println("\n--- BOOTING FIRMWARE V7 (PING-PONG) ---");
  Serial.print("Connecting to WiFi");
  
  WiFi.begin(ssid, password);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout++;
    if (timeout > 20) ESP.restart(); // Restart jika lama tidak konek
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("Device Version: ");
  Serial.println(currentVersion);

  // Cek update dengan logika anti-loop
  check_and_update();
}

void loop() {
  // --- POLA BARU: PING-PONG (Bolak Balik) ---
  
  // Gerak Maju (0 -> 1 -> 2 -> 3)
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(150);
    digitalWrite(leds[i], LOW);
  }

  // Gerak Mundur (2 -> 1) 
  // Kita tidak nyalakan 3 dan 0 lagi agar gerakan terlihat halus
  for(int i=2; i>0; i--) {
    digitalWrite(leds[i], HIGH);
    delay(150);
    digitalWrite(leds[i], LOW);
  }
}

// --- FUNGSI OTA (SMART CHECK) ---

int get_server_version() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  Serial.println("Checking version.txt on GitHub...");
  
  // Tambahkan header cache-control agar tidak mengambil versi lama dari cache server
  http.begin(client, versionURL);
  http.addHeader("Cache-Control", "no-cache");
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.trim();
    http.end();
    return payload.toInt();
  }
  http.end();
  return 0;
}

void update_progress(int cur, int total) {
  Serial.printf("Downloading V6: %d%%\r", (cur * 100) / total);
}

void check_and_update() {
  int serverVersion = get_server_version();
  Serial.print("Server Version: ");
  Serial.println(serverVersion);

  if (serverVersion > currentVersion) {
    Serial.println("New firmware found! Updating...");
    
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(12000);
    
    httpUpdate.onProgress(update_progress);
    httpUpdate.rebootOnUpdate(false);

    t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

    if (ret == HTTP_UPDATE_OK) {
      Serial.println("\nUpdate Success! Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.printf("\nUpdate Failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    }
  } else {
    Serial.println("Device is up to date.");
  }
}