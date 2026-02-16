#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

const char* ssid = "hadahade";
const char* password = "levion1234";

// URL file biner di GitHub (Gunakan link "Raw")
String binURL = "https://raw.githubusercontent.com/username/repo/main/firmware.bin";

int leds[] = {18, 19, 21, 22};
int currentVersion = 1; // Ubah jadi 2 saat upload ke GitHub

void setup() {
  Serial.begin(115200);
  for(int i=0; i<4; i++) pinMode(leds[i], OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected. Version: " + String(currentVersion));
  check_for_update();
}

void loop() {
  // Efek Running LED sebagai indikator firmware aktif
  for(int i=0; i<4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(100);
    digitalWrite(leds[i], LOW);
  }
}

void check_for_update() {
  WiFiClientSecure client;
  client.setInsecure(); // Mengabaikan validasi SSL untuk kemudahan testing

  t_httpUpdate_return ret = httpUpdate.update(client, binURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Update Gagal: (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("Tidak ada update terbaru.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update Berhasil!");
      break;
  }
}void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
