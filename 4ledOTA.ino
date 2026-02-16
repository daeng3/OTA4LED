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

// VERSI 9 (Wajib update file version.txt di GitHub jadi angka 9)
const int currentVersion = 9;

// Pin LED
int leds[] = {18, 19, 21, 22};

// --- TIMER SETTINGS ---
unsigned long lastCheckTime = 0;
// Cek setiap 20 detik
const long checkInterval = 20000; 

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inisialisasi LED
  for(int i=0; i<4; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  Serial.println("\n--- BOOTING FIRMWARE V9 (POLICE STROBE) ---");
  connect_wifi();
  
  Serial.print("Current Device Version: ");
  Serial.println(currentVersion);
}

void loop() {
  // 1. JALANKAN ANIMASI LED (Pola Strobo)
  run_police_animation();

  // 2. CEK UPDATE OTOMATIS
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastCheckTime >= checkInterval) {
    lastCheckTime = currentMillis; 
    
    // Cek koneksi sebelum request
    if (WiFi.status() == WL_CONNECTED) {
      check_and_update();
    } else {
      Serial.println("WiFi terputus! Mencoba reconnect...");
      connect_wifi();
    }
  }
}

// --- FUNGSI ANIMASI V9 (STROBO) ---
void run_police_animation() {
  // KEDIP KIRI (LED 0 & 1) - 3 Kali Cepat
  for(int j=0; j<3; j++) {
    digitalWrite(leds[0], HIGH);
    digitalWrite(leds[1], HIGH);
    delay(40);
    digitalWrite(leds[0], LOW);
    digitalWrite(leds[1], LOW);
    delay(40);
  }
  
  // KEDIP KANAN (LED 2 & 3) - 3 Kali Cepat
  for(int j=0; j<3; j++) {
    digitalWrite(leds[2], HIGH);
    digitalWrite(leds[3], HIGH);
    delay(40);
    digitalWrite(leds[2], LOW);
    digitalWrite(leds[3], LOW);
    delay(40);
  }
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
  
  Serial.print("[Auto-Check V9] Checking version.txt... ");
  
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
    Serial.printf("Downloading V9: %d%%\n", (cur * 100) / total);
  }
}

void check_and_update() {
  int serverVersion = get_server_version();

  if (serverVersion > currentVersion) {
    Serial.println(">>> UPDATE BARU DITEMUKAN! Memulai Download V9... <<<");
    
    // Matikan semua LED saat download tanda sedang proses
    for(int i=0; i<4; i++) digitalWrite(leds[i], LOW);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000); 
    
    httpUpdate.onProgress(update_progress);
    httpUpdate.rebootOnUpdate(true); 

    t_httpUpdate_return ret = httpUpdate.update(client, firmwareURL);

    if (ret == HTTP_UPDATE_FAILED) {
      Serial.printf("Update Gagal (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    }
  }
}