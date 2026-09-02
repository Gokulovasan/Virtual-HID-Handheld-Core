# OpenHID Handheld Framework (`Virtual-HID-Handheld-Core`)

`Virtual-HID-Handheld-Core` is an industrial-grade, zero-hardware C99 input middleware layer designed to convert compact USB keyboards into virtual HID gamepads with relative mouse-pointer emulation. Developed specifically for low-power Linux single-board computers (such as Raspberry Pi) running retro-emulation ecosystems (RetroArch, Batocera, Lakka)[cite: 2], the software bridges raw keypresses to native Linux kernel input interfaces without requiring physical hardware modifications or custom microcontrollers[cite: 2].

---

## Overview & Problem Statement

Building portable retro-gaming handhelds typically requires custom PCB fabrication, complex matrix wiring, and dedicated analog joystick integration[cite: 2]. 

`Virtual-HID-Handheld-Core` eliminates physical hardware complexity by operating at the Linux kernel boundary[cite: 2]. By utilizing `/dev/input/event*` (`evdev`) and `/dev/uinput` primitives[cite: 2, 5], the engine captures low-level key matrix signals from standard compact USB keyboards and remaps them into a unified virtual gamepad controller with dynamic mouse acceleration—achieving a fully functional handheld input interface through software alone[cite: 2, 5].

---

## Key Features

* **Kernel-Level Input Interception (`evdev`):** Uses POSIX file descriptors to read raw `input_event` structures directly from the USB keyboard device node[cite: 2, 5].
* **Exclusive Device Locking (`EVIOCGRAB`):** Locks the targeted keyboard device at runtime to prevent keypress leakage into host terminal sessions or desktop environments[cite: 2, 5].
* **Virtual Gamepad Emulation (`uinput`):** Instantiates a virtual USB HID gamepad emitting native Linux input signals (`BTN_SOUTH`, `BTN_EAST`, `BTN_NORTH`, `BTN_WEST`, `BTN_TL`, `BTN_TR`, `BTN_START`, `BTN_SELECT`) recognized out-of-the-box by emulators[cite: 2, 5].
* **Relative Mouse Vector Engine:** Maps a dedicated directional key cluster (`I/J/K/L`) to relative cursor coordinates (`REL_X`, `REL_Y`) for full menu navigation without requiring a physical mouse[cite: 2, 5].
* **Deterministic C99 Architecture:** Written in strict C99 without dynamic memory allocation (`malloc`/`free`)[cite: 2, 3], guaranteeing zero runtime heap fragmentation and predictable $O(1)$ execution performance[cite: 2].

---

## Architecture Pipeline
