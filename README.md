# ESP8266 Evil Twin Attack Tool

![Project Demo](demo.gif) *(Add actual demo GIF later)*

A portable WiFi penetration testing tool using ESP8266 that creates fake access points to capture credentials.

## Features

- 🕵️‍♂️ Evil Twin access point with captive portal
- 📝 Credential logging to SD card
- 📊 OLED menu interface with navigation buttons
- 📶 Real-time client monitoring
- ⚙️ Configurable settings (SSID, logging toggle)

## Hardware Requirements

| Component       | Connection       | Notes                          |
|----------------|------------------|--------------------------------|
| ESP8266        | -                | NodeMCU or Wemos D1 recommended|
| OLED (SSD1306) | SPI interface    | 128x64 resolution              |
| SD Card Module | SPI interface    | Shares bus with OLED           |
| Buttons        | 3x Tactile       | Up/Down/Select navigation      |
| Resistors      | 1x 10kΩ         | Only for D0 (GPIO16) button    |

**Pin Connections:**

| ESP8266 Pin | Connected To     |
|-------------|------------------|
| D0 (GPIO16) | BTN_DOWN + 10kΩ pull-up |
| D1 (GPIO5)  | BTN_SELECT       |
| D2 (GPIO4)  | BTN_UP           |
| D3 (GPIO0)  | SD_CS            |
| D4 (GPIO2)  | OLED_RESET       |
| D5 (GPIO14) | OLED_CLK + SD_CLK|
| D7 (GPIO13) | OLED_MOSI + SD_MOSI|
| D8 (GPIO15) | OLED_DC          |

## Installation

1. **Arduino IDE Setup**:
   ```bash
   # Install ESP8266 board package
   Board Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
Required Libraries:

Adafruit SSD1306

Adafruit GFX

DNSServer

SD (built-in)

Upload Code:

Select NodeMCU 1.0 board

Set upload speed to 115200

Compile and upload

Usage
Navigation:

UP/DOWN: Cycle through menu options

SELECT: Confirm selection

Menu Structure:

MAIN MENU
> Evil Twin
  > WiFi: ON/OFF
  > Mode: EVIL/NORMAL
  > Back
> View Clients
> View Logs
> Settings
  > Logging: ON/OFF
  > Back
Captured Data:

Credentials saved to /creds.txt on SD card

Format: timestamp,username,password

Ethical Considerations
⚠️ Warning: This tool is for:

Educational purposes only

Security research

Penetration testing with explicit permission

Unauthorized use may violate laws like:

Computer Fraud and Abuse Act (CFAA)

General Data Protection Regulation (GDPR)

Troubleshooting
Issue	Solution
OLED not displaying	Check GPIO15 (D8) initialization
SD card fails	Verify proper SPI connections
ESP won't boot	Remove physical pull-up on GPIO15
Menu navigation stuck	Check button debounce in code
Future Enhancements
Multiple AP profile support

WiFi network scanning

Data exfiltration via email/webhook

Battery level monitoring

License
MIT License - See LICENSE for details.