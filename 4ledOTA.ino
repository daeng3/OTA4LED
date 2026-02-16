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
const String versionURL  = repoRaw + "version.txt"; // URL Gatekeeper

// VERSI 5 (Ubah ini jadi 5)
const int currentVersion = 5;

// Pin LED
int leds[] = {18, 19, 21, 22};

void setup() {
  Serial.begin(115200);
  delay(1000); 

  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  Serial.println("\n--- BOOTING FIRMWARE V5 (SMART CHECK) ---");
  Serial.print("Connecting to WiFi");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("Device Version: ");
  Serial.println(currentVersion);

  // Cek update dengan logika versi
  check_and_update();
}

void loop() {
  // Pola V5: Cepat (Running LED Fast)
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(100); // Lebih cepat dari V4
    digitalWrite(leds[i], LOW);
  }
}

// Fungsi untuk mengambil angka versi dari version.txt di GitHub
int get_server_version() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  Serial.println("Checking version.txt on GitHub...");
  
  if (http.begin(client, versionURL)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      payload.trim(); // Hapus spasi/enter yang tidak perlu
      http.end();
      return payload.toInt(); // Ubah text jadi angka
    } else {
      Serial.printf("Gagal ambil versi. HTTP Code: %d\n", httpCode);
    }
    http.end();
  }
  return 0; // Return 0 jika gagal konek
}

void update_progress(int cur, int total) {
  Serial.printf("Downloading: %d%%\r", (cur * 100) / total);
}

void check_and_update() {
  int serverVersion = get_server_version();
  Serial.print("Server Version: ");
  Serial.println(serverVersion);

  // --- LOGIKA UTAMA PENCEGAH LOOP ---
  if (serverVersion > currentVersion) {
    Serial.println("New version available! Starting update...");
    
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(12000);
    httpUpdate.onProgress(update_progress);
    httpUpdate.rebootOnUpdate(false);

    t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("Update Failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_OK:
        Serial.println("Update Success! Rebooting...");
        delay(1000);
        ESP.restart();
        break;
    }
  } else {
    Serial.println("----------------------------------");
    Serial.println("Device is up to date. No action.");
    Serial.println("----------------------------------");
  }
}