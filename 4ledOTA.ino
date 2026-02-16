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

// VERSI 8 (Jangan lupa update version.txt di GitHub jadi 8)
const int currentVersion = 8;

// Pin LED
int leds[] = {18, 19, 21, 22};

// --- TIMER SETTINGS ---
unsigned long lastCheckTime = 0;
// Cek setiap 20 detik (Sedikit diperlama agar tidak spamming)
const long checkInterval = 20000; 

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inisialisasi LED
  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  Serial.println("\n--- BOOTING FIRMWARE V8 (LOADING BAR) ---");
  connect_wifi();
  
  Serial.print("Current Device Version: ");
  Serial.println(currentVersion);
}

void loop() {
  // 1. JALANKAN ANIMASI LED (Pola Loading Bar)
  run_loading_animation();

  // 2. CEK UPDATE OTOMATIS (Non-Blocking)
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastCheckTime >= checkInterval) {
    lastCheckTime = currentMillis; 
    
    // Cek koneksi sebelum request ke GitHub
    if (WiFi.status() == WL_CONNECTED) {
      check_and_update();
    } else {
      Serial.println("WiFi Putus! Mencoba reconnect...");
      connect_wifi();
    }
  }
}

// --- FUNGSI ANIMASI V8 ---
void run_loading_animation() {
  // Nyalakan satu per satu (Menumpuk)
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(200); 
  }
  
  delay(500); // Tahan sebentar saat penuh

  // Matikan semua serentak
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], LOW);
  }
  delay(300); // Jeda sebelum mulai lagi
}

// --- FUNGSI WIFI & OTA ---

void connect_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout++;
    if (timeout > 10) break; 
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
  } else {
    Serial.println("\nWiFi Failed (Skip)");
  }
}

int get_server_version() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  Serial.print("[Auto-Check] Checking version.txt... ");
  
  http.begin(client, versionURL);
  http.addHeader("Cache-Control", "no-cache"); 
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.trim();
    int ver = payload.toInt();
    Serial.printf("Server: %d | Device: %d\n", ver, currentVersion);
    http.end();
    return ver;
  } else {
    Serial.printf("Error HTTP: %d\n", httpCode);
    http.end();
    return 0;
  }
}

void update_progress(int cur, int total) {
  if (cur % (total / 10) == 0) { 
    Serial.printf("Downloading V8: %d%%\n", (cur * 100) / total);
  }
}

void check_and_update() {
  int serverVersion = get_server_version();

  if (serverVersion > currentVersion) {
    Serial.println(">>> UPDATE DITEMUKAN! Memulai Download V8... <<<");
    
    // Matikan semua LED saat download tanda sedang sibuk
    for(int i=0; i<4; i++) digitalWrite(leds[i], LOW);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000); // Timeout agak lamaan dikit
    
    httpUpdate.onProgress(update_progress);
    httpUpdate.rebootOnUpdate(true); 

    t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

    if (ret == HTTP_UPDATE_FAILED) {
      Serial.printf("Update Gagal (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    }
  } else {
    // Tidak ada update, lanjut loop biasa
  }
}