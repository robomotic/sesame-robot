/*
 * espnow-protocol.h
 * 
 * Shared ESP-NOW protocol definitions for the Sesame Robot system.
 * Include this file in both the ESP32-C3 Controller and the ESP32-S3 Robot firmware.
 *
 * Message format: [1 byte: msg_type] [1 byte: msg_id] [N bytes: payload]
 * Max ESP-NOW payload: 250 bytes
 *
 * Architecture:
 *   Computer/RPi  <--USB Serial-->  ESP32-C3 Controller  <--ESP-NOW-->  ESP32-S3 Robot
 */

#pragma once

#include <Arduino.h>

// ============================================================================
// Protocol Version
// ============================================================================
#define SESAME_PROTOCOL_VERSION  1

// ============================================================================
// ESP-NOW Configuration
// ============================================================================
// Choose a channel different from your WiFi router (1, 6, or 11 recommended)
#define ESPNOW_CHANNEL  1

// ============================================================================
// Message Type Constants
// ============================================================================

// --- Command Messages (Controller -> Robot) ---
#define MSG_MOTION_CMD    0x01  // Play a motion sequence
#define MSG_FACE_CMD      0x02  // Set face expression
#define MSG_STOP_CMD      0x03  // Stop current motion
#define MSG_SERVO_SET     0x04  // Set individual servo angle
#define MSG_SERVO_ALL     0x05  // Set all servos to one angle
#define MSG_TRIM_SET      0x06  // Set subtrim for one servo
#define MSG_TRIM_GET      0x07  // Request subtrim values
#define MSG_TRIM_RESET    0x08  // Reset all subtrims to 0
#define MSG_CONFIG_SET    0x09  // Set a config parameter
#define MSG_CONFIG_GET    0x0A  // Request config values
#define MSG_STATUS_GET    0x0B  // Request robot status
#define MSG_PING          0x0C  // Latency measurement

// --- Response Messages (Robot -> Controller) ---
#define MSG_ACK           0x80  // Acknowledgement
#define MSG_TRIM_DATA     0x81  // Subtrim values response
#define MSG_CONFIG_DATA   0x82  // Config values response
#define MSG_STATUS_DATA   0x83  // Status response
#define MSG_PONG          0x84  // Ping response
#define MSG_SENSOR_DATA   0x85  // Future: MPU6050 data

// ============================================================================
// ACK Status Codes
// ============================================================================
#define ACK_OK            0x00
#define ACK_ERR           0x01
#define ACK_ERR_UNKNOWN   0x02  // Unknown command
#define ACK_ERR_INVALID   0x03  // Invalid parameters

// ============================================================================
// Config Keys (for MSG_CONFIG_SET)
// ============================================================================
#define CONFIG_KEY_FRAME_DELAY        0x01
#define CONFIG_KEY_WALK_CYCLES        0x02
#define CONFIG_KEY_MOTOR_CURRENT_DLY  0x03
#define CONFIG_KEY_FACE_FPS           0x04
#define CONFIG_KEY_SENSOR_MODE        0x05  // 0=off, 1=accel, 2=gyro, 3=magneto, 4=rpy
#define CONFIG_KEY_SENSOR_RATE        0x06  // ms between sensor updates (0=off)

// ============================================================================
// Payload Size Limits
// ============================================================================
#define MAX_CMD_STRING_LEN   16  // Max motion command string length (incl. null)
#define MAX_FACE_STRING_LEN  24  // Max face name string length (incl. null)
#define MAX_STATUS_CMD_LEN   16  // Max command name in status response
#define MAX_STATUS_FACE_LEN  24  // Max face name in status response
#define NUM_SERVOS            8  // Number of servo channels

// ============================================================================
// Message Header
// ============================================================================
typedef struct __attribute__((packed)) {
  uint8_t msg_type;
  uint8_t msg_id;
} sesame_msg_header_t;

// ============================================================================
// Command Payloads (Controller -> Robot)
// ============================================================================

// MSG_MOTION_CMD (0x01) - Play a named motion
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  char command[MAX_CMD_STRING_LEN];  // Null-terminated string
} sesame_motion_cmd_t;

// MSG_FACE_CMD (0x02) - Set face expression
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  char face[MAX_FACE_STRING_LEN];    // Null-terminated string
} sesame_face_cmd_t;

// MSG_STOP_CMD (0x03) - Stop current motion (header only, no payload)
// Just send sesame_msg_header_t

// MSG_SERVO_SET (0x04) - Set one servo angle
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint8_t channel;   // 0-7
  uint8_t angle;     // 0-180
} sesame_servo_set_t;

// MSG_SERVO_ALL (0x05) - Set all servos to same angle
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint8_t angle;     // 0-180
} sesame_servo_all_t;

// MSG_TRIM_SET (0x06) - Set subtrim for one servo
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint8_t channel;   // 0-7
  int8_t  value;     // -90 to +90
} sesame_trim_set_t;

// MSG_TRIM_GET (0x07) - Request subtrim values (header only)
// MSG_TRIM_RESET (0x08) - Reset all trims (header only)
// Just send sesame_msg_header_t

// MSG_CONFIG_SET (0x09) - Set a config parameter
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint8_t key;       // CONFIG_KEY_*
  int16_t value;
} sesame_config_set_t;

// MSG_CONFIG_GET (0x0A) - Request config (header only)
// MSG_STATUS_GET (0x0B) - Request status (header only)
// Just send sesame_msg_header_t

// MSG_PING (0x0C) - Latency measurement
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint32_t timestamp_us;
} sesame_ping_t;

// ============================================================================
// Response Payloads (Robot -> Controller)
// ============================================================================

// MSG_ACK (0x80) - Command acknowledgement
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint8_t orig_msg_id;  // ID of the message being acknowledged
  uint8_t status;        // ACK_OK, ACK_ERR, etc.
} sesame_ack_t;

// MSG_TRIM_DATA (0x81) - Subtrim values
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  int8_t subtrim[NUM_SERVOS];  // 8 subtrim values
} sesame_trim_data_t;

// MSG_CONFIG_DATA (0x82) - All config values
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  int16_t frameDelay;
  int16_t walkCycles;
  int16_t motorCurrentDelay;
  int16_t faceFps;
  int16_t sensorMode;      // 0=off, 1=accel, 2=gyro, 3=magneto, 4=rpy
  int16_t sensorRate;      // ms between sensor updates (0=off)
} sesame_config_data_t;

// MSG_STATUS_DATA (0x83) - Robot status
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  char currentCommand[MAX_STATUS_CMD_LEN];   // Null-terminated
  char currentFace[MAX_STATUS_FACE_LEN];     // Null-terminated
} sesame_status_data_t;

// MSG_PONG (0x84) - Ping response
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint32_t orig_timestamp_us;  // Original timestamp from ping
  uint32_t robot_timestamp_us; // Robot's timestamp when it received ping
} sesame_pong_t;

// MSG_SENSOR_DATA (0x85) - MPU6050 sensor data
// mode: 0=off, 1=accel, 2=gyro, 3=magneto, 4=rpy
typedef struct __attribute__((packed)) {
  sesame_msg_header_t header;
  uint8_t  mode;              // Sensor mode (1=accel, 2=gyro, 3=magneto, 4=rpy)
  float    ax, ay, az;        // Acceleration (m/s^2)
  float    gx, gy, gz;        // Gyroscope (rad/s)
  float    mx, my, mz;        // Magnetometer (uT)
  float    roll, pitch, yaw;  // Orientation angles (radians)
  uint32_t timestamp_us;      // Microsecond timestamp
} sesame_sensor_data_t;

// ============================================================================
// Union for receiving any message type
// ============================================================================
typedef union {
  sesame_msg_header_t   header;
  sesame_motion_cmd_t   motion_cmd;
  sesame_face_cmd_t     face_cmd;
  sesame_servo_set_t    servo_set;
  sesame_servo_all_t    servo_all;
  sesame_trim_set_t     trim_set;
  sesame_config_set_t   config_set;
  sesame_ping_t         ping;
  sesame_ack_t          ack;
  sesame_trim_data_t    trim_data;
  sesame_config_data_t  config_data;
  sesame_status_data_t  status_data;
  sesame_pong_t         pong;
  sesame_sensor_data_t  sensor_data;
  uint8_t raw[250];       // Max ESP-NOW payload
} sesame_msg_t;

// ============================================================================
// Helper: Build message header
// ============================================================================
inline void sesame_msg_init(sesame_msg_header_t* hdr, uint8_t type, uint8_t id) {
  hdr->msg_type = type;
  hdr->msg_id = id;
}

// ============================================================================
// Helper: Get message size for a given type
// ============================================================================
inline size_t sesame_msg_size(uint8_t msg_type) {
  switch (msg_type) {
    case MSG_MOTION_CMD:   return sizeof(sesame_motion_cmd_t);
    case MSG_FACE_CMD:     return sizeof(sesame_face_cmd_t);
    case MSG_STOP_CMD:     return sizeof(sesame_msg_header_t);
    case MSG_SERVO_SET:    return sizeof(sesame_servo_set_t);
    case MSG_SERVO_ALL:    return sizeof(sesame_servo_all_t);
    case MSG_TRIM_SET:     return sizeof(sesame_trim_set_t);
    case MSG_TRIM_GET:     return sizeof(sesame_msg_header_t);
    case MSG_TRIM_RESET:   return sizeof(sesame_msg_header_t);
    case MSG_CONFIG_SET:   return sizeof(sesame_config_set_t);
    case MSG_CONFIG_GET:   return sizeof(sesame_msg_header_t);
    case MSG_STATUS_GET:   return sizeof(sesame_msg_header_t);
    case MSG_PING:         return sizeof(sesame_ping_t);
    case MSG_ACK:          return sizeof(sesame_ack_t);
    case MSG_TRIM_DATA:    return sizeof(sesame_trim_data_t);
    case MSG_CONFIG_DATA:  return sizeof(sesame_config_data_t);
    case MSG_STATUS_DATA:  return sizeof(sesame_status_data_t);
    case MSG_PONG:         return sizeof(sesame_pong_t);
    case MSG_SENSOR_DATA:  return sizeof(sesame_sensor_data_t);
    default:               return sizeof(sesame_msg_header_t);
  }
}
