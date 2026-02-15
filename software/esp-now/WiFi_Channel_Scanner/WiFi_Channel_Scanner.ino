/*
 * WiFi Channel Scanner for ESP32
 * Use this to find which channels are busy in your area
 * Upload to either board to scan for WiFi networks
 * Helps you choose the best ESP-NOW channel
 */

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== WiFi Channel Scanner ===");
  Serial.println("Scanning for WiFi networks...\n");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop() {
  Serial.println("Starting scan...");
  
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.printf("Found %d networks\n\n", n);
    
    // Count networks per channel
    int channelCount[14] = {0};  // Channels 1-13
    
    Serial.println("SSID                             | Signal | Channel");
    Serial.println("-------------------------------------------------");
    
    for (int i = 0; i < n; i++) {
      int channel = WiFi.channel(i);
      int rssi = WiFi.RSSI(i);
      String ssid = WiFi.SSID(i);
      
      // Truncate long SSIDs
      if (ssid.length() > 32) {
        ssid = ssid.substring(0, 29) + "...";
      }
      
      Serial.printf("%-32s | %4d   | %d\n", 
                    ssid.c_str(), 
                    rssi,
                    channel);
      
      if (channel >= 1 && channel <= 13) {
        channelCount[channel]++;
      }
    }
    
    // Show channel usage summary
    Serial.println("\n=== Channel Usage Summary ===");
    Serial.println("Channel | Networks | Recommendation");
    Serial.println("--------|----------|---------------");
    
    for (int ch = 1; ch <= 13; ch++) {
      String recommendation = "";
      
      if (channelCount[ch] == 0) {
        recommendation = "EXCELLENT - No traffic";
      } else if (channelCount[ch] <= 2) {
        recommendation = "GOOD - Light traffic";
      } else if (channelCount[ch] <= 5) {
        recommendation = "FAIR - Moderate traffic";
      } else {
        recommendation = "BUSY - Avoid if possible";
      }
      
      Serial.printf("   %2d   |    %2d    | %s\n", 
                    ch, 
                    channelCount[ch],
                    recommendation.c_str());
    }
    
    // Recommend best channels
    Serial.println("\n=== Recommendations for ESP-NOW ===");
    
    // Find channels 1, 6, 11 with least traffic (non-overlapping)
    int bestChannel = 1;
    int minCount = channelCount[1];
    
    if (channelCount[6] < minCount) {
      bestChannel = 6;
      minCount = channelCount[6];
    }
    if (channelCount[11] < minCount) {
      bestChannel = 11;
      minCount = channelCount[11];
    }
    
    Serial.printf("Best non-overlapping channel: %d (%d networks)\n", bestChannel, minCount);
    Serial.println("\nNon-overlapping channels (1, 6, 11):");
    Serial.printf("  Channel 1:  %d networks - %s\n", 
                  channelCount[1], 
                  (channelCount[1] <= 2) ? "GOOD choice" : "Moderate");
    Serial.printf("  Channel 6:  %d networks - %s\n", 
                  channelCount[6], 
                  (channelCount[6] <= 2) ? "GOOD choice" : "Moderate");
    Serial.printf("  Channel 11: %d networks - %s\n", 
                  channelCount[11], 
                  (channelCount[11] <= 2) ? "GOOD choice" : "Moderate");
    
    Serial.println("\nUpdate both sketches with:");
    Serial.printf("  #define ESPNOW_CHANNEL %d\n", bestChannel);
  }
  
  Serial.println("\n========================================");
  Serial.println("Scanning again in 10 seconds...\n");
  delay(10000);
}
