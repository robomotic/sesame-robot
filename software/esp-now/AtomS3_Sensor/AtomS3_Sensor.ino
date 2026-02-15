/*
 * AtomS3 Lite ESP32-S3 - Sensor/Transmitter
 * Simulates MPU6050 readings, sends to C3 Mini, receives servo commands
 * Measures throughput and round-trip time
 * Button control and RGB LED status indication
 */

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <M5AtomS3.h>

// MAC Address of the ESP32-C3 Mini (you'll need to update this after running C3 sketch)
uint8_t c3MiniAddress[] = {0xAC, 0xA7, 0x04, 0xD3, 0x3D, 0x34};

// WiFi channel configuration (1-13)
// MUST MATCH the channel set on the C3 Mini!
// Choose a channel different from your WiFi router to avoid interference
// Check your router settings - common channels are 1, 6, 11
#define ESPNOW_CHANNEL 1  // Change this to match C3 Mini and avoid your WiFi

// Data structures
typedef struct sensor_data {
  float ax;
  float ay;
  float az;
  uint32_t timestamp_us;
} sensor_data;

typedef struct servo_data {
  uint16_t j1;
  uint16_t j2;
  uint16_t j3;
  uint16_t j4;
  uint16_t j5;
  uint16_t j6;
  uint16_t j7;
  uint16_t j8;
  uint32_t timestamp_us;
} servo_data;

sensor_data outgoingSensorData;
servo_data incomingServoData;

// Statistics
unsigned long packetsSent = 0;
unsigned long packetsReceived = 0;
unsigned long sendFailures = 0;
unsigned long lastStatsTime = 0;
uint32_t totalRoundTrip_us = 0;
uint32_t maxRoundTrip_us = 0;
uint32_t minRoundTrip_us = 999999;
uint32_t lastSentTimestamp = 0;

// Send rate control
unsigned long lastSendTime = 0;
const int SEND_INTERVAL_MS = 1;  // Adjust this to test different rates (5ms = 200Hz)

// Transmission control
bool transmitting = false;

// LED status modes
enum LEDStatus {
  LED_INIT,           // Cyan - Initializing
  LED_READY,          // Blue - Ready but not transmitting
  LED_TRANSMITTING,   // Green - Actively transmitting
  LED_RECEIVING,      // Yellow pulse - Receiving servo data
  LED_ERROR           // Red - Connection error
};

LEDStatus currentStatus = LED_INIT;
unsigned long lastLEDUpdate = 0;
unsigned long lastReceiveTime = 0;

// Callback when data is received (ESP32 core 3.x uses esp_now_recv_info_t)
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
  uint32_t receiveTime_us = micros();
  lastReceiveTime = millis();  // Track for LED status
  
  memcpy(&incomingServoData, data, sizeof(incomingServoData));
  packetsReceived++;
  
  // Calculate round-trip time (if this is a response to our last send)
  if (lastSentTimestamp > 0) {
    uint32_t roundTrip_us = receiveTime_us - lastSentTimestamp;
    totalRoundTrip_us += roundTrip_us;
    
    if (roundTrip_us > maxRoundTrip_us) maxRoundTrip_us = roundTrip_us;
    if (roundTrip_us < minRoundTrip_us) minRoundTrip_us = roundTrip_us;
  }
}

// Callback when data is sent (signature depends on ESP32 core version)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    // Success is already counted in packetsSent
  } else {
    sendFailures++;
  }
}

// Generate simulated MPU6050 data
void generateSensorData() {
  static float angle = 0;
  angle += 0.1;
  
  // Simulate accelerometer readings (±2g range, with some variation)
  outgoingSensorData.ax = sin(angle) * 2.0 + random(-100, 100) / 100.0;
  outgoingSensorData.ay = cos(angle) * 2.0 + random(-100, 100) / 100.0;
  outgoingSensorData.az = 1.0 + sin(angle * 0.5) * 0.5 + random(-50, 50) / 100.0;
  outgoingSensorData.timestamp_us = micros();
  
  lastSentTimestamp = outgoingSensorData.timestamp_us;
}

// Update RGB LED based on status using M5AtomS3 library
void updateLED() {
  static uint8_t brightness = 0;
  static bool increasing = true;
  uint32_t color = 0x000000;  // Default black
  
  switch (currentStatus) {
    case LED_INIT:
      // Cyan - initializing
      color = 0x00FFFF;
      break;
      
    case LED_READY: {
      // Blue breathing - ready but not transmitting
      if (increasing) {
        brightness += 5;
        if (brightness >= 200) increasing = false;
      } else {
        brightness -= 5;
        if (brightness <= 50) increasing = true;
      }
      // Create blue with breathing effect
      color = brightness;  // Blue channel only
      break;
    }
      
    case LED_TRANSMITTING:
      // Green with yellow pulse when receiving
      if (millis() - lastReceiveTime < 100) {
        // Yellow pulse for 100ms after receiving data
        color = 0xFFFF00;
      } else {
        // Solid green when transmitting
        color = 0x00FF00;
      }
      break;
      
    case LED_RECEIVING:
      // Yellow - receiving (momentary)
      color = 0xFFFF00;
      break;
      
    case LED_ERROR:
      // Red blinking - error
      if ((millis() / 500) % 2) {
        color = 0xFF0000;
      } else {
        color = 0x000000;
      }
      break;
  }
  
  AtomS3.dis.drawpix(color);
  AtomS3.update();
}

// Handle button press using M5AtomS3 library
void checkButton() {
  // M5AtomS3 handles button reading automatically with AtomS3.update()
  // BtnA is the built-in button
  if (AtomS3.BtnA.wasPressed()) {
    transmitting = !transmitting;
    
    if (transmitting) {
      Serial.println("\n>>> TRANSMISSION STARTED <<<\n");
      currentStatus = LED_TRANSMITTING;
    } else {
      Serial.println("\n>>> TRANSMISSION STOPPED <<<\n");
      currentStatus = LED_READY;
    }
  }
}

void setup() {
  // Initialize M5AtomS3 (true enables display)
  AtomS3.begin(true);
  AtomS3.dis.setBrightness(100);
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== AtomS3 Lite Sensor ===");
  
  // Set initial LED status
  currentStatus = LED_INIT;
  updateLED();
  
  // Set device as a Wi-Fi Station and start it
  WiFi.mode(WIFI_STA);
  WiFi.begin();  // Start WiFi to initialize MAC address
  delay(100);    // Give WiFi time to initialize
  
  // Print MAC address BEFORE setting channel
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Now set WiFi channel to match C3 Mini and avoid interference
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.printf("WiFi Channel: %d\n", ESPNOW_CHANNEL);
  Serial.println("Copy this MAC address to the C3 Mini sketch!");
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    currentStatus = LED_ERROR;
    updateLED();
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer (C3 Mini)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, c3MiniAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;  // Must match the WiFi channel
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    currentStatus = LED_ERROR;
    updateLED();
    return;
  }
  
  Serial.println("ESP-NOW Initialized");
  Serial.printf("Sending sensor data every %d ms\n", SEND_INTERVAL_MS);
  Serial.println("\n*** PRESS BUTTON TO START/STOP TRANSMISSION ***\n");
  Serial.println("LED Status:");
  Serial.println("  Blue (breathing) = Ready");
  Serial.println("  Green = Transmitting");
  Serial.println("  Yellow pulse = Receiving data");
  Serial.println("  Red (blinking) = Error\n");
  
  currentStatus = LED_READY;
  lastStatsTime = millis();
  lastSendTime = millis();
}

void loop() {
  // Update M5AtomS3 (handles button reading)
  AtomS3.update();
  
  // Check button state
  checkButton();
  
  // Update LED status (every 20ms for smooth animations)
  if (millis() - lastLEDUpdate >= 20) {
    updateLED();
    lastLEDUpdate = millis();
  }
  
  // Send sensor data at specified interval (only if transmitting)
  if (transmitting && (millis() - lastSendTime >= SEND_INTERVAL_MS)) {
    lastSendTime = millis();
    
    generateSensorData();
    
    esp_err_t result = esp_now_send(c3MiniAddress, (uint8_t *)&outgoingSensorData, sizeof(outgoingSensorData));
    
    if (result == ESP_OK) {
      packetsSent++;
    } else {
      sendFailures++;
      // Show error on LED if send fails repeatedly
      if (sendFailures > 10 && sendFailures > packetsSent) {
        currentStatus = LED_ERROR;
      }
    }
  }
  
  // Print statistics every 2 seconds (only if transmitting)
  if (transmitting && (millis() - lastStatsTime >= 2000)) {
    float duration_sec = (millis() - lastStatsTime) / 1000.0;
    float sendRate = packetsSent / duration_sec;
    float receiveRate = packetsReceived / duration_sec;
    float avgRoundTrip_us = (packetsReceived > 0) ? (totalRoundTrip_us / packetsReceived) : 0;
    float successRate = (packetsSent + sendFailures > 0) ? 
                        (100.0 * packetsSent / (packetsSent + sendFailures)) : 0;
    
    Serial.println("\n========== STATISTICS ==========");
    Serial.printf("Send Rate: %.1f packets/sec\n", sendRate);
    Serial.printf("Receive Rate: %.1f packets/sec\n", receiveRate);
    Serial.printf("Packets Sent: %lu\n", packetsSent);
    Serial.printf("Send Failures: %lu\n", sendFailures);
    Serial.printf("Success Rate: %.1f%%\n", successRate);
    Serial.printf("Servo Commands Received: %lu\n", packetsReceived);
    Serial.printf("Avg Round-Trip: %.2f ms\n", avgRoundTrip_us / 1000.0);
    Serial.printf("Min Round-Trip: %.2f ms\n", minRoundTrip_us / 1000.0);
    Serial.printf("Max Round-Trip: %.2f ms\n", maxRoundTrip_us / 1000.0);
    Serial.printf("Last Sensor: ax=%.2f, ay=%.2f, az=%.2f\n",
                  outgoingSensorData.ax, outgoingSensorData.ay, outgoingSensorData.az);
    Serial.printf("Last Servos: j1=%d, j2=%d, j3=%d, j4=%d, j5=%d, j6=%d, j7=%d, j8=%d\n",
                  incomingServoData.j1, incomingServoData.j2, incomingServoData.j3, 
                  incomingServoData.j4, incomingServoData.j5, incomingServoData.j6,
                  incomingServoData.j7, incomingServoData.j8);
    Serial.println("================================\n");
    
    // Reset counters
    packetsSent = 0;
    packetsReceived = 0;
    sendFailures = 0;
    totalRoundTrip_us = 0;
    maxRoundTrip_us = 0;
    minRoundTrip_us = 999999;
    lastStatsTime = millis();
  }
  
  // Small delay to prevent watchdog issues
  delay(1);
}
