#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "hadahade";
const char* password = "levion1234";

// --- KONFIGURASI GITHUB OTA ---
// Gunakan link RAW (harus diawali raw.githubusercontent.com)
const String firmwareURL = "https://raw.githubusercontent.com/daeng3/OTA4LED/refs/heads/main/4ledOTA.ino.bin";

// Versi firmware saat ini (Tingkatkan angka ini setiap kali mau update)
const int currentVersion = 2;

// Pin LED
int leds[] = {18, 19, 21, 22};

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi LED
  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
  }

  // Koneksi WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("Current Firmware Version: ");
  Serial.println(currentVersion);

  // Jalankan pengecekan update saat pertama kali menyala
  check_for_update();
}

void loop() {
  // Efek Running LED (Indikator firmware aktif)
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(200);
    digitalWrite(leds[i], LOW);
  }
}

void check_for_update() {
  Serial.println("Checking for firmware updates...");
  
  WiFiClientSecure client;
  client.setInsecure(); // Mengabaikan validasi SSL agar mudah untuk testing

  // Proses update
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Update Gagal! Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Tidak ada update biner baru.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update Berhasil! Rebooting...");
      break;
  }
}