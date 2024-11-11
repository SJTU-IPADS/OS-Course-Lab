#pragma once
#include <uspi/usbfunction.h>

/*
 * Serial driver contains a set of standard interfaces. A specific 
 * serial device's driver need to implement these interfaces to
 * provide service.
 */
typedef struct TUSBSerialDriver {
	int (*write)(void *serial_device, void *buffer, int size);
	int (*read)(void *serial_device, void *buffer, int size);
	int (*open)(void *serial_device);
	int (*close)(void *serial_device);
	void (*set_termios)(void *serial_device);
	void (*get_termios)(void *serial_device);
	int (*tiocmget)(void *serial_device);
	int (*tiocmset)(void *serial_device, int set, int clear);
	int (*dtr_rts)(void *serial_device, int on);
} TUSBSerialDriver;

/*
 * A unified abstraction for serial devices. The first two fields
 * of any serial device must be TUSBFunction and TUSBSerialDriver.
 * So serial_dispatch can call the serial driver functions in an
 * unified way such as serial_device->m_USBSerialDriver.write().
 */
typedef struct TUSBSerialDevice {
	TUSBFunction m_USBFunction;
	TUSBSerialDriver m_USBSerialDriver;
} TUSBSerialDevice;
