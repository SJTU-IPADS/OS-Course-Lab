/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#pragma once
#include <chcore/type.h>
#include <dev_messenger.h>

#define RPLIDAR_CMD_SYNC_BYTE       0xA5
#define RPLIDAR_CMDFLAG_HAS_PAYLOAD 0x80

#define RPLIDAR_ANS_SYNC_BYTE1 0xA5
#define RPLIDAR_ANS_SYNC_BYTE2 0x5A

#define RPLIDAR_ANS_PKTFLAG_LOOP 0x1

#define RPLIDAR_ANS_HEADER_SIZE_MASK     0x3FFFFFFF
#define RPLIDAR_ANS_HEADER_SUBTYPE_SHIFT (30)

// Response
// ------------------------------------------
#define RPLIDAR_ANS_TYPE_DEVINFO   0x4
#define RPLIDAR_ANS_TYPE_DEVHEALTH 0x6

#define RPLIDAR_ANS_TYPE_MEASUREMENT 0x81
// Added in FW ver 1.17
#define RPLIDAR_ANS_TYPE_MEASUREMENT_CAPSULED 0x82
#define RPLIDAR_ANS_TYPE_MEASUREMENT_HQ       0x83

// Added in FW ver 1.17
#define RPLIDAR_ANS_TYPE_SAMPLE_RATE 0x15
// added in FW ver 1.23alpha
#define RPLIDAR_ANS_TYPE_MEASUREMENT_CAPSULED_ULTRA 0x84
// added in FW ver 1.24
#define RPLIDAR_ANS_TYPE_GET_LIDAR_CONF             0x20
#define RPLIDAR_ANS_TYPE_SET_LIDAR_CONF             0x21
#define RPLIDAR_ANS_TYPE_MEASUREMENT_DENSE_CAPSULED 0x85
#define RPLIDAR_ANS_TYPE_ACC_BOARD_FLAG             0xFF

#define RPLIDAR_RESP_ACC_BOARD_FLAG_MOTOR_CTRL_SUPPORT_MASK (0x1)

// Commands
//-----------------------------------------

// Commands without payload and response
#define RPLIDAR_CMD_STOP       0x25
#define RPLIDAR_CMD_SCAN       0x20
#define RPLIDAR_CMD_FORCE_SCAN 0x21
#define RPLIDAR_CMD_RESET      0x40

// Commands without payload but have response
#define RPLIDAR_CMD_GET_DEVICE_INFO   0x50
#define RPLIDAR_CMD_GET_DEVICE_HEALTH 0x52

#define RPLIDAR_CMD_GET_SAMPLERATE 0x59 // added in fw 1.17

#define RPLIDAR_CMD_HQ_MOTOR_SPEED_CTRL 0xA8

// Commands with payload and have response
#define RPLIDAR_CMD_EXPRESS_SCAN   0x82 // added in fw 1.17
#define RPLIDAR_CMD_HQ_SCAN        0x83 // added in fw 1.24
#define RPLIDAR_CMD_GET_LIDAR_CONF 0x84 // added in fw 1.24
#define RPLIDAR_CMD_SET_LIDAR_CONF 0x85 // added in fw 1.24
// add for A2 to set RPLIDAR motor pwm when using accessory board
#define RPLIDAR_CMD_SET_MOTOR_PWM      0xF0
#define RPLIDAR_CMD_GET_ACC_BOARD_FLAG 0xFF

#define RPLIDAR_STATUS_OK      0x0
#define RPLIDAR_STATUS_WARNING 0x1
#define RPLIDAR_STATUS_ERROR   0x2

#define RPLIDAR_CONF_SCAN_COMMAND_STD         0
#define RPLIDAR_CONF_SCAN_COMMAND_EXPRESS     1
#define RPLIDAR_CONF_SCAN_COMMAND_HQ          2
#define RPLIDAR_CONF_SCAN_COMMAND_BOOST       3
#define RPLIDAR_CONF_SCAN_COMMAND_STABILITY   4
#define RPLIDAR_CONF_SCAN_COMMAND_SENSITIVITY 5

#define RPLIDAR_CONF_ANGLE_RANGE         0x00000000
#define RPLIDAR_CONF_DESIRED_ROT_FREQ    0x00000001
#define RPLIDAR_CONF_SCAN_COMMAND_BITMAP 0x00000002
#define RPLIDAR_CONF_MIN_ROT_FREQ        0x00000004
#define RPLIDAR_CONF_MAX_ROT_FREQ        0x00000005
#define RPLIDAR_CONF_MAX_DISTANCE        0x00000060

#define RPLIDAR_CONF_SCAN_MODE_COUNT            0x00000070
#define RPLIDAR_CONF_SCAN_MODE_US_PER_SAMPLE    0x00000071
#define RPLIDAR_CONF_SCAN_MODE_MAX_DISTANCE     0x00000074
#define RPLIDAR_CONF_SCAN_MODE_ANS_TYPE         0x00000075
#define RPLIDAR_CONF_SCAN_MODE_TYPICAL          0x0000007C
#define RPLIDAR_CONF_SCAN_MODE_NAME             0x0000007F
#define RPLIDAR_EXPRESS_SCAN_STABILITY_BITMAP   4
#define RPLIDAR_EXPRESS_SCAN_SENSITIVITY_BITMAP 5

#define MAX_SCAN_NODES 8192

#define RPLIDAR_RESP_MEASUREMENT_CHECKBIT    (0x1 << 0)
#define RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT 1

#define RPLIDAR_RESP_MEASUREMENT_SYNCBIT       (0x1 << 0)
#define RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT 2

#define PACKED           __attribute__((packed))
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))

typedef struct _rplidar_cmd_packet_t {
        u8 syncByte; // must be RPLIDAR_CMD_SYNC_BYTE
        u8 cmd_flag;
        u8 size;
        u8 data[0];
} PACKED rplidar_cmd_packet_t;

typedef struct _rplidar_ans_header_t {
        u8 syncByte1; // must be RPLIDAR_ANS_SYNC_BYTE1
        u8 syncByte2; // must be RPLIDAR_ANS_SYNC_BYTE2
        u32 size_q30_subtype; // see _u32 size:30; _u32 subType:2;
        u8 type;
} PACKED rplidar_ans_header_t;

typedef struct _rplidar_response_device_info_t {
        u8 model;
        u16 firmware_version;
        u8 hardware_version;
        u8 serialnum[16];
} PACKED rplidar_response_device_info_t;

typedef struct _rplidar_payload_get_scan_conf_t {
        u32 type;
        u8 reserved[32];
} PACKED rplidar_payload_get_scan_conf_t;

typedef struct _rplidar_response_device_health_t {
        u8 status;
        u16 error_code;
} PACKED rplidar_response_device_health_t;

typedef struct _rplidar_response_measurement_node_t {
        u8 sync_quality; // syncbit:1;syncbit_inverse:1;quality:6;
        u16 angle_q6_checkbit; // check_bit:1;angle_q6:15;
        u16 distance_q2;
} PACKED rplidar_response_measurement_node_t;

typedef struct rplidar_response_measurement_node_hq_t {
        u16 angle_z_q14;
        u32 dist_mm_q2;
        u8 quality;
        u8 flag;
} PACKED rplidar_response_measurement_node_hq_t;

// helper functions
struct dev_desc *lidar_connect(const char *dev_name);
int lidar_disconnect(struct dev_desc *dev_desc);
void stop(struct dev_desc *dev_desc);
void setDTR(struct dev_desc *dev_desc);
void clearDTR(struct dev_desc *dev_desc);
int stopMotor(struct dev_desc *dev_desc);
int startMotor(struct dev_desc *dev_desc);
int sendCommand(struct dev_desc *dev_desc, u8 cmd, const void *payload,
                size_t payloadsize);
u32 checkMotorCtrlSupport(struct dev_desc *dev_desc);
int getDevInfo(struct dev_desc *dev_desc,
               rplidar_response_device_info_t *devinfo);
int getHealth(struct dev_desc *dev_desc,
              rplidar_response_device_health_t *health_info);
void plot_histogram(rplidar_response_measurement_node_t *nodes, size_t count);
int compareNode(const void *nodebuffer1, const void *nodebuffer2);
int ascendScanData(rplidar_response_measurement_node_t *nodebuffer,
                   size_t count);
int waitScanData(struct dev_desc *dev_desc,
                 rplidar_response_measurement_node_t *nodebuffer, size_t count);
int cacheScanData(struct dev_desc *dev_desc);
int startScan(struct dev_desc *dev_desc, int force);

// static inline functions
static inline u16 getDistanceQ2(const rplidar_response_measurement_node_t *node)
{
        return node->distance_q2;
}

static inline void setAngle(rplidar_response_measurement_node_t *node, float v)
{
        u16 checkbit = node->angle_q6_checkbit
                       & RPLIDAR_RESP_MEASUREMENT_CHECKBIT;
        node->angle_q6_checkbit =
                (((u16)(v * 64.0f)) << RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)
                | checkbit;
}

static inline float getAngle(const rplidar_response_measurement_node_t *node)
{
        return (node->angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)
               / 64.f;
}
