# ESP-NOW Quick Debug Checks

Use this checklist when the robot or controller isn't responding as expected.

---

## Controller Boot Debug

When the ESP32-C3 controller boots, it prints:

```
=== ESP32-C3 Sesame Controller ===
Protocol version: 1
Controller MAC: AA:BB:CC:DD:EE:FF
ESP-NOW Channel: 1
Robot MAC: DC:54:75:C8:43:00
Robot peer registered
```

**Expected:**
- Controller MAC is present ✓
- Robot MAC is configured ✓
- "Robot peer registered" appears ✓

**If Robot MAC shows all `FF:FF:FF:FF:FF:FF`:**
- Run `SET_ROBOT_MAC <mac>` with the robot's MAC address
- The robot MAC is printed by the robot at boot (see Robot Boot Debug)

---

## Robot Boot Debug

When the robot boots, it prints:

```
=== Sesame Robot (ESP-NOW) ===
MAC Address: DC:54:75:C8:43:00
ESP-NOW Channel: 1
Copy this MAC address to the ESP32-C3 Controller!
...
Controller peer registered: AA:BB:CC:DD:EE:FF
```

**Expected:**
- Robot MAC printed at top ✓
- "Controller peer registered" appears **after the controller first contacts it** ✓

**If "Controller MAC not set":**
- Expected on first boot — robot will auto-register when controller sends first message
- If it never registers, check that the controller MAC is correct

---

## Basic Connectivity Test

**On controller:**
```
PING
```
**Expected response:**
```
PONG rtt=1.23ms
```

**If timeout/error:**
1. Verify both devices show same ESP-NOW channel
2. Verify controller knows robot MAC (`MAC` command shows controller's own MAC, `SET_ROBOT_MAC` was used)
3. Power cycle both boards
4. Bring boards within 1m of each other

---

## Command round-trip Test

**On controller:**
```
CMD wave
```
**Expected robot serial output:**
```
[ESP-NOW] Motion: wave
```
**Expected controller response:**
```
OK
```

Then the robot plays the wave animation.

**If no response on robot:**
- Controller logs `OK SENT` but robot doesn't print → ESP-NOW packet not arriving
  - Check MAC addresses
  - Check channel match
  - Check distance/interference
- Controller logs error → send failure
- No controller response at all → robot never ACKed

---

## Status Check

**On controller:**
```
STATUS
```
**Expected response:**
```
STATUS cmd=wave face=wave
```

Useful to verify robot's internal state.

---

## ESP-NOW Channel Mismatch

**Symptoms:**
- PING times out
- Commands never arrive
- No "Robot peer registered" on robot

**Fix:**
1. On controller: `CHANNEL 1` (or 6, 11 — pick one)
2. On robot: Change `ESPNOW_CHANNEL` in `espnow-protocol.h` to same value
3. Reflash robot
4. Restart controller

**Tip:** Use the `WiFi_Channel_Scanner` sketch to see which channels are crowded.

---

## MAC Address Mismatch

**Symptom:** PING sends but never gets PONG. Controller prints "ERR Response timeout".

**Debug:**
1. Robot boot log prints its MAC — copy it exactly
2. On controller: `SET_ROBOT_MAC <that_mac>`
3. Wait 2 seconds, then `PING`

**Controller MAC** — just informational; robot auto-accepts first sender. If you want to lock it down, you would hardcode it in the robot firmware (currently not required).

---

## Controller keeps losing peer registration

ESP-NOW peers can be deleted by WiFi events or power loss.

**Fix:** The controller re-adds the peer on every boot (in `setup()`). If the robot changes MAC or the pairing is lost:
1. `SET_ROBOT_MAC <current_robot_mac>` on controller
2. Restart both devices

---

## Robot serial debug commands

While ESP-NOW is running, you can still connect a serial monitor to the **robot** and use the built-in CLI:

```
help                → Show commands
mac                 → Show robot's MAC + channel
rn wf               → Walk forward
rn wb               → Walk backward
rn tl / tr          → Turn
rn st               → Stand
rn wv / dn / sw     → Wave / dance / swim
subtrim             → List all trims
st 0 5              → Set servo 0 trim +5
st reset            → Reset all trims
0 90                → Set servo 0 to 90°
all 90              → Set all servos to 90°
```

These commands work alongside ESP-NOW and update the same internal state.

---

## Watching Real-time ESP-NOW Traffic

Controller sends are logged as `OK SENT`. Responses are logged as they arrive.

**Robot-side logging** (Serial Monitor):
- `[ESP-NOW] Motion: forward` — Motion command received
- `[ESP-NOW] Face: happy` — Face command received
- `[ESP-NOW] Trim get` — Trim query received

If the robot prints nothing, ESP-NOW packets aren't being received at all.

---

## Common Pitfalls

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| Controller boots, `SET_ROBOT_MAC` works, but PING fails | Robot MAC changed (refreshed) or channel mismatch | Re-copy robot MAC, run `SET_ROBOT_MAC` again; verify channel |
| Robot boots, prints MAC, but controller says "Robot peer not registered" | Controller MAC is all 0xFF (needs to send first message to auto-register) | Send any command (e.g., `PING`) from controller |
| PONG received but >50ms latency | Boards are far apart or WiFi interference | Reduce distance, change to channel 6 or 11 |
| Servo jitter during movements | `motorCurrentDelay` too low | `CONFIG_SET motorCurrentDelay 80` |
| Face doesn't change on `FACE happy` | Face name typo or missing bitmap | Check serial log; valid names are in `face-bitmaps.h` |
| `CMD forward` does nothing | `currentCommand` cleared immediately (motion function returned early) | Check if `pressingCheck()` interrupted; try `CMD rest` and `CMD wave` first |
| Controller resets repeatedly | Power issue (USB cable bad) | Use known-good USB cable / powered hub |

---

## Message Flow Diagram

```
Computer
   │
   ├─ "CMD wave\n" ─────────────────────┐
   │                                     ▼
   │                      Controller (ESP32-C3)
   │                        Parses "wave"
   │                        Builds binary MSG_MOTION_CMD
   │                        msg_id = 42
   │                        esp_now_send() → Robot
   │                                     ├─ Prints "OK SENT"
   │                                     │
   ▼                                     ▼
Robot (ESP32-S3)            ESP-NOW Wireless (1-5 ms)
   │                         Packet arrives
   ▼                        onEspNowRecv()
Parses header               msg_type = 0x01
   │                        msg_id = 42
   │                        → handleMotionCmd()
   │                        → currentCommand = "wave"
   │                        → runWavePose()
   │                        → Sends back ACK
   │                        (msg_type = 0x80,
   │                         orig_msg_id = 42)
   │
   ▼
Controller receives ACK
   │
   ├─ Matches orig_msg_id=42 to pending request
   └─ Prints "OK"
```

---

## Serial Port Tips

- **Baud rate:** 115200
- **USB identification:**
  - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
  - macOS: `/dev/cu.usbmodem*`
  - Windows: `COM3` (or higher)
- Many serial terminals can connect (Arduino Serial Monitor, PuTTY, screen, minicom, Python `pyserial`)

**Python quick connect:**
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
ser.write(b'PING\n')
print(ser.readline().decode())
# → PONG rtt=...
```

---

## Re-flashing Tips

If you need to re-flash either board:
- **Robot:** Arduino IDE → Board: ESP32S3 Dev → upload `sesame-firmware-espnow.ino`
- **Controller:** Arduino IDE → Board: ESP32C3 Dev → upload `ESP32_C3_Sesame.ino`

After flashing, open Serial Monitor at 115200 baud to see the startup messages.

**Important:** The robot's MAC must be set in the controller's NVS. After re-flashing the controller, you may need to run `SET_ROBOT_MAC` again.
