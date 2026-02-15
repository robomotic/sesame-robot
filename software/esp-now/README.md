# ESP-NOW Speed Test - Setup Instructions

## Overview
This test measures ESP-NOW communication performance between two ESP32 boards:
- **ESP32-C3 Mini**: Controller/Receiver (connected to computer)
- **AtomS3 Lite ESP32-S3**: Sensor/Transmitter (simulating robot sensor)

## Setup Steps

### 1. Install Required Library
The AtomS3 sketch requires the FastLED library for RGB LED control:
1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Search for "FastLED"
4. Install "FastLED by Daniel Garcia"

### 2. Program ESP32-C3 Mini First
### 2. Program ESP32-C3 Mini First
1. Open `ESP32_C3_Controller.ino` in Arduino IDE
2. Select board: "ESP32C3 Dev Module" (or appropriate C3 board)
3. Upload the sketch
4. Open Serial Monitor (115200 baud)
5. **COPY THE MAC ADDRESS** displayed on startup (format: XX:XX:XX:XX:XX:XX)

### 3. Program AtomS3 Lite Second
1. Open `AtomS3_Sensor.ino` in Arduino IDE
2. **IMPORTANT**: Replace the MAC address on line 11 with the C3 Mini's MAC address:
   ```cpp
   uint8_t c3MiniAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
   ```
3. Select board: "ESP32S3 Dev Module" (or M5Stack AtomS3)
4. Upload the sketch
5. Open Serial Monitor (115200 baud)
6. **COPY THE MAC ADDRESS** displayed

### 4. Update C3 Mini with AtomS3 MAC
1. Go back to `ESP32_C3_Controller.ino`
2. Replace the MAC address on line 11 with the AtomS3's MAC address
3. Re-upload to the C3 Mini

### 5. Run the Test
1. Both boards should now communicate
2. **Press the button on the AtomS3** to start/stop transmission
3. Watch the RGB LED for status (see LED Status Guide below)
4. Statistics appear every 2 seconds on both serial monitors when transmitting

## AtomS3 Controls

### Button Control
- **Press once**: Start transmission (LED turns green)
- **Press again**: Stop transmission (LED returns to breathing blue)
- Transmission only happens when active - saves power and lets you control when to test

### LED Status Guide
- **Cyan**: Initializing ESP-NOW
- **Blue (breathing)**: Ready, waiting for button press
- **Green (solid)**: Actively transmitting sensor data
- **Yellow (pulse)**: Receiving servo commands from C3 (overlays on green)
- **Red (blinking)**: Connection error or repeated send failures

## Test Configuration

### WiFi Channel Selection (Important!)
To avoid interference with your local WiFi network, you should configure ESP-NOW to use a different channel:

1. **Find your router's WiFi channel:**
   - Check your router's admin page (usually 192.168.1.1 or 192.168.0.1)
   - Look for "Wireless Settings" or "WiFi Channel"
   - Note which channel your 2.4GHz network uses (typically 1, 6, or 11)

2. **Choose an ESP-NOW channel:**
   - Pick a non-overlapping channel different from your WiFi
   - Best options: 1, 6, or 11 (these don't overlap with each other)
   - Example: If your WiFi is on channel 6, use channel 1 or 11 for ESP-NOW

3. **Set the channel in BOTH sketches:**
   - In `ESP32_C3_Controller.ino` line 14: `#define ESPNOW_CHANNEL 1`
   - In `AtomS3_Sensor.ino` line 19: `#define ESPNOW_CHANNEL 1`
   - **Both must use the SAME channel!**

**Channel Reference:**
- Channels 1, 6, 11 don't overlap - best for avoiding interference
- If your WiFi is on channel 1, use 6 or 11
- If your WiFi is on channel 6, use 1 or 11
- If your WiFi is on channel 11, use 1 or 6

### Default Settings
- **Send Rate**: AtomS3 sends sensor data every 5ms (200 Hz)
- **Processing Loop**: C3 Mini processes every 10ms (100 Hz)
- **Data Sizes**: 
  - Sensor data: 16 bytes (3 floats + timestamp)
  - Servo data: 20 bytes (8 uint16_t + timestamp)

### Adjusting Test Parameters

**Change Send Rate** (in AtomS3_Sensor.ino, line 44):
```cpp
const int SEND_INTERVAL_MS = 5;  // Try 1, 5, 10, 20, 50
```

**Change Processing Rate** (in ESP32_C3_Controller.ino, line 70):
```cpp
if (processStart - lastProcessTime >= 10) {  // Try 5, 10, 20, 50
```

## Metrics Explained

### On C3 Mini (Controller)
- **Receive Rate**: Packets/sec from AtomS3
- **Send Rate**: Servo commands/sec sent back
- **Avg Latency**: Time from when AtomS3 sent data until C3 received it
- **Min/Max Latency**: Best and worst one-way latency

### On AtomS3 (Sensor)
- **Send Rate**: Sensor packets/sec transmitted
- **Success Rate**: % of packets sent successfully
- **Receive Rate**: Servo commands/sec received from C3
- **Round-Trip Time**: Time from sending sensor data to receiving servo commands back

## Performance Expectations

Typical ESP-NOW performance:
- **Latency**: 1-5 ms (one-way)
- **Round-Trip**: 5-20 ms
- **Max Throughput**: 200-500 packets/sec (depends on packet size and distance)
- **Range**: 100-200m line of sight, 10-50m with obstacles

### Observed Test Results

#### Standard Mode (5ms Interval)
During a standard test run with default settings:
- **Send Rate**: ~185 packets/sec
- **Receive Rate**: ~91 packets/sec (at 10ms processing loop)
- **Success Rate**: 100.0%
- **Latencies**:
  - **Average Round-Trip**: 2.85 - 2.92 ms
  - **Minimum Round-Trip**: 0.81 - 2.77 ms
  - **Maximum Round-Trip**: 3.99 - 5.09 ms

#### High-Performance Mode (1ms Interval)
Pushing the limits with `SEND_INTERVAL_MS = 1`:
- **Send Rate**: ~478 packets/sec
- **Receive Rate**: ~94 packets/sec
- **Success Rate**: 100.0%
- **Latencies**:
  - **Average Round-Trip**: **0.80 - 1.10 ms**
  - **Minimum Round-Trip**: 0.03 - 0.59 ms
  - **Maximum Round-Trip**: 2.08 - 3.13 ms

These results confirm that the link is highly stable and capable of supporting extremely high-frequency control loops with sub-1ms round-trip latency when optimized.

## Troubleshooting

**LED not working:**
1. Verify FastLED library is installed
2. Check correct board selection (ESP32S3 Dev Module)
3. Some AtomS3 clones use different LED pins - try pin 2 if pin 35 doesn't work

**Button not responding:**
1. Wait for initialization (cyan LED) to complete
2. LED should turn blue when ready
3. Press firmly - there should be a tactile click
4. Check serial monitor for "TRANSMISSION STARTED/STOPPED" messages

**No communication:**
1. Verify MAC addresses are correctly copied (include 0x prefix)
2. Check both boards are on same WiFi channel (auto-negotiated)
3. Ensure boards are within range (start close, ~1m apart)

**High packet loss:**
- Check WiFi channel configuration - make sure both boards use the same channel
- If using the same channel as your WiFi, change to a non-overlapping channel
- Increase SEND_INTERVAL_MS on AtomS3
- Check for WiFi interference (microwaves, other 2.4GHz devices)
- Reduce distance between boards

**Inconsistent latency:**
- Normal with WiFi congestion
- Try changing WiFi channel if possible
- Reduce other 2.4GHz devices nearby

## Testing Different Scenarios

### Maximum Throughput Test
Set `SEND_INTERVAL_MS = 1` on AtomS3 to send at ~1000 Hz and observe packet loss.

### Latency Test
Set `SEND_INTERVAL_MS = 50` for slower rate, measure minimum achievable latency.

### Real-World Robot Simulation
- Keep default 5ms send rate (200 Hz - typical IMU rate)
- Monitor if C3 can keep up with 10ms processing loop
- Check round-trip times stay under 20-30ms for responsive control

## Next Steps

After testing, you can:
1. Replace simulated sensor data with real MPU6050 readings
2. Add actual servo control code on AtomS3
3. Implement error handling and retry logic
4. Add acknowledgment system for critical commands
5. Optimize packet structures for your specific needs
