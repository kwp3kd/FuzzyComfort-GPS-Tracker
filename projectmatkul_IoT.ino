/*
 * ============================================================
 *  ESP32 - Fuzzy Comfort + GPS Logger
 *  OLED 128x32 dengan tampilan bergantian tiap 5 detik:
 *    Halaman 1: Suhu & Kelembaban
 *    Halaman 2: Skor Fuzzy & Status
 *    Halaman 3: Latitude & Longitude
 * ============================================================
 *
 *  WIRING:
 *  DHT22  : VCC -> 3.3V, GND -> GND, DATA -> GPIO 4 (+ pull-up 10k)
 *  OLED   : SDA -> GPIO 21, SCL -> GPIO 22, VCC -> 3.3V, GND -> GND
 *  GPS    : TX  -> GPIO 16 (RX2 ESP32)
 *           RX  -> GPIO 17 (TX2 ESP32)
 *           VCC -> 3.3V/5V (sesuai modul), GND -> GND
 *
 *  Board IDE  : ESP32 Dev Module
 *  Serial Mon : 115200
 * ============================================================
 */

#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <TinyGPS++.h>

// ================== KONFIGURASI USER ==================
const char* WIFI_SSID     = "SESUAIKAN MILIK ANDA";        // <-- GANTI
const char* WIFI_PASSWORD = "SESUAIKAN MILIK ANDA";    // <-- GANTI

unsigned long myChannelNumber = SESUAIKAN MILIK ANDA;           // <-- GANTI Channel ID
const char*   myWriteAPIKey   = "SESUAIKAN MILIK ANDA"; // <-- GANTI Write API Key

// ================== PIN ==================
#define DHTPIN   4
#define DHTTYPE  DHT22
#define I2C_SDA  21
#define I2C_SCL  22
#define GPS_RX   16
#define GPS_TX   17

// ================== OLED 128x32 ==================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================== OBJEK ==================
DHT dht(DHTPIN, DHTTYPE);
TinyGPSPlus gps;
WiFiClient client;

// ================== VARIABEL ==================
float hum = 0, temp = 0, skor = 0;
String lat_data = "0", lng_data = "0";
String labelSkor = "N/A";

unsigned long prevDisplay  = 0;
unsigned long prevPage     = 0;
unsigned long prevTS       = 0;

const long INTERVAL_DISPLAY = 1000;  // update data OLED 1 detik
const long PAGE_DURATION    = 5000;  // ganti halaman tiap 5 detik
const long INTERVAL_TS      = 20000; // kirim ThingSpeak 20 detik

int currentPage = 0;                 // 0, 1, 2
const int TOTAL_PAGES = 3;

// ============================================================
//                  FUNGSI KEANGGOTAAN FUZZY
// ============================================================
float suhuDingin(float x){ if(x<=20) return 1; if(x>=25) return 0; return (25-x)/5.0; }
float suhuNyaman(float x){ if(x<=22 || x>=30) return 0; if(x<26) return (x-22)/4.0; return (30-x)/4.0; }
float suhuPanas (float x){ if(x<=28) return 0; if(x>=35) return 1; return (x-28)/7.0; }

float humKering(float x){ if(x<=30) return 1; if(x>=45) return 0; return (45-x)/15.0; }
float humNyaman(float x){ if(x<=40 || x>=70) return 0; if(x<55) return (x-40)/15.0; return (70-x)/15.0; }
float humLembab(float x){ if(x<=65) return 0; if(x>=85) return 1; return (x-65)/20.0; }

// ============================================================
//                  FUZZY INFERENCE (Mamdani)
// ============================================================
float fuzzyComfort(float t, float h) {
  float dT = suhuDingin(t), nT = suhuNyaman(t), pT = suhuPanas(t);
  float dH = humKering(h),  nH = humNyaman(h),  lH = humLembab(h);

  float a1 = min(dT, dH);
  float a2 = min(dT, nH);
  float a3 = min(dT, lH);
  float a4 = min(nT, dH);
  float a5 = min(nT, nH);
  float a6 = min(nT, lH);
  float a7 = min(pT, dH);
  float a8 = min(pT, nH);
  float a9 = min(pT, lH);

  float num = a1*50 + a2*65 + a3*40 + a4*70 + a5*100 +
              a6*55 + a7*45 + a8*50 + a9*25;
  float den = a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
  return (den == 0) ? 0 : num / den;
}

String labelDariSkor(float s) {
  if (s >= 80) return "NYAMAN";
  if (s >= 60) return "CUKUP";
  if (s >= 40) return "KURANG";
  return "BURUK";
}

// ============================================================
//              FUNGSI TAMPILAN OLED 128x32
// ============================================================
void drawPageIndicator() {
  // Pojok kanan atas: "1/3", "2/3", "3/3"
  display.setTextSize(1);
  String ind = String(currentPage + 1) + "/" + String(TOTAL_PAGES);
  int w = ind.length() * 6;                 // font 5x7 -> 6px per char
  display.setCursor(SCREEN_WIDTH - w, 0);
  display.print(ind);
}

void drawPage0() {
  // Halaman 1: Suhu & Kelembaban
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("SUHU & KELEMBABAN"));

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(temp, 1);
  display.print(F(" C"));

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print(F("Hum: "));
  display.print(hum, 1);
  display.print(F(" %"));
}

void drawPage1() {
  // Halaman 2: Skor Fuzzy & Status
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("SKOR KENYAMANAN"));

  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(skor, 1);

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print(F("Status: "));
  display.print(labelSkor);
}

void drawPage2() {
  // Halaman 3: Latitude & Longitude
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("GPS LOKASI"));

  display.setTextSize(1);
  display.setCursor(0, 10);
  display.print(F("Lat: "));
  display.print(lat_data);

  display.setCursor(0, 20);
  display.print(F("Lng: "));
  display.print(lng_data);

  // Indikator GPS valid / belum
  display.setCursor(0, 30);
  if (gps.location.isValid() && gps.location.age() < 5000) {
    display.print(F("GPS: OK"));
  } else {
    display.print(F("GPS: menunggu..."));
  }
}

void renderOLED() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (currentPage) {
    case 0: drawPage0(); break;
    case 1: drawPage1(); break;
    case 2: drawPage2(); break;
  }

  drawPageIndicator();
  display.display();
}

// ============================================================
//                        SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED gagal init, cek alamat I2C (0x3C/0x3D)"));
    for (;;) delay(1000);
  }

  // Splash screen
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10, 5);  display.println(F("Fuzzy Comfort"));
  display.setCursor(10, 18); display.println(F("GPS Logger"));
  display.display();
  delay(1500);

  dht.begin();
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting WiFi"));

  display.clearDisplay();
  display.setCursor(0, 5);  display.println(F("Connecting WiFi..."));
  display.display();

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();

  display.clearDisplay();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    display.setCursor(0, 5);  display.println(F("WiFi OK"));
    display.setCursor(0, 18); display.print(WiFi.localIP());
  } else {
    Serial.println(F("WiFi gagal, jalan offline"));
    display.setCursor(0, 5);  display.println(F("WiFi gagal"));
    display.setCursor(0, 18); display.println(F("Mode offline"));
  }
  display.display();
  delay(2000);

  ThingSpeak.begin(client);
  Serial.println(F("Sistem siap."));
}

// ============================================================
//                         LOOP
// ============================================================
void loop() {
  // --- Baca GPS ---
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read()) && gps.location.isValid()) {
      lat_data = String(gps.location.lat(), 6);
      lng_data = String(gps.location.lng(), 6);
    }
  }

  unsigned long now = millis();

  // --- Baca DHT + hitung fuzzy tiap 1 detik ---
  if (now - prevDisplay >= INTERVAL_DISPLAY) {
    prevDisplay = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      hum = h;
      temp = t;
      skor = fuzzyComfort(temp, hum);
      labelSkor = labelDariSkor(skor);
    }
  }

  // --- Ganti halaman tiap 5 detik ---
  if (now - prevPage >= PAGE_DURATION) {
    prevPage = now;
    currentPage = (currentPage + 1) % TOTAL_PAGES;
  }

  // --- Render OLED tiap 1 detik (atau saat halaman berubah) ---
  static unsigned long lastRender = 0;
  if (now - lastRender >= INTERVAL_DISPLAY) {
    lastRender = now;
    renderOLED();
  }

  // --- Kirim ThingSpeak tiap 20 detik ---
  if (now - prevTS >= INTERVAL_TS) {
    prevTS = now;

    if (WiFi.status() == WL_CONNECTED) {
      ThingSpeak.setField(1, lat_data);
      ThingSpeak.setField(2, lng_data);
      ThingSpeak.setField(3, hum);
      ThingSpeak.setField(4, temp);
      ThingSpeak.setField(5, skor);

      int code = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      Serial.printf("ThingSpeak: %d\n", code);
    } else {
      Serial.println(F("WiFi terputus, skip upload"));
    }
  }
}
