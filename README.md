# ESP32 Smart PC Power Controller

A smart ESP32-based controller that sits inside your PC case and provides remote power control capabilities through WiFi, web interface, and Wake-on-LAN functionality.

## 🚀 Features

- **WiFi Captive Portal**: Easy initial setup without pre-configuration
- **Beautiful Web Interface**: Modern, responsive design with animations and visual feedback
- **Direct PC Control**: Connects directly to motherboard front panel header
- **Power State Tracking**: Monitors PC status via power LED and network ping
- **REST API**: Programmatic control via HTTP endpoints
- **Wake-on-LAN Support**: Detects magic packets and automatically powers on PC
- **No Relays Required**: Direct GPIO connection for reliable operation
- **Real-time Status**: LED indicators and web-based status monitoring
- **Enhanced UX**: Interactive buttons, loading states, and smooth animations
- **mDNS Support**: Access via `wol-esp.local` hostname for easy discovery
- **Debug Dashboard**: Comprehensive system monitoring and logging interface

## 🛠️ Hardware Requirements

- **ESP32 Development Board** (ESP32-WROOM-32 or similar)
- **4 Jumper Wires** for connections:
  - 1 wire: 5V from USB header to ESP32 VIN
  - 1 wire: GND from USB header to ESP32 GND
  - 1 wire: Power switch pin to ESP32 GPIO2
  - 1 wire: Power LED wire to ESP32 GPIO4
- **Small Enclosure** to mount in PC case (optional)
- **Status LEDs** for GPIO5 and GPIO15 (optional)

## 🔌 Pin Connections (4-Wire Configuration)

| ESP32 Pin | Connection | Description |
|-----------|------------|-------------|
| VIN       | 5V from USB header | Power supply from motherboard USB header |
| GND       | GND from USB header | Common ground from USB header |
| GPIO2     | Power Switch + | Connects to motherboard power switch pin (pulls LOW when needed) |
| GPIO4     | Power LED wire | Reads PC power LED voltage (5V=OFF, 3.3V=ON) |

**Note**: GPIO5 (Status LED) and GPIO15 (WOL LED) are optional and can be left unconnected.

## 📐 Wiring Diagram (4-Wire Configuration)

```
Motherboard USB Header          ESP32 Board
┌─────────────────┐            ┌─────────────────┐
│                 │            │                 │
│   USB +5V ─────┼────────────┼─── VIN         │
│                 │            │                 │
│   USB GND ─────┼────────────┼─── GND          │
│                 │            │                 │
└─────────────────┘            └─────────────────┘

Motherboard Front Panel Header  ESP32 Board
┌─────────────────────────────┐ ┌─────────────────┐
│                             │ │                 │
│  PWRBTN# (Power Switch +) ─┼─┼─── GPIO2       │
│                             │ │                 │
│  PWR_LED+ (Power LED +) ───┼─┼─── GPIO4       │
│                             │ │                 │
└─────────────────────────────┘ └─────────────────┘
```

**Key Points:**
- **Power**: ESP32 gets 5V and GND from motherboard USB header
- **Power Switch**: GPIO2 connects to power switch pin (pulls LOW when needed)
- **Power LED**: GPIO4 reads power LED voltage to determine PC state
- **No additional grounds needed** - USB header provides common ground
- **GPIO2 Behavior**: Pin is normally floating (INPUT), changes to OUTPUT only when pulling LOW

## 📦 Installation

### 1. Arduino IDE Setup

1. Install Arduino IDE (version 1.8.19 or later)
2. Add ESP32 board support:
   - Go to `File > Preferences`
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to `Tools > Board > Boards Manager`
   - Search for "ESP32" and install "ESP32 by Espressif Systems"

### 2. Install Required Libraries

Go to `Sketch > Include Library > Manage Libraries` and install:

- **WiFi** (usually included with ESP32)
- **WebServer** (usually included with ESP32)
- **DNSServer** (usually included with ESP32)
- **EEPROM** (usually included with ESP32)
- **ArduinoJson** by Benoit Blanchon
- **HTTPClient** (usually included with ESP32)
- **WiFiUdp** (usually included with ESP32)
- **ESPmDNS** (usually included with ESP32)
- **ESP32Ping** by David Antliff (for proper ICMP ping functionality)

### 3. Upload the Code

1. Connect ESP32 to your computer via USB
2. Select board: `Tools > Board > ESP32 Arduino > ESP32 Dev Module`
3. Select port: `Tools > Port > [Your ESP32 Port]`
4. Open `ESP32_Smart_PC_Controller.ino`
5. Click Upload button

## ⚙️ Initial Setup

### First Boot (WiFi Configuration)

1. **Power on the ESP32** (connect to motherboard USB header)
2. **Connect to WiFi**: The ESP32 will create a network called "ESP32-PC-Controller"
3. **Password**: Use `password123` to connect
4. **Captive Portal**: Open any web browser and navigate to any website
   - You should automatically see a "Click here to login" or similar message
   - If not, try visiting `http://192.168.4.1` directly
5. **Configuration Page**: You'll be redirected to the ESP32 configuration page
6. **Enter Details**:
   - **WiFi SSID**: Your home network name
   - **WiFi Password**: Your network password
   - **PC MAC Address**: Your PC's MAC address (e.g., AA:BB:CC:DD:EE:FF)
   - **PC IP Address**: Your PC's local IP address (e.g., 192.168.1.100)
7. **Save Configuration**: Click "Save Configuration"
8. **Restart**: The ESP32 will restart and connect to your WiFi

### Finding Your PC's MAC Address

**Windows:**
```cmd
ipconfig /all
```
Look for "Physical Address" under your network adapter.

**macOS/Linux:**
```bash
ifconfig
```
Look for "ether" followed by the MAC address.

### Finding Your PC's IP Address

**Windows:**
```cmd
ipconfig
```

**macOS/Linux:**
```bash
ifconfig
```

## 🧪 Testing

### Testing the Captive Portal

If the ESP32 is conncted visit it via its IP Adress or at http://wol-esp.local

If you're not seeing the captive portal login message, use these test files:

**Python Test Script:**
```bash
python test_captive_portal.py
```

**HTML Test Page:**
Open `test_captive_portal.html` in your browser after connecting to the ESP32 WiFi network.

**Manual Testing:**
1. Connect to "ESP32-PC-Controller" WiFi
2. Try visiting: `google.com`, `facebook.com`, or any website
3. You should be redirected to the ESP32 configuration page
4. If not working, visit `http://192.168.4.1` directly

**Visual Confirmation:**
- The captive portal should show a beautiful, modern interface
- All form fields should have proper validation and formatting
- Buttons should have smooth hover effects and animations

### Testing Wake-on-LAN

**Python Test Script:**
```bash
python test_wol.py AA:BB:CC:DD:EE:FF 192.168.4.1
```

**Manual Test via Web Interface:**
Visit `http://[ESP32_IP]/test_wol` to manually trigger WOL

**Status Check:**
Visit `http://[ESP32_IP]/status` to see WOL configuration and status

**LED Behavior Verification:**
- WOL LED should blink exactly 3 times when triggered
- Each blink cycle should be ~600ms (300ms ON + 300ms OFF)
- Total sequence should complete in ~1.5 seconds
- LED should stop blinking and remain OFF after sequence

## 💻 Usage

### Web Interface

Once configured, access the controller at `http://[ESP32_IP_ADDRESS]`
or at
`http://wol-esp.local`

The web interface provides:
- **Current PC Status**: Real-time power state display with animated indicators
- **Power On**: Simulates power button press with visual feedback
- **Shutdown**: Requests normal shutdown with confirmation
- **Force Off**: Holds power button for 5 seconds with safety warning
- **Configuration**: Access to settings with enhanced form validation
- **Debug Dashboard**: Comprehensive system monitoring and logging interface
- **Navigation Links**: Easy access between main page, configuration, and debug dashboard
- **Responsive Design**: Works perfectly on desktop, tablet, and mobile devices
- **Modern UI**: Beautiful gradients, animations, and interactive elements

### Access Methods

- **Direct IP**: `http://192.168.4.1` (AP mode) or `http://[ESP32_IP]` (WiFi mode)
- **mDNS Hostname**: `http://wol-esp.local` (works on most networks)
- **Captive Portal**: Automatically redirects when connecting to ESP32 AP

### Enhanced User Experience Features

#### Visual Feedback
- **Animated Status Indicators**: Pulsing animations for different PC states
- **Interactive Buttons**: Hover effects, loading states, and smooth transitions
- **Real-time Updates**: Status refreshes automatically every 2 seconds
- **Color-coded States**: Different colors for each power state (Off, Starting, On, Shutting Down)

#### Form Validation & UX
- **Input Formatting**: Automatic MAC address and IP address formatting
- **Real-time Validation**: Instant feedback on form inputs
- **Loading States**: Visual feedback during configuration saving
- **Success Messages**: Clear confirmation when settings are saved
- **Error Handling**: Graceful error handling with user-friendly messages

#### Responsive Design
- **Mobile-First**: Optimized for touch devices and small screens
- **Flexible Layout**: Adapts to different screen sizes automatically
- **Touch-Friendly**: Large buttons and inputs for mobile use
- **Modern Typography**: Clean, readable fonts with proper contrast

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Main control page |
| `/config` | GET | Configuration page |
| `/debug` | GET | Debug dashboard with system monitoring |
| `/on` | POST | Power on PC |
| `/off` | POST | Shutdown PC |
| `/forceoff` | POST | Force shutdown PC |
| `/status` | GET | Get current status (JSON) |
| `/save_config` | POST | Save configuration |
| `/get_config` | GET | Get current configuration |
| `/test_wol` | GET | Manually trigger WOL test |
| `/reset_wol_led` | GET | Reset WOL LED state (for debugging) |
| `/force_state` | GET | Force power state (for debugging) |
| `/force_on_if_ping` | GET | Force state to ON if ping succeeds |
| `/debug_pins` | GET | Get current pin states (for debugging) |
| `/get_logs` | GET | Get system activity logs (JSON) |
| `/test` | GET | Test endpoint for captive portal |

### Example API Usage

**Power on PC:**
```bash
curl -X POST http://[ESP32_IP]/on
```

**Get status:**
```bash
curl http://[ESP32_IP]/status
```

**Response:**
```json
{
  "state": "on",
  "stateText": "PC is On",
  "powerLed": 1,
  "wifi": true,
  "wolMac": "AA:BB:CC:DD:EE:FF",
  "wolEnabled": true,
  "wolLedBlinking": false,
  "wolBlinkCount": 0,
  "wolLedState": 0
}
```

## 🔋 Power States

The controller tracks 4 power states using a reliable combination of front panel LED and network ping:

1. **OFF**: Front panel LED is OFF AND ping doesn't return
2. **STARTING**: Front panel LED is ON/OFF AND ping doesn't return (WOL message received)
3. **ON**: Front panel LED is ON AND ping returns successfully
4. **SHUTTING DOWN**: Front panel LED is ON/OFF AND ping doesn't return

### State Transition Logic

- **OFF → STARTING**: When LED turns ON but ping still fails
- **STARTING → ON**: When LED is ON and ping succeeds
- **STARTING → OFF**: When LED turns OFF and ping fails (startup failure)
- **ON → SHUTTING DOWN**: When LED turns OFF or ping fails
- **SHUTTING DOWN → OFF**: When LED is OFF and ping fails

### Safety Features

- **Startup Timeout**: If stuck in STARTING for 1+ minute, forces to final state (ON or OFF)
- **Shutdown Timeout**: If stuck in SHUTTING_DOWN for 1+ minute, forces to OFF
- **Ping Timeout**: If ON but ping fails for 1+ minute, assumes shutdown
- **Real-time Monitoring**: State updates every 10 seconds with ping checks
- **Maximum Transition Time**: Power states never remain in "powering on" or "powering off" for more than 1 minute

## 🌐 Wake-on-LAN Support

The ESP32 can detect Wake-on-LAN magic packets and automatically power on your PC. This is useful for:

- Remote desktop applications (Moonlight, Parsec)
- Network management tools
- Home automation systems
- Remote power management
- Gaming applications (Steam Remote Play, etc.)
- Server management and monitoring

### How WOL Works

1. **Magic Packet Structure**: WOL packets contain 6 bytes of 0xFF followed by 16 repetitions of the target MAC address
2. **UDP Port 9**: ESP32 listens on UDP port 9 for WOL packets
3. **MAC Address Matching**: Only packets targeting the configured PC MAC address will trigger power-on
4. **LED Indication**: WOL LED blinks exactly 3 times when a valid magic packet is detected
5. **Automatic Power-On**: PC automatically powers on when valid WOL packet is received
6. **Serial Logging**: All WOL events are logged to Serial Monitor for debugging

### WOL Requirements

- **PC Configuration**: PC must have Wake-on-LAN enabled in BIOS/UEFI
- **Network Support**: Network must allow UDP broadcast packets on port 9
- **MAC Address**: Correct PC MAC address must be configured in ESP32
- **Power State**: PC must be in sleep/hibernate state (not fully powered off)

**Note**: Some networks may block broadcast packets or UDP port 9 for security reasons.

## 🚨 Troubleshooting

### Common Issues

**ESP32 won't connect to WiFi:**
- Check SSID and password
- Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check signal strength

**Power button not working:**
- Verify wiring connections
- Check that GPIO2 is properly connected
- Ensure motherboard supports front panel power switch

**Web interface not accessible:**
- Check if ESP32 is connected to WiFi
- Verify IP address in Serial Monitor
- Try accessing `http://wol-esp.local` (mDNS hostname)
- Check firewall settings
- Verify mDNS is working: `ping wol-esp.local`

**Captive portal not working:**
- Ensure you're connected to "ESP32-PC-Controller" WiFi network
- Try visiting `http://192.168.4.1` directly
- Check Serial Monitor for DNS server messages
- Some devices may require manual navigation to the ESP32 IP
- Try clearing browser cache and cookies

**PC status not updating:**
- Verify power LED connection
- Check PC IP address configuration
- Ensure PC is on same network

**Wake-on-LAN not working:**
- Verify MAC address is correctly configured in ESP32
- Check if PC has Wake-on-LAN enabled in BIOS/UEFI
- Ensure PC is in sleep/hibernate state (not fully powered off)
- Check if network allows UDP broadcast packets on port 9
- Verify ESP32 is connected to the same network as the WOL sender
- Check Serial Monitor for "WOL magic packet detected" messages
- Try the manual WOL test at `/test_wol` endpoint

**WOL LED not blinking or stuck:**
- Check Serial Monitor for WOL LED debug messages
- Use `/reset_wol_led` endpoint to reset LED state
- Verify WOL LED is connected to GPIO15
- Check `/status` endpoint for `wolBlinkCount` and `wolLedState` values
- LED should blink exactly 3 times for each WOL packet received
- If LED gets stuck, it will automatically reset after 10 seconds

**Power state stuck or incorrect:**
- Use `/force_state?state=off` to force OFF state
- Use `/force_state?state=on` to force ON state
- Use `/force_state?state=starting` to force STARTING state
- Use `/force_state?state=shutting` to force SHUTTING_DOWN state
- Check `/status` endpoint for `pingResult` and `timeInState` values
- States automatically timeout after 1 minute maximum

**Debug and Logging Issues:**
- Check `/debug` page for comprehensive system monitoring
- Use `/get_logs` endpoint to get JSON format of activity logs
- Logs show last 20 events with timestamps and categories
- Clear logs manually using the "Clear Logs" button on debug page
- Logs include WOL detection, power changes, state transitions, and ping results

### Serial Monitor Debugging

Connect to the ESP32 via USB and open Serial Monitor (115200 baud) to see debug information:

```
ESP32 Smart PC Power Controller Starting...
Starting AP mode...
AP SSID: ESP32-PC-Controller
AP IP address: 192.168.4.1
Captive portal should now be accessible
Web server started
DNS server started for captive portal
UDP socket started on port 9 for WOL detection
mDNS responder started: wol-esp.local
Setup complete!
```

## ⚠️ Safety Notes

- **Always power off PC before making connections**
- **Double-check wiring before powering on**
- **Use appropriate wire gauge for connections**
- **Ensure proper grounding**
- **Test with multimeter if unsure about connections**

## 🔧 Advanced Configuration

### Customizing Pin Assignments

Edit the pin definitions at the top of the code:

```cpp
#define POWER_SWITCH_PIN 2      // Change to desired GPIO
#define POWER_LED_PIN 4         // Change to desired GPIO
#define STATUS_LED_PIN 5        // Change to desired GPIO
#define WOL_LED_PIN 15          // Change to desired GPIO
```

### Debug Dashboard

The debug dashboard (`/debug`) provides comprehensive system monitoring and real-time logging:

#### **Real-time System Monitoring**
- **System Status**: Current power state, LED status, ping results, WiFi status, uptime
- **Wake-on-LAN Info**: MAC address, enabled status, LED blinking state, blink count
- **Network Information**: AP IP, SSID, target PC IP, ping intervals, last ping time
- **State Machine**: Time in current state, last state change, state change count, ping success/fail counts

#### **Activity Logging System**
- **Live Event Log**: Tracks last 20 system events with timestamps
- **Event Categories**: 
  - `wol`: Wake-on-LAN packet detection and actions
  - `power`: Manual power button presses and power requests
  - `state`: Power state transitions and changes
  - `ping`: Network connectivity test results
  - `error`: System errors and failures
- **Auto-refresh**: Updates every 2 seconds with toggle option
- **Log Management**: Clear logs button and persistent storage
- **Mobile Responsive**: Works perfectly on all devices

#### **Access Methods**
- **Web Interface**: Navigate to `/debug` from any page
- **Direct URL**: `http://[ESP32_IP]/debug` or `http://wol-esp.local/debug`
- **Navigation Links**: Available from main page and configuration page

### Web Interface Customization

The HTML interface can be customized by modifying the embedded HTML strings:
- **Main Page**: Edit `html_index` variable for power control interface
- **Config Page**: Edit `html_config` variable for configuration interface
- **Debug Page**: Edit the debug endpoint HTML for monitoring interface
- **Styling**: Modify CSS within the `<style>` tags for custom appearance
- **Animations**: Adjust timing and effects in CSS animations

### Logging System Configuration

The built-in logging system tracks system events automatically:
- **Log Storage**: Last 20 events stored in memory with timestamps
- **Event Categories**: WOL, power, state, ping, error, and info events
- **API Access**: `/get_logs` endpoint provides JSON format for external tools
- **Serial Output**: All logs also printed to Serial Monitor for debugging
- **Custom Events**: Add custom logging with `addLogEntry(message, type)` function

### Modifying Timeouts

Adjust timing values as needed:

```cpp
#define PING_TIMEOUT 1000       // Ping timeout in milliseconds
#define DEBOUNCE_DELAY 50       // Button debounce delay
#define WOL_PORT 9              // WOL UDP port (standard is 9)
```

### Adding Custom Features

The modular code structure makes it easy to add:
- Additional power control methods
- Custom status indicators
- Integration with home automation systems
- Email/SMS notifications
- Logging and analytics
- Custom web pages and endpoints
- Additional LED indicators
- Sensor integration (temperature, humidity, etc.)
- MQTT support for IoT integration

## 📁 Project Files

- **`ESP32_Smart_PC_Controller.ino`** - Main Arduino sketch with debug dashboard and logging
- **`test_captive_portal.py`** - Python script to test captive portal
- **`test_captive_portal.html`** - Browser-based test interface
- **`test_wol.py`** - Python script to test Wake-on-LAN functionality
- **`README.md`** - This documentation file
- **`WIRING_DIAGRAM.md`** - Detailed wiring instructions

### New Features Added
- **Debug Dashboard**: Comprehensive system monitoring at `/debug`
- **Activity Logging**: Real-time event tracking with 20-event history
- **mDNS Support**: Access via `wol-esp.local` hostname
- **Enhanced Navigation**: Links between all interface pages
- **Logging API**: `/get_logs` endpoint for external tool integration

## 🔍 Testing Tools

### Captive Portal Testing
- **`test_captive_portal.py`**: Tests all captive portal endpoints
- **`test_captive_portal.html`**: Browser-based testing interface

### Wake-on-LAN Testing
- **`test_wol.py`**: Sends WOL magic packets for testing
- **`/test_wol` endpoint**: Manual WOL trigger via web interface
- **`/status` endpoint**: Shows WOL configuration and status

### Web Interface Testing
- **Main Interface**: Test power control buttons and status updates
- **Configuration Page**: Verify form validation and submission
- **Responsive Design**: Test on different screen sizes and devices
- **Animation Performance**: Ensure smooth animations on target devices

## 📚 Additional Resources

- **ESP32 Documentation**: https://docs.espressif.com/projects/esp-idf/
- **Arduino ESP32**: https://github.com/espressif/arduino-esp32
- **Wake-on-LAN Specification**: https://en.wikipedia.org/wiki/Wake-on-LAN
- **Modern CSS Techniques**: https://developer.mozilla.org/en-US/docs/Web/CSS
- **Responsive Web Design**: https://developer.mozilla.org/en-US/docs/Learn/CSS/CSS_layout/Responsive_Design
- **Web Animations**: https://developer.mozilla.org/en-US/docs/Web/API/Web_Animations_API

## 📄 License

This project is open source and available under the MIT License.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit:
- Issues and bug reports
- Feature requests
- Pull requests
- Documentation improvements

## 🆘 Support

If you encounter issues or have questions:

1. **Check the troubleshooting section** above
2. **Review the code comments** in the Arduino sketch
3. **Check the Serial Monitor output** for debug information
4. **Use the provided test tools** to diagnose issues
5. **Open an issue** on the project repository

## 🎯 Quick Start Checklist

- [ ] Upload code to ESP32
- [ ] Connect to "ESP32-PC-Controller" WiFi network
- [ ] Configure WiFi credentials and PC information
- [ ] Test captive portal functionality
- [ ] Test mDNS access via `wol-esp.local`
- [ ] Test Wake-on-LAN functionality
- [ ] Verify power control operations
- [ ] Check debug dashboard at `/debug`
- [ ] Verify activity logging system
- [ ] Test navigation between all pages
- [ ] Check Serial Monitor for any errors
- [ ] Test web interface responsiveness
- [ ] Verify WOL LED blinking behavior (exactly 3 blinks)
- [ ] Test configuration form validation
- [ ] Verify smooth animations and transitions

---

**Happy Building! 🚀**

*Built with ❤️ using ESP32 and Arduino*
