#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <BluetoothSerial.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pinout dari Cyber-Deck
#define VRX_PIN 34
#define VRY_PIN 35
#define SW_PIN 32

// Bluetooth Serial Instance
BluetoothSerial SerialBT;

// State Control
bool wifiActive = false;
bool btActive = false;

int selectedMenuItem = 0; // 0: WiFi Control, 1: Bluetooth Control
bool lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;

void renderUI(); // Forward declaration

void setup() {
 Wire.begin(21, 22);
 pinMode(SW_PIN, INPUT_PULLUP);

 display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 display.clearDisplay();

 // Mula-mula matikan WiFi & Bluetooth
 WiFi.mode(WIFI_OFF);

 renderUI();
}

void drawHeader() {
 display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
 display.setTextColor(SSD1306_BLACK);
 display.setCursor(8, 2);
 display.print("WIRELESS CONTROLLER");
 display.setTextColor(SSD1306_WHITE);
}

void renderUI() {
 display.clearDisplay();
 drawHeader();

 // --- ITEM 1: WIFI ---
 int yWiFi = 18;
 if (selectedMenuItem == 0) {
 display.fillRect(2, yWiFi - 2, 124, 20, SSD1306_WHITE);
 display.setTextColor(SSD1306_BLACK);
 } else {
 display.drawRect(2, yWiFi - 2, 124, 20, SSD1306_WHITE);
 display.setTextColor(SSD1306_WHITE);
 }
 display.setCursor(6, yWiFi + 4);
 display.print("WiFi AP : ");
 display.print(wifiActive ? "[ ON ]" : "[ OFF ]");

 // --- ITEM 2: BLUETOOTH ---
 int yBT = 42;
 if (selectedMenuItem == 1) {
 display.fillRect(2, yBT - 2, 124, 20, SSD1306_WHITE);
 display.setTextColor(SSD1306_BLACK);
 } else {
 display.drawRect(2, yBT - 2, 124, 20, SSD1306_WHITE);
 display.setTextColor(SSD1306_WHITE);
 }
 display.setCursor(6, yBT + 4);
 display.print("Bluetooth: ");
 display.print(btActive ? "[ ON ]" : "[ OFF ]");

 display.display();
}

void toggleWiFi() {
 wifiActive = !wifiActive;
 if (wifiActive) {
 WiFi.mode(WIFI_AP);
 WiFi.softAP("ESP32_CyberDeck", "12345678"); // SSID & Password
 } else {
 WiFi.softAPdisconnect(true);
 WiFi.mode(WIFI_OFF);
 }
}

void toggleBluetooth() {
 btActive = !btActive;
 if (btActive) {
 SerialBT.begin("ESP32_CyberDeck_BT"); // Nama Device Bluetooth
 } else {
 SerialBT.end();
 }
}

void loop() {
 int rawY = analogRead(VRY_PIN);
 bool btnState = digitalRead(SW_PIN);

 // Navigasi Menu menggunakan Joystick Y Axis
 if (rawY < 1000) {
 if (selectedMenuItem != 0) {
 selectedMenuItem = 0;
 renderUI();
 delay(200);
 }
 } else if (rawY > 3000) {
 if (selectedMenuItem != 1) {
 selectedMenuItem = 1;
 renderUI();
 delay(200);
 }
 }

 // Handle Klik Tombol (Debounce)
 if (btnState == LOW && lastBtnState == HIGH && (millis() - lastDebounceTime > 200)) {
 lastDebounceTime = millis();

 if (selectedMenuItem == 0) {
 toggleWiFi();
 } else if (selectedMenuItem == 1) {
 toggleBluetooth();
 }

 renderUI();
 }
 lastBtnState = btnState;

 delay(20);
}
