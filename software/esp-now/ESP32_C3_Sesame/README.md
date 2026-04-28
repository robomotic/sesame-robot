# ESP-NOW Sesame Controller

Control the Sesame Robot wirelessly using ESP-NOW instead of WiFi. An ESP32-C3 Mini connects to your computer or Raspberry Pi via USB and bridges text serial commands to the robot over ESP-NOW.

## Architecture

```
┌─────────────┐    USB Serial     ┌─────────────────┐    ESP-NOW     ┌──────────────┐
│  Computer / │ ←──(text cmds)──→ │  ESP32-C3 Mini  │ ←──(binary)──→ │  ESP32-S3    │
│  Raspberry  │   115200 baud     │  (Controller)   │   ~1-5ms RTT  │  (Robot)     │
│  Pi         │                   │                 │               │  8 Servos    │
└─────────────┘                   └─────────────────┘               │  OLED Face   │
                                                                    └──────────────┘
```

**Why ESP-NOW instead of WiFi?**
- Sub-5ms latency (vs 20-100ms for HTTP over WiFi)
- No access point or router needed
- Lower power consumption
- More reliable — no TCP overhead, reconnection delays, or DNS issues
- Direct peer-to-peer at the radio level

## Hardware Required

| Component | Role | Notes |
|-----------|------|-------|
| **ESP32-C3 Mini** | Controller (USB bridge) | Any ESP32-C3 board works |
| **ESP32-S3 board** | Robot (runs servos + display) | With Sesame Distro Board |
| **USB cable** | Computer ↔ Controller | USB-C or Micro-USB depending on board |

No wiring between the two boards — they communicate wirelessly.

## Quick Start

### 1. Flash the Robot Firmware

1. Open `firmware/sesame-firmware-espnow.ino` in Arduino IDE
2. Select board: **ESP32S3 Dev Module** (or your specific S3 board)
3. Install required libraries (same as original firmware):
   - `ESP32Servo` v3.0.9
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
4. Upload the sketch
5. Open Serial Monitor (115200 baud)
6. **Copy the MAC address** printed at startup:
   ```
   === Sesame Robot (ESP-NOW) ===
   MAC Address: DC:54:75:C8:43:00
   ESP-NOW Channel: 1
   ```

### 2. Flash the Controller Firmware

1. Open `software/esp-now/ESP32_C3_Sesame/ESP32_C3_Sesame.ino` in Arduino IDE
2. Select board: **ESP32C3 Dev Module**
3. Upload the sketch
4. Open Serial Monitor (115200 baud)
5. Set the robot's MAC address (from step 1):
   ```
   SET_ROBOT_MAC DC:54:75:C8:43:00
   ```
   This is saved to flash — you only need to do it once.

### 3. Test the Connection

```
PING
```
Expected response:
```
PONG rtt=2.35ms
```

Try a motion:
```
CMD wave
```

### WiFi Channel Configuration

Both boards must use the same ESP-NOW channel. The default is channel 1.

To avoid interference with your local WiFi router:
- If your router uses channel 1 → set ESP-NOW to channel 6 or 11
- If your router uses channel 6 → set ESP-NOW to channel 1 or 11
- If your router uses channel 11 → set ESP-NOW to channel 1 or 6

**On the controller** (persisted to flash):
```
CHANNEL 6
```
Then restart the controller.

**On the robot**, change `ESPNOW_CHANNEL` in `espnow-protocol.h`:
```cpp
#define ESPNOW_CHANNEL 6
```
Then reflash the robot.

Use the `WiFi_Channel_Scanner` sketch to find which channels are in use near you.

## Serial Command Reference

All commands are text, one per line, newline terminated (`\n`). Connect to the ESP32-C3 controller at 115200 baud.

### Motion Commands

| Command | Description |
|---------|-------------|
| `CMD forward` | Walk forward |
| `CMD backward` | Walk backward |
| `CMD left` | Turn left |
| `CMD right` | Turn right |
| `CMD rest` | Rest position (all servos 90°) |
| `CMD stand` | Standing pose |
| `CMD wave` | Wave animation |
| `CMD dance` | Dance animation |
| `CMD swim` | Swim animation |
| `CMD point` | Point forward |
| `CMD pushup` | Push-up animation |
| `CMD bow` | Bow animation |
| `CMD cute` | Cute pose |
| `CMD freaky` | Freaky animation |
| `CMD worm` | Worm animation |
| `CMD shake` | Shake animation |
| `CMD shrug` | Shrug animation |
| `CMD dead` | Play dead |
| `CMD crab` | Crab walk |
| `STOP` | Stop current motion |

### Face Commands

| Command | Description |
|---------|-------------|
| `FACE happy` | Happy expression |
| `FACE sad` | Sad expression |
| `FACE angry` | Angry expression |
| `FACE surprised` | Surprised expression |
| `FACE sleepy` | Sleepy expression |
| `FACE love` | Love expression |
| `FACE excited` | Excited expression |
| `FACE confused` | Confused expression |
| `FACE thinking` | Thinking expression |
| `FACE idle` | Idle with blink animation |
| `FACE default` | Default face |
| `FACE talk_happy` | Talking happy (for speech sync) |
| `FACE talk_sad` | Talking sad |
| ... | (all emotions have `talk_` variants) |

### Servo Control

| Command | Description |
|---------|-------------|
| `SERVO <ch> <angle>` | Set servo channel (0-7) to angle (0-180) |
| `SERVO_ALL <angle>` | Set all servos to same angle |

**Channel mapping:**

| Channel | Name | Position |
|---------|------|----------|
| 0 | R1 | Right front hip |
| 1 | R2 | Right rear hip |
| 2 | L1 | Left front hip |
| 3 | L2 | Left rear hip |
| 4 | R4 | Right rear knee |
| 5 | R3 | Right front knee |
| 6 | L3 | Left front knee |
| 7 | L4 | Left rear knee |

### Servo Trimming

| Command | Description |
|---------|-------------|
| `TRIM_SET <ch> <val>` | Set subtrim for channel (0-7), value -90 to +90 |
| `TRIM_GET` | Get all current subtrim values |
| `TRIM_RESET` | Reset all trims to 0 |

### Configuration

| Command | Description |
|---------|-------------|
| `CONFIG_SET frameDelay <ms>` | Delay between animation frames (default: 100) |
| `CONFIG_SET walkCycles <n>` | Walk cycles per command (default: 10) |
| `CONFIG_SET motorCurrentDelay <ms>` | Delay between servo moves (default: 40) |
| `CONFIG_SET faceFps <n>` | Face animation FPS (default: 8) |
| `CONFIG_SET sensorMode <n>` | Sensor mode: 0=off, 1=accel, 2=gyro, 3=magneto, 4=rpy |
| `CONFIG_SET sensorRate <ms>` | Sensor update rate in ms (0=off) |
| `CONFIG_GET` | Get all config values |

### Sensor Commands

| Command | Description |
|---------|-------------|
| `PING` | `PONG rtt=<ms>` | Round-trip latency |
| `STATUS` | `STATUS cmd=<motion> face=<name>` | Current robot state |

### System Commands

| Command | Response | Description |
|---------|----------|-------------|
| `MAC` | `MAC <aa:bb:cc:dd:ee:ff>` | Controller MAC address |
| `CHANNEL [n]` | `CHANNEL <n>` | Get/set ESP-NOW channel |
| `SET_ROBOT_MAC <mac>` | `OK ...` | Set robot MAC (persisted) |
| `HELP` | (help text) | Show all commands |

### Response Format

| Response | Meaning |
|----------|---------|
| `OK` | Command acknowledged by robot |
| `OK SENT` | Command sent, awaiting robot acknowledgement |
| `ERR <message>` | Error with description |
| `TRIM <v0> ... <v7>` | Subtrim values for all 8 servos |
| `CONFIG frameDelay=... walkCycles=... motorCurrentDelay=... faceFps=... sensorMode=... sensorRate=...` | All config values |
| `STATUS cmd=... face=...` | Robot status |
| `PONG rtt=<ms>` | Round-trip time |
| `SENSOR mode=<n> ax=... ay=... az=... gx=... gy=... gz=... mx=... my=... mz=... roll=... pitch=... yaw=...` | MPU6050 sensor data |

## ESP-NOW Binary Protocol

For developers building custom controllers or extending the protocol.

### Message Format

Every ESP-NOW message starts with a 2-byte header:

```
Byte 0: msg_type (uint8)
Byte 1: msg_id   (uint8, sequence number 0-255)
Remaining: payload (type-specific)
```

Maximum ESP-NOW payload is 250 bytes.

### Command Messages (Controller → Robot)

| Type | Hex | Struct | Payload |
|------|-----|--------|---------|
| Motion | `0x01` | `sesame_motion_cmd_t` | `char[16]` command name |
| Face | `0x02` | `sesame_face_cmd_t` | `char[24]` face name |
| Stop | `0x03` | (header only) | — |
| Servo Set | `0x04` | `sesame_servo_set_t` | `uint8 channel, uint8 angle` |
| Servo All | `0x05` | `sesame_servo_all_t` | `uint8 angle` |
| Trim Set | `0x06` | `sesame_trim_set_t` | `uint8 channel, int8 value` |
| Trim Get | `0x07` | (header only) | — |
| Trim Reset | `0x08` | (header only) | — |
| Config Set | `0x09` | `sesame_config_set_t` | `uint8 key, int16 value` |
| Config Get | `0x0A` | (header only) | — |
| Status Get | `0x0B` | (header only) | — |
| Ping | `0x0C` | `sesame_ping_t` | `uint32 timestamp_us` |

### Response Messages (Robot → Controller)

| Type | Hex | Struct | Payload |
|------|-----|--------|---------|
| ACK | `0x80` | `sesame_ack_t` | `uint8 orig_msg_id, uint8 status` |
| Trim Data | `0x81` | `sesame_trim_data_t` | `int8[8]` subtrim values |
| Config Data | `0x82` | `sesame_config_data_t` | `int16 × 4` config values |
| Status Data | `0x83` | `sesame_status_data_t` | `char[16] + char[24]` cmd + face |
| Pong | `0x84` | `sesame_pong_t` | `uint32 × 2` timestamps |
| Sensor Data | `0x85` | `sesame_sensor_data_t` | `float × 6 + uint32` (future: MPU6050) |

### ACK Status Codes

| Code | Hex | Meaning |
|------|-----|---------|
| OK | `0x00` | Success |
| Error | `0x01` | General error |
| Unknown | `0x02` | Unknown command type |
| Invalid | `0x03` | Invalid parameters |

### Config Keys

| Key | Hex | Parameter |
|-----|-----|-----------|
| `0x01` | Frame delay | `frameDelay` (ms) |
| `0x02` | Walk cycles | `walkCycles` (count) |
| `0x03` | Motor current delay | `motorCurrentDelay` (ms) |
| `0x04` | Face FPS | `faceFps` (Hz) |

All struct definitions are in `software/esp-now/espnow-protocol.h`. Include this header in any custom firmware to use the protocol.

## Python Example

Control the robot from Python via the controller's serial port:

```python
import serial
import time

# Connect to the ESP32-C3 controller
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=2)
time.sleep(2)  # Wait for boot

def send(cmd):
    """Send command and print response."""
    ser.write(f"{cmd}\n".encode())
    time.sleep(0.1)
    while ser.in_waiting:
        print(ser.readline().decode().strip())

# Test connection
send("PING")

# Play a motion
send("CMD wave")
time.sleep(5)

# Set a face
send("FACE happy")

# Direct servo control
send("SERVO 0 135")   # R1 to 135 degrees

# Read trims
send("TRIM_GET")

# Set a trim
send("TRIM_SET 0 5")  # +5 degree offset on R1

# Get robot status
send("STATUS")

ser.close()
```

On Raspberry Pi, the serial port is typically `/dev/ttyUSB0` or `/dev/ttyACM0`.

## Troubleshooting

### No response to PING

1. **Check MAC address**: Run `MAC` on the controller to verify its address. The robot prints its MAC at boot in the serial monitor.
2. **Check channel**: Both must use the same `ESPNOW_CHANNEL`. Default is 1.
3. **Check distance**: Start with boards < 1 meter apart.
4. **Check power**: Both boards need adequate power (USB or battery).
5. **Re-set robot MAC**: `SET_ROBOT_MAC <robot_mac>` on the controller.

### Robot not responding to commands

1. **Check robot serial monitor**: The robot prints `[ESP-NOW] Motion: wave` etc. when it receives commands. If you see nothing, ESP-NOW isn't reaching it.
2. **Auto-registration**: The robot auto-registers the controller on the first received message. If the controller MAC changes, restart the robot.
3. **Verify firmware**: Make sure the robot is running `sesame-firmware-espnow.ino`, not the WiFi version.

### High latency (>10ms)

1. **Channel interference**: Use `WiFi_Channel_Scanner` to find a clear channel.
2. **Distance**: ESP-NOW latency increases slightly with distance.
3. **Other 2.4GHz devices**: Bluetooth, microwaves, and other WiFi networks can interfere.

### Servo jitter or incorrect angles

1. Use `TRIM_SET` to calibrate each servo.
2. Check `motorCurrentDelay` — too low can cause power brownouts: `CONFIG_SET motorCurrentDelay 40`
3. Ensure adequate power supply for the servo rail.

### Robot serial debug

The robot still accepts direct serial commands for debugging. Connect a serial monitor to the robot's USB port and type `help`:

```
=== Serial Debug Commands ===
  <motor 0-7> <angle 0-180>  Set servo
  all <angle>                Set all servos
  rn wf/wb/tl/tr             Walk/turn
  rn rs/st/wv/dn/sw/pt       Poses
  mac                         Show MAC address
  subtrim / st                Show trims
  help                        This help
```

## File Reference

| File | Description |
|------|-------------|
| `software/esp-now/espnow-protocol.h` | Shared protocol definitions (include in both firmwares) |
| `software/esp-now/ESP32_C3_Sesame/ESP32_C3_Sesame.ino` | Controller firmware (ESP32-C3) |
| `firmware/sesame-firmware-espnow.ino` | Robot firmware, ESP-NOW version (ESP32-S3) |
| `firmware/sesame-firmware-main.ino` | Robot firmware, original WiFi version (unchanged) |
| `firmware/face-bitmaps.h` | OLED face bitmap data (shared by both firmware versions) |
| `firmware/movement-sequences.h` | Servo movement sequences (shared by both firmware versions) |
| `software/esp-now/WiFi_Channel_Scanner/` | Utility to scan WiFi channels |

## Differences from WiFi Firmware

| Feature | WiFi (`sesame-firmware-main.ino`) | ESP-NOW (`sesame-firmware-espnow.ino`) |
|---------|-----------------------------------|----------------------------------------|
| Control method | WiFi AP + WebServer + captive portal | ESP-NOW radio (no WiFi AP) |
| Latency | 20-100ms (HTTP) | 1-5ms |
| Range | WiFi range (~30m indoor) | ESP-NOW range (~100m line of sight) |
| UI | Built-in web controller | External (serial terminal, Python, etc.) |
| Dependencies | WiFi, WebServer, DNSServer, ESPmDNS | WiFi (STA mode only), esp_now |
| Power | Higher (WiFi AP active) | Lower (no AP, no HTTP stack) |
| Serial debug CLI | Yes | Yes (identical commands) |
| Servo/face/motion code | Identical | Identical |
| NVS storage | Identical | Identical |
