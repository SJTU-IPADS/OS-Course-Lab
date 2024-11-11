
#include <sched.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/input/mouse.h>
#include <circle/types.h>
#include <circle/usb/usbkeyboard.h>

#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/sd_defs.h>
#include <chcore-internal/usb_devmgr_defs.h>
#include <circle/bcm2711.h>
#include "../circle.hpp"
extern "C" {
#include "libpipe.h"
}
#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif
#define PAGE_SIZE 0x1000

static void init_io_mapping(void)
{
        int ret;
        cap_t pcie_map_cap = usys_create_device_pmo(
                ARM_PCIE_HOST_BASE,
                ROUND_UP(ARM_PCIE_HOST_END - ARM_PCIE_HOST_BASE, PAGE_SIZE));
        ret = usys_map_pmo(
                SELF_CAP, pcie_map_cap, ARM_PCIE_HOST_BASE, VM_READ | VM_WRITE);

        BUG_ON(ret < 0);
        cap_t xhci_map_cap = usys_create_device_pmo(
                ARM_XHCI_BASE,
                ROUND_UP(ARM_XHCI_END - ARM_XHCI_BASE, PAGE_SIZE));
        ret = usys_map_pmo(
                SELF_CAP, xhci_map_cap, ARM_XHCI_BASE, VM_READ | VM_WRITE);

        BUG_ON(ret < 0);
}

class USBDriver {
    public:
        USBDriver(void);
        ~USBDriver(void);

        boolean Initialize(void);
        cap_t getMousePipe(void)
        {
                return mouse_pipe_pmo;
        }
        cap_t getKeyboardPipe(void)
        {
                return keyboard_pipe_pmo;
        }
        static USBDriver *Get(void);
        void UpdateLEDs(void);

    private:
        void MouseStatusHandler(unsigned nButtons, int nPosX, int nPosY,
                                int nWheelMove);
        static void MouseStatusHandlerForRegister(unsigned nButtons, int nPosX,
                                                  int nPosY, int nWheelMove)
        {
                s_pThis->MouseStatusHandler(nButtons, nPosX, nPosY, nWheelMove);
        }
        static void MouseRemovedHandler(CDevice *pDevice, void *pContext);
        void KeyStatusHandlerRaw(unsigned char ucModifiers,
                                 const unsigned char RawKeys[6]);
        static void
        KeyStatusHandlerRawForRegister(unsigned char ucModifiers,
                                       const unsigned char RawKeys[6])
        {
                s_pThis->KeyStatusHandlerRaw(ucModifiers, RawKeys);
        }
        static void KeyboardRemovedHandler(CDevice *pDevice, void *pContext);

    private:
        // do not change this order
        CUSBHCIDevice m_USBHCI;
        CMouseDevice *volatile m_pMouse;
        CUSBKeyboardDevice *volatile m_pKeyboard;

        static USBDriver *s_pThis;

        void RegisterMouseAndKeyboard();

    private:
        int keyboard_pipe_pmo = -1;
        struct pipe *keyboard_pipe = nullptr;
        int mouse_pipe_pmo = -1;
        struct pipe *mouse_pipe = nullptr;
        void initMouseKeyPipe(void);
        int LED_Status = 0;
};

USBDriver *USBDriver::s_pThis = 0;

USBDriver::USBDriver(void)
        : m_USBHCI(CInterruptSystem::Get(), CTimer::Get(), TRUE)
        , // TRUE: enable plug-and-play
        m_pMouse(0)
        , m_pKeyboard(0)
{
}

USBDriver::~USBDriver(void)
{
        s_pThis = 0;
}

boolean USBDriver::Initialize(void)
{
        boolean bOK = TRUE;
        if (bOK) {
                bOK = m_USBHCI.Initialize();
        }
        initMouseKeyPipe();
        RegisterMouseAndKeyboard();
        return bOK;
}

USBDriver *USBDriver::Get(void)
{
        if (s_pThis == NULL) {
                s_pThis = new USBDriver;
        }
        return s_pThis;
}

void USBDriver::RegisterMouseAndKeyboard(void)
{
        for (int i = 0; i < 10; i++) {
                // This must be called from TASK_LEVEL to update the tree of
                // connected USB devices.
                boolean bUpdated = m_USBHCI.UpdatePlugAndPlay();

                if (bUpdated && m_pMouse == 0) {
                        m_pMouse = (CMouseDevice *)CDeviceNameService::Get()
                                           ->GetDevice("mouse1", FALSE);
                        if (m_pMouse != 0) {
                                m_pMouse->RegisterRemovedHandler(
                                        MouseRemovedHandler);
                                m_pMouse->RegisterStatusHandler(
                                        MouseStatusHandlerForRegister);
                        }
                }
                if (bUpdated && m_pKeyboard == 0) {
                        m_pKeyboard =
                                (CUSBKeyboardDevice *)CDeviceNameService::Get()
                                        ->GetDevice("ukbd1", FALSE);
                        if (m_pKeyboard != 0) {
                                m_pKeyboard->RegisterRemovedHandler(
                                        KeyboardRemovedHandler);
                                m_pKeyboard->RegisterKeyStatusHandlerRaw(
                                        KeyStatusHandlerRawForRegister, TRUE);
                        }
                }

                if (m_pKeyboard != 0) {
                        // CUSBKeyboardDevice::UpdateLEDs() must not be called
                        // in interrupt context, that's why this must be done
                        // here. This does nothing in raw mode.
                        m_pKeyboard->UpdateLEDs();
                }
                if (m_pMouse != nullptr && m_pKeyboard != nullptr) {
                        break;
                }
        }
}

void USBDriver::initMouseKeyPipe(void)
{
        mouse_pipe_pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);
        mouse_pipe = create_pipe_from_pmo(PAGE_SIZE, mouse_pipe_pmo);
        pipe_init(mouse_pipe);

        keyboard_pipe_pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);
        keyboard_pipe = create_pipe_from_pmo(PAGE_SIZE, keyboard_pipe_pmo);
        pipe_init(keyboard_pipe);
}

static void usb_get_keyboard_pipe_handler(ipc_msg_t *ipc_msg,
                                          badge_t client_badge)
{
        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, USBDriver::Get()->getKeyboardPipe());
        ipc_return_with_cap(ipc_msg, 0);
}

static void usb_get_mouse_pipe_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, USBDriver::Get()->getMousePipe());
        ipc_return_with_cap(ipc_msg, 0);
}

void USBDriver::MouseRemovedHandler(CDevice *pDevice, void *pContext)
{
        assert(s_pThis != 0);

        s_pThis->m_pMouse = 0;

        printf("Please attach a USB mouse!");
}

void USBDriver::KeyboardRemovedHandler(CDevice *pDevice, void *pContext)
{
        assert(s_pThis != 0);

        s_pThis->m_pMouse = 0;

        printf("Please attach a USB keyboard!");
}

void USBDriver::MouseStatusHandler(unsigned int nButtons, int nDisplacementX,
                                   int nDisplacementY, int nScrollVector)
{
        struct {
                u32 buttons;
                int x, y, scroll;
        } event;

        if (mouse_pipe == NULL)
                return;

        event.buttons = nButtons;
        event.x = nDisplacementX;
        event.y = nDisplacementY;
        event.scroll = nScrollVector;

        pipe_write_exact(mouse_pipe, &event, sizeof(event));
}

void USBDriver::KeyStatusHandlerRaw(unsigned char ucModifiers,
                                    const unsigned char RawKeys[6])
{
        struct {
                u32 mods_depressed;
                u32 mods_latched;
                u32 mods_locked;
                u32 keys[6];
        } event = {0};
        u16 keysym;
        u32 i;
        if (m_pKeyboard == NULL)
                return;

        if (m_pKeyboard->m_Behaviour.m_hTimer != 0) {
                CTimer::Get()->CancelKernelTimer(
                        m_pKeyboard->m_Behaviour.m_hTimer);
                m_pKeyboard->m_Behaviour.m_hTimer = 0;
        }

        if (keyboard_pipe == NULL)
                return;

        if (ucModifiers & (LSHIFT | RSHIFT))
                event.mods_depressed |= 1 << 0;
        if (ucModifiers & (LCTRL | RCTRL))
                event.mods_depressed |= 1 << 2;
        if (ucModifiers & (ALT | ALTGR))
                event.mods_depressed |= 1 << 3;

        for (i = 0; i < 6; i++) {
                keysym = m_pKeyboard->m_Behaviour.m_KeyMap
                                 .m_KeyMap[RawKeys[i]][0];

                if (keysym == KeyCapsLock)
                        event.mods_depressed |= 1 << 1;
                else if (keysym == KeyNumLock)
                        event.mods_depressed |= 1 << 4;
                else if (keysym == KeyScrollLock)
                        event.mods_depressed |= 1 << 5;

                event.keys[i] = RawKeys[i];
        }

        /* Lock status */
        static bool num_lock = true;
        static bool caps_lock = false;
        static bool scroll_lock = false;

        /* Key status */
        static bool caps_lock_last = false;
        static bool num_lock_last = false;
        static bool scroll_lock_last = false;
        
        bool caps_lock_current = event.mods_depressed & (1 << 1);
        bool num_lock_current = event.mods_depressed & (1 << 4);
        bool scroll_lock_current = event.mods_depressed & (1 << 5);

        /* Check if the lock keys are pressed. */
        if (caps_lock_current && !caps_lock_last) {
                caps_lock = !caps_lock;
        }
        if (num_lock_current && !num_lock_last) {
                num_lock = !num_lock;
        }
        if (scroll_lock_current && !scroll_lock_last) {
                scroll_lock = !scroll_lock;
        }

        caps_lock_last = caps_lock_current;
        num_lock_last = num_lock_current;
        scroll_lock_last = scroll_lock_current;

        LED_Status = ((caps_lock ? 1 : 0) | (num_lock ? 2 : 0) |
                     (scroll_lock ? 4 : 0));


        if (caps_lock)
                event.mods_locked |= 1 << 1;
        if (num_lock)
                event.mods_locked |= 1 << 4;
        if (scroll_lock)
                event.mods_locked |= 1 << 5;

        pipe_write_exact(keyboard_pipe, &event, sizeof(event));
}

void USBDriver::UpdateLEDs(void)
{
        if (m_pKeyboard != 0) {
                m_pKeyboard->UpdateLEDs();
        }
}

void* UpdateLedThread(void *args)
{
        while (1) {
                USBDriver::Get()->UpdateLEDs();
                usys_yield();
        }
}

DEFINE_SERVER_HANDLER(usb_mouse_keyboard_dispatch)
{
        struct usb_devmgr_request *req;
        int ret = 0;
        req = (struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);

        switch (req->req) {
        case USB_DEVMGR_GET_KEYBOARD_PIPE: {
                usb_get_keyboard_pipe_handler(ipc_msg, client_badge);
        } break;
        case USB_DEVMGR_GET_MOUSE_PIPE: {
                usb_get_mouse_pipe_handler(ipc_msg, client_badge);
        } break;
        default:
                break;
        }

        ipc_return(ipc_msg, ret);
}


void *usb_mouse_and_keyboard(void *args)
{
        int *thread_ret = (int *)malloc(sizeof(int));
        *thread_ret = 0;
        init_io_mapping();
        bind_to_cpu(0);

#if 0
	void *token;
	/*
	 * A token for starting the procmgr server.
	 * This is just for preventing users run procmgr in the Shell.
	 * It is actually harmless to remove this.
	 */
	token = strstr(argv[0], KERNEL_SERVER);
	if (token == NULL) {
		printf("procmgr: I am a system server instead of an (Shell) "
		      "application. Bye!\n");
		usys_exit(-1);
	}
#endif
        /*
         * Init the USB driver framework.
         */
        if (!USBDriver::Get()->Initialize()) {
                *thread_ret = -EIO;
                return thread_ret;
        }

        pthread_t tid;

        pthread_create(&tid, NULL, UpdateLedThread, NULL);

        *thread_ret = ipc_register_server(usb_mouse_keyboard_dispatch,
                                          DEFAULT_CLIENT_REGISTER_HANDLER);

        return thread_ret;
}