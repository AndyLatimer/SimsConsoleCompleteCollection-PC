#pragma once
#include <cstdint>

namespace input {

// the console controller's report, analog buttons are 0-255
struct Pad {
    bool connected = false;
    uint16_t buttons = 0;  // dpad up/down/left/right, start, back, left thumb, right thumb
    uint8_t analog[8] = {};  // A B X Y black white left trigger right trigger
    int16_t lx = 0, ly = 0, rx = 0, ry = 0;
};

void Init();  // window thread, before boot
// the controller on that port, then GameInfo::pad
Pad Poll(int port);
void Rumble(int port, uint16_t left, uint16_t right);
// the controller on that port as SDL names it ("DualSense Wireless Controller"), "" for none
const char* Name(int port);
// the light bar colour of the pads that have one (DualSense, DualShock 4), set now and on every plug-in
void SetLight(uint8_t r, uint8_t g, uint8_t b);

}  // namespace input
