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

#include <stdlib.h>
#include <uspi/usbcp210x.h>
#include <uspi/usbhostcontroller.h>
#include <uspi/devicenameservice.h>
#include <uspi/usbconfigparser.h>
#include <uspi/assert.h>
#include <uspi/usbhid.h>
#include <uspios.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>

static const char FromCP210X[] = "CP210x";
static int demo(TUSBCp210xDevice *pThis);
static int cp210x_read_vendor_block(TUSBCp210xDevice *pThis, u8 type, u16 val,
				    void *buf, int bufsize);
static void cp210x_init_max_speed(struct cp210x_serial_private *serial);
static int cp210x_read_reg_block(TUSBCp210xDevice *pThis, u8 req, void *buf,
				 int bufsize);
//static int cp210x_read_u8_reg(TUSBCp210xDevice *pThis, u8 req, u8 *val);
static int cp210x_read_u16_reg(TUSBCp210xDevice *pThis, u8 req, u16 *val);
static int cp210x_read_u32_reg(TUSBCp210xDevice *pThis, u8 req, u32 *val);
static int cp210x_write_reg_block(TUSBCp210xDevice *pThis, u8 req, void *buf,
				  int bufsize);
static int cp210x_write_u16_reg(TUSBCp210xDevice *pThis, u8 req, u16 val);
static int cp210x_write_u32_reg(TUSBCp210xDevice *pThis, u8 req, u32 val);
static int cp210x_detect_swapped_line_ctl(TUSBCp210xDevice *pThis);
static int cp210x_open(TUSBCp210xDevice *pThis);
static void cp210x_get_termios(TUSBCp210xDevice *pThis);
static int cp210x_get_line_ctl(TUSBCp210xDevice *pThis, u16 *ctl);
static void cp210x_change_speed(TUSBCp210xDevice *pThis, u32 baud);
static int cp210x_tiocmset_port(TUSBCp210xDevice *pThis, unsigned int set,
				unsigned int clear);
static void cp210x_set_termios(TUSBCp210xDevice *pThis);
static int senddata(TUSBCp210xDevice *pThis, const u8 *data, size_t size);
static void cp210x_write_bulk_callback(TUSBRequest *pURB, void *pParam,
				       void *pContext);
static u32 checkMotorCtrlSupport(TUSBCp210xDevice *pThis);
static int sendCommand(TUSBCp210xDevice *pThis, u8 cmd, const void *payload,
		       size_t payloadsize);
//static int waitResponseHeader(TUSBCp210xDevice *pThis, rplidar_ans_header_t * header);
static int stopMotor(TUSBCp210xDevice *pThis);
static int startMotor(TUSBCp210xDevice *pThis);
static void setDTR(TUSBCp210xDevice *pThis);
static void clearDTR(TUSBCp210xDevice *pThis);
static int getDevInfo(TUSBCp210xDevice *pThis,
		      rplidar_response_device_info_t *devinfo);
static int recvdata(TUSBCp210xDevice *pThis, u8 *data, size_t size);
static int getHealth(TUSBCp210xDevice *pThis,
		     rplidar_response_device_health_t *health_info);
static int startScan(TUSBCp210xDevice *pThis, boolean force,
		     boolean useTypicalScan, u32 options);
static int getTypicalScanMode(TUSBCp210xDevice *pThis, u16 *outMode);
static int startScanExpress(TUSBCp210xDevice *pThis, boolean force,
			    u16 scanMode, u32 options);
static void stop(TUSBCp210xDevice *pThis);
static int startScanNormal(TUSBCp210xDevice *pThis, boolean force);
static int cacheScanData(TUSBCp210xDevice *pThis);
static int waitScanData(TUSBCp210xDevice *pThis,
			rplidar_response_measurement_node_t *nodebuffer,
			size_t count);
static inline u16
getDistanceQ2(const rplidar_response_measurement_node_t *node);
static inline void setAngle(rplidar_response_measurement_node_t *node, float v);
static inline float getAngle(const rplidar_response_measurement_node_t *node);
static int ascendScanData(rplidar_response_measurement_node_t *nodebuffer,
			  size_t count);
static void plot_histogram(rplidar_response_measurement_node_t *nodes,
			   size_t count);
static int cp210x_tiocmget(TUSBCp210xDevice *pThis);
static int cp210x_tiocmset(TUSBCp210xDevice *pThis, int set, int clear);

// serial driver function
static int cp210x_serial_open(void *serial_driver);
static int cp210x_serial_write(void *serial_driver, void *data, int datalen);
static int cp210x_serial_read(void *serial_driver, void *data, int datalen);
static void cp210x_serial_set_termios(void *serial_device);
static void cp210x_serial_get_termios(void *serial_device);
static int cp210x_serial_tiocmget(void *serial_device);
static int cp210x_serial_tiocmset(void *serial_device, int set, int clear);

static unsigned s_nDeviceNumber = 0;

/* USBCp210xDevice constructor */
void USBCp210xDevice(TUSBCp210xDevice *pThis, TUSBFunction *pDevice)
{
	assert(pThis != 0);

	USBFunctionCopy(&pThis->m_USBFunction, pDevice);
	pThis->m_USBFunction.Configure = USBCP210XConfigure;
	pThis->bulk_in_ep = 0;
	pThis->bulk_out_ep = 0;
	pThis->priv_data = malloc(sizeof(struct cp210x_serial_private));
	pThis->c_flag = 0;

	pThis->m_USBSerialDriver.write = cp210x_serial_write;
	pThis->m_USBSerialDriver.open = cp210x_serial_open;
	pThis->m_USBSerialDriver.read = cp210x_serial_read;
	pThis->m_USBSerialDriver.get_termios = cp210x_serial_get_termios;
	pThis->m_USBSerialDriver.set_termios = cp210x_serial_set_termios;
	pThis->m_USBSerialDriver.tiocmget = cp210x_serial_tiocmget;
	pThis->m_USBSerialDriver.tiocmset = cp210x_serial_tiocmset;
}
/* USBCp210xDevice deconstructor */
void _CUSBCp210xDevice(TUSBCp210xDevice *pThis)
{
	assert(pThis != 0);

	if (pThis->bulk_in_ep != 0) {
		_USBEndpoint(pThis->bulk_in_ep);
		free(pThis->bulk_in_ep);
		pThis->bulk_in_ep = 0;
	}

	if (pThis->bulk_out_ep != 0) {
		_USBEndpoint(pThis->bulk_out_ep);
		free(pThis->bulk_out_ep);
		pThis->bulk_out_ep = 0;
	}

	if (pThis->priv_data != 0) {
		free(pThis->priv_data);
		pThis->priv_data = 0;
	}

	_USBFunction(&pThis->m_USBFunction);
}

/* wrapper function for serial driver */
static int cp210x_serial_open(void *serial_driver)
{
	return cp210x_open((TUSBCp210xDevice *)serial_driver);
}

static int cp210x_serial_write(void *serial_driver, void *data, int datalen)
{
	return senddata((TUSBCp210xDevice *)serial_driver, data, datalen);
}

static int cp210x_serial_read(void *serial_driver, void *data, int datalen)
{
	return recvdata((TUSBCp210xDevice *)serial_driver, data, datalen);
}

static void cp210x_serial_set_termios(void *serial_device)
{
	cp210x_set_termios((TUSBCp210xDevice *)serial_device);
}

static void cp210x_serial_get_termios(void *serial_device)
{
	cp210x_get_termios((TUSBCp210xDevice *)serial_device);
}

static int cp210x_serial_tiocmget(void *serial_device)
{
	return cp210x_tiocmget((TUSBCp210xDevice *)serial_device);
}

static int cp210x_serial_tiocmset(void *serial_device, int set, int clear)
{
	return cp210x_tiocmset((TUSBCp210xDevice *)serial_device, set, clear);
}

/* Device configuration function */
boolean USBCP210XConfigure(TUSBFunction *pUSBFunction)
{
	/******************************************** Initialize ********************************************/
	printf("Hello from a cp210x device\n");

	TUSBCp210xDevice *pThis = (TUSBCp210xDevice *)pUSBFunction;
	int ret;

	if ((ret = USBFunctionGetNumEndpoints(&pThis->m_USBFunction)) < 1) {
		USBFunctionConfigurationError(&pThis->m_USBFunction,
					      FromCP210X);
		return FALSE;
	}

	TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc =
			(TUSBEndpointDescriptor *)USBFunctionGetDescriptor(
				&pThis->m_USBFunction, DESCRIPTOR_ENDPOINT))
	       != 0) {
		if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x0 // Output EP
		    && (pEndpointDesc->bmAttributes & 0x3F) == 0x02) // Bulk EP
		{
			printf("bulk out ep found\n");
			assert(pThis->bulk_out_ep == 0);
			pThis->bulk_out_ep = malloc(sizeof(TUSBEndpoint));
			assert(pThis->bulk_out_ep != 0);
			// max output size: 64
			USBEndpoint2(
				pThis->bulk_out_ep,
				USBFunctionGetDevice(&pThis->m_USBFunction),
				(TUSBEndpointDescriptor *)pEndpointDesc);
		} else if ((pEndpointDesc->bEndpointAddress & 0x80)
				   == 0x80 // Input EP
			   && (pEndpointDesc->bmAttributes & 0x3F)
				      == 0x02) // Bulk EP
		{
			printf("bulk in ep found\n");
			assert(pThis->bulk_in_ep == 0);
			pThis->bulk_in_ep = malloc(sizeof(TUSBEndpoint));
			assert(pThis->bulk_in_ep != 0);

			USBEndpoint2(
				pThis->bulk_in_ep,
				USBFunctionGetDevice(&pThis->m_USBFunction),
				(TUSBEndpointDescriptor *)pEndpointDesc);
		}
	}

	/* Make sure that all endpoints are found. */
	assert(pThis->bulk_in_ep != 0);
	assert(pThis->bulk_out_ep != 0);

	/* Get part num */
	u8 *part_num = malloc(4);
	cp210x_read_vendor_block(
		pThis, REQTYPE_DEVICE_TO_HOST, CP210X_GET_PARTNUM, part_num, 1);
	pThis->priv_data->partnum = *part_num;
	printf("part num: %x\n", *part_num);

	/* Init min and max speed */
	cp210x_init_max_speed(pThis->priv_data);

	cp210x_detect_swapped_line_ctl(pThis);

	TString DeviceName;
	String(&DeviceName);
	StringFormat(&DeviceName, "CP210X-%u", s_nDeviceNumber++);
	DeviceNameServiceAddDevice(
		DeviceNameServiceGet(),
		StringGet(&DeviceName),
		StringGet(USBDeviceGetName(pThis->m_USBFunction.m_pDevice,
					   DeviceNameVendor)),
		pThis,
		FALSE,
		USB_SERIAL);
	_String(&DeviceName);

	demo(pThis);
	return TRUE;
}

static int cp210x_read_vendor_block(TUSBCp210xDevice *pThis, u8 type, u16 val,
				    void *buf, int bufsize)
{
	int ret;

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		type,
		CP210X_VENDOR_SPECIFIC,
		val,
		0,
		buf,
		bufsize);
	assert(ret == bufsize);
	return 0;
}

static void cp210x_init_max_speed(struct cp210x_serial_private *priv)
{
	boolean use_actual_rate = FALSE;
	u32 min = 300;
	u32 max;

	switch (priv->partnum) {
	case CP210X_PARTNUM_CP2102:
		max = 1000000;
		break;
	default:
		max = 2000000;
		break;
	}

	priv->min_speed = min;
	priv->max_speed = max;
	priv->use_actual_rate = use_actual_rate;
}

/*
 * Reads a variable-sized block of CP210X_ registers, identified by req.
 * Returns data into buf in native USB byte order.
 */
static int cp210x_read_reg_block(TUSBCp210xDevice *pThis, u8 req, void *buf,
				 int bufsize)
{
	int ret;

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQTYPE_INTERFACE_TO_HOST,
		req,
		0,
		0,
		buf,
		bufsize);
	assert(ret == bufsize);
	return 0;
}

/*
 * Reads any 8-bit CP210X_ register identified by req.
 */
// static int cp210x_read_u8_reg(TUSBCp210xDevice *pThis, u8 req, u8 *val)
// {
// 	return cp210x_read_reg_block(pThis, req, val, sizeof(*val));
// }

/*
 * Reads any 16-bit CP210X_ register identified by req.
 */
static int cp210x_read_u16_reg(TUSBCp210xDevice *pThis, u8 req, u16 *val)
{
	u16 le16_val;
	int err;

	u16 *tmp_buffer = malloc(4);
	err = cp210x_read_reg_block(pThis, req, tmp_buffer, sizeof(le16_val));
	if (err)
		return err;

	*val = *tmp_buffer;

	return 0;
}

/*
 * Reads any 32-bit CP210X_ register identified by req.
 */
static int cp210x_read_u32_reg(TUSBCp210xDevice *pThis, u8 req, u32 *val)
{
	u32 le32_val;
	int err;

	err = cp210x_read_reg_block(pThis, req, &le32_val, sizeof(le32_val));
	if (err) {
		/*
		 * FIXME Some callers don't bother to check for error,
		 * at least give them consistent junk until they are fixed
		 */
		*val = 0;
		return err;
	}

	*val = le32_val;

	return 0;
}

/*
 * Writes a variable-sized block of CP210X_ registers, identified by req.
 * Data in buf must be in native USB byte order.
 */
static int cp210x_write_reg_block(TUSBCp210xDevice *pThis, u8 req, void *buf,
				  int bufsize)
{
	int ret;

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQTYPE_HOST_TO_INTERFACE,
		req,
		0,
		0,
		buf,
		bufsize);
	assert(ret == bufsize);
	return 0;
}

/*
 * Writes any 16-bit CP210X_ register (req) whose value is passed
 * entirely in the wValue field of the USB request.
 */
static int cp210x_write_u16_reg(TUSBCp210xDevice *pThis, u8 req, u16 val)
{
	int ret;

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQTYPE_HOST_TO_INTERFACE,
		req,
		val,
		0,
		NULL,
		0);
	if (ret < 0) {
		printf("failed set request 0x%x status: %d\n", req, ret);
	}

	return 0;
}

/*
 * Writes any 32-bit CP210X_ register identified by req.
 */
static int cp210x_write_u32_reg(TUSBCp210xDevice *pThis, u8 req, u32 val)
{
	u32 le32_val = val;

	return cp210x_write_reg_block(pThis, req, &le32_val, sizeof(le32_val));
}

/*
 * Detect CP2108 GET_LINE_CTL bug and activate workaround.
 * Write a known good value 0x800, read it back.
 * If it comes back swapped the bug is detected.
 * Preserve the original register value.
 */
static int cp210x_detect_swapped_line_ctl(TUSBCp210xDevice *pThis)
{
	u16 line_ctl_save = 0;
	u16 line_ctl_test = 0;
	int err;

	err = cp210x_read_u16_reg(pThis, CP210X_GET_LINE_CTL, &line_ctl_save);
	printf("line_ctl_save: %d\n", line_ctl_save);
	if (err)
		return err;

	err = cp210x_write_u16_reg(pThis, CP210X_SET_LINE_CTL, 0x800);
	if (err)
		return err;

	err = cp210x_read_u16_reg(pThis, CP210X_GET_LINE_CTL, &line_ctl_test);
	printf("line_ctl_test: %d\n", line_ctl_test);
	if (err)
		return err;

	return cp210x_write_u16_reg(pThis, CP210X_SET_LINE_CTL, line_ctl_save);
}

static int cp210x_open(TUSBCp210xDevice *pThis)
{
	int ret;
	u32 baud;

	ret = cp210x_write_u16_reg(pThis, CP210X_IFC_ENABLE, UART_ENABLE);
	if (ret) {
		printf("%s - Unable to enable UART\n", __func__);
		return ret;
	} else {
		printf("%s successfully enable UART\n", __func__);
	}

	/* Configure the termios structure */
	cp210x_get_termios(pThis);

	/* For now, just set baud rate to 115200 by default */
	cp210x_change_speed(pThis, 115200);

	cp210x_read_u32_reg(pThis, CP210X_GET_BAUDRATE, &baud);
	assert(baud == 115200);
	// return usb_serial_generic_open(tty, port);
	return 0;
}

static void cp210x_get_termios(TUSBCp210xDevice *pThis)
{
	printf("[Zixuan] cp210x_get_termios gets called\n");
	u32 baud;
	u16 bits;
	u32 cflag = 0;
	struct cp210x_flow_ctl flow_ctl;

	cp210x_read_u32_reg(pThis, CP210X_GET_BAUDRATE, &baud);
	printf("[Zixuan] - baud rate = %d\n", baud);

	cp210x_get_line_ctl(pThis, &bits);
	printf("[Zixuan] - bits = %d\n", bits);

	// Simple impletation
	cflag &= ~CSIZE;
	cflag |= CS8;
	cflag &= ~PARENB;
	cflag |= CRTSCTS;
	pThis->c_flag = cflag;

	cp210x_read_reg_block(
		pThis, CP210X_GET_FLOW, &flow_ctl, sizeof(flow_ctl));
	printf("[Zixuan] - flow_ctl.ulControlHandshake = %d\n",
	       flow_ctl.ulControlHandshake);

	/* After getting bits and flow_ctl, the driver do some stuff about cflag in Linux */
}

/*
 * Must always be called instead of cp210x_read_u16_reg(CP210X_GET_LINE_CTL)
 * to workaround cp2108 bug and get correct value.
 */
static int cp210x_get_line_ctl(TUSBCp210xDevice *pThis, u16 *ctl)
{
	int err;

	err = cp210x_read_u16_reg(pThis, CP210X_GET_LINE_CTL, ctl);
	if (err)
		return err;

	return 0;
}

static void cp210x_change_speed(TUSBCp210xDevice *pThis, u32 baud)
{
	int ret;

	ret = cp210x_write_u32_reg(pThis, CP210X_SET_BAUDRATE, baud);

	if (ret) {
		printf("failed to set baud rate to %u\n", baud);
	}
}

static int cp210x_tiocmset_port(TUSBCp210xDevice *pThis, unsigned int set,
				unsigned int clear)
{
	u16 control = 0;

	if (set & TIOCM_RTS) {
		control |= CONTROL_RTS;
		control |= CONTROL_WRITE_RTS;
	}
	if (set & TIOCM_DTR) {
		control |= CONTROL_DTR;
		control |= CONTROL_WRITE_DTR;
	}
	if (clear & TIOCM_RTS) {
		control &= ~CONTROL_RTS;
		control |= CONTROL_WRITE_RTS;
	}
	if (clear & TIOCM_DTR) {
		control &= ~CONTROL_DTR;
		control |= CONTROL_WRITE_DTR;
	}

	printf("[Zixuan] %s - control = 0x%.4x\n", __func__, control);

	return cp210x_write_u16_reg(pThis, CP210X_SET_MHS, control);
}

static void cp210x_set_termios(TUSBCp210xDevice *pThis)
{
	/*
	 * This function compares original cflag and new cflag given by
	 * user wrapped in a termios. For now, we just ignore this function
	 * to reduce complexity. 
	 */
}

static int cp210x_tiocmget(TUSBCp210xDevice *pThis)
{
	//TODO: we do not need this function for now.
	return 0;
}

static int cp210x_tiocmset(TUSBCp210xDevice *pThis, int set, int clear)
{
	return cp210x_tiocmset_port(pThis, set, clear);
}

static u32 checkMotorCtrlSupport(TUSBCp210xDevice *pThis)
{
	u32 flag = 0;
	sendCommand(pThis, RPLIDAR_CMD_GET_ACC_BOARD_FLAG, &flag, sizeof(flag));
	return 0;
}

static int sendCommand(TUSBCp210xDevice *pThis, u8 cmd, const void *payload,
		       size_t payloadsize)
{
	u8 pkt_header[10];
	rplidar_cmd_packet_t *header = (rplidar_cmd_packet_t *)pkt_header;
	u8 checksum = 0;

	if (payloadsize && payload) {
		if (payloadsize && payload) {
			cmd |= RPLIDAR_CMDFLAG_HAS_PAYLOAD;
		}
	}
	header->syncByte = RPLIDAR_CMD_SYNC_BYTE;
	header->cmd_flag = cmd;

	// send header first
	senddata(pThis, pkt_header, 2);

	// send payload if any
	if (cmd & RPLIDAR_CMDFLAG_HAS_PAYLOAD) {
		checksum ^= RPLIDAR_CMD_SYNC_BYTE;
		checksum ^= cmd;
		checksum ^= (payloadsize & 0xFF);

		// calc checksum
		for (size_t pos = 0; pos < payloadsize; ++pos) {
			checksum ^= ((u8 *)payload)[pos];
		}

		// send size
		u8 sizebyte = payloadsize;
		senddata(pThis, &sizebyte, 1);

		// send payload
		senddata(pThis, (const u8 *)payload, sizebyte);

		// send checksum
		senddata(pThis, &checksum, 1);
	}

	return 0;
}

// static int waitResponseHeader(TUSBCp210xDevice *pThis, rplidar_ans_header_t * header)
// {
// 	return recvdata(pThis, (u8 *)header, sizeof(*header));
// }

static int stopMotor(TUSBCp210xDevice *pThis)
{
	// RPLIDAR A1
	setDTR(pThis);
	return 0;
}

static int startMotor(TUSBCp210xDevice *pThis)
{
	// RPLIDAR A1
	clearDTR(pThis);
	return 0;
}

static void setDTR(TUSBCp210xDevice *pThis)
{
	cp210x_tiocmset_port(pThis, 2, 0);
}

static void clearDTR(TUSBCp210xDevice *pThis)
{
	cp210x_tiocmset_port(pThis, 0, 2);
}

static int getDevInfo(TUSBCp210xDevice *pThis,
		      rplidar_response_device_info_t *devinfo)
{
	rplidar_ans_header_t *response_header;
	u8 *buffer = malloc(32);
	u8 *dev_buffer;

	sendCommand(pThis, RPLIDAR_CMD_GET_DEVICE_INFO, NULL, 0);
	recvdata(pThis, buffer, 27);
	response_header = (rplidar_ans_header_t *)buffer;
	//waitResponseHeader(pThis, &response_header);

	// verify whether we got a correct header
	if (response_header->type != RPLIDAR_ANS_TYPE_DEVINFO) {
		printf("func: %s, line:%d get a wrong header\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	u32 header_size = (response_header->size_q30_subtype
			   & RPLIDAR_ANS_HEADER_SIZE_MASK);
	if (header_size < sizeof(rplidar_response_device_info_t)) {
		printf("func: %s, line:%d header_size < sizeof(rplidar_response_device_info_t)\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	dev_buffer = buffer + sizeof(rplidar_ans_header_t);
	for (int i = 0; i < sizeof(rplidar_response_device_info_t); ++i) {
		((u8 *)devinfo)[i] = dev_buffer[i];
	}
	free(buffer);
	return 0;
}

static int getHealth(TUSBCp210xDevice *pThis,
		     rplidar_response_device_health_t *health_info)
{
	rplidar_ans_header_t *response_header;
	u8 *buffer = malloc(16);
	u8 *health_buffer;

	sendCommand(pThis, RPLIDAR_CMD_GET_DEVICE_HEALTH, NULL, 0);
	recvdata(pThis, buffer, 10);
	response_header = (rplidar_ans_header_t *)buffer;

	// verify whether we got a correct header
	if (response_header->type != RPLIDAR_ANS_TYPE_DEVHEALTH) {
		printf("func: %s, line:%d get a wrong header\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	u32 header_size = (response_header->size_q30_subtype
			   & RPLIDAR_ANS_HEADER_SIZE_MASK);
	if (header_size < sizeof(rplidar_response_device_health_t)) {
		printf("func: %s, line:%d header_size < sizeof(rplidar_response_device_info_t)\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	health_buffer = buffer + sizeof(rplidar_ans_header_t);
	for (int i = 0; i < sizeof(rplidar_response_device_health_t); ++i) {
		((u8 *)health_info)[i] = health_buffer[i];
	}
	free(buffer);
	return 0;
}

/// Start scan
///
/// \param force            Force the core system to output scan data regardless whether the scanning motor is rotating or not.
/// \param useTypicalScan   Use lidar's typical scan mode or use the compatibility mode (2k sps)
/// \param options          Scan options (please use 0)
/// \param outUsedScanMode  The scan mode selected by lidar
static int startScan(TUSBCp210xDevice *pThis, boolean force,
		     boolean useTypicalScan, u32 options)
{
	boolean ifSupportLidarConf = TRUE;
	int ans;

	if (useTypicalScan) {
		//if support lidar config protocol
		if (ifSupportLidarConf) {
			u16 typicalMode;
			// We should get std mode.
			ans = getTypicalScanMode(pThis, &typicalMode);
			if (ans)
				return -1;

			//call startScanExpress to do the job
			return startScanExpress(pThis, FALSE, typicalMode, 0);
		}
		//if old version of triangle lidar
		else {
			// We support lidar config protocol
			assert(0);
		}
	}

	// 'useTypicalScan' is false, just use normal scan mode
	return startScanNormal(pThis, force);
}

static int startScanExpress(TUSBCp210xDevice *pThis, boolean force,
			    u16 scanMode, u32 options)
{
	stop(pThis); //force the previous operation to stop
	if (scanMode == RPLIDAR_CONF_SCAN_COMMAND_STD) {
		return startScan(pThis, force, FALSE, 0);
	}
	return 0;
}

static int startScanNormal(TUSBCp210xDevice *pThis, boolean force)
{
	u8 *buffer = malloc(64);
	memset(buffer, 0, 64);

	stop(pThis); //force the previous operation to stop

	sendCommand(pThis,
		    force ? RPLIDAR_CMD_FORCE_SCAN : RPLIDAR_CMD_SCAN,
		    NULL,
		    0);

	// waiting for confirmation
	rplidar_ans_header_t *response_header;
	recvdata(pThis, buffer, 64);

	response_header = (rplidar_ans_header_t *)buffer;

	// verify whether we got a correct header
	if (response_header->type != RPLIDAR_ANS_TYPE_MEASUREMENT) {
		printf("func: %s, line:%d get a wrong header\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	u32 header_size = (response_header->size_q30_subtype
			   & RPLIDAR_ANS_HEADER_SIZE_MASK);
	if (header_size < sizeof(rplidar_response_measurement_node_t)) {
		printf("func: %s, line:%d invalid data\n", __func__, __LINE__);
		return -1;
	}

	return cacheScanData(pThis);
}

static int cacheScanData(TUSBCp210xDevice *pThis)
{
	rplidar_response_measurement_node_t test_buf[128];
	size_t count = 360;
	rplidar_response_measurement_node_t local_buf[count];

	//rplidar_response_measurement_node_hq_t local_scan[MAX_SCAN_NODES];
	//size_t scan_count = 0;
	//int ret;
	memset(test_buf, 0, sizeof(test_buf));
	memset(local_buf, 0, sizeof(local_buf));
	//memset(local_scan, 0, sizeof(local_scan));

	waitScanData(
		pThis,
		test_buf,
		128); // always discard the first data since it may be incomplete
	waitScanData(pThis, local_buf, count);

	// for (int pos = 0; pos < 128; ++pos) {
	// 	printf("node: %d, sync_quality:%02x, angle_q6_checkbit:%04x, distance_q2:%04x\n",
	// 	pos, test_buf[pos].sync_quality, test_buf[pos].angle_q6_checkbit, test_buf[pos].distance_q2);
	// }

	// for (int pos = 0; pos < 128; ++pos) {
	// 	 printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
	//             (test_buf[pos].sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ?"S ":"  ",
	//             (test_buf[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)/64.0f,
	//             test_buf[pos].distance_q2/4.0f,
	//             test_buf[pos].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
	// }

	printf("sizeof localbuf:%d\n", sizeof(local_buf));
	for (int pos = 0; pos < count; ++pos) {
		if (getAngle(&local_buf[pos]) > 360.0f)
			printf("find a angle > 360 before ascendScanData\n");
	}
	ascendScanData(local_buf, count);
	plot_histogram(local_buf, count);
	printf("Do you want to see all the data? (y/n) ");
	//int key = getchar();
	if (/*key == 'Y' || key == 'y'*/ 1) {
		printf("\n*************************************************\n");
		for (int pos = 0; pos < count; ++pos) {
			printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
			       (local_buf[pos].sync_quality
				& RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ?
				       "S " :
				       "  ",
			       (local_buf[pos].angle_q6_checkbit
				>> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)
				       / 64.0f,
			       local_buf[pos].distance_q2 / 4.0f,
			       local_buf[pos].sync_quality
				       >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
		}
	}

	return 0;
}

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

static int compareNode(const void *nodebuffer1, const void *nodebuffer2)
{
	nodebuffer1 = (rplidar_response_measurement_node_t *)nodebuffer1;
	nodebuffer2 = (rplidar_response_measurement_node_t *)nodebuffer2;
	return (getAngle(nodebuffer1) < getAngle(nodebuffer2)) ? -1 : 1;
}

static int ascendScanData(rplidar_response_measurement_node_t *nodebuffer,
			  size_t count)
{
	//return 0;
	float inc_origin_angle = 360.0f / count;
	size_t i = 0;

	//Tune head
	for (i = 0; i < count; i++) {
		if (getDistanceQ2(&nodebuffer[i]) == 0) {
			continue;
		} else {
			while (i != 0) {
				i--;
				float expect_angle =
					getAngle(&nodebuffer[i + 1])
					- inc_origin_angle;
				if (expect_angle < 0.0f)
					expect_angle = 0.0f;
				if (expect_angle > 360.0f)
					printf("[warning] expect_angle > 360\n");
				setAngle(&nodebuffer[i], expect_angle);
			}
			break;
		}
	}

	// all the data is invalid
	if (i == count)
		return -1;

	//Tune tail
	for (i = count - 1; i >= 0; i--) {
		if (getDistanceQ2(&nodebuffer[i]) == 0) {
			continue;
		} else {
			while (i != (count - 1)) {
				i++;
				float expect_angle =
					getAngle(&nodebuffer[i - 1])
					+ inc_origin_angle;
				if (expect_angle > 360.0f)
					expect_angle -= 360.0f;
				if (expect_angle > 360.0f)
					printf("[warning] expect_angle > 360\n");
				setAngle(&nodebuffer[i], expect_angle);
			}
			break;
		}
	}

	//Fill invalid angle in the scan
	float frontAngle = getAngle(&nodebuffer[0]);
	for (i = 1; i < count; i++) {
		if (getDistanceQ2(&nodebuffer[i]) == 0) {
			float expect_angle = frontAngle + i * inc_origin_angle;
			if (expect_angle > 360.0f)
				expect_angle -= 360.0f;
			if (expect_angle > 360.0f)
				printf("[warning] expect_angle > 360\n");
			setAngle(&nodebuffer[i], expect_angle);
		}
	}

	qsort(nodebuffer,
	      count,
	      sizeof(rplidar_response_measurement_node_t),
	      compareNode);
	return 0;
}

static int waitScanData(TUSBCp210xDevice *pThis,
			rplidar_response_measurement_node_t *nodebuffer,
			size_t count)
{
	size_t recvNodeCount = 0;
	int recvSize = 0;
	u8 *recvBuffer = malloc(256);
	u8 buffer[sizeof(rplidar_response_measurement_node_t)];

	while (recvNodeCount < count) {
		recvSize = recvdata(pThis, recvBuffer, 256);

		int recvPos = 0;
		for (size_t pos = 0; pos < recvSize; ++pos) {
			u8 currentByte = recvBuffer[pos];
			switch (recvPos) {
			case 0: // expect the sync bit and its reverse in this byte
			{
				u8 tmp = (currentByte >> 1);
				if ((tmp ^ currentByte) & 0x1) {
					// pass
				} else {
					continue;
				}

			} break;
			case 1: // expect the highest bit to be 1
			{
				if (currentByte
				    & RPLIDAR_RESP_MEASUREMENT_CHECKBIT) {
					// pass
				} else {
					recvPos = 0;
					continue;
				}
			} break;
			}
			buffer[recvPos++] = currentByte;

			if (recvPos
			    == sizeof(rplidar_response_measurement_node_t)) {
				rplidar_response_measurement_node_t *node =
					(rplidar_response_measurement_node_t *)
						buffer;
				nodebuffer[recvNodeCount] = *node;
				recvNodeCount++;
				recvPos = 0;
				if (recvNodeCount == count)
					return 0;
			}
		}
	}
	return 0;
}

#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))

static void plot_histogram(rplidar_response_measurement_node_t *nodes,
			   size_t count)
{
	const int BARCOUNT = 75;
	const int MAXBARHEIGHT = 20;
	const float ANGLESCALE = 360.0f / BARCOUNT;

	float histogram[BARCOUNT];
	for (int pos = 0; pos < _countof(histogram); ++pos) {
		histogram[pos] = 0.0f;
	}

	float max_val = 0;
	for (int pos = 0; pos < (int)count; ++pos) {
		int int_deg = (int)((nodes[pos].angle_q6_checkbit
				     >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)
				    / 64.0f / ANGLESCALE);
		if (int_deg >= BARCOUNT)
			int_deg = 0;
		float cachedd = histogram[int_deg];
		if (cachedd == 0.0f) {
			cachedd = nodes[pos].distance_q2 / 4.0f;
		} else {
			cachedd = (nodes[pos].distance_q2 / 4.0f + cachedd)
				  / 2.0f;
		}

		if (cachedd > max_val)
			max_val = cachedd;
		histogram[int_deg] = cachedd;
	}

	for (int height = 0; height < MAXBARHEIGHT; ++height) {
		float threshold_h =
			(MAXBARHEIGHT - height - 1) * (max_val / MAXBARHEIGHT);
		for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
			if (histogram[xpos] >= threshold_h) {
				putc('*', stdout);
			} else {
				putc(' ', stdout);
			}
		}
		printf("\n");
	}
	for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
		putc('-', stdout);
	}
	printf("\n");
}

// static int waitNode(TUSBCp210xDevice *pThis, rplidar_response_measurement_node_t *node)
// {
// 	int recvPos = 0;
// 	u8  recvBuffer[sizeof(rplidar_response_measurement_node_t)];
// 	u8 *nodeBuffer = (u8 *)node;

// 	while(1) {
// 		size_t remainSize = sizeof(rplidar_response_measurement_node_t) - recvPos;
// 		size_t recvSize;

// 		recvSize = recvdata(pThis, recvBuffer, remainSize);

// 		if (recvSize > remainSize) recvSize = remainSize;
// 		for (size_t pos = 0; pos < recvSize; ++pos) {
// 			u8 currentByte = recvBuffer[pos];
// 			switch (recvPos) {
// 			case 0: // expect the sync bit and its reverse in this byte
// 			{
// 				u8 tmp = (currentByte>>1);
// 				if ( (tmp ^ currentByte) & 0x1 ) {
// 					// pass
// 				} else {
// 					continue;
// 				}

// 			}
// 			break;
// 			case 1: // expect the highest bit to be 1
// 			{
// 				if (currentByte & RPLIDAR_RESP_MEASUREMENT_CHECKBIT) {
// 					// pass
// 				} else {
// 					recvPos = 0;
// 					continue;
// 				}
// 			}
// 			break;
// 			}
// 			nodeBuffer[recvPos++] = currentByte;

// 			if (recvPos == sizeof(rplidar_response_measurement_node_t)) {
// 				return 0;
// 			}
// 		}
// 	}
// }

static void stop(TUSBCp210xDevice *pThis)
{
	sendCommand(pThis, RPLIDAR_CMD_STOP, NULL, 0);
}

static int getTypicalScanMode(TUSBCp210xDevice *pThis, u16 *outMode)
{
	u32 ans;
	rplidar_payload_get_scan_conf_t query = {0};
	rplidar_ans_header_t *response_header;
	u32 *replyType;
	u16 *mode;
	u8 *buffer = malloc(64);
	memset(buffer, 0, 64);
	query.type = RPLIDAR_CONF_SCAN_MODE_TYPICAL;

	ans = sendCommand(
		pThis, RPLIDAR_CMD_GET_LIDAR_CONF, &query, sizeof(query));
	recvdata(pThis, buffer, 64);

	response_header = (rplidar_ans_header_t *)buffer;

	// verify whether we got a correct header
	if (response_header->type != RPLIDAR_ANS_TYPE_GET_LIDAR_CONF) {
		printf("func: %s, line:%d get a wrong header\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	u32 header_size = (response_header->size_q30_subtype
			   & RPLIDAR_ANS_HEADER_SIZE_MASK);
	if (header_size < sizeof(u32)) {
		return -1;
	}

	//check if returned type is same as asked type
	replyType = (u32 *)(buffer + sizeof(rplidar_ans_header_t));
	if (*replyType != RPLIDAR_CONF_SCAN_MODE_TYPICAL) {
		printf("func: %s, line:%d reply type mismatch.\n",
		       __func__,
		       __LINE__);
		return -1;
	}

	mode = (u16 *)(buffer + sizeof(rplidar_ans_header_t) + sizeof(u32));
	*outMode = *mode;
	return ans;
}

static int senddata(TUSBCp210xDevice *pThis, const u8 *data, size_t size)
{
	if (data == NULL || size == 0)
		return 0;

	// DWHCI requires that buffer address must be DWORD aligned.
	if (((u64)data & 0x3) != 0) {
		u8 *tmpbuffer = malloc(size);
		for (int i = 0; i < size; ++i) {
			tmpbuffer[i] = data[i];
		}
		data = tmpbuffer;
	}
	TUSBRequest *urb = malloc(sizeof(TUSBRequest));
	USBRequest(urb, pThis->bulk_out_ep, (void *)data, size, 0);
	urb->m_pCompletionContext = (void *)pThis;
	urb->m_pCompletionRoutine = cp210x_write_bulk_callback;
	DWHCIDeviceSubmitBlockingRequest(
		USBFunctionGetHost(&pThis->m_USBFunction), urb);
	return 0;
}

static int recvdata(TUSBCp210xDevice *pThis, u8 *data, size_t size)
{
	int i;
	u8 *recvbuffer = 0;
	TUSBRequest *urb = malloc(sizeof(TUSBRequest));

	if (((u64)data & 0x3) != 0) {
		recvbuffer = malloc(size);
		USBRequest(urb, pThis->bulk_in_ep, (void *)recvbuffer, size, 0);
		DWHCIDeviceSubmitBlockingRequest(
			USBFunctionGetHost(&pThis->m_USBFunction), urb);
		for (i = 0; i < size; ++i) {
			data[i] = recvbuffer[i];
		}

		free(recvbuffer);
	} else {
		USBRequest(urb, pThis->bulk_in_ep, (void *)data, size, 0);
		DWHCIDeviceSubmitBlockingRequest(
			USBFunctionGetHost(&pThis->m_USBFunction), urb);
	}
	return urb->m_nResultLen;
}

static void cp210x_write_bulk_callback(TUSBRequest *pURB, void *pParam,
				       void *pContext)
{
	int status = pURB->m_bStatus;
	assert(status == 1);
}

static int demo(TUSBCp210xDevice *pThis)
{
	return 0;
	//int ret;
	printf("Ultra simple LIDAR data grabber for RPLIDAR.\n");
	// open and connect
	cp210x_open(pThis);
	cp210x_tiocmset_port(pThis, 6, 0);
	cp210x_set_termios(pThis);
	cp210x_tiocmset_port(pThis, 0, 2);
	checkMotorCtrlSupport(pThis);
	stopMotor(pThis);

	// get dev info
	rplidar_response_device_info_t devinfo;
	getDevInfo(pThis, &devinfo);

	// print out the device serial number, firmware and hardware version number..
	printf("RPLIDAR S/N: ");
	for (int pos = 0; pos < 16; ++pos) {
		printf("%02X", devinfo.serialnum[pos]);
	}

	printf("\n"
	       "Firmware Ver: %d.%02d\n"
	       "Hardware Rev: %d\n",
	       devinfo.firmware_version >> 8,
	       devinfo.firmware_version & 0xFF,
	       (int)devinfo.hardware_version);

	rplidar_response_device_health_t healthinfo = {0};
	getHealth(pThis, &healthinfo);
	printf("RPLidar health status : %d\n", healthinfo.status);
	if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
		printf("Error, rplidar internal error detected. Please reboot the device to retry.\n");
		return -1;
	}

	startMotor(pThis);

	startScan(pThis, 0, 1, 0);
	stop(pThis);
	stopMotor(pThis);
	return 0;
}