/*
 * ESP32_C3_Sesame - ESP-NOW Controller for Sesame Robot
 * 
 * Runs on an ESP32-C3 Mini connected to a computer or Raspberry Pi via USB serial.
 * Bridges text serial commands to ESP-NOW binary protocol for controlling the robot.
 *
 * Serial Protocol (115200 baud, newline terminated):
 *   CMD <motion>           - Play motion (forward, backward, left, right, rest, stand,
 *                            wave, dance, swim, point, pushup, bow, cute, freaky,
 *                            worm, shake, shrug, dead, crab)
 *   STOP                   - Stop current motion
 *   FACE <name>            - Set face expression (happy, sad, idle, walk, etc.)
 *   SERVO <ch> <angle>     - Set servo channel (0-7) to angle (0-180)
 *   SERVO_ALL <angle>      - Set all servos to same angle
 *   TRIM_SET <ch> <val>    - Set subtrim for channel (0-7), value (-90 to 90)
 *   TRIM_GET               - Get all subtrim values
 *   TRIM_RESET             - Reset all subtrims to 0
 *   CONFIG_SET <key> <val> - Set config (frameDelay, walkCycles, motorCurrentDelay, faceFps)
 *   CONFIG_GET             - Get all config values
 *   STATUS                 - Get robot status (current command + face)
 *   PING                   - Measure round-trip latency
 *   MAC                    - Show this controller's MAC address
 *   CHANNEL <n>            - Set ESP-NOW channel (1-13), requires restart
 *   HELP                   - Show available commands
 *
 * Wiring: USB only (no GPIO needed)
 * Board: ESP32-C3 Mini (or any ESP32-C3 board)
 */

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include "../espnow-protocol.h"

// ============================================================================
// Configuration
// ============================================================================

// MAC Address of the Robot (ESP32-S3)
// ** UPDATE THIS with the MAC printed by the robot firmware at boot **
uint8_t robotAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Response timeout (ms)
#define RESPONSE_TIMEOUT_MS  2000

// ============================================================================
// State
// ============================================================================
static uint8_t msgIdCounter = 0;
static bool peerRegistered = false;
static Preferences prefs;

// Pending request tracking
static uint8_t pendingMsgId = 0;
static uint8_t pendingMsgType = 0;
static unsigned long pendingSentTime = 0;
static bool pendingResponse = false;

// Ping tracking
static uint32_t pingSentTimestamp_us = 0;

// ESP-NOW channel (persisted)
static uint8_t espnowChannel = ESPNOW_CHANNEL;

// Serial input buffer
#define SERIAL_BUF_SIZE 128
static char serialBuf[SERIAL_BUF_SIZE];
static uint8_t serialBufPos = 0;

// ============================================================================
// Forward declarations
// ============================================================================
void processSerialCommand(const char* line);
void sendMotionCmd(const char* cmd);
void sendFaceCmd(const char* face);
void sendStopCmd();
void sendServoSet(uint8_t channel, uint8_t angle);
void sendServoAll(uint8_t angle);
void sendTrimSet(uint8_t channel, int8_t value);
void sendTrimGet();
void sendTrimReset();
void sendConfigSet(const char* key, int16_t value);
void sendConfigGet();
void sendStatusGet();
void sendPing();
bool sendEspNow(const uint8_t* data, size_t len);
uint8_t nextMsgId();
uint8_t configKeyFromString(const char* key);
void printHelp();

// ============================================================================
// ESP-NOW Callbacks
// ============================================================================

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("ERR ESP-NOW send failed");
    pendingResponse = false;
  }
}

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
  if (data_len < (int)sizeof(sesame_msg_header_t)) return;

  const sesame_msg_header_t* hdr = (const sesame_msg_header_t*)data;

  switch (hdr->msg_type) {
    case MSG_ACK: {
      if (data_len >= (int)sizeof(sesame_ack_t)) {
        const sesame_ack_t* ack = (const sesame_ack_t*)data;
        if (ack->status == ACK_OK) {
          Serial.println("OK");
        } else if (ack->status == ACK_ERR_UNKNOWN) {
          Serial.println("ERR Unknown command on robot");
        } else if (ack->status == ACK_ERR_INVALID) {
          Serial.println("ERR Invalid parameters");
        } else {
          Serial.println("ERR Robot error");
        }
        if (pendingResponse && ack->orig_msg_id == pendingMsgId) {
          pendingResponse = false;
        }
      }
      break;
    }

    case MSG_TRIM_DATA: {
      if (data_len >= (int)sizeof(sesame_trim_data_t)) {
        const sesame_trim_data_t* td = (const sesame_trim_data_t*)data;
        Serial.printf("TRIM %d %d %d %d %d %d %d %d\n",
          td->subtrim[0], td->subtrim[1], td->subtrim[2], td->subtrim[3],
          td->subtrim[4], td->subtrim[5], td->subtrim[6], td->subtrim[7]);
        pendingResponse = false;
      }
      break;
    }

    case MSG_CONFIG_DATA: {
      if (data_len >= (int)sizeof(sesame_config_data_t)) {
        const sesame_config_data_t* cd = (const sesame_config_data_t*)data;
        Serial.printf("CONFIG frameDelay=%d walkCycles=%d motorCurrentDelay=%d faceFps=%d sensorMode=%d sensorRate=%d\n",
          cd->frameDelay, cd->walkCycles, cd->motorCurrentDelay, cd->faceFps,
          cd->sensorMode, cd->sensorRate);
        pendingResponse = false;
      }
      break;
    }

    case MSG_STATUS_DATA: {
      if (data_len >= (int)sizeof(sesame_status_data_t)) {
        const sesame_status_data_t* sd = (const sesame_status_data_t*)data;
        Serial.printf("STATUS cmd=%s face=%s\n", sd->currentCommand, sd->currentFace);
        pendingResponse = false;
      }
      break;
    }

    case MSG_PONG: {
      if (data_len >= (int)sizeof(sesame_pong_t)) {
        const sesame_pong_t* pong = (const sesame_pong_t*)data;
        uint32_t now_us = micros();
        uint32_t rtt_us = now_us - pong->orig_timestamp_us;
        Serial.printf("PONG rtt=%.2fms\n", rtt_us / 1000.0f);
        pendingResponse = false;
      }
      break;
    }

    case MSG_SENSOR_DATA: {
      if (data_len >= (int)sizeof(sesame_sensor_data_t)) {
        const sesame_sensor_data_t* sd = (const sesame_sensor_data_t*)data;
        Serial.printf("SENSOR mode=%d ax=%.3f ay=%.3f az=%.3f gx=%.3f gy=%.3f gz=%.3f mx=%.3f my=%.3f mz=%.3f roll=%.3f pitch=%.3f yaw=%.3f\n",
          sd->mode, sd->ax, sd->ay, sd->az, sd->gx, sd->gy, sd->gz,
          sd->mx, sd->my, sd->mz, sd->roll, sd->pitch, sd->yaw);
      }
      break;
    }

    default:
      Serial.printf("ERR Unknown response type 0x%02X\n", hdr->msg_type);
      break;
  }
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Load persisted channel
  prefs.begin("sesame_c3", true);
  espnowChannel = prefs.getUChar("channel", ESPNOW_CHANNEL);
  
  // Load persisted robot MAC
  size_t macLen = prefs.getBytesLength("robotMAC");
  if (macLen == 6) {
    prefs.getBytes("robotMAC", robotAddress, 6);
  }
  prefs.end();

  Serial.println();
  Serial.println("=== ESP32-C3 Sesame Controller ===");
  Serial.printf("Protocol version: %d\n", SESAME_PROTOCOL_VERSION);

  // Init WiFi in STA mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(espnowChannel, WIFI_SECOND_CHAN_NONE);

  Serial.printf("Controller MAC: %s\n", WiFi.macAddress().c_str());
  Serial.printf("ESP-NOW Channel: %d\n", espnowChannel);
  Serial.printf("Robot MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    robotAddress[0], robotAddress[1], robotAddress[2],
    robotAddress[3], robotAddress[4], robotAddress[5]);

  // Check if robot MAC is set
  bool macDefault = true;
  for (int i = 0; i < 6; i++) {
    if (robotAddress[i] != 0xFF) { macDefault = false; break; }
  }
  if (macDefault) {
    Serial.println("WARNING: Robot MAC not configured!");
    Serial.println("Use SET_ROBOT_MAC <aa:bb:cc:dd:ee:ff> to set it.");
  }

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERR ESP-NOW init failed!");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register robot as peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, robotAddress, 6);
  peerInfo.channel = espnowChannel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    peerRegistered = true;
    Serial.println("Robot peer registered");
  } else {
    Serial.println("ERR Failed to register robot peer");
  }

  Serial.println();
  Serial.println("Ready. Type HELP for commands.");
  Serial.println();
}

// ============================================================================
// Main Loop
// ============================================================================
void loop() {
  // Read serial input line by line
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBufPos > 0) {
        serialBuf[serialBufPos] = '\0';
        processSerialCommand(serialBuf);
        serialBufPos = 0;
      }
    } else if (serialBufPos < SERIAL_BUF_SIZE - 1) {
      serialBuf[serialBufPos++] = c;
    }
  }

  // Check for response timeout
  if (pendingResponse && (millis() - pendingSentTime > RESPONSE_TIMEOUT_MS)) {
    Serial.println("ERR Response timeout");
    pendingResponse = false;
  }
}

// ============================================================================
// Serial Command Parser
// ============================================================================
void processSerialCommand(const char* line) {
  // Skip leading whitespace
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '\0') return;

  // Make a mutable copy for tokenizing
  char buf[SERIAL_BUF_SIZE];
  strncpy(buf, line, SERIAL_BUF_SIZE - 1);
  buf[SERIAL_BUF_SIZE - 1] = '\0';

  // Extract command verb (first token)
  char* verb = strtok(buf, " \t");
  if (verb == NULL) return;

  // Convert verb to uppercase for case-insensitive matching
  for (char* p = verb; *p; p++) *p = toupper(*p);

  if (strcmp(verb, "CMD") == 0) {
    char* arg = strtok(NULL, " \t");
    if (arg == NULL) {
      Serial.println("ERR Usage: CMD <motion>");
      return;
    }
    // Convert to lowercase for motion names
    for (char* p = arg; *p; p++) *p = tolower(*p);
    sendMotionCmd(arg);
  }
  else if (strcmp(verb, "STOP") == 0) {
    sendStopCmd();
  }
  else if (strcmp(verb, "FACE") == 0) {
    char* arg = strtok(NULL, " \t");
    if (arg == NULL) {
      Serial.println("ERR Usage: FACE <name>");
      return;
    }
    for (char* p = arg; *p; p++) *p = tolower(*p);
    sendFaceCmd(arg);
  }
  else if (strcmp(verb, "SERVO") == 0) {
    char* chStr = strtok(NULL, " \t");
    char* angStr = strtok(NULL, " \t");
    if (chStr == NULL || angStr == NULL) {
      Serial.println("ERR Usage: SERVO <channel 0-7> <angle 0-180>");
      return;
    }
    int ch = atoi(chStr);
    int ang = atoi(angStr);
    if (ch < 0 || ch >= NUM_SERVOS) {
      Serial.printf("ERR Channel must be 0-%d\n", NUM_SERVOS - 1);
      return;
    }
    if (ang < 0 || ang > 180) {
      Serial.println("ERR Angle must be 0-180");
      return;
    }
    sendServoSet((uint8_t)ch, (uint8_t)ang);
  }
  else if (strcmp(verb, "SERVO_ALL") == 0) {
    char* angStr = strtok(NULL, " \t");
    if (angStr == NULL) {
      Serial.println("ERR Usage: SERVO_ALL <angle 0-180>");
      return;
    }
    int ang = atoi(angStr);
    if (ang < 0 || ang > 180) {
      Serial.println("ERR Angle must be 0-180");
      return;
    }
    sendServoAll((uint8_t)ang);
  }
  else if (strcmp(verb, "TRIM_SET") == 0) {
    char* chStr = strtok(NULL, " \t");
    char* valStr = strtok(NULL, " \t");
    if (chStr == NULL || valStr == NULL) {
      Serial.printf("ERR Usage: TRIM_SET <channel 0-%d> <value -90..90>\n", NUM_SERVOS - 1);
      return;
    }
    int ch = atoi(chStr);
    int val = atoi(valStr);
    if (ch < 0 || ch >= NUM_SERVOS) {
      Serial.printf("ERR Channel must be 0-%d\n", NUM_SERVOS - 1);
      return;
    }
    if (val < -90 || val > 90) {
      Serial.println("ERR Value must be -90 to 90");
      return;
    }
    sendTrimSet((uint8_t)ch, (int8_t)val);
  }
  else if (strcmp(verb, "TRIM_GET") == 0) {
    sendTrimGet();
  }
  else if (strcmp(verb, "TRIM_RESET") == 0) {
    sendTrimReset();
  }
  else if (strcmp(verb, "CONFIG_SET") == 0) {
    char* keyStr = strtok(NULL, " \t");
    char* valStr = strtok(NULL, " \t");
    if (keyStr == NULL || valStr == NULL) {
      Serial.println("ERR Usage: CONFIG_SET <key> <value>");
      Serial.println("    Keys: frameDelay, walkCycles, motorCurrentDelay, faceFps");
      return;
    }
    int16_t val = (int16_t)atoi(valStr);
    sendConfigSet(keyStr, val);
  }
  else if (strcmp(verb, "CONFIG_GET") == 0) {
    sendConfigGet();
  }
  else if (strcmp(verb, "STATUS") == 0) {
    sendStatusGet();
  }
  else if (strcmp(verb, "PING") == 0) {
    sendPing();
  }
  else if (strcmp(verb, "MAC") == 0) {
    Serial.printf("MAC %s\n", WiFi.macAddress().c_str());
  }
  else if (strcmp(verb, "CHANNEL") == 0) {
    char* chStr = strtok(NULL, " \t");
    if (chStr == NULL) {
      Serial.printf("CHANNEL %d\n", espnowChannel);
      return;
    }
    int ch = atoi(chStr);
    if (ch < 1 || ch > 13) {
      Serial.println("ERR Channel must be 1-13");
      return;
    }
    espnowChannel = (uint8_t)ch;
    prefs.begin("sesame_c3", false);
    prefs.putUChar("channel", espnowChannel);
    prefs.end();
    Serial.printf("OK Channel set to %d (restart to apply)\n", espnowChannel);
  }
  else if (strcmp(verb, "SET_ROBOT_MAC") == 0) {
    char* macStr = strtok(NULL, " \t");
    if (macStr == NULL) {
      Serial.println("ERR Usage: SET_ROBOT_MAC <aa:bb:cc:dd:ee:ff>");
      return;
    }
    uint8_t newMac[6];
    if (sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &newMac[0], &newMac[1], &newMac[2],
               &newMac[3], &newMac[4], &newMac[5]) != 6) {
      Serial.println("ERR Invalid MAC format. Use aa:bb:cc:dd:ee:ff");
      return;
    }
    
    // Remove old peer if registered
    if (peerRegistered) {
      esp_now_del_peer(robotAddress);
      peerRegistered = false;
    }
    
    // Update MAC
    memcpy(robotAddress, newMac, 6);
    
    // Save to NVS
    prefs.begin("sesame_c3", false);
    prefs.putBytes("robotMAC", robotAddress, 6);
    prefs.end();
    
    // Register new peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, robotAddress, 6);
    peerInfo.channel = espnowChannel;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      peerRegistered = true;
      Serial.printf("OK Robot MAC set to %02X:%02X:%02X:%02X:%02X:%02X\n",
        robotAddress[0], robotAddress[1], robotAddress[2],
        robotAddress[3], robotAddress[4], robotAddress[5]);
    } else {
      Serial.println("ERR Failed to register new robot peer");
    }
  }
  else if (strcmp(verb, "HELP") == 0) {
    printHelp();
  }
  else {
    Serial.printf("ERR Unknown command: %s\n", verb);
    Serial.println("Type HELP for available commands.");
  }
}

// ============================================================================
// Command Senders
// ============================================================================

void sendMotionCmd(const char* cmd) {
  sesame_motion_cmd_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_MOTION_CMD, nextMsgId());
  strncpy(msg.command, cmd, MAX_CMD_STRING_LEN - 1);
  msg.command[MAX_CMD_STRING_LEN - 1] = '\0';
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendFaceCmd(const char* face) {
  sesame_face_cmd_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_FACE_CMD, nextMsgId());
  strncpy(msg.face, face, MAX_FACE_STRING_LEN - 1);
  msg.face[MAX_FACE_STRING_LEN - 1] = '\0';
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendStopCmd() {
  sesame_msg_header_t msg;
  sesame_msg_init(&msg, MSG_STOP_CMD, nextMsgId());
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendServoSet(uint8_t channel, uint8_t angle) {
  sesame_servo_set_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_SERVO_SET, nextMsgId());
  msg.channel = channel;
  msg.angle = angle;
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendServoAll(uint8_t angle) {
  sesame_servo_all_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_SERVO_ALL, nextMsgId());
  msg.angle = angle;
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendTrimSet(uint8_t channel, int8_t value) {
  sesame_trim_set_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_TRIM_SET, nextMsgId());
  msg.channel = channel;
  msg.value = value;
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendTrimGet() {
  sesame_msg_header_t msg;
  sesame_msg_init(&msg, MSG_TRIM_GET, nextMsgId());
  pendingResponse = true;
  pendingSentTime = millis();
  sendEspNow((uint8_t*)&msg, sizeof(msg));
}

void sendTrimReset() {
  sesame_msg_header_t msg;
  sesame_msg_init(&msg, MSG_TRIM_RESET, nextMsgId());
  sendEspNow((uint8_t*)&msg, sizeof(msg));
}

void sendConfigSet(const char* key, int16_t value) {
  uint8_t configKey = configKeyFromString(key);
  if (configKey == 0) {
    Serial.println("ERR Unknown config key. Valid: frameDelay, walkCycles, motorCurrentDelay, faceFps");
    return;
  }

  sesame_config_set_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_CONFIG_SET, nextMsgId());
  msg.key = configKey;
  msg.value = value;
  
  if (sendEspNow((uint8_t*)&msg, sizeof(msg))) {
    Serial.println("OK SENT");
  }
}

void sendConfigGet() {
  sesame_msg_header_t msg;
  sesame_msg_init(&msg, MSG_CONFIG_GET, nextMsgId());
  pendingResponse = true;
  pendingSentTime = millis();
  sendEspNow((uint8_t*)&msg, sizeof(msg));
}

void sendStatusGet() {
  sesame_msg_header_t msg;
  sesame_msg_init(&msg, MSG_STATUS_GET, nextMsgId());
  pendingResponse = true;
  pendingSentTime = millis();
  sendEspNow((uint8_t*)&msg, sizeof(msg));
}

void sendPing() {
  sesame_ping_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_PING, nextMsgId());
  msg.timestamp_us = micros();
  pingSentTimestamp_us = msg.timestamp_us;
  pendingResponse = true;
  pendingSentTime = millis();
  sendEspNow((uint8_t*)&msg, sizeof(msg));
}

// ============================================================================
// Helpers
// ============================================================================

bool sendEspNow(const uint8_t* data, size_t len) {
  if (!peerRegistered) {
    Serial.println("ERR Robot peer not registered. Set MAC with SET_ROBOT_MAC.");
    return false;
  }
  
  esp_err_t result = esp_now_send(robotAddress, data, len);
  if (result != ESP_OK) {
    Serial.printf("ERR esp_now_send failed: %d\n", result);
    return false;
  }
  
  // Track for ACK/response timeout
  const sesame_msg_header_t* hdr = (const sesame_msg_header_t*)data;
  pendingMsgId = hdr->msg_id;
  pendingMsgType = hdr->msg_type;
  pendingSentTime = millis();
  
  return true;
}

uint8_t nextMsgId() {
  return msgIdCounter++;
}

uint8_t configKeyFromString(const char* key) {
  // Case-insensitive comparison
  char lower[32];
  strncpy(lower, key, 31);
  lower[31] = '\0';
  for (char* p = lower; *p; p++) *p = tolower(*p);
  
  if (strcmp(lower, "framedelay") == 0)          return CONFIG_KEY_FRAME_DELAY;
  if (strcmp(lower, "walkcycles") == 0)          return CONFIG_KEY_WALK_CYCLES;
  if (strcmp(lower, "motorcurrentdelay") == 0)   return CONFIG_KEY_MOTOR_CURRENT_DLY;
  if (strcmp(lower, "facefps") == 0)             return CONFIG_KEY_FACE_FPS;
  if (strcmp(lower, "sensormode") == 0)          return CONFIG_KEY_SENSOR_MODE;
  if (strcmp(lower, "sensordelay") == 0 || strcmp(lower, "sensorrate") == 0) return CONFIG_KEY_SENSOR_RATE;
  return 0;
}

void printHelp() {
  Serial.println();
  Serial.println("=== Sesame Robot Controller - Commands ===");
  Serial.println();
  Serial.println("Motion:");
  Serial.println("  CMD <motion>            Play motion sequence");
  Serial.println("    Motions: forward, backward, left, right, rest, stand,");
  Serial.println("             wave, dance, swim, point, pushup, bow, cute,");
  Serial.println("             freaky, worm, shake, shrug, dead, crab");
  Serial.println("  STOP                    Stop current motion");
  Serial.println();
  Serial.println("Face:");
  Serial.println("  FACE <name>             Set face expression");
  Serial.println("    Faces: default, idle, walk, rest, stand, happy, sad,");
  Serial.println("           angry, surprised, sleepy, love, excited,");
  Serial.println("           confused, thinking, talk_happy, talk_sad, ...");
  Serial.println();
  Serial.println("Servos:");
  Serial.println("  SERVO <ch> <angle>      Set servo (ch: 0-7, angle: 0-180)");
  Serial.println("  SERVO_ALL <angle>       Set all servos to angle");
  Serial.println("    Channels: 0=R1, 1=R2, 2=L1, 3=L2, 4=R4, 5=R3, 6=L3, 7=L4");
  Serial.println();
  Serial.println("Trimming:");
  Serial.println("  TRIM_SET <ch> <val>     Set subtrim (ch: 0-7, val: -90..90)");
  Serial.println("  TRIM_GET                Get all subtrim values");
  Serial.println("  TRIM_RESET              Reset all trims to 0");
  Serial.println();
  Serial.println("Config:");
  Serial.println("  CONFIG_SET <key> <val>  Set config parameter");
  Serial.println("    Keys: frameDelay, walkCycles, motorCurrentDelay, faceFps");
  Serial.println("          sensorMode, sensorRate");
  Serial.println("  CONFIG_GET              Get all config values");
  Serial.println();
  Serial.println("System:");
  Serial.println("  STATUS                  Get robot status");
  Serial.println("  PING                    Measure round-trip latency");
  Serial.println("  MAC                     Show controller MAC address");
  Serial.println("  CHANNEL [n]             Get/set ESP-NOW channel (1-13)");
  Serial.println("  SET_ROBOT_MAC <mac>     Set robot MAC (aa:bb:cc:dd:ee:ff)");
  Serial.println("  HELP                    Show this help");
  Serial.println();
}
