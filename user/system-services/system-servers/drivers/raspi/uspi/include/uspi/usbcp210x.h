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

#ifndef _usb_cp210x_h
#define _usb_cp210x_h
#include <uspi/usbfunction.h>
#include <uspi/usbendpoint.h>
#include <uspi/usbrequest.h>
#include <uspi/types.h>
#include <uspi/macros.h>
#include <uspi/usbserial.h>

/* Config request types */
#define REQTYPE_HOST_TO_INTERFACE 0x41
#define REQTYPE_INTERFACE_TO_HOST 0xc1
#define REQTYPE_HOST_TO_DEVICE    0x40
#define REQTYPE_DEVICE_TO_HOST    0xc0

/* Config request codes */
#define CP210X_IFC_ENABLE      0x00
#define CP210X_SET_BAUDDIV     0x01
#define CP210X_GET_BAUDDIV     0x02
#define CP210X_SET_LINE_CTL    0x03
#define CP210X_GET_LINE_CTL    0x04
#define CP210X_SET_BREAK       0x05
#define CP210X_IMM_CHAR        0x06
#define CP210X_SET_MHS         0x07
#define CP210X_GET_MDMSTS      0x08
#define CP210X_SET_XON         0x09
#define CP210X_SET_XOFF        0x0A
#define CP210X_SET_EVENTMASK   0x0B
#define CP210X_GET_EVENTMASK   0x0C
#define CP210X_SET_CHAR        0x0D
#define CP210X_GET_CHARS       0x0E
#define CP210X_GET_PROPS       0x0F
#define CP210X_GET_COMM_STATUS 0x10
#define CP210X_RESET           0x11
#define CP210X_PURGE           0x12
#define CP210X_SET_FLOW        0x13
#define CP210X_GET_FLOW        0x14
#define CP210X_EMBED_EVENTS    0x15
#define CP210X_GET_EVENTSTATE  0x16
#define CP210X_SET_CHARS       0x19
#define CP210X_GET_BAUDRATE    0x1D
#define CP210X_SET_BAUDRATE    0x1E
#define CP210X_VENDOR_SPECIFIC 0xFF

/* CP210X_IFC_ENABLE */
#define UART_ENABLE  0x0001
#define UART_DISABLE 0x0000

/* CP210X_(SET|GET)_BAUDDIV */
#define BAUD_RATE_GEN_FREQ 0x384000

/* CP210X_(SET|GET)_LINE_CTL */
#define BITS_DATA_MASK 0X0f00
#define BITS_DATA_5    0X0500
#define BITS_DATA_6    0X0600
#define BITS_DATA_7    0X0700
#define BITS_DATA_8    0X0800
#define BITS_DATA_9    0X0900

#define BITS_PARITY_MASK  0x00f0
#define BITS_PARITY_NONE  0x0000
#define BITS_PARITY_ODD   0x0010
#define BITS_PARITY_EVEN  0x0020
#define BITS_PARITY_MARK  0x0030
#define BITS_PARITY_SPACE 0x0040

#define BITS_STOP_MASK 0x000f
#define BITS_STOP_1    0x0000
#define BITS_STOP_1_5  0x0001
#define BITS_STOP_2    0x0002

/* CP210X_SET_BREAK */
#define BREAK_ON  0x0001
#define BREAK_OFF 0x0000

/* CP210X_(SET_MHS|GET_MDMSTS) */
#define CONTROL_DTR       0x0001
#define CONTROL_RTS       0x0002
#define CONTROL_CTS       0x0010
#define CONTROL_DSR       0x0020
#define CONTROL_RING      0x0040
#define CONTROL_DCD       0x0080
#define CONTROL_WRITE_DTR 0x0100
#define CONTROL_WRITE_RTS 0x0200

/* CP210X_VENDOR_SPECIFIC values */
#define CP210X_READ_2NCONFIG  0x000E
#define CP210X_READ_LATCH     0x00C2
#define CP210X_GET_PARTNUM    0x370B
#define CP210X_GET_PORTCONFIG 0x370C
#define CP210X_GET_DEVICEMODE 0x3711
#define CP210X_WRITE_LATCH    0x37E1

/* Part number definitions */
#define CP210X_PARTNUM_CP2101        0x01
#define CP210X_PARTNUM_CP2102        0x02
#define CP210X_PARTNUM_CP2103        0x03
#define CP210X_PARTNUM_CP2104        0x04
#define CP210X_PARTNUM_CP2105        0x05
#define CP210X_PARTNUM_CP2108        0x08
#define CP210X_PARTNUM_CP2102N_QFN28 0x20
#define CP210X_PARTNUM_CP2102N_QFN24 0x21
#define CP210X_PARTNUM_CP2102N_QFN20 0x22
#define CP210X_PARTNUM_UNKNOWN       0xFF

#define GENMASK(h, l)                    \
	(((~UL(0)) - (UL(1) << (l)) + 1) \
	 & (~UL(0) >> (BITS_PER_LONG - 1 - (h))))
#define BIT(nr) (UL(1) << (nr))

#define CP210X_SERIAL_DTR_MASK         GENMASK(1, 0)
#define CP210X_SERIAL_DTR_SHIFT(_mode) (_mode)
#define CP210X_SERIAL_CTS_HANDSHAKE    BIT(3)
#define CP210X_SERIAL_DSR_HANDSHAKE    BIT(4)
#define CP210X_SERIAL_DCD_HANDSHAKE    BIT(5)
#define CP210X_SERIAL_DSR_SENSITIVITY  BIT(6)

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
//added in FW ver 1.23alpha
#define RPLIDAR_ANS_TYPE_MEASUREMENT_CAPSULED_ULTRA 0x84
//added in FW ver 1.24
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

#define RPLIDAR_CMD_GET_SAMPLERATE 0x59 //added in fw 1.17

#define RPLIDAR_CMD_HQ_MOTOR_SPEED_CTRL 0xA8

// Commands with payload and have response
#define RPLIDAR_CMD_EXPRESS_SCAN   0x82 //added in fw 1.17
#define RPLIDAR_CMD_HQ_SCAN        0x83 //added in fw 1.24
#define RPLIDAR_CMD_GET_LIDAR_CONF 0x84 //added in fw 1.24
#define RPLIDAR_CMD_SET_LIDAR_CONF 0x85 //added in fw 1.24
//add for A2 to set RPLIDAR motor pwm when using accessory board
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

// /* modem lines */
// #define TIOCM_LE	0x001
// #define TIOCM_DTR	0x002
// #define TIOCM_RTS	0x004
// #define TIOCM_ST	0x008
// #define TIOCM_SR	0x010
// #define TIOCM_CTS	0x020
// #define TIOCM_CAR	0x040
// #define TIOCM_RNG	0x080
// #define TIOCM_DSR	0x100
// #define TIOCM_CD	TIOCM_CAR
// #define TIOCM_RI	TIOCM_RNG
// #define TIOCM_OUT1	0x2000
// #define TIOCM_OUT2	0x4000
// #define TIOCM_LOOP	0x8000

struct cp210x_serial_private {
	u8 partnum;
	u32 min_speed;
	u32 max_speed;
	boolean use_actual_rate;
};

struct cp210x_flow_ctl {
	u32 ulControlHandshake;
	u32 ulFlowReplace;
	u32 ulXonLimit;
	u32 ulXoffLimit;
} PACKED;

typedef struct TUSBCp210xDevice {
	TUSBFunction m_USBFunction;
	TUSBSerialDriver m_USBSerialDriver;
	TUSBEndpoint *bulk_in_ep;
	TUSBEndpoint *bulk_out_ep;
	struct cp210x_serial_private *priv_data;
	u32 c_flag;
} TUSBCp210xDevice;

typedef struct _rplidar_cmd_packet_t {
	u8 syncByte; //must be RPLIDAR_CMD_SYNC_BYTE
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

void USBCp210xDevice(TUSBCp210xDevice *pThis, TUSBFunction *pFunction);
void _CUSBCp210xDevice(TUSBCp210xDevice *pThis);
boolean USBCP210XConfigure(TUSBFunction *pUSBFunction);

#endif
