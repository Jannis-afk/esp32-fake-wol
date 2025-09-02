/*
 * ESP32 Smart PC Power Controller
 * 
 * Features:
 * - Wi-Fi captive portal for configuration
 * - Web server with power control UI
 * - Wake-on-LAN magic packet sniffer
 * - Direct connection to PC front panel header
 * - Power state tracking via LED and ping
 * - REST API endpoints
 */

 #include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ESP32Ping.h>
 
 // Pin definitions
 #define POWER_SWITCH_PIN 2      // GPIO2 - connects to PC power switch
 #define POWER_LED_PIN 4         // GPIO4 - reads PC power LED state
 #define STATUS_LED_PIN 5        // GPIO5 - ESP32 status indicator
 #define WOL_LED_PIN 15          // GPIO15 - blinks when WoL packet detected
 
 // Configuration
 #define EEPROM_SIZE 512
 #define CONFIG_MAGIC 0x12345678
 #define DNS_PORT 53
 #define WEB_PORT 80
 #define WOL_PORT 9
 #define PING_TIMEOUT 1000
 #define DEBOUNCE_DELAY 50
 
 // WiFi AP Configuration (Easy to change)
 #define AP_SSID "ESP32-PC-Controller"
 #define AP_PASSWORD "password123"
#define MDNS_HOSTNAME "wol-esp"
 
 // Power states
 enum PowerState {
   PC_OFF = 0,
   PC_STARTING = 1,
   PC_ON = 2,
   PC_SHUTTING_DOWN = 3
 };
 
 // Configuration structure
 struct Config {
   uint32_t magic;
   char wifi_ssid[32];
   char wifi_password[64];
   char pc_mac[18];
   char pc_ip[16];
   bool configured;
 };
 
 // Global variables
WebServer webServer(WEB_PORT);
 DNSServer dnsServer;
WiFiUDP udp;
 Config config;
 PowerState currentState = PC_OFF;
 bool lastPowerLedState = false;
 unsigned long lastStateChange = 0;
 unsigned long lastPingTime = 0;
 bool apMode = false;
 bool wolLedBlinking = false;
 unsigned long lastWolBlink = 0;
int wolBlinkCount = 0;

// Simple logging system
struct LogEntry {
  unsigned long timestamp;
  String message;
  String type;
};

#define MAX_LOG_ENTRIES 20
LogEntry logEntries[MAX_LOG_ENTRIES];
int logIndex = 0;

// Track recent events for state determination
unsigned long lastWolTime = 0;
unsigned long lastStartButtonTime = 0;
unsigned long lastShutdownButtonTime = 0;
 
 // HTML pages
 const char* html_index = R"rawliteral(
 <!DOCTYPE html>
 <html>
 <head>
     <title>PC Power Controller</title>
     <meta name="viewport" content="width=device-width, initial-scale=1">
     <style>
          * { margin: 0; padding: 0; box-sizing: border-box; }
          
          body { 
              font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
              background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
              min-height: 100vh;
              padding: 20px;
          }
          
          .container { 
              max-width: 800px; 
              margin: 0 auto; 
              background: rgba(255, 255, 255, 0.95);
              backdrop-filter: blur(10px);
              padding: 40px; 
              border-radius: 20px; 
              box-shadow: 0 20px 40px rgba(0,0,0,0.1);
              border: 1px solid rgba(255, 255, 255, 0.2);
          }
          
          .header {
              text-align: center;
              margin-bottom: 40px;
          }
          
          .header h1 {
              font-size: 2.5em;
              color: #2c3e50;
              margin-bottom: 10px;
              font-weight: 300;
          }
          
          .header .subtitle {
              color: #7f8c8d;
              font-size: 1.1em;
          }
          
          .status-container {
              margin-bottom: 40px;
          }
          
          .status { 
              padding: 25px; 
              margin: 20px 0; 
              border-radius: 15px; 
              text-align: center; 
              font-weight: 600;
              font-size: 1.2em;
              box-shadow: 0 5px 15px rgba(0,0,0,0.1);
              transition: all 0.3s ease;
              position: relative;
              overflow: hidden;
          }
          
          .status::before {
              content: '';
              position: absolute;
              top: 0;
              left: 0;
              right: 0;
              height: 4px;
              background: currentColor;
          }
          
          .status.off { 
              background: linear-gradient(135deg, #ff6b6b, #ee5a52); 
              color: white;
              animation: pulse 2s infinite;
          }
          
          .status.starting { 
              background: linear-gradient(135deg, #f39c12, #e67e22); 
              color: white;
              animation: pulse 1s infinite;
          }
          
          .status.on { 
              background: linear-gradient(135deg, #2ecc71, #27ae60); 
              color: white;
          }
          
          .status.shutting { 
              background: linear-gradient(135deg, #e67e22, #d35400); 
              color: white;
              animation: pulse 1.5s infinite;
          }
          
          @keyframes pulse {
              0%, 100% { transform: scale(1); }
              50% { transform: scale(1.02); }
          }
          
          .controls {
              display: grid;
              grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
              gap: 20px;
              margin-bottom: 40px;
          }
          
          .button { 
              background: linear-gradient(135deg, #3498db, #2980b9); 
              color: white; 
              border: none; 
              padding: 18px 30px; 
              border-radius: 12px; 
              cursor: pointer; 
              font-size: 16px; 
              font-weight: 600;
              transition: all 0.3s ease;
              box-shadow: 0 5px 15px rgba(52, 152, 219, 0.3);
              position: relative;
              overflow: hidden;
          }
          
          .button::before {
              content: '';
              position: absolute;
              top: 0;
              left: -100%;
              width: 100%;
              height: 100%;
              background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
              transition: left 0.5s;
          }
          
          .button:hover::before {
              left: 100%;
          }
          
          .button:hover { 
              transform: translateY(-3px);
              box-shadow: 0 8px 25px rgba(52, 152, 219, 0.4);
          }
          
          .button:active {
              transform: translateY(-1px);
          }
          
          .button.danger { 
              background: linear-gradient(135deg, #e74c3c, #c0392b);
              box-shadow: 0 5px 15px rgba(231, 76, 60, 0.3);
          }
          
          .button.danger:hover {
              box-shadow: 0 8px 25px rgba(231, 76, 60, 0.4);
          }
          
          .button.success { 
              background: linear-gradient(135deg, #2ecc71, #27ae60);
              box-shadow: 0 5px 15px rgba(46, 204, 113, 0.3);
          }
          
          .button.success:hover {
              box-shadow: 0 8px 25px rgba(46, 204, 113, 0.4);
          }
          
          .config-link { 
              text-align: center; 
              margin-top: 30px;
              padding: 20px;
              background: rgba(52, 152, 219, 0.1);
              border-radius: 12px;
              border: 2px dashed rgba(52, 152, 219, 0.3);
          }
          
          .config-link a { 
              color: #3498db; 
              text-decoration: none; 
              font-weight: 600;
              font-size: 1.1em;
              padding: 12px 24px;
              border-radius: 8px;
              transition: all 0.3s ease;
              display: inline-block;
          }
          
          .config-link a:hover {
              background: rgba(52, 152, 219, 0.1);
              transform: translateY(-2px);
          }
          
          .status-indicator {
              display: inline-block;
              width: 12px;
              height: 12px;
              border-radius: 50%;
              margin-right: 10px;
              animation: blink 1s infinite;
          }
          
          @keyframes blink {
              0%, 50% { opacity: 1; }
              51%, 100% { opacity: 0.3; }
          }
          
          .status.off .status-indicator { background: #ff6b6b; }
          .status.starting .status-indicator { background: #f39c12; }
          .status.on .status-indicator { background: #2ecc71; }
          .status.shutting .status-indicator { background: #e67e22; }
          
          @media (max-width: 768px) {
              .container { padding: 20px; margin: 10px; }
              .controls { grid-template-columns: 1fr; }
              .header h1 { font-size: 2em; }
          }
     </style>
 </head>
 <body>
     <div class="container">
          <div class="header">
              <h1> PC Power Controller</h1>
              <div class="subtitle">Smart remote control for your computer</div>
          </div>
          
          <div class="status-container">
              <div id="status" class="status off">
                  <span class="status-indicator"></span>
                  Status: Unknown
              </div>
          </div>
          
          <div class="controls">
              <button class="button success" onclick="powerOn()">
                   Power On
              </button>
              <button class="button" onclick="shutdown()">
                   Shutdown
              </button>
              <button class="button danger" onclick="forceOff()">
                   Force Off
              </button>
         </div>
         
         <div class="config-link">
              <a href="/config"> Configuration Settings</a>
              <a href="/debug"> Debug Dashboard</a>
         </div>
     </div>
     
     <script>
         function updateStatus() {
             fetch('/status')
                 .then(response => response.json())
                 .then(data => {
                     const statusDiv = document.getElementById('status');
                     statusDiv.className = 'status ' + data.state;
                      statusDiv.innerHTML = '<span class="status-indicator"></span>Status: ' + data.stateText;
                  })
                  .catch(error => {
                      console.error('Error updating status:', error);
                 });
         }
         
         function powerOn() {
              const button = event.target;
              button.disabled = true;
              button.textContent = ' Powering On...';
              
              fetch('/on', {method: 'POST'})
                  .then(() => {
                      setTimeout(() => {
                          button.disabled = false;
                          button.textContent = ' Power On';
                          updateStatus();
                      }, 1000);
                  });
         }
         
         function shutdown() {
              const button = event.target;
              button.disabled = true;
              button.textContent = ' Shutting Down...';
              
              fetch('/off', {method: 'POST'})
                  .then(() => {
                      setTimeout(() => {
                          button.disabled = false;
                          button.textContent = ' Shutdown';
                          updateStatus();
                      }, 1000);
                  });
         }
         
         function forceOff() {
              if(confirm(' Are you sure you want to force shutdown?\n\nThis will immediately cut power to your PC!')) {
                  const button = event.target;
                  button.disabled = true;
                  button.textContent = ' Force Shutting Down...';
                  
                  fetch('/forceoff', {method: 'POST'})
                      .then(() => {
                          setTimeout(() => {
                              button.disabled = false;
                              button.textContent = ' Force Off';
                              updateStatus();
                          }, 1000);
                      });
             }
         }
         
         // Update status every 2 seconds
         setInterval(updateStatus, 2000);
         updateStatus();
     </script>
 </body>
 </html>
 )rawliteral";
 
 const char* html_config = R"rawliteral(
 <!DOCTYPE html>
 <html>
 <head>
     <title>Configuration - PC Power Controller</title>
     <meta name="viewport" content="width=device-width, initial-scale=1">
     <style>
          * { margin: 0; padding: 0; box-sizing: border-box; }
          
          body { 
              font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
              background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
              min-height: 100vh;
              padding: 20px;
          }
          
          .container { 
              max-width: 700px; 
              margin: 0 auto; 
              background: rgba(255, 255, 255, 0.95);
              backdrop-filter: blur(10px);
              padding: 40px; 
              border-radius: 20px; 
              box-shadow: 0 20px 40px rgba(0,0,0,0.1);
              border: 1px solid rgba(255, 255, 255, 0.2);
          }
          
          .header {
              text-align: center;
              margin-bottom: 40px;
          }
          
          .header h1 {
              font-size: 2.5em;
              color: #2c3e50;
              margin-bottom: 10px;
              font-weight: 300;
          }
          
          .header .subtitle {
              color: #7f8c8d;
              font-size: 1.1em;
          }
          
          .form-group { 
              margin-bottom: 25px; 
              position: relative;
          }
          
          label { 
              display: block; 
              margin-bottom: 8px; 
              font-weight: 600;
              color: #2c3e50;
              font-size: 1.1em;
          }
          
          .input-wrapper {
              position: relative;
              transition: all 0.3s ease;
          }
          
          .input-wrapper::before {
              content: '';
              position: absolute;
              bottom: 0;
              left: 0;
              width: 0;
              height: 2px;
              background: linear-gradient(135deg, #3498db, #2980b9);
              transition: width 0.3s ease;
          }
          
          .input-wrapper:focus-within::before {
              width: 100%;
          }
          
          input[type="text"], input[type="password"] { 
              width: 100%; 
              padding: 15px 20px; 
              border: 2px solid #e0e0e0; 
              border-radius: 12px; 
              box-sizing: border-box;
              font-size: 16px;
              transition: all 0.3s ease;
              background: rgba(255, 255, 255, 0.9);
              color: #2c3e50;
          }
          
          input[type="text"]:focus, input[type="password"]:focus { 
              outline: none;
              border-color: #3498db;
              background: white;
              box-shadow: 0 5px 15px rgba(52, 152, 219, 0.2);
              transform: translateY(-2px);
          }
          
          input::placeholder {
              color: #bdc3c7;
              font-style: italic;
          }
          
          .button { 
              background: linear-gradient(135deg, #3498db, #2980b9); 
              color: white; 
              border: none; 
              padding: 18px 40px; 
              border-radius: 12px; 
              cursor: pointer; 
              font-size: 18px; 
              font-weight: 600;
              transition: all 0.3s ease;
              box-shadow: 0 5px 15px rgba(52, 152, 219, 0.3);
              position: relative;
              overflow: hidden;
              width: 100%;
              margin-top: 20px;
          }
          
          .button::before {
              content: '';
              position: absolute;
              top: 0;
              left: -100%;
              width: 100%;
              height: 100%;
              background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
              transition: left 0.5s;
          }
          
          .button:hover::before {
              left: 100%;
          }
          
          .button:hover { 
              transform: translateY(-3px);
              box-shadow: 0 8px 25px rgba(52, 152, 219, 0.4);
          }
          
          .button:active {
              transform: translateY(-1px);
          }
          
          .button:disabled {
              background: #bdc3c7;
              cursor: not-allowed;
              transform: none;
              box-shadow: none;
          }
          
          .back-link { 
              text-align: center; 
              margin-top: 30px;
              padding: 20px;
              background: rgba(52, 152, 219, 0.1);
              border-radius: 12px;
              border: 2px dashed rgba(52, 152, 219, 0.3);
          }
          
          .back-link a { 
              color: #3498db; 
              text-decoration: none; 
              font-weight: 600;
              font-size: 1.1em;
              padding: 12px 24px;
              border-radius: 8px;
              transition: all 0.3s ease;
              display: inline-block;
              margin: 0 10px;
          }
          
          .back-link a:hover {
              background: rgba(52, 152, 219, 0.1);
              transform: translateY(-2px);
          }
          
          .field-icon {
              position: absolute;
              right: 15px;
              top: 50%;
              transform: translateY(-50%);
              color: #7f8c8d;
              font-size: 18px;
          }
          
          .success-message {
              background: linear-gradient(135deg, #2ecc71, #27ae60);
              color: white;
              padding: 20px;
              border-radius: 12px;
              text-align: center;
              margin: 20px 0;
              font-weight: 600;
              animation: slideIn 0.5s ease;
          }
          
          @keyframes slideIn {
              from { opacity: 0; transform: translateY(-20px); }
              to { opacity: 1; transform: translateY(0); }
          }
          
          .loading {
              display: inline-block;
              width: 20px;
              height: 20px;
              border: 3px solid rgba(255,255,255,.3);
              border-radius: 50%;
              border-top-color: #fff;
              animation: spin 1s ease-in-out infinite;
              margin-right: 10px;
          }
          
          @keyframes spin {
              to { transform: rotate(360deg); }
          }
          
          @media (max-width: 768px) {
              .container { padding: 20px; margin: 10px; }
              .header h1 { font-size: 2em; }
              input[type="text"], input[type="password"] { padding: 12px 15px; }
          }
     </style>
 </head>
 <body>
     <div class="container">
          <div class="header">
              <h1> Configuration</h1>
              <div class="subtitle">Setup your PC Power Controller</div>
          </div>
         
         <form id="configForm">
             <div class="form-group">
                  <label for="ssid"> Wi-Fi Network Name (SSID)</label>
                  <div class="input-wrapper">
                      <input type="text" id="ssid" name="ssid" placeholder="Enter your WiFi network name" required>
                      <span class="field-icon"> </span>
                  </div>
             </div>
             
             <div class="form-group">
                  <label for="password"> Wi-Fi Password</label>
                  <div class="input-wrapper">
                      <input type="password" id="password" name="password" placeholder="Enter your WiFi password" required>
                      <span class="field-icon"> </span>
                  </div>
             </div>
             
             <div class="form-group">
                  <label for="pc_mac"> PC MAC Address</label>
                  <div class="input-wrapper">
                 <input type="text" id="pc_mac" name="pc_mac" placeholder="AA:BB:CC:DD:EE:FF" required>
                      <span class="field-icon"> </span>
                  </div>
             </div>
             
             <div class="form-group">
                  <label for="pc_ip"> PC IP Address</label>
                  <div class="input-wrapper">
                 <input type="text" id="pc_ip" name="pc_ip" placeholder="192.168.1.100" required>
                      <span class="field-icon"> </span>
             </div>
             </div>
              
              <button type="submit" class="button" id="submitBtn">
                   Save Configuration
              </button>
         </form>
         
         <div class="back-link">
              <a href="/"> Back to Main Control</a>
              <a href="/debug"> Debug Dashboard</a>
         </div>
     </div>
     
     <script>
         document.getElementById('configForm').onsubmit = function(e) {
             e.preventDefault();
              
              const submitBtn = document.getElementById('submitBtn');
              const originalText = submitBtn.innerHTML;
              
              // Show loading state
              submitBtn.disabled = true;
              submitBtn.innerHTML = '<span class="loading"></span>Saving Configuration...';
             
             const formData = new FormData();
             formData.append('ssid', document.getElementById('ssid').value);
             formData.append('password', document.getElementById('password').value);
             formData.append('pc_mac', document.getElementById('pc_mac').value);
             formData.append('pc_ip', document.getElementById('pc_ip').value);
             
             fetch('/save_config', {
                 method: 'POST',
                 body: formData
             })
             .then(response => response.text())
             .then(data => {
                  // Show success message
                  const successDiv = document.createElement('div');
                  successDiv.className = 'success-message';
                  successDiv.innerHTML = ' Configuration saved successfully! ESP32 will restart in 2 seconds...';
                  
                  const form = document.getElementById('configForm');
                  form.parentNode.insertBefore(successDiv, form.nextSibling);
                  
                  // Redirect after 2 seconds
                  setTimeout(() => {
                      window.location.href = '/';
                  }, 2000);
              })
              .catch(error => {
                  console.error('Error saving configuration:', error);
                  submitBtn.disabled = false;
                  submitBtn.innerHTML = originalText;
                  alert(' Error saving configuration. Please try again.');
             });
         };
         
         // Load current config
         fetch('/get_config')
             .then(response => response.json())
             .then(data => {
                 if(data.ssid) document.getElementById('ssid').value = data.ssid;
                 if(data.pc_mac) document.getElementById('pc_mac').value = data.pc_mac;
                 if(data.pc_ip) document.getElementById('pc_ip').value = data.pc_ip;
              })
              .catch(error => {
                  console.error('Error loading configuration:', error);
              });
          
          // Add input validation and formatting
          document.getElementById('pc_mac').addEventListener('input', function(e) {
              let value = e.target.value.replace(/[^A-Fa-f0-9]/g, '');
              if (value.length > 12) value = value.substr(0, 12);
              
              let formatted = '';
              for (let i = 0; i < value.length; i++) {
                  if (i > 0 && i % 2 === 0) formatted += ':';
                  formatted += value[i];
              }
              e.target.value = formatted.toUpperCase();
          });
          
          document.getElementById('pc_ip').addEventListener('input', function(e) {
              let value = e.target.value.replace(/[^0-9.]/g, '');
              let parts = value.split('.');
              if (parts.length > 4) parts = parts.slice(0, 4);
              
              e.target.value = parts.join('.');
             });
     </script>
 </body>
 </html>
 )rawliteral";
 
 void setup() {
   Serial.begin(115200);
   Serial.println("ESP32 Smart PC Power Controller Starting...");
   
        // Initialize pins
     pinMode(POWER_SWITCH_PIN, OUTPUT);
     pinMode(POWER_LED_PIN, INPUT); // No pull-up - let external circuit control the level
     pinMode(STATUS_LED_PIN, OUTPUT);
     pinMode(WOL_LED_PIN, OUTPUT);
     digitalWrite(POWER_SWITCH_PIN, HIGH); // Normally high, pull low to "press"
     digitalWrite(STATUS_LED_PIN, LOW);
     digitalWrite(WOL_LED_PIN, LOW);
     
     // Debug power LED state
     Serial.printf("Initial Power LED state: %s\n", digitalRead(POWER_LED_PIN) ? "HIGH" : "LOW");
   
   // Initialize EEPROM
   EEPROM.begin(EEPROM_SIZE);
   loadConfig();
   
   // Start status LED blink
   blinkStatusLed();
   
   if (config.configured && strlen(config.wifi_ssid) > 0) {
     // Try to connect to saved WiFi
     connectToWiFi();
   }
   
   if (WiFi.status() != WL_CONNECTED) {
     // Start AP mode
     startAPMode();
   }
   
   // Setup web server
   setupWebServer();
   
   // Start DNS server for captive portal
   dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
   Serial.println("DNS server started for captive portal");
   
   // Initialize UDP socket for WOL detection
   if (udp.begin(WOL_PORT)) {
     Serial.printf("UDP socket started on port %d for WOL detection\n", WOL_PORT);
   } else {
     Serial.println("Failed to start UDP socket for WOL detection");
   }
   
   // Initialize mDNS
   if (MDNS.begin(MDNS_HOSTNAME)) {
     Serial.printf("mDNS responder started: %s.local\n", MDNS_HOSTNAME);
     MDNS.addService("http", "tcp", 80);
     MDNS.addService("wol", "udp", 9);
   } else {
     Serial.println("Error setting up mDNS responder");
   }
   
   Serial.println("Setup complete!");
 }
 
 void loop() {
   // Handle DNS requests (for captive portal)
   if (apMode) {
     dnsServer.processNextRequest();
   }
   
   // Handle web server
   webServer.handleClient();
   
   // Update power state
   updatePowerState();
   
   // Check for WoL magic packets
   checkForMagicPackets();
   
   // Blink status LED
   static unsigned long lastBlink = 0;
   if (millis() - lastBlink > 1000) {
     blinkStatusLed();
     lastBlink = millis();
   }
   
   // Simple watchdog - print status every 10 seconds
   static unsigned long lastWatchdog = 0;
   if (millis() - lastWatchdog > 10000) {
     Serial.printf("Watchdog: State=%s, PowerLED=%s, WiFi=%s\n", 
                   getStateString(currentState).c_str(),
                   digitalRead(POWER_LED_PIN) ? "HIGH" : "LOW",
                   WiFi.status() == WL_CONNECTED ? "Connected" : "AP Mode");
     lastWatchdog = millis();
   }
   
   delay(10);
 }
 
 void addLogEntry(String message, String type) {
  logEntries[logIndex].timestamp = millis();
  logEntries[logIndex].message = message;
  logEntries[logIndex].type = type;
  
  logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;
  
  // Also print to Serial for debugging
  Serial.printf("[LOG] %s: %s\n", type.c_str(), message.c_str());
 }
 
 void loadConfig() {
   EEPROM.get(0, config);
   if (config.magic != CONFIG_MAGIC) {
     // First time or invalid config
     memset(&config, 0, sizeof(config));
     config.magic = CONFIG_MAGIC;
     config.configured = false;
     saveConfig();
   }
 }
 
 void saveConfig() {
   EEPROM.put(0, config);
   EEPROM.commit();
 }
 
 void connectToWiFi() {
   Serial.printf("Connecting to WiFi: %s\n", config.wifi_ssid);
   
   WiFi.begin(config.wifi_ssid, config.wifi_password);
   
   int attempts = 0;
   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
     delay(500);
     Serial.print(".");
     attempts++;
   }
   
   if (WiFi.status() == WL_CONNECTED) {
     Serial.println("\nWiFi connected!");
     Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
     apMode = false;
   } else {
     Serial.println("\nWiFi connection failed, starting AP mode");
     startAPMode();
   }
 }
 
 void startAPMode() {
   Serial.println("Starting AP mode...");
   
   WiFi.mode(WIFI_AP);
   WiFi.softAP(AP_SSID, AP_PASSWORD);
   
   Serial.printf("AP SSID: %s\n", AP_SSID);
   Serial.printf("AP IP address: %s\n", WiFi.softAPIP().toString().c_str());
   Serial.println("Captive portal should now be accessible");
   apMode = true;
 }
 
 void setupWebServer() {
   // Main page
   webServer.on("/", HTTP_GET, []() {
     webServer.send(200, "text/html", html_index);
   });
   
   // Configuration page
   webServer.on("/config", HTTP_GET, []() {
     webServer.send(200, "text/html", html_config);
   });
   
   // API endpoints
   webServer.on("/on", HTTP_POST, []() {
     pressPowerButton();
     webServer.send(200, "text/plain", "Power button pressed");
   });
   
   webServer.on("/off", HTTP_POST, []() {
     pressPowerButton();
     webServer.send(200, "text/plain", "Shutdown requested");
   });
   
   webServer.on("/forceoff", HTTP_POST, []() {
     forcePowerOff();
     webServer.send(200, "text/plain", "Force shutdown executed");
   });
   
   webServer.on("/status", HTTP_GET, []() {
       bool powerLedState = digitalRead(POWER_LED_PIN);
       bool pingResult = pingPC();
       
     String json = "{\"state\":\"" + getStateString(currentState) + "\",";
     json += "\"stateText\":\"" + getStateText(currentState) + "\",";
       json += "\"powerLed\":" + String(powerLedState) + ",";
       json += "\"pingResult\":" + String(pingResult) + ",";
       json += "\"wifi\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
       json += "\"wolMac\":\"" + String(config.pc_mac) + "\",";
       json += "\"wolEnabled\":" + String(strlen(config.pc_mac) > 0 ? "true" : "false") + ",";
       json += "\"wolLedBlinking\":" + String(wolLedBlinking ? "true" : "false") + ",";
       json += "\"wolBlinkCount\":" + String(wolBlinkCount) + ",";
       json += "\"wolLedState\":" + String(digitalRead(WOL_LED_PIN)) + ",";
       json += "\"timeInState\":" + String((millis() - lastStateChange) / 1000) + "}";
     
     webServer.send(200, "application/json", json);
   });
   
   // Configuration endpoints
   webServer.on("/save_config", HTTP_POST, []() {
     if (webServer.hasArg("ssid")) {
       strcpy(config.wifi_ssid, webServer.arg("ssid").c_str());
     }
     if (webServer.hasArg("password")) {
       strcpy(config.wifi_password, webServer.arg("password").c_str());
     }
     if (webServer.hasArg("pc_mac")) {
       strcpy(config.pc_mac, webServer.arg("pc_mac").c_str());
     }
     if (webServer.hasArg("pc_ip")) {
       strcpy(config.pc_ip, webServer.arg("pc_ip").c_str());
     }
     
     config.configured = true;
     saveConfig();
     
     webServer.send(200, "text/plain", "Configuration saved");
     
     // Restart ESP32 to apply new WiFi settings
     delay(1000);
     ESP.restart();
   });
   
   webServer.on("/get_config", HTTP_GET, []() {
     String json = "{\"ssid\":\"" + String(config.wifi_ssid) + "\",";
     json += "\"pc_mac\":\"" + String(config.pc_mac) + "\",";
     json += "\"pc_ip\":\"" + String(config.pc_ip) + "\"}";
     
     webServer.send(200, "application/json", json);
   });
   
   // Captive portal endpoints for different devices
   webServer.on("/generate_204", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   webServer.on("/fwlink", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   webServer.on("/hotspot-detect.html", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   webServer.on("/ncsi.txt", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   webServer.on("/connecttest.txt", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   webServer.on("/redirect", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   // Additional captive portal endpoints for various devices
   webServer.on("/success.txt", HTTP_GET, []() {
     webServer.send(200, "text/plain", "success");
   });
   
   webServer.on("/portal", HTTP_GET, []() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   // Test endpoint for captive portal verification
   webServer.on("/test", HTTP_GET, []() {
     webServer.send(200, "text/plain", "Captive portal is working! ESP32 IP: " + WiFi.softAPIP().toString());
   });
   
        // Test WOL endpoint (for testing purposes)
     webServer.on("/test_wol", HTTP_GET, []() {
       if (strlen(config.pc_mac) > 0) {
         Serial.println("Manual WOL test triggered");
         wolLedBlinking = true;
         wolBlinkCount = 0;
         lastWolBlink = millis();
         digitalWrite(WOL_LED_PIN, HIGH); // Start with LED on
         pressPowerButton();
         webServer.send(200, "text/plain", "WOL test triggered for MAC: " + String(config.pc_mac));
       } else {
         webServer.send(400, "text/plain", "No MAC address configured");
       }
     });
     
          // Reset WOL LED state endpoint (for debugging)
     webServer.on("/reset_wol_led", HTTP_GET, []() {
       Serial.println("Manual WOL LED reset triggered");
       wolLedBlinking = false;
       wolBlinkCount = 0;
       digitalWrite(WOL_LED_PIN, LOW);
       webServer.send(200, "text/plain", "WOL LED state reset");
     });
     
     // Force state to ON if ping succeeds (for debugging)
     webServer.on("/force_on_if_ping", HTTP_GET, []() {
       bool pingResult = pingPC();
       if (pingResult) {
         currentState = PC_ON;
         lastStateChange = millis();
         addLogEntry("State forced to ON due to successful ping", "state");
         webServer.send(200, "text/plain", "State forced to ON - ping successful");
       } else {
         webServer.send(400, "text/plain", "Cannot force ON - ping failed");
       }
     });
     
     // Force power state endpoint (for debugging)
     webServer.on("/force_state", HTTP_GET, []() {
       String stateParam = webServer.hasArg("state") ? webServer.arg("state") : "";
       PowerState newState = currentState;
       
       if (stateParam == "off") {
         newState = PC_OFF;
         Serial.println("Manual state change: FORCE -> OFF");
       } else if (stateParam == "on") {
         newState = PC_ON;
         Serial.println("Manual state change: FORCE -> ON");
       } else if (stateParam == "starting") {
         newState = PC_STARTING;
         Serial.println("Manual state change: FORCE -> STARTING");
       } else if (stateParam == "shutting") {
         newState = PC_SHUTTING_DOWN;
         Serial.println("Manual state change: FORCE -> SHUTTING_DOWN");
       }
       
       if (newState != currentState) {
         currentState = newState;
         lastStateChange = millis();
         webServer.send(200, "text/plain", "State forced to: " + getStateString(currentState));
       } else {
         webServer.send(400, "text/plain", "Invalid state parameter. Use: off, on, starting, shutting");
       }
     });
     
     // Debug endpoint for checking pin states
     webServer.on("/debug_pins", HTTP_GET, []() {
       String json = "{";
       json += "\"powerLedPin\":" + String(digitalRead(POWER_LED_PIN)) + ",";
       json += "\"powerSwitchPin\":" + String(digitalRead(POWER_SWITCH_PIN)) + ",";
       json += "\"statusLedPin\":" + String(digitalRead(STATUS_LED_PIN)) + ",";
       json += "\"wolLedPin\":" + String(digitalRead(WOL_LED_PIN)) + ",";
       json += "\"millis\":" + String(millis());
       json += "}";
       
       webServer.send(200, "application/json", json);
     });
     
     // Get logs API endpoint
     webServer.on("/get_logs", HTTP_GET, []() {
       String json = "[";
       bool first = true;
       
       // Start from the most recent log entry
       for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
         int idx = (logIndex - 1 - i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
         if (logEntries[idx].timestamp > 0) {
           if (!first) json += ",";
           json += "{";
           json += "\"timestamp\":" + String(logEntries[idx].timestamp) + ",";
           json += "\"message\":\"" + logEntries[idx].message + "\",";
           json += "\"type\":\"" + logEntries[idx].type + "\"";
           json += "}";
           first = false;
         }
       }
       json += "]";
       
       webServer.send(200, "application/json", json);
     });
     
     // Debug page endpoint with comprehensive logging
     webServer.on("/debug", HTTP_GET, []() {
       String html = R"rawliteral(
     <!DOCTYPE html>
     <html lang="en">
     <head>
         <meta charset="UTF-8">
         <meta name="viewport" content="width=device-width, initial-scale=1.0">
         <title>ESP32 Debug - Smart PC Controller</title>
         <style>
             * {
                 margin: 0;
                 padding: 0;
                 box-sizing: border-box;
             }
             
             body {
                 font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                 background: linear-gradient(135deg, #1a202c 0%, #2d3748 100%);
                 min-height: 100vh;
                 padding: 20px;
                 color: #e2e8f0;
             }
             
             .container {
                 max-width: 1200px;
                 margin: 0 auto;
             }
             
             .header {
                 text-align: center;
                 margin-bottom: 30px;
                 padding: 20px;
                 background: rgba(255, 255, 255, 0.1);
                 border-radius: 15px;
                 backdrop-filter: blur(10px);
             }
             
             h1 {
                 font-size: 2.5em;
                 margin-bottom: 10px;
                 background: linear-gradient(135deg, #667eea, #764ba2);
                 -webkit-background-clip: text;
                 -webkit-text-fill-color: transparent;
                 background-clip: text;
             }
             
             .subtitle {
                 color: #a0aec0;
                 font-size: 1.1em;
             }
             
             .grid {
                 display: grid;
                 grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
                 gap: 20px;
                 margin-bottom: 30px;
             }
             
             .card {
                 background: rgba(255, 255, 255, 0.1);
                 border-radius: 15px;
                 padding: 25px;
                 backdrop-filter: blur(10px);
                 border: 1px solid rgba(255, 255, 255, 0.1);
             }
             
             .card h2 {
                 color: #667eea;
                 margin-bottom: 20px;
                 font-size: 1.5em;
                 border-bottom: 2px solid #667eea;
                 padding-bottom: 10px;
             }
             
             .info-row {
                 display: flex;
                 justify-content: space-between;
                 align-items: center;
                 padding: 10px 0;
                 border-bottom: 1px solid rgba(255, 255, 255, 0.1);
             }
             
             .info-row:last-child {
                 border-bottom: none;
             }
             
             .info-label {
                 font-weight: 600;
                 color: #cbd5e0;
             }
             
             .info-value {
                 color: #e2e8f0;
                 font-family: 'Courier New', monospace;
                 background: rgba(0, 0, 0, 0.3);
                 padding: 5px 10px;
                 border-radius: 5px;
                 min-width: 120px;
                 text-align: center;
             }
             
             .status-indicator {
                 display: inline-block;
                 width: 12px;
                 height: 12px;
                 border-radius: 50%;
                 margin-right: 10px;
             }
             
             .status-on { background-color: #48bb78; }
             .status-off { background-color: #e53e3e; }
             .status-starting { background-color: #ed8936; }
             .status-shutting { background-color: #9f7aea; }
             .status-unknown { background-color: #a0aec0; }
             
             .log-container {
                 max-height: 400px;
                 overflow-y: auto;
                 background: rgba(0, 0, 0, 0.3);
                 border-radius: 10px;
                 padding: 15px;
                 margin-top: 15px;
             }
             
             .log-entry {
                 padding: 8px 0;
                 border-bottom: 1px solid rgba(255, 255, 255, 0.1);
                 font-family: 'Courier New', monospace;
                 font-size: 0.9em;
             }
             
             .log-entry:last-child {
                 border-bottom: none;
             }
             
             .log-time {
                 color: #a0aec0;
                 margin-right: 15px;
             }
             
             .log-event {
                 color: #e2e8f0;
             }
             
             .log-wol { color: #48bb78; }
             .log-power { color: #ed8936; }
             .log-state { color: #667eea; }
             .log-error { color: #e53e3e; }
             .log-ping { color: #38b2ac; }
             .log-wifi { color: #9f7aea; }
             
             .refresh-btn {
                 position: fixed;
                 bottom: 20px;
                 right: 20px;
                 background: linear-gradient(135deg, #667eea, #764ba2);
                 color: white;
                 border: none;
                 border-radius: 50%;
                 width: 60px;
                 height: 60px;
                 font-size: 1.5em;
                 cursor: pointer;
                 box-shadow: 0 8px 25px rgba(102, 126, 234, 0.3);
                 transition: all 0.3s ease;
             }
             
             .refresh-btn:hover {
                 transform: scale(1.1);
                 box-shadow: 0 12px 35px rgba(102, 126, 234, 0.4);
             }
             
             .refresh-btn:active {
                 transform: scale(0.95);
             }
             
             .auto-refresh {
                 position: fixed;
                 bottom: 20px;
                 left: 20px;
                 background: rgba(255, 255, 255, 0.1);
                 padding: 15px;
                 border-radius: 10px;
                 backdrop-filter: blur(10px);
                 border: 1px solid rgba(255, 255, 255, 0.1);
             }
             
             .auto-refresh label {
                 display: flex;
                 align-items: center;
                 gap: 10px;
                 color: #e2e8f0;
                 font-size: 0.9em;
             }
             
             .auto-refresh input[type="checkbox"] {
                 width: 18px;
                 height: 18px;
                 accent-color: #667eea;
             }
             
             .clear-logs {
                 position: fixed;
                 bottom: 20px;
                 left: 200px;
                 background: rgba(255, 255, 255, 0.1);
                 padding: 15px;
                 border-radius: 10px;
                 backdrop-filter: blur(10px);
                 border: 1px solid rgba(255, 255, 255, 0.1);
                 color: #e2e8f0;
                 text-decoration: none;
                 font-size: 0.9em;
                 transition: all 0.3s ease;
             }
             
             .clear-logs:hover {
                 background: rgba(255, 255, 255, 0.2);
                 transform: translateY(-2px);
             }
             
             @media (max-width: 768px) {
                 .grid {
                     grid-template-columns: 1fr;
                 }
                 
                 .card {
                     padding: 20px;
                 }
                 
                 h1 {
                     font-size: 2em;
                 }
                 
                 .clear-logs {
                     left: 20px;
                     bottom: 80px;
                 }
             }
         </style>
     </head>
     <body>
         <div class="container">
             <div class="header">
                 <h1>ESP32 Debug Dashboard</h1>
                 <div class="subtitle">Smart PC Controller - Real-time System Information</div>
             </div>
             
             <div class="grid">
                 <div class="card">
                     <h2>System Status</h2>
                     <div class="info-row">
                         <span class="info-label">Current State:</span>
                         <span class="info-value" id="currentState">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Power LED:</span>
                         <span class="info-value" id="powerLed">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Ping Result:</span>
                         <span class="info-value" id="pingResult">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">WiFi Status:</span>
                         <span class="info-value" id="wifiStatus">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Uptime:</span>
                         <span class="info-value" id="uptime">Loading...</span>
                     </div>
                 </div>
                 
                 <div class="card">
                     <h2>Wake-on-LAN</h2>
                     <div class="info-row">
                         <span class="info-label">WOL MAC:</span>
                         <span class="info-value" id="wolMac">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">WOL Enabled:</span>
                         <span class="info-value" id="wolEnabled">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">WOL LED Blinking:</span>
                         <span class="info-value" id="wolLedBlinking">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">WOL LED State:</span>
                         <span class="info-value" id="wolLedState">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">WOL Blink Count:</span>
                         <span class="info-value" id="wolBlinkCount">Loading...</span>
                     </div>
                 </div>
                 
                 <div class="card">
                     <h2>Network Information</h2>
                     <div class="info-row">
                         <span class="info-label">AP IP Address:</span>
                         <span class="info-value" id="apIP">Loading...</span>
                         <span class="info-value" id="apIP">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">AP SSID:</span>
                         <span class="info-value" id="apSSID">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Target PC IP:</span>
                         <span class="info-value" id="targetIP">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Ping Interval:</span>
                         <span class="info-value" id="pingInterval">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Last Ping:</span>
                         <span class="info-value" id="lastPing">Loading...</span>
                     </div>
                 </div>
                 
                 <div class="card">
                     <h2>State Machine</h2>
                     <div class="info-row">
                         <span class="info-label">Time in State:</span>
                         <span class="info-value" id="timeInState">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Last State Change:</span>
                         <span class="info-value" id="lastStateChange">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">State Change Count:</span>
                         <span class="info-value" id="stateChangeCount">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Ping Success Count:</span>
                         <span class="info-value" id="pingSuccessCount">Loading...</span>
                     </div>
                     <div class="info-row">
                         <span class="info-label">Ping Fail Count:</span>
                         <span class="info-value" id="pingFailCount">Loading...</span>
                     </div>
                 </div>
             </div>
             
             <div class="card">
                 <h2>Activity Log (Last 20 Events)</h2>
                 <div class="log-container" id="logContainer">
                     <div class="log-entry">
                         <span class="log-time">[Loading...]</span>
                         <span class="log-event">Initializing debug dashboard...</span>
                     </div>
                 </div>
             </div>
             
             <button class="refresh-btn" onclick="refreshData()" title="Refresh Data">↻</button>
             
             <div class="auto-refresh">
                 <label>
                     <input type="checkbox" id="autoRefresh" checked>
                     Auto-refresh every 2s
                 </label>
             </div>
             
             <a href="#" class="clear-logs" onclick="clearLogs()">Clear Logs</a>
         </div>
         
         <script>
             let logEntries = [];
             let stateChangeCount = 0;
             let pingSuccessCount = 0;
             let pingFailCount = 0;
             let lastPingTime = 0;
             
             function addLogEntry(event, type = 'info') {
                 const now = new Date();
                 const timeStr = now.toLocaleTimeString();
                 const logEntry = {
                     time: timeStr,
                     event: event,
                     type: type
                 };
                 
                 logEntries.unshift(logEntry);
                 
                 // Keep only last 20 entries
                 if (logEntries.length > 20) {
                     logEntries.pop();
                 }
                 
                 updateLogDisplay();
             }
             
             function updateLogDisplay() {
                 const container = document.getElementById('logContainer');
                 container.innerHTML = '';
                 
                 logEntries.forEach(entry => {
                     const div = document.createElement('div');
                     div.className = `log-entry log-${entry.type}`;
                     div.innerHTML = `
                         <span class="log-time">[${entry.time}]</span>
                         <span class="log-event">${entry.event}</span>
                     `;
                     container.appendChild(div);
                 });
             }
             
             function clearLogs() {
                 logEntries = [];
                 addLogEntry('Logs cleared manually', 'info');
                 updateLogDisplay();
             }
             
             function formatUptime(millis) {
                 if (!millis) return 'Unknown';
                 const seconds = Math.floor(millis / 1000);
                 const minutes = Math.floor(seconds / 60);
                 const hours = Math.floor(minutes / 60);
                 const days = Math.floor(hours / 24);
                 
                 if (days > 0) return `${days}d ${hours % 24}h ${minutes % 60}m`;
                 if (hours > 0) return `${hours}h ${minutes % 60}m`;
                 if (minutes > 0) return `${minutes}m ${seconds % 60}s`;
                 return `${seconds}s`;
             }
             
             function formatTime(millis) {
                 if (!millis) return 'Never';
                 const seconds = Math.floor((Date.now() - millis) / 1000);
                 if (seconds < 60) return `${seconds}s ago`;
                 if (seconds < 3600) return `${Math.floor(seconds / 60)}m ago`;
                 return `${Math.floor(seconds / 3600)}h ago`;
             }
             
             function updateDisplay(data) {
                 // Update system status
                 document.getElementById('currentState').textContent = data.stateText || data.state;
                 document.getElementById('powerLed').textContent = data.powerLed ? 'ON' : 'OFF';
                 document.getElementById('pingResult').textContent = data.pingResult ? 'Success' : 'Failed';
                 document.getElementById('wifiStatus').textContent = data.wifi ? 'Connected' : 'Disconnected';
                 document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);
                 
                 // Update WOL info
                 document.getElementById('wolMac').textContent = data.wolMac || 'Not Set';
                 document.getElementById('wolEnabled').textContent = data.wolEnabled ? 'Yes' : 'No';
                 document.getElementById('wolLedBlinking').textContent = data.wolLedBlinking ? 'Yes' : 'No';
                 document.getElementById('wolLedState').textContent = data.wolLedState ? 'ON' : 'Off';
                 document.getElementById('wolBlinkCount').textContent = data.wolBlinkCount || '0';
                 
                 // Update network info
                 document.getElementById('apIP').textContent = data.apIP || '192.168.4.1';
                 document.getElementById('apSSID').textContent = data.apSSID || 'ESP32_Controller';
                 document.getElementById('targetIP').textContent = data.targetIP || '192.168.1.100';
                 document.getElementById('pingInterval').textContent = '10s';
                 document.getElementById('lastPing').textContent = formatTime(lastPingTime);
                 
                 // Update state machine info
                 document.getElementById('timeInState').textContent = data.timeInState ? `${data.timeInState}s` : '0s';
                 document.getElementById('lastStateChange').textContent = formatTime(data.lastStateChange || 0);
                 document.getElementById('stateChangeCount').textContent = stateChangeCount;
                 document.getElementById('pingSuccessCount').textContent = pingSuccessCount;
                 document.getElementById('pingFailCount').textContent = pingFailCount;
                 
                 // Track ping results for logging
                 if (data.pingResult !== undefined) {
                     if (data.pingResult) {
                         pingSuccessCount++;
                         addLogEntry('Ping successful to target PC', 'ping');
                     } else {
                         pingFailCount++;
                         addLogEntry('Ping failed to target PC', 'error');
                     }
                     lastPingTime = Date.now();
                 }
             }
             
             function refreshData() {
                 fetch('/status')
                     .then(response => response.json())
                     .then(data => {
                         updateDisplay(data);
                         addLogEntry('Status refreshed', 'info');
                     })
                     .catch(error => {
                         console.error('Error fetching status:', error);
                         addLogEntry('Failed to fetch status: ' + error.message, 'error');
                     });
             }
             
             // Auto-refresh functionality
             let autoRefreshInterval;
             
             document.getElementById('autoRefresh').addEventListener('change', function(e) {
                 if (e.target.checked) {
                     autoRefreshInterval = setInterval(refreshData, 2000);
                     addLogEntry('Auto-refresh enabled', 'info');
                 } else {
                     clearInterval(autoRefreshInterval);
                     addLogEntry('Auto-refresh disabled', 'info');
                 }
             });
             
             // Initial load
             addLogEntry('Debug dashboard initialized', 'info');
             refreshData();
             
             // Start auto-refresh
             if (document.getElementById('autoRefresh').checked) {
                 autoRefreshInterval = setInterval(refreshData, 2000);
             }
         </script>
     </body>
     </html>
     )rawliteral";
       
       webServer.send(200, "text/html", html);
   });
   
   // Captive portal - redirect all unknown requests to main page
   webServer.onNotFound([]() {
     webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
     webServer.send(302, "text/plain", "");
   });
   
   webServer.begin();
   Serial.println("Web server started");
 }
 
 void pressPowerButton() {
  Serial.println("Power button pressed");
  addLogEntry("Power button pressed manually", "power");
  
  // Simulate button press (pull low then high)
  digitalWrite(POWER_SWITCH_PIN, LOW);
  delay(DEBOUNCE_DELAY);
  digitalWrite(POWER_SWITCH_PIN, HIGH);
  
  // Determine action based on current state and power LED
  bool powerLedState = digitalRead(POWER_LED_PIN);
  
  if (currentState == PC_OFF || currentState == PC_SHUTTING_DOWN) {
    // PC is off or shutting down - power on request
    Serial.println("Power ON requested");
    addLogEntry("Power ON requested", "power");
    currentState = PC_STARTING;
    // Note: lastStartButtonTime will be set in updatePowerState when state changes
  } else if (currentState == PC_ON || currentState == PC_STARTING) {
    // PC is on or starting - shutdown request
    Serial.println("Shutdown requested");
    addLogEntry("Shutdown requested", "power");
    currentState = PC_SHUTTING_DOWN;
    // Note: lastShutdownButtonTime will be set in updatePowerState when state changes
   }
   
   lastStateChange = millis();
  Serial.printf("State set to: %s\n", getStateString(currentState).c_str());
 }
 
 void forcePowerOff() {
   Serial.println("Force power off executed");
   
   // Hold power button down for 5 seconds
   digitalWrite(POWER_SWITCH_PIN, LOW);
   delay(5000);
   digitalWrite(POWER_SWITCH_PIN, HIGH);
   
   currentState = PC_OFF;
   lastStateChange = millis();
 }
 
 void updatePowerState() {
   bool powerLedState = digitalRead(POWER_LED_PIN);
   static unsigned long lastPingCheck = 0;
   static bool lastPingResult = false;
   
   // Update ping result every 10 seconds
   if (millis() - lastPingCheck > 10000) {
     bool newPingResult = pingPC();
     if (newPingResult != lastPingResult) {
       // Log ping result change
       String logMessage = "Ping " + String(newPingResult ? "SUCCESS" : "FAILED") + " to PC";
       addLogEntry(logMessage, "ping");
     }
     lastPingResult = newPingResult;
     lastPingCheck = millis();
     
     // Debug power LED reading and state
     bool currentPowerLed = digitalRead(POWER_LED_PIN);
     Serial.printf("Ping result: %s, Power LED: %s (raw: %d), Current State: %s\n", 
                   lastPingResult ? "SUCCESS" : "FAILED", 
                   currentPowerLed ? "ON" : "OFF",
                   currentPowerLed,
                   getStateString(currentState).c_str());
     
     // Debug event timestamps
     unsigned long currentTime = millis();
     Serial.printf("Event timestamps - WOL: %lu, Start: %lu, Shutdown: %lu\n",
                   lastWolTime > 0 ? (currentTime - lastWolTime) / 1000 : 0,
                   lastStartButtonTime > 0 ? (currentTime - lastStartButtonTime) / 1000 : 0,
                   lastShutdownButtonTime > 0 ? (currentTime - lastShutdownButtonTime) / 1000 : 0);
   }
   
   // Simple and reliable state machine based on clear requirements
   PowerState newState = currentState;
   
   // State determination logic - much simpler now
   
   // State determination logic - much simpler now
   if (lastPingResult) {
     // Ping succeeds = PC is ON
     if (currentState != PC_ON) {
       newState = PC_ON;
       Serial.printf("State change: %s -> ON (ping success)\n", getStateString(currentState).c_str());
     }
   } else {
     // Ping fails = PC is OFF, unless we have recent start events
     unsigned long currentTime = millis();
     bool hasRecentStartEvent = false;
     
     // Check for recent start events (WOL or button press in last 60 seconds)
     if (lastWolTime > 0 && (currentTime - lastWolTime) < 60000) {
       hasRecentStartEvent = true;
     }
     if (lastStartButtonTime > 0 && (currentTime - lastStartButtonTime) < 60000) {
       hasRecentStartEvent = true;
     }
     
     // Check for recent shutdown button press
     bool hasRecentShutdownEvent = (lastShutdownButtonTime > 0 && (currentTime - lastShutdownButtonTime) < 60000);
     
     if (hasRecentStartEvent && !lastPingResult) {
       // Recent start event + ping fails = STARTING
       if (currentState != PC_STARTING) {
         newState = PC_STARTING;
         Serial.printf("State change: %s -> STARTING (recent start event + ping failed)\n", getStateString(currentState).c_str());
       }
     } else if (hasRecentShutdownEvent && !lastPingResult) {
       // Recent shutdown event + ping fails = SHUTTING_DOWN
       if (currentState != PC_SHUTTING_DOWN) {
         newState = PC_SHUTTING_DOWN;
         Serial.printf("State change: %s -> SHUTTING_DOWN (recent shutdown event + ping failed)\n", getStateString(currentState).c_str());
       }
     } else if (!lastPingResult) {
       // No recent events + ping fails = OFF
       if (currentState != PC_OFF) {
         newState = PC_OFF;
         Serial.printf("State change: %s -> OFF (no recent events + ping failed)\n", getStateString(currentState).c_str());
       }
     }
   }
   
   // Update state if changed
   if (newState != currentState) {
     PowerState oldState = currentState;
     currentState = newState;
     lastStateChange = millis();
     
     // Set event timestamps based on state changes
     if (currentState == PC_STARTING) {
       lastStartButtonTime = millis();
     } else if (currentState == PC_SHUTTING_DOWN) {
       lastShutdownButtonTime = millis();
     }
     
     Serial.printf("Power state changed: %s -> %s\n", 
                   getStateString(oldState).c_str(), 
                   getStateString(currentState).c_str());
     
     // Log state change
     String logMessage = "Power state changed: " + getStateString(oldState) + " -> " + getStateString(currentState);
     addLogEntry(logMessage, "state");
   }
   
      // Update last power LED state for change detection
   lastPowerLedState = powerLedState;
 }
 
 bool pingPC() {
   if (strlen(config.pc_ip) == 0) return false;
   
   // Parse IP address
   IPAddress targetIP;
   if (!targetIP.fromString(config.pc_ip)) {
     Serial.printf("Invalid IP address: %s\n", config.pc_ip);
     return false;
   }
   
   // Use ESP32Ping library for proper ICMP ping
   bool pingResult = Ping.ping(targetIP);
   
   if (pingResult) {
     Serial.printf("Ping to %s: SUCCESS\n", config.pc_ip);
     addLogEntry("Ping successful to " + String(config.pc_ip), "ping");
   } else {
     Serial.printf("Ping to %s: FAILED\n", config.pc_ip);
     addLogEntry("Ping failed to " + String(config.pc_ip), "ping");
   }
   
   return pingResult;
 }
 
 // Parse MAC address string to byte array
 void parseMacAddress(const char* macStr, uint8_t* macBytes) {
   int values[6];
   sscanf(macStr, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
   for (int i = 0; i < 6; i++) {
     macBytes[i] = (uint8_t)values[i];
   }
 }
 
 // Check if received packet is a valid WOL magic packet for our PC
 bool isValidWolPacket(const uint8_t* packet, int packetSize) {
   if (packetSize < 102) return false;
   
   // WOL magic packet structure:
   // 6 bytes: FF FF FF FF FF FF (magic header)
   // 16 repetitions of target MAC address (6 bytes each)
   
   // Check magic header
   for (int i = 0; i < 6; i++) {
     if (packet[i] != 0xFF) return false;
   }
   
   // Parse our PC's MAC address
   uint8_t targetMac[6];
   parseMacAddress(config.pc_mac, targetMac);
   
   // Check if any of the 16 MAC repetitions match our target
   for (int rep = 0; rep < 16; rep++) {
     int offset = 6 + (rep * 6);
     bool match = true;
     for (int i = 0; i < 6; i++) {
       if (packet[offset + i] != targetMac[i]) {
         match = false;
         break;
       }
     }
     if (match) return true;
   }
   
   return false;
 }
 
 void checkForMagicPackets() {
   // Always check for WOL packets regardless of PC state (for LED indication)
   if (strlen(config.pc_mac) > 0) {
     // Check for UDP packets on WOL port
     int packetSize = udp.parsePacket();
     if (packetSize > 0) {
       uint8_t buffer[1024];
       int len = udp.read(buffer, sizeof(buffer));
       
       if (len > 0 && isValidWolPacket(buffer, len)) {
         Serial.println("WOL magic packet detected!");
         addLogEntry("WOL magic packet detected!", "wol");
         
         // Update WOL timestamp for state machine
         lastWolTime = millis();
         
         // Start WOL LED blinking sequence (3 blinks) - always blink regardless of PC state
         wolLedBlinking = true;
         wolBlinkCount = 0;
         lastWolBlink = millis();
         digitalWrite(WOL_LED_PIN, HIGH); // Start with LED on
         
         // Only power on PC if it's currently off
         if (currentState == PC_OFF) {
           Serial.println("Powering on PC...");
           addLogEntry("Powering on PC via WOL", "wol");
           pressPowerButton();
         } else {
           Serial.println("PC is already on or starting - LED will still blink");
           addLogEntry("WOL received but PC already on", "wol");
         }
       }
     }
   }
   
        // Handle WOL LED blinking - exactly 3 blinks
     if (wolLedBlinking) {
       unsigned long currentTime = millis();
       
       // Handle millis() overflow safely
       unsigned long timeDiff;
       if (currentTime >= lastWolBlink) {
         timeDiff = currentTime - lastWolBlink;
       } else {
         timeDiff = (0xFFFFFFFF - lastWolBlink) + currentTime;
       }
       
       // 300ms between state changes (ON->OFF->ON->OFF->ON->OFF = 3 complete blinks)
       if (timeDiff > 300) {
         if (wolBlinkCount < 6) { // 6 state changes = 3 complete blinks
           digitalWrite(WOL_LED_PIN, !digitalRead(WOL_LED_PIN));
           wolBlinkCount++;
           lastWolBlink = currentTime;
           Serial.printf("WOL LED blink %d/6\n", wolBlinkCount);
         } else {
           // Finished 3 blinks, turn off LED and stop blinking
           wolLedBlinking = false;
           digitalWrite(WOL_LED_PIN, LOW);
           Serial.println("WOL LED blinking sequence complete");
         }
       }
       
       // Safety timeout: if blinking takes too long, reset it
       if (timeDiff > 10000) { // 10 second timeout
         Serial.println("WOL LED blinking timeout - resetting");
         wolLedBlinking = false;
         wolBlinkCount = 0;
         digitalWrite(WOL_LED_PIN, LOW);
       }
   }
 }
 
 void blinkStatusLed() {
   static bool ledState = false;
   ledState = !ledState;
   digitalWrite(STATUS_LED_PIN, ledState);
 }
 
 String getStateString(PowerState state) {
   switch (state) {
     case PC_OFF: return "off";
     case PC_STARTING: return "starting";
     case PC_ON: return "on";
     case PC_SHUTTING_DOWN: return "shutting";
     default: return "unknown";
   }
 }
 
 String getStateText(PowerState state) {
   switch (state) {
     case PC_OFF: return "PC is Off";
     case PC_STARTING: return "PC is Starting";
     case PC_ON: return "PC is On";
     case PC_SHUTTING_DOWN: return "PC is Shutting Down";
     default: return "Unknown State";
   }
 }
 