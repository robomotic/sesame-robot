/*
 * Sesame Robot Firmware - ESP-NOW Version
 * 
 * Receives commands via ESP-NOW from the ESP32-C3 Sesame Controller.
 * Controls 8 servos, OLED face display, and movement sequences.
 *
 * Board: ESP32-S3 (with Sesame Distro Board)
 * Display: SSD1306 128x64 OLED via I2C
 * Communication: ESP-NOW (no WiFi AP/WebServer)
 *
 * The serial port is kept for direct debugging (type 'help' for commands).
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "face-bitmaps.h"
#include "movement-sequences.h"

// Include the shared ESP-NOW protocol definitions.
// Adjust this path if the file is located elsewhere relative to your sketch.
#include "../software/esp-now/espnow-protocol.h"

// ============================================================================
// ESP-NOW Configuration
// ============================================================================

// MAC Address of the ESP32-C3 Controller
// ** UPDATE THIS with the MAC printed by the controller at boot **
uint8_t controllerAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ============================================================================
// Display Configuration
// ============================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C

// I2C Pins for Distro Board V2
//#define I2C_SDA 8
//#define I2C_SCL 9

// I2C Pins for Distro Board
#define I2C_SDA 21
#define I2C_SCL 22

// I2C Pins for S2 Mini Board
//#define I2C_SDA 33
//#define I2C_SCL 35

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================================
// Global State for Face Animations
// ============================================================================
String currentCommand = "";
String currentFaceName = "default";
const unsigned char* const* currentFaceFrames = nullptr;
uint8_t currentFaceFrameCount = 0;
uint8_t currentFaceFrameIndex = 0;
unsigned long lastFaceFrameMs = 0;
int faceFps = 8;
FaceAnimMode currentFaceMode = FACE_ANIM_LOOP;
int8_t faceFrameDirection = 1;
bool faceAnimFinished = false;
int currentFaceFps = 0;
bool idleActive = false;
bool idleBlinkActive = false;
unsigned long nextIdleBlinkMs = 0;
uint8_t idleBlinkRepeatsLeft = 0;

// ============================================================================
// Servo Configuration
// ============================================================================
Servo servos[8];

// Sesame Distro Board V2 Pinout
//const int servoPins[8] = {4, 5, 6, 7, 15, 16, 17, 18};

// Sesame Distro Board Pinout
const int servoPins[8] = {15, 2, 23, 19, 4, 16, 17, 18};

// Lolin S2 Mini Pinout
//const int servoPins[8] = {1, 2, 4, 6, 8, 10, 13, 14};

// Subtrim values for each servo (offset in degrees)
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Track current target angle for each servo (for real-time trim updates)
int currentServoAngle[8] = {90, 90, 90, 90, 90, 90, 90, 90};

// Set to true to enable saving/loading subtrim values from NVS
#define SAVE_TRIMS false

// ============================================================================
// NVS Persistent Storage
// ============================================================================
Preferences prefs;

void saveSubtrimToNVS() {
  if (!SAVE_TRIMS) return;
  prefs.begin("sesame", false);
  prefs.putBytes("subtrim", servoSubtrim, sizeof(servoSubtrim));
  prefs.end();
  Serial.println("Subtrim values saved to NVS");
}

void loadSubtrimFromNVS() {
  if (!SAVE_TRIMS) {
    Serial.println("NVS loading disabled, using default subtrim values");
    return;
  }
  prefs.begin("sesame", true);
  size_t len = prefs.getBytesLength("subtrim");
  if (len == sizeof(servoSubtrim)) {
    prefs.getBytes("subtrim", servoSubtrim, sizeof(servoSubtrim));
    Serial.println("Subtrim values loaded from NVS:");
    for (int i = 0; i < 8; i++) {
      Serial.print("Motor "); Serial.print(i); Serial.print(": ");
      if (servoSubtrim[i] >= 0) Serial.print("+");
      Serial.println(servoSubtrim[i]);
    }
  } else {
    Serial.println("No subtrim values in NVS, using defaults");
  }
  prefs.end();
}

void applySubtrimToCurrentPose() {
  for (int i = 0; i < 8; i++) {
    int adjustedAngle = constrain(currentServoAngle[i] + servoSubtrim[i], 0, 180);
    servos[i].write(adjustedAngle);
    delay(5);
  }
}

// ============================================================================
// Animation Constants
// ============================================================================
int frameDelay = 100;
int walkCycles = 10;
int motorCurrentDelay = 40;

void saveSettingsToNVS() {
  prefs.begin("sesame", false);
  prefs.putInt("frameDelay", frameDelay);
  prefs.putInt("walkCycles", walkCycles);
  prefs.putInt("motorCurrentDelay", motorCurrentDelay);
  prefs.putInt("faceFps", faceFps);
  prefs.end();
  Serial.println("Settings saved to NVS");
}

void loadSettingsFromNVS() {
  prefs.begin("sesame", true);
  if (prefs.isKey("frameDelay")) {
    int val = prefs.getInt("frameDelay");
    if (val > 0) frameDelay = val;
  }
  if (prefs.isKey("walkCycles")) {
    int val = prefs.getInt("walkCycles");
    if (val > 0) walkCycles = val;
  }
  if (prefs.isKey("motorCurrentDelay")) {
    int val = prefs.getInt("motorCurrentDelay");
    if (val >= 0) motorCurrentDelay = val;
  }
  if (prefs.isKey("faceFps")) {
    int val = prefs.getInt("faceFps");
    if (val > 0) faceFps = val;
  }
  prefs.end();
  Serial.println("Settings loaded from NVS:");
  Serial.print("  frameDelay: "); Serial.println(frameDelay);
  Serial.print("  walkCycles: "); Serial.println(walkCycles);
  Serial.print("  motorCurrentDelay: "); Serial.println(motorCurrentDelay);
  Serial.print("  faceFps: "); Serial.println(faceFps);
}

// ============================================================================
// Face Lookup Tables
// ============================================================================
struct FaceEntry {
  const char* name;
  const unsigned char* const* frames;
  uint8_t maxFrames;
};

static const uint8_t MAX_FACE_FRAMES = 6;

#define MAKE_FACE_FRAMES(name) \
  const unsigned char* const face_##name##_frames[] = { \
    epd_bitmap_##name, epd_bitmap_##name##_1, epd_bitmap_##name##_2, \
    epd_bitmap_##name##_3, epd_bitmap_##name##_4, epd_bitmap_##name##_5 \
  };

#define X(name) MAKE_FACE_FRAMES(name)
FACE_LIST
#undef X
#undef MAKE_FACE_FRAMES

const FaceEntry faceEntries[] = {
#define X(name) { #name, face_##name##_frames, MAX_FACE_FRAMES },
  FACE_LIST
#undef X
  { "default", face_defualt_frames, MAX_FACE_FRAMES }
};

struct FaceFpsEntry {
  const char* name;
  uint8_t fps;
};

const FaceFpsEntry faceFpsEntries[] = {
  { "walk", 1 },
  { "rest", 1 },
  { "swim", 1 },
  { "dance", 1 },
  { "wave", 1 },
  { "point", 5 },
  { "stand", 1 },
  { "cute", 1 },
  { "pushup", 1 },
  { "freaky", 1 },
  { "bow", 1 },
  { "worm", 1 },
  { "shake", 1 },
  { "shrug", 1 },
  { "dead", 2 },
  { "crab", 1 },
  { "idle", 1 },
  { "idle_blink", 7 },
  { "default", 1 },
  // Conversational faces (manually controlled by Python - no auto-animation)
  { "happy", 1 },
  { "talk_happy", 1 },
  { "sad", 1 },
  { "talk_sad", 1 },
  { "angry", 1 },
  { "talk_angry", 1 },
  { "surprised", 1 },
  { "talk_surprised", 1 },
  { "sleepy", 1 },
  { "talk_sleepy", 1 },
  { "love", 1 },
  { "talk_love", 1 },
  { "excited", 1 },
  { "talk_excited", 1 },
  { "confused", 1 },
  { "talk_confused", 1 },
  { "thinking", 1 },
  { "talk_thinking", 1 },
};

// ============================================================================
// Function Prototypes
// ============================================================================
void setServoAngle(uint8_t channel, int angle);
void updateFaceBitmap(const unsigned char* bitmap);
void setFace(const String& faceName);
void setFaceMode(FaceAnimMode mode);
void setFaceWithMode(const String& faceName, FaceAnimMode mode);
void updateAnimatedFace();
void delayWithFace(unsigned long ms);
void enterIdle();
void exitIdle();
void updateIdleBlink();
int getFaceFpsForName(const String& faceName);
bool pressingCheck(String cmd, int ms);

// ESP-NOW handlers
void onEspNowRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len);
void onEspNowSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void sendAck(uint8_t origMsgId, uint8_t status);
void sendTrimData(uint8_t msgId);
void sendConfigData(uint8_t msgId);
void sendStatusData(uint8_t msgId);
void sendPong(uint8_t msgId, uint32_t origTimestamp);
bool sendToController(const uint8_t* data, size_t len);

// ============================================================================
// ESP-NOW Message Handling
// ============================================================================

// Track if controller peer is registered
static bool controllerPeerRegistered = false;

void onEspNowSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Optional: track send success/failure for debugging
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println(F("[ESP-NOW] Send failed"));
  }
}

void onEspNowRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
  if (data_len < (int)sizeof(sesame_msg_header_t)) return;

  const sesame_msg_header_t* hdr = (const sesame_msg_header_t*)data;

  // Auto-register controller peer from first received message if not set
  if (!controllerPeerRegistered && recv_info != nullptr) {
    bool isDefault = true;
    for (int i = 0; i < 6; i++) {
      if (controllerAddress[i] != 0xFF) { isDefault = false; break; }
    }
    if (isDefault) {
      memcpy(controllerAddress, recv_info->src_addr, 6);
      
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(peerInfo));
      memcpy(peerInfo.peer_addr, controllerAddress, 6);
      peerInfo.channel = ESPNOW_CHANNEL;
      peerInfo.encrypt = false;
      
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        controllerPeerRegistered = true;
        Serial.printf("[ESP-NOW] Auto-registered controller: %02X:%02X:%02X:%02X:%02X:%02X\n",
          controllerAddress[0], controllerAddress[1], controllerAddress[2],
          controllerAddress[3], controllerAddress[4], controllerAddress[5]);
      }
    }
  }

  switch (hdr->msg_type) {
    case MSG_MOTION_CMD: {
      if (data_len >= (int)sizeof(sesame_motion_cmd_t)) {
        const sesame_motion_cmd_t* cmd = (const sesame_motion_cmd_t*)data;
        String motion = String(cmd->command);
        motion.trim();
        Serial.printf("[ESP-NOW] Motion: %s\n", motion.c_str());
        currentCommand = motion;
        exitIdle();
        sendAck(hdr->msg_id, ACK_OK);
      } else {
        sendAck(hdr->msg_id, ACK_ERR_INVALID);
      }
      break;
    }

    case MSG_FACE_CMD: {
      if (data_len >= (int)sizeof(sesame_face_cmd_t)) {
        const sesame_face_cmd_t* cmd = (const sesame_face_cmd_t*)data;
        String face = String(cmd->face);
        face.trim();
        Serial.printf("[ESP-NOW] Face: %s\n", face.c_str());
        setFace(face);
        sendAck(hdr->msg_id, ACK_OK);
      } else {
        sendAck(hdr->msg_id, ACK_ERR_INVALID);
      }
      break;
    }

    case MSG_STOP_CMD: {
      Serial.println(F("[ESP-NOW] Stop"));
      currentCommand = "";
      sendAck(hdr->msg_id, ACK_OK);
      break;
    }

    case MSG_SERVO_SET: {
      if (data_len >= (int)sizeof(sesame_servo_set_t)) {
        const sesame_servo_set_t* cmd = (const sesame_servo_set_t*)data;
        if (cmd->channel < NUM_SERVOS && cmd->angle <= 180) {
          Serial.printf("[ESP-NOW] Servo %d = %d\n", cmd->channel, cmd->angle);
          setServoAngle(cmd->channel, cmd->angle);
          sendAck(hdr->msg_id, ACK_OK);
        } else {
          sendAck(hdr->msg_id, ACK_ERR_INVALID);
        }
      } else {
        sendAck(hdr->msg_id, ACK_ERR_INVALID);
      }
      break;
    }

    case MSG_SERVO_ALL: {
      if (data_len >= (int)sizeof(sesame_servo_all_t)) {
        const sesame_servo_all_t* cmd = (const sesame_servo_all_t*)data;
        if (cmd->angle <= 180) {
          Serial.printf("[ESP-NOW] All servos = %d\n", cmd->angle);
          for (int i = 0; i < NUM_SERVOS; i++) {
            setServoAngle(i, cmd->angle);
          }
          sendAck(hdr->msg_id, ACK_OK);
        } else {
          sendAck(hdr->msg_id, ACK_ERR_INVALID);
        }
      } else {
        sendAck(hdr->msg_id, ACK_ERR_INVALID);
      }
      break;
    }

    case MSG_TRIM_SET: {
      if (data_len >= (int)sizeof(sesame_trim_set_t)) {
        const sesame_trim_set_t* cmd = (const sesame_trim_set_t*)data;
        if (cmd->channel < NUM_SERVOS && cmd->value >= -90 && cmd->value <= 90) {
          Serial.printf("[ESP-NOW] Trim %d = %d\n", cmd->channel, cmd->value);
          servoSubtrim[cmd->channel] = cmd->value;
          applySubtrimToCurrentPose();
          saveSubtrimToNVS();
          sendAck(hdr->msg_id, ACK_OK);
        } else {
          sendAck(hdr->msg_id, ACK_ERR_INVALID);
        }
      } else {
        sendAck(hdr->msg_id, ACK_ERR_INVALID);
      }
      break;
    }

    case MSG_TRIM_GET: {
      Serial.println(F("[ESP-NOW] Trim get"));
      sendTrimData(hdr->msg_id);
      break;
    }

    case MSG_TRIM_RESET: {
      Serial.println(F("[ESP-NOW] Trim reset"));
      for (int i = 0; i < NUM_SERVOS; i++) servoSubtrim[i] = 0;
      applySubtrimToCurrentPose();
      saveSubtrimToNVS();
      sendAck(hdr->msg_id, ACK_OK);
      break;
    }

    case MSG_CONFIG_SET: {
      if (data_len >= (int)sizeof(sesame_config_set_t)) {
        const sesame_config_set_t* cmd = (const sesame_config_set_t*)data;
        bool valid = true;
        switch (cmd->key) {
          case CONFIG_KEY_FRAME_DELAY:
            frameDelay = cmd->value;
            Serial.printf("[ESP-NOW] Config frameDelay = %d\n", frameDelay);
            break;
          case CONFIG_KEY_WALK_CYCLES:
            walkCycles = cmd->value;
            Serial.printf("[ESP-NOW] Config walkCycles = %d\n", walkCycles);
            break;
          case CONFIG_KEY_MOTOR_CURRENT_DLY:
            motorCurrentDelay = cmd->value;
            Serial.printf("[ESP-NOW] Config motorCurrentDelay = %d\n", motorCurrentDelay);
            break;
          case CONFIG_KEY_FACE_FPS:
            faceFps = max(1, (int)cmd->value);
            Serial.printf("[ESP-NOW] Config faceFps = %d\n", faceFps);
            break;
          default:
            valid = false;
            break;
        }
        if (valid) {
          saveSettingsToNVS();
          sendAck(hdr->msg_id, ACK_OK);
        } else {
          sendAck(hdr->msg_id, ACK_ERR_UNKNOWN);
        }
      } else {
        sendAck(hdr->msg_id, ACK_ERR_INVALID);
      }
      break;
    }

    case MSG_CONFIG_GET: {
      Serial.println(F("[ESP-NOW] Config get"));
      sendConfigData(hdr->msg_id);
      break;
    }

    case MSG_STATUS_GET: {
      Serial.println(F("[ESP-NOW] Status get"));
      sendStatusData(hdr->msg_id);
      break;
    }

    case MSG_PING: {
      if (data_len >= (int)sizeof(sesame_ping_t)) {
        const sesame_ping_t* ping = (const sesame_ping_t*)data;
        sendPong(hdr->msg_id, ping->timestamp_us);
      }
      break;
    }

    default: {
      Serial.printf("[ESP-NOW] Unknown msg type: 0x%02X\n", hdr->msg_type);
      sendAck(hdr->msg_id, ACK_ERR_UNKNOWN);
      break;
    }
  }
}

// ============================================================================
// ESP-NOW Response Senders
// ============================================================================

bool sendToController(const uint8_t* data, size_t len) {
  if (!controllerPeerRegistered) return false;
  esp_err_t result = esp_now_send(controllerAddress, data, len);
  return (result == ESP_OK);
}

void sendAck(uint8_t origMsgId, uint8_t status) {
  sesame_ack_t ack;
  memset(&ack, 0, sizeof(ack));
  sesame_msg_init(&ack.header, MSG_ACK, origMsgId);
  ack.orig_msg_id = origMsgId;
  ack.status = status;
  sendToController((uint8_t*)&ack, sizeof(ack));
}

void sendTrimData(uint8_t msgId) {
  sesame_trim_data_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_TRIM_DATA, msgId);
  memcpy(msg.subtrim, servoSubtrim, NUM_SERVOS);
  sendToController((uint8_t*)&msg, sizeof(msg));
}

void sendConfigData(uint8_t msgId) {
  sesame_config_data_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_CONFIG_DATA, msgId);
  msg.frameDelay = (int16_t)frameDelay;
  msg.walkCycles = (int16_t)walkCycles;
  msg.motorCurrentDelay = (int16_t)motorCurrentDelay;
  msg.faceFps = (int16_t)faceFps;
  sendToController((uint8_t*)&msg, sizeof(msg));
}

void sendStatusData(uint8_t msgId) {
  sesame_status_data_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_STATUS_DATA, msgId);
  strncpy(msg.currentCommand, currentCommand.c_str(), MAX_STATUS_CMD_LEN - 1);
  msg.currentCommand[MAX_STATUS_CMD_LEN - 1] = '\0';
  strncpy(msg.currentFace, currentFaceName.c_str(), MAX_STATUS_FACE_LEN - 1);
  msg.currentFace[MAX_STATUS_FACE_LEN - 1] = '\0';
  sendToController((uint8_t*)&msg, sizeof(msg));
}

void sendPong(uint8_t msgId, uint32_t origTimestamp) {
  sesame_pong_t msg;
  memset(&msg, 0, sizeof(msg));
  sesame_msg_init(&msg.header, MSG_PONG, msgId);
  msg.orig_timestamp_us = origTimestamp;
  msg.robot_timestamp_us = micros();
  sendToController((uint8_t*)&msg, sizeof(msg));
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  // Load subtrim values from NVS
  loadSubtrimFromNVS();

  // Load settings from NVS
  loadSettingsFromNVS();

  // I2C Init for ESP32
  Wire.begin(I2C_SDA, I2C_SCL);

  // OLED Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("SSD1306 allocation failed."));
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("ESP-NOW Init..."));
  display.display();

  // --- ESP-NOW INITIALIZATION ---
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.println(F("\n=== Sesame Robot (ESP-NOW) ==="));
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.printf("ESP-NOW Channel: %d\n", ESPNOW_CHANNEL);
  Serial.println(F("Copy this MAC address to the ESP32-C3 Controller!"));

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ERROR: ESP-NOW init failed!"));
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("ESP-NOW INIT FAILED"));
    display.display();
    while (1) { delay(1000); }
  }

  // Register callbacks (ESP32 core 3.x API)
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSent);

  // Register controller peer if MAC is configured (not all 0xFF)
  bool macDefault = true;
  for (int i = 0; i < 6; i++) {
    if (controllerAddress[i] != 0xFF) { macDefault = false; break; }
  }

  if (!macDefault) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, controllerAddress, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      controllerPeerRegistered = true;
      Serial.printf("Controller peer registered: %02X:%02X:%02X:%02X:%02X:%02X\n",
        controllerAddress[0], controllerAddress[1], controllerAddress[2],
        controllerAddress[3], controllerAddress[4], controllerAddress[5]);
    } else {
      Serial.println(F("Failed to register controller peer"));
    }
  } else {
    Serial.println(F("Controller MAC not set - will auto-register on first message"));
  }

  // PWM Init
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 732, 2929);
  }
  delay(10);

  // Show rest face on startup without moving motors
  setFace("rest");

  // Update OLED with ready status
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("ESP-NOW Ready"));
  display.setCursor(0, 12);
  display.setTextSize(1);
  display.printf("CH: %d", ESPNOW_CHANNEL);
  display.display();
  delay(1500);

  // Show the rest face
  setFace("rest");

  Serial.println(F("\nESP-NOW initialized. Waiting for commands..."));
  Serial.println(F("Type 'help' in serial for local debug commands.\n"));
}

// ============================================================================
// Main Loop
// ============================================================================
void loop() {
  // Update face animations
  updateAnimatedFace();
  updateIdleBlink();

  // Process motion commands (set by ESP-NOW or serial CLI)
  if (currentCommand != "") {
    String cmd = currentCommand;
    if (cmd == "forward") runWalkPose();
    else if (cmd == "backward") runWalkBackward();
    else if (cmd == "left") runTurnLeft();
    else if (cmd == "right") runTurnRight();
    else if (cmd == "rest") { runRestPose(); if (currentCommand == "rest") currentCommand = ""; }
    else if (cmd == "stand") { runStandPose(1); if (currentCommand == "stand") currentCommand = ""; }
    else if (cmd == "wave") runWavePose();
    else if (cmd == "dance") runDancePose();
    else if (cmd == "swim") runSwimPose();
    else if (cmd == "point") runPointPose();
    else if (cmd == "pushup") runPushupPose();
    else if (cmd == "bow") runBowPose();
    else if (cmd == "cute") runCutePose();
    else if (cmd == "freaky") runFreakyPose();
    else if (cmd == "worm") runWormPose();
    else if (cmd == "shake") runShakePose();
    else if (cmd == "shrug") runShrugPose();
    else if (cmd == "dead") runDeadPose();
    else if (cmd == "crab") runCrabPose();
    else {
      Serial.printf("Unknown command: %s\n", cmd.c_str());
      currentCommand = "";
    }
  }

  // Serial CLI for debugging
  if (Serial.available()) {
    static char command_buffer[32];
    static byte buffer_pos = 0;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (buffer_pos > 0) {
        command_buffer[buffer_pos] = '\0';
        int motorNum, angle;
        if (strcmp(command_buffer, "help") == 0) {
          Serial.println(F("\n=== Serial Debug Commands ==="));
          Serial.println(F("  <motor 0-7> <angle 0-180>  Set servo"));
          Serial.println(F("  all <angle>                Set all servos"));
          Serial.println(F("  rn wf/wb/tl/tr             Walk/turn"));
          Serial.println(F("  rn rs/st/wv/dn/sw/pt       Poses"));
          Serial.println(F("  rn pu/bw/ct/fk/wm/sk       Poses"));
          Serial.println(F("  rn sg/dd/cb                 Poses"));
          Serial.println(F("  subtrim / st                Show trims"));
          Serial.println(F("  st <motor> <value>          Set trim"));
          Serial.println(F("  st reset                    Reset trims"));
          Serial.println(F("  st save                     Save trims"));
          Serial.println(F("  settings reset              Reset settings"));
          Serial.println(F("  mac                         Show MAC address"));
          Serial.println(F("  help                        This help\n"));
        }
        else if (strcmp(command_buffer, "mac") == 0) {
          Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
          Serial.printf("Channel: %d\n", ESPNOW_CHANNEL);
        }
        else if(strcmp(command_buffer, "run walk") == 0 || strcmp(command_buffer, "rn wf") == 0) { currentCommand = "forward"; runWalkPose(); currentCommand = ""; }
        else if(strcmp(command_buffer, "rn wb") == 0) { currentCommand = "backward"; runWalkBackward(); currentCommand = ""; }
        else if(strcmp(command_buffer, "rn tl") == 0) { currentCommand = "left"; runTurnLeft(); currentCommand = ""; }
        else if(strcmp(command_buffer, "rn tr") == 0) { currentCommand = "right"; runTurnRight(); currentCommand = ""; }
        else if(strcmp(command_buffer, "run rest") == 0 || strcmp(command_buffer, "rn rs") == 0) runRestPose();
        else if(strcmp(command_buffer, "run stand") == 0 || strcmp(command_buffer, "rn st") == 0) runStandPose(1);
        else if(strcmp(command_buffer, "rn wv") == 0) { currentCommand = "wave"; runWavePose(); }
        else if(strcmp(command_buffer, "rn dn") == 0) { currentCommand = "dance"; runDancePose(); }
        else if(strcmp(command_buffer, "rn sw") == 0) { currentCommand = "swim"; runSwimPose(); }
        else if(strcmp(command_buffer, "rn pt") == 0) { currentCommand = "point"; runPointPose(); }
        else if(strcmp(command_buffer, "rn pu") == 0) { currentCommand = "pushup"; runPushupPose(); }
        else if(strcmp(command_buffer, "rn bw") == 0) { currentCommand = "bow"; runBowPose(); }
        else if(strcmp(command_buffer, "rn ct") == 0) { currentCommand = "cute"; runCutePose(); }
        else if(strcmp(command_buffer, "rn fk") == 0) { currentCommand = "freaky"; runFreakyPose(); }
        else if(strcmp(command_buffer, "rn wm") == 0) { currentCommand = "worm"; runWormPose(); }
        else if(strcmp(command_buffer, "rn sk") == 0) { currentCommand = "shake"; runShakePose(); }
        else if(strcmp(command_buffer, "rn sg") == 0) { currentCommand = "shrug"; runShrugPose(); }
        else if(strcmp(command_buffer, "rn dd") == 0) { currentCommand = "dead"; runDeadPose(); }
        else if(strcmp(command_buffer, "rn cb") == 0) { currentCommand = "crab"; runCrabPose(); }
        else if (strcmp(command_buffer, "subtrim") == 0 || strcmp(command_buffer, "st") == 0) {
          Serial.println("Subtrim values:");
          for (int i = 0; i < 8; i++) {
            Serial.print("Motor "); Serial.print(i); Serial.print(": ");
            if (servoSubtrim[i] >= 0) Serial.print("+");
            Serial.println(servoSubtrim[i]);
          }
        }
        else if (strcmp(command_buffer, "subtrim save") == 0 || strcmp(command_buffer, "st save") == 0) {
          saveSubtrimToNVS();
          Serial.println("Subtrim values saved to NVS");
        }
        else if (strncmp(command_buffer, "subtrim reset", 13) == 0 || strncmp(command_buffer, "st reset", 8) == 0) {
          for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
          applySubtrimToCurrentPose();
          saveSubtrimToNVS();
          Serial.println("All subtrim values reset to 0 and saved to NVS");
        }
        else if (strcmp(command_buffer, "settings reset") == 0) {
          frameDelay = 100;
          walkCycles = 10;
          motorCurrentDelay = 40;
          faceFps = 8;
          saveSettingsToNVS();
          Serial.println("Settings reset to defaults and saved to NVS");
        }
        else if (strncmp(command_buffer, "subtrim ", 8) == 0 || strncmp(command_buffer, "st ", 3) == 0) {
          const char* params = (command_buffer[1] == 't') ? command_buffer + 3 : command_buffer + 8;
          int trimMotor, trimValue;
          if (sscanf(params, "%d %d", &trimMotor, &trimValue) == 2) {
            if (trimMotor >= 0 && trimMotor < 8) {
              if (trimValue >= -90 && trimValue <= 90) {
                servoSubtrim[trimMotor] = trimValue;
                applySubtrimToCurrentPose();
                saveSubtrimToNVS();
                Serial.print("Motor "); Serial.print(trimMotor); Serial.print(" subtrim set to ");
                if (trimValue >= 0) Serial.print("+");
                Serial.println(trimValue);
              } else {
                Serial.println("Subtrim value must be between -90 and +90");
              }
            } else {
              Serial.println("Invalid motor number (0-7)");
            }
          }
        }
        else if (strncmp(command_buffer, "all ", 4) == 0) {
             if (sscanf(command_buffer + 4, "%d", &angle) == 1) {
                 for (int i = 0; i < 8; i++) setServoAngle(i, angle);
                 Serial.print("All servos set to "); Serial.println(angle);
             }
        }
        else if (sscanf(command_buffer, "%d %d", &motorNum, &angle) == 2) {
             if (motorNum >= 0 && motorNum < 8) {
                 setServoAngle(motorNum, angle);
                 Serial.print("Servo "); Serial.print(motorNum); Serial.print(" set to "); Serial.println(angle);
             } else {
                 Serial.println("Invalid motor number (0-7)");
             }
        }
        buffer_pos = 0;
      }
    } else if (buffer_pos < sizeof(command_buffer) - 1) {
      command_buffer[buffer_pos++] = c;
    }
  }
}

// ============================================================================
// Face Display Functions
// ============================================================================

void updateFaceBitmap(const unsigned char* bitmap) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmap, 128, 64, SSD1306_WHITE);
  display.display();
}

uint8_t countFrames(const unsigned char* const* frames, uint8_t maxFrames) {
  if (frames == nullptr || frames[0] == nullptr) return 0;
  uint8_t count = 0;
  for (uint8_t i = 0; i < maxFrames; i++) {
    if (frames[i] == nullptr) break;
    count++;
  }
  return count;
}

void setFace(const String& faceName) {
  if (faceName == currentFaceName && currentFaceFrames != nullptr) return;

  currentFaceName = faceName;
  currentFaceFrameIndex = 0;
  lastFaceFrameMs = 0;
  faceFrameDirection = 1;
  faceAnimFinished = false;
  currentFaceFps = getFaceFpsForName(faceName);

  currentFaceFrames = face_defualt_frames;
  currentFaceFrameCount = countFrames(face_defualt_frames, MAX_FACE_FRAMES);

  for (size_t i = 0; i < (sizeof(faceEntries) / sizeof(faceEntries[0])); i++) {
    if (faceName.equalsIgnoreCase(faceEntries[i].name)) {
      currentFaceFrames = faceEntries[i].frames;
      currentFaceFrameCount = countFrames(faceEntries[i].frames, faceEntries[i].maxFrames);
      break;
    }
  }

  if (currentFaceFrameCount == 0) {
    currentFaceFrames = face_defualt_frames;
    currentFaceFrameCount = countFrames(face_defualt_frames, MAX_FACE_FRAMES);
    currentFaceName = "default";
    currentFaceFps = getFaceFpsForName(currentFaceName);
  }

  if (currentFaceFrameCount > 0 && currentFaceFrames[0] != nullptr) {
    updateFaceBitmap(currentFaceFrames[0]);
  }
}

void setFaceMode(FaceAnimMode mode) {
  currentFaceMode = mode;
  faceFrameDirection = 1;
  faceAnimFinished = false;
}

void setFaceWithMode(const String& faceName, FaceAnimMode mode) {
  setFaceMode(mode);
  setFace(faceName);
}

int getFaceFpsForName(const String& faceName) {
  for (size_t i = 0; i < (sizeof(faceFpsEntries) / sizeof(faceFpsEntries[0])); i++) {
    if (faceName.equalsIgnoreCase(faceFpsEntries[i].name)) {
      return faceFpsEntries[i].fps;
    }
  }
  return faceFps;
}

void updateAnimatedFace() {
  if (currentFaceFrames == nullptr || currentFaceFrameCount <= 1) return;
  if (currentFaceMode == FACE_ANIM_ONCE && faceAnimFinished) return;

  unsigned long now = millis();
  int fps = max(1, (currentFaceFps > 0 ? currentFaceFps : faceFps));
  unsigned long interval = 1000UL / fps;
  if (now - lastFaceFrameMs >= interval) {
    lastFaceFrameMs = now;
    if (currentFaceMode == FACE_ANIM_LOOP) {
      currentFaceFrameIndex = (currentFaceFrameIndex + 1) % currentFaceFrameCount;
    } else if (currentFaceMode == FACE_ANIM_ONCE) {
      if (currentFaceFrameIndex + 1 >= currentFaceFrameCount) {
        currentFaceFrameIndex = currentFaceFrameCount - 1;
        faceAnimFinished = true;
      } else {
        currentFaceFrameIndex++;
      }
    } else {
      if (faceFrameDirection > 0) {
        if (currentFaceFrameIndex + 1 >= currentFaceFrameCount) {
          faceFrameDirection = -1;
          if (currentFaceFrameIndex > 0) currentFaceFrameIndex--;
        } else {
          currentFaceFrameIndex++;
        }
      } else {
        if (currentFaceFrameIndex == 0) {
          faceFrameDirection = 1;
          if (currentFaceFrameCount > 1) currentFaceFrameIndex++;
        } else {
          currentFaceFrameIndex--;
        }
      }
    }
    updateFaceBitmap(currentFaceFrames[currentFaceFrameIndex]);
  }
}

void delayWithFace(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    updateAnimatedFace();
    delay(5);
  }
}

void scheduleNextIdleBlink(unsigned long minMs, unsigned long maxMs) {
  unsigned long now = millis();
  unsigned long interval = (unsigned long)random(minMs, maxMs);
  nextIdleBlinkMs = now + interval;
}

void enterIdle() {
  idleActive = true;
  idleBlinkActive = false;
  idleBlinkRepeatsLeft = 0;
  setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
  scheduleNextIdleBlink(3000, 7000);
}

void exitIdle() {
  idleActive = false;
  idleBlinkActive = false;
}

void updateIdleBlink() {
  if (!idleActive) return;

  if (!idleBlinkActive) {
    if (millis() >= nextIdleBlinkMs) {
      idleBlinkActive = true;
      if (idleBlinkRepeatsLeft == 0 && random(0, 100) < 30) {
        idleBlinkRepeatsLeft = 1; // double blink
      }
      setFaceWithMode("idle_blink", FACE_ANIM_ONCE);
    }
    return;
  }

  if (currentFaceMode == FACE_ANIM_ONCE && faceAnimFinished) {
    idleBlinkActive = false;
    setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
    if (idleBlinkRepeatsLeft > 0) {
      idleBlinkRepeatsLeft--;
      scheduleNextIdleBlink(120, 220);
    } else {
      scheduleNextIdleBlink(3000, 7000);
    }
  }
}

// ============================================================================
// Servo Helpers
// ============================================================================

void setServoAngle(uint8_t channel, int angle) {
  if (channel < 8) {
    currentServoAngle[channel] = angle;
    int adjustedAngle = constrain(angle + servoSubtrim[channel], 0, 180);
    servos[channel].write(adjustedAngle);
    delayWithFace(motorCurrentDelay);
  }
}

bool pressingCheck(String cmd, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    updateAnimatedFace();
    if (currentCommand != cmd) {
      runStandPose(1);
      return false;
    }
    yield();
  }
  return true;
}
