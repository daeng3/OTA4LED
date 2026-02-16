#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "hadahade";
const char* password = "levion1234";

// --- KONFIGURASI GITHUB OTA ---
// Link Raw ke file .bin di GitHub
const String firmwareURL = "https://raw.githubusercontent.com/daeng3/OTA4LED/main/4ledOTA.ino.bin";

// VERSI 4: Ubah angka ini setiap kali membuat update baru
const int currentVersion = 4;

// Pin LED
int leds[] = {18, 19, 21, 22};

// Indikator visual progress bar di Serial Monitor
void update_progress(int cur, int total) {
  Serial.printf("Downloading: %d%%\r", (cur * 100) / total);
}

void setup() {
  Serial.begin(115200);
  delay(1000); 

  // Inisialisasi LED
  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  Serial.println("\n--- BOOTING FIRMWARE V4 (FLIP-FLOP) ---");
  Serial.print("Connecting to WiFi");
  
  WiFi.begin(ssid, password);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout++;
    // Restart jika 15 detik tidak konek (opsional, agar tidak hang)
    if (timeout > 30) ESP.restart();
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("Running Version: ");
  Serial.println(currentVersion);

  // Cek update OTA
  check_for_update();
}

void loop() {
  // --- POLA FLIP-FLOP (Baru di V4) ---
  // Kondisi 1: LED Ganjil Nyala, Genap Mati
  // O - X - O - X
  digitalWrite(leds[0], HIGH); // 18 ON
  digitalWrite(leds[1], LOW);  // 19 OFF
  digitalWrite(leds[2], HIGH); // 21 ON
  digitalWrite(leds[3], LOW);  // 22 OFF
  delay(500);

  // Kondisi 2: LED Ganjil Mati, Genap Nyala
  // X - O - X - O
  digitalWrite(leds[0], LOW);  // 18 OFF
  digitalWrite(leds[1], HIGH); // 19 ON
  digitalWrite(leds[2], LOW);  // 21 OFF
  digitalWrite(leds[3], HIGH); // 22 ON
  delay(500);
}

void check_for_update() {
  Serial.println("Checking for updates from GitHub...");
  
  WiFiClientSecure client;
  client.setInsecure(); // Melewati validasi SSL
  client.setTimeout(12000);

  // Pasang callback progress bar
  httpUpdate.onProgress(update_progress);
  
  // Mencegah reboot otomatis agar kita bisa lihat log suksesnya dulu
  httpUpdate.rebootOnUpdate(false);

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("\nUpdate Failed. Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("\nDevice is up to date!");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("\nUpdate Downloaded Successfully!");
      Serial.println("Rebooting to apply new firmware...");
      delay(1000);
      ESP.restart();
      break;
  }
}