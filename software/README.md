# Software

Software applications for making the most of your Sesame Robot.

## Tools

### [Sesame Studio](sesame-studio/)
A visual Python-based tool for creating animations.

- **Graphical Interface**: layout joints on screen with guides.
- **Code Generator**: outputs C++ code ready for firmware.

[Go to Sesame Studio Documentation ->](sesame-studio/README.md)

### ESP-NOW Controller (`esp-now/`)
Lightweight wireless control using ESP-NOW (no WiFi router needed). An ESP32-C3 Mini connects via USB to your computer and bridges serial commands to the robot over ESP-NOW radio.

- **Low latency**: 1-5 ms round-trip vs 20-100ms over HTTP
- **No network setup**: Peer-to-peer, no access point required
- **Simple protocol**: Text commands over serial (easy from Python, shell, Node.js)
- **Full feature parity**: motions, faces, servo trim, configuration

**Components:**
- `espnow-protocol.h` — shared binary protocol header
- `ESP32_C3_Sesame/` — controller firmware (ESP32-C3)
- `sesame-firmware-espnow.ino` — robot firmware (ESP32-S3)

[Go to ESP-NOW Documentation ->](esp-now/ESP32_C3_Sesame/README.md)

[ESP-NOW Quick Debug Checks ->](esp-now/DEBUG_CHECKS.md)

### Sesame Companion App
A Python application for advanced robot control and interaction over your local network.

- **Voice Control**: Integrate with speech recognition for voice commands.
- **Emotion Mapping**: Automatically set robot expressions based on sentiment analysis.
- **Remote API Control**: Send commands and face changes via the JSON API.
- **Network Mode**: Requires robot firmware with network mode enabled.
- **Home Automation**: Example integrations with Home Assistant and other platforms.

**Note:** The Companion App is a separate repository and requires the latest firmware with network mode enabled.

[Go to Sesame Companion App Repository ->](https://github.com/dorianborian/sesame-companion-app)


