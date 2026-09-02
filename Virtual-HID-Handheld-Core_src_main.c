/**
 * @file main.c
 * @brief Entry point for Virtual-HID-Handheld-Core
 *
 * Intercepts low-level keyboard scan codes using Linux evdev/uinput and 
 * maps them to a virtual gamepad with relative mouse vector acceleration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "hid_handheld.h"

static volatile bool g_keep_running = true;

static void signal_handler(int sig) {
    (void)sig;
    g_keep_running = false;
}

static int setup_uinput_device(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open /dev/uinput");
        return -1;
    }

    /* Enable Key/Button events */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_SOUTH);  /* Gamepad Button A */
    ioctl(fd, UI_SET_KEYBIT, BTN_EAST);   /* Gamepad Button B */
    ioctl(fd, UI_SET_KEYBIT, BTN_NORTH);  /* Gamepad Button X */
    ioctl(fd, UI_SET_KEYBIT, BTN_WEST);   /* Gamepad Button Y */
    ioctl(fd, UI_SET_KEYBIT, BTN_TL);     /* L1 Trigger */
    ioctl(fd, UI_SET_KEYBIT, BTN_TR);     /* R1 Trigger */
    ioctl(fd, UI_SET_KEYBIT, BTN_START);  /* Start Button */
    ioctl(fd, UI_SET_KEYBIT, BTN_SELECT); /* Select Button */
    
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);   /* Mouse Left Click */
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);  /* Mouse Right Click */

    /* Enable Relative Pointer movement for Virtual Mouse */
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;  /* Virtual HID Vendor ID */
    usetup.id.product = 0x5678; /* Virtual HID Product ID */
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "OpenHID-Handheld-Controller");

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("Failed to configure virtual uinput device");
        close(fd);
        return -1;
    }

    return fd;
}

static void emit_event(int ufd, uint16_t type, uint16_t code, int32_t val) {
    struct input_event ie;
    memset(&ie, 0, sizeof(ie));
    ie.type = type;
    ie.code = code;
    ie.value = val;
    write(ufd, &ie, sizeof(ie));

    /* Sync event packet */
    memset(&ie, 0, sizeof(ie));
    ie.type = EV_SYN;
    ie.code = SYN_REPORT;
    ie.value = 0;
    write(ufd, &ie, sizeof(ie));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s /dev/input/eventX\n", argv[0]);
        printf("Note: Run with sudo/root privileges to access evdev and uinput.\n");
        return EXIT_FAILURE;
    }

    const char *device_path = argv[1];
    printf("[OpenHID Framework] Initializing Virtual-HID-Handheld-Core...\n");
    printf("[OpenHID Framework] Target Input Device: %s\n", device_path);

    int input_fd = open(device_path, O_RDONLY);
    if (input_fd < 0) {
        fprintf(stderr, "Error opening device %s: %s\n", device_path, strerror(errno));
        return EXIT_FAILURE;
    }

    /* Grab exclusive access to keyboard device */
    if (ioctl(input_fd, EVIOCGRAB, 1) < 0) {
        perror("Warning: Could not grab exclusive access to keyboard");
    }

    int uinput_fd = setup_uinput_device();
    if (uinput_fd < 0) {
        close(input_fd);
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("[OpenHID Framework] Engine active. Press Ctrl+C to terminate.\n");

    struct input_event ev;
    int mouse_step = 8; /* Vector acceleration increment */

    while (g_keep_running) {
        ssize_t bytes = read(input_fd, &ev, sizeof(ev));
        if (bytes < (ssize_t)sizeof(ev)) {
            continue;
        }

        if (ev.type == EV_KEY) {
            switch (ev.code) {
                /* Action Cluster Mapping (Z, X, C, V -> Gamepad A, B, X, Y) */
                case KEY_Z: emit_event(uinput_fd, EV_KEY, BTN_SOUTH, ev.value); break;
                case KEY_X: emit_event(uinput_fd, EV_KEY, BTN_EAST,  ev.value); break;
                case KEY_C: emit_event(uinput_fd, EV_KEY, BTN_NORTH, ev.value); break;
                case KEY_V: emit_event(uinput_fd, EV_KEY, BTN_WEST,  ev.value); break;

                /* Shoulder Buttons (Tab -> L1, Backspace -> R1) */
                case KEY_TAB:       emit_event(uinput_fd, EV_KEY, BTN_TL, ev.value); break;
                case KEY_BACKSPACE: emit_event(uinput_fd, EV_KEY, BTN_TR, ev.value); break;

                /* System Buttons (Enter -> Start, Space -> Select) */
                case KEY_ENTER: emit_event(uinput_fd, EV_KEY, BTN_START,  ev.value); break;
                case KEY_SPACE: emit_event(uinput_fd, EV_KEY, BTN_SELECT, ev.value); break;

                /* Virtual Mouse Click Mapping (U -> Left Click, O -> Right Click) */
                case KEY_U: emit_event(uinput_fd, EV_KEY, BTN_LEFT,  ev.value); break;
                case KEY_O: emit_event(uinput_fd, EV_KEY, BTN_RIGHT, ev.value); break;

                /* Virtual Mouse Cursor Translation (I/J/K/L) */
                case KEY_I: 
                    if (ev.value) emit_event(uinput_fd, EV_REL, REL_Y, -mouse_step); 
                    break;
                case KEY_K: 
                    if (ev.value) emit_event(uinput_fd, EV_REL, REL_Y, mouse_step); 
                    break;
                case KEY_J: 
                    if (ev.value) emit_event(uinput_fd, EV_REL, REL_X, -mouse_step); 
                    break;
                case KEY_L: 
                    if (ev.value) emit_event(uinput_fd, EV_REL, REL_X, mouse_step); 
                    break;

                default: break;
            }
        }
    }

    printf("\n[OpenHID Framework] Shutting down engine...\n");
    ioctl(input_fd, EVIOCGRAB, 0);
    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    close(input_fd);

    printf("[OpenHID Framework] Cleanup complete.\n");
    return EXIT_SUCCESS;
}
