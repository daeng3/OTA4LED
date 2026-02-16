#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "hadahade";
const char* password = "levion1234";

// --- KONFIGURASI GITHUB OTA ---
// Link Raw standar (Saya rapikan sedikit format linknya agar lebih kompatibel)
const String firmwareURL = "https://raw.githubusercontent.com/daeng3/OTA4LED/main/4ledOTA.ino.bin";

// Versi firmware ini (Pastikan angka ini sinkron dengan version.txt jika dipakai)
const int currentVersion = 3;

// Pin LED
int leds[] = {18, 19, 21, 22};

// Fungsi Callback untuk menampilkan Progress Download
void update_progress(int cur, int total) {
  Serial.printf("Download Progress: %d%%\r", (cur * 100) / total);
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi LED
  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW); // Pastikan mati dulu
  }

  // Koneksi WiFi dengan Timeout
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int timeout_counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout_counter++;
    if(timeout_counter > 20) { // Jika 10 detik tidak konek, restart
      Serial.println("\nWiFi Timeout! Restarting...");
      ESP.restart();
    }
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("Current Firmware Version: ");
  Serial.println(currentVersion);

  // Jalankan update
  check_for_update();
}

void loop() {
  // Efek LED (Akan jalan jika update gagal atau tidak ada update)
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(200);
    digitalWrite(leds[i], LOW);
  }
}

void check_for_update() {
  Serial.println("Checking for firmware updates...");
  
  WiFiClientSecure client;
  client.setInsecure(); // Abaikan sertifikat SSL (Penting untuk GitHub)
  client.setTimeout(12000); // Set timeout koneksi max 12 detik

  // --- SETTING CALLBACK PROGRESS ---
  // Ini yang membuat tidak terlihat stuck
  httpUpdate.onProgress(update_progress); 

  // Agar ESP32 tidak otomatis restart setelah update selesai (opsional, agar kita bisa lihat log sukses)
  httpUpdate.rebootOnUpdate(false); 

  // Lakukan Update
  // Note: httpUpdate akan otomatis mengikuti Redirect (302) dari GitHub
  t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("\nUpdate Gagal! Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("\nTidak ada update biner baru.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("\nUpdate Berhasil! Sedang Reboot...");
      delay(1000);
      ESP.restart();
      break;
  }
}