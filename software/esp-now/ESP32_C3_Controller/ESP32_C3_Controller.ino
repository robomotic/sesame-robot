/*
 * ESP32-C3 Mini - Controller/Receiver
 * Receives MPU6050 data from AtomS3, processes it, sends servo commands back
 * Measures throughput and latency
 */

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// MAC Address of the AtomS3 Lite (you'll need to update this)
uint8_t atomS3Address[] = {0xDC, 0x54, 0x75, 0xC8, 0x43, 0x00};

// WiFi channel configuration (1-13)
// Choose a channel different from your WiFi router to avoid interference
// Check your router settings - common channels are 1, 6, 11
// Use 1, 6, or 11 for best results (non-overlapping channels)
#define ESPNOW_CHANNEL 1  // Change this to avoid your WiFi channel

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

sensor_data incomingSensorData;
servo_data outgoingServoData;

// Statistics
unsigned long packetsReceived = 0;
unsigned long packetsSent = 0;
unsigned long lastStatsTime = 0;
uint32_t totalLatency_us = 0;
uint32_t maxLatency_us = 0;
uint32_t minLatency_us = 999999;
unsigned long lastProcessTime = 0;

// Callback when data is received (ESP32 core 3.x uses esp_now_recv_info_t)
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
  uint32_t receiveTime_us = micros();
  
  memcpy(&incomingSensorData, data, sizeof(incomingSensorData));
  packetsReceived++;
  
  // Calculate latency (time since sensor sent the data)
  uint32_t latency_us = receiveTime_us - incomingSensorData.timestamp_us;
  totalLatency_us += latency_us;
  
  if (latency_us > maxLatency_us) maxLatency_us = latency_us;
  if (latency_us < minLatency_us) minLatency_us = latency_us;
  
  // Simulate 10ms processing loop
  unsigned long processStart = millis();
  if (processStart - lastProcessTime >= 10) {
    lastProcessTime = processStart;
    
    // Simulate calculations (simple computation based on sensor data)
    float magnitude = sqrt(incomingSensorData.ax * incomingSensorData.ax + 
                          incomingSensorData.ay * incomingSensorData.ay + 
                          incomingSensorData.az * incomingSensorData.az);
    
    // Generate servo positions (500-2500 microseconds range)
    outgoingServoData.j1 = 500 + (uint16_t)(abs(incomingSensorData.ax) * 200);
    outgoingServoData.j2 = 500 + (uint16_t)(abs(incomingSensorData.ay) * 200);
    outgoingServoData.j3 = 500 + (uint16_t)(abs(incomingSensorData.az) * 200);
    outgoingServoData.j4 = 1000 + (uint16_t)(magnitude * 100);
    outgoingServoData.j5 = 1500 + (uint16_t)(sin(millis() / 1000.0) * 500);
    outgoingServoData.j6 = 1500 + (uint16_t)(cos(millis() / 1000.0) * 500);
    outgoingServoData.j7 = 1500;
    outgoingServoData.j8 = 1500;
    outgoingServoData.timestamp_us = micros();
    
    // Send servo commands back
    esp_err_t result = esp_now_send(atomS3Address, (uint8_t *)&outgoingServoData, sizeof(outgoingServoData));
    if (result == ESP_OK) {
      packetsSent++;
    }
  }
}

// Callback when data is sent (ESP32 core 3.x uses wifi_tx_info_t)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // Optional: track send success/failure
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32-C3 Controller ===");
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Set WiFi channel to avoid interference
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  
  // Print MAC address and channel
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.printf("WiFi Channel: %d\n", ESPNOW_CHANNEL);
  Serial.println("Copy this MAC address to the AtomS3 sketch!");
  Serial.println("Make sure AtomS3 uses the SAME channel!\n");
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer (AtomS3)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, atomS3Address, 6);
  peerInfo.channel = ESPNOW_CHANNEL;  // Must match the WiFi channel
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP-NOW Initialized");
  Serial.println("Waiting for data from AtomS3...\n");
  
  lastStatsTime = millis();
}

void loop() {
  // Print statistics every 2 seconds
  if (millis() - lastStatsTime >= 2000) {
    float duration_sec = (millis() - lastStatsTime) / 1000.0;
    float receiveRate = packetsReceived / duration_sec;
    float sendRate = packetsSent / duration_sec;
    float avgLatency_us = (packetsReceived > 0) ? (totalLatency_us / packetsReceived) : 0;
    
    Serial.println("\n========== STATISTICS ==========");
    Serial.printf("Receive Rate: %.1f packets/sec\n", receiveRate);
    Serial.printf("Send Rate: %.1f packets/sec\n", sendRate);
    Serial.printf("Packets Received: %lu\n", packetsReceived);
    Serial.printf("Packets Sent: %lu\n", packetsSent);
    Serial.printf("Avg Latency: %.2f ms\n", avgLatency_us / 1000.0);
    Serial.printf("Min Latency: %.2f ms\n", minLatency_us / 1000.0);
    Serial.printf("Max Latency: %.2f ms\n", maxLatency_us / 1000.0);
    Serial.printf("Last Sensor Data: ax=%.2f, ay=%.2f, az=%.2f\n", 
                  incomingSensorData.ax, incomingSensorData.ay, incomingSensorData.az);
    Serial.printf("Last Servo Data: j1=%d, j2=%d, j3=%d, j4=%d\n",
                  outgoingServoData.j1, outgoingServoData.j2, 
                  outgoingServoData.j3, outgoingServoData.j4);
    Serial.println("================================\n");
    
    // Reset counters
    packetsReceived = 0;
    packetsSent = 0;
    totalLatency_us = 0;
    maxLatency_us = 0;
    minLatency_us = 999999;
    lastStatsTime = millis();
  }
  
  delay(10);
}
