/**
 * @file hid_handheld.h
 * @brief OpenHID Handheld Framework Engine Core Header
 *
 * Industrial C99 interface for translating raw Linux input events into 
 * virtual gamepad and accelerated mouse signals via uinput.
 */

#ifndef HID_HANDHELD_H
#define HID_HANDHELD_H

#include <stdint.h>
#include <stdbool.h>

#define ENGINE_OK          0
#define ENGINE_ERR_INIT   -1
#define ENGINE_ERR_DEVICE -2
#define ENGINE_ERR_WRITE  -3

typedef struct {
    int input_fd;
    int uinput_fd;
    bool is_running;
    uint8_t mouse_speed;
} hid_engine_t;

int hid_engine_init(hid_engine_t *engine, const char *device_path);
int hid_engine_process_loop(hid_engine_t *engine);
void hid_engine_shutdown(hid_engine_t *engine);

#endif /* HID_HANDHELD_H */
