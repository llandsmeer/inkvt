
#pragma once

#include "rounded_rect.hpp"

const int NOMOD = 0;
const int SHIFT = 1;
const int CTRL = 2;
const int FN = 3;
const int ALT = 4;

struct kbkey {
    int mod;
    const char * text;
    const char * normal;
    const char * shift;
    const char * fn;
    float x;
    float y;
    float w;
    float h;
    RoundedRect rrect;
};

#define OSK_NKEYS 61u
#define OSK_W 15u
#define OSK_H 5u
struct kbkey osk_keys[61] = {
    { .mod = NOMOD, .text = "`", .normal = "`", .shift = "~", .fn = "", .x = 0.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "1", .normal = "1", .shift = "!", .fn = "F1", .x = 1.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "2", .normal = "2", .shift = "@", .fn = "F2", .x = 2.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "3", .normal = "3", .shift = "#", .fn = "F3", .x = 3.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "4", .normal = "4", .shift = "$", .fn = "F4", .x = 4.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "5", .normal = "5", .shift = "%", .fn = "F5", .x = 5.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "6", .normal = "6", .shift = "^", .fn = "F6", .x = 6.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "7", .normal = "7", .shift = "&", .fn = "F7", .x = 7.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "8", .normal = "8", .shift = "*", .fn = "F8", .x = 8.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "9", .normal = "9", .shift = "(", .fn = "F9", .x = 9.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "0", .normal = "0", .shift = ")", .fn = "F10", .x = 10.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "-", .normal = "-", .shift = "_", .fn = "F11", .x = 11.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "=", .normal = "=", .shift = "+", .fn = "F12", .x = 12.00f, .y = 0.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Backspace", .normal = "\177", .shift = "\177", .fn = "", .x = 13.00f, .y = 0.00f, .w = 2.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Tab", .normal = "\011", .shift = "\033[Z", .fn = "", .x = 0.00f, .y = 1.00f, .w = 1.50f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "q", .normal = "q", .shift = "Q", .fn = "", .x = 1.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "w", .normal = "w", .shift = "W", .fn = "", .x = 2.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "e", .normal = "e", .shift = "E", .fn = "", .x = 3.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "r", .normal = "r", .shift = "R", .fn = "", .x = 4.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "t", .normal = "t", .shift = "T", .fn = "", .x = 5.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "y", .normal = "y", .shift = "Y", .fn = "", .x = 6.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "u", .normal = "u", .shift = "U", .fn = "", .x = 7.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "i", .normal = "i", .shift = "I", .fn = "", .x = 8.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "o", .normal = "o", .shift = "O", .fn = "", .x = 9.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "p", .normal = "p", .shift = "P", .fn = "", .x = 10.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "[", .normal = "[", .shift = "{", .fn = "", .x = 11.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "]", .normal = "]", .shift = "}", .fn = "", .x = 12.50f, .y = 1.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "\134", .normal = "\134", .shift = "|", .fn = "", .x = 13.50f, .y = 1.00f, .w = 1.50f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Escape", .normal = "\033", .shift = "\033", .fn = "", .x = 0.00f, .y = 2.00f, .w = 1.75f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "a", .normal = "a", .shift = "A", .fn = "", .x = 1.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "s", .normal = "s", .shift = "S", .fn = "", .x = 2.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "d", .normal = "d", .shift = "D", .fn = "", .x = 3.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "f", .normal = "f", .shift = "F", .fn = "", .x = 4.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "g", .normal = "g", .shift = "G", .fn = "", .x = 5.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "h", .normal = "h", .shift = "H", .fn = "", .x = 6.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "j", .normal = "j", .shift = "J", .fn = "", .x = 7.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "k", .normal = "k", .shift = "K", .fn = "", .x = 8.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "l", .normal = "l", .shift = "L", .fn = "", .x = 9.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = ";", .normal = ";", .shift = ":", .fn = "", .x = 10.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "'", .normal = "'", .shift = "\042", .fn = "", .x = 11.75f, .y = 2.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Enter", .normal = "\012", .shift = "\012", .fn = "", .x = 12.75f, .y = 2.00f, .w = 2.25f, .h = 1.00f, {}, },
    { .mod = SHIFT, .text = "Shift", .normal = "", .shift = "", .fn = "", .x = 0.00f, .y = 3.00f, .w = 2.25f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "z", .normal = "z", .shift = "Z", .fn = "", .x = 2.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "x", .normal = "x", .shift = "X", .fn = "", .x = 3.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "c", .normal = "c", .shift = "C", .fn = "", .x = 4.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "v", .normal = "v", .shift = "V", .fn = "", .x = 5.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "b", .normal = "b", .shift = "B", .fn = "", .x = 6.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "n", .normal = "n", .shift = "N", .fn = "", .x = 7.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "m", .normal = "m", .shift = "M", .fn = "", .x = 8.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = ",", .normal = ",", .shift = "<", .fn = "", .x = 9.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = ".", .normal = ".", .shift = ">", .fn = "", .x = 10.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "/", .normal = "/", .shift = "?", .fn = "", .x = 11.25f, .y = 3.00f, .w = 1.00f, .h = 1.00f, {}, },
    { .mod = SHIFT, .text = "Shift", .normal = "", .shift = "", .fn = "", .x = 12.25f, .y = 3.00f, .w = 2.75f, .h = 1.00f, {}, },
    { .mod = CTRL, .text = "Ctrl", .normal = "", .shift = "", .fn = "", .x = 0.00f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
    { .mod = FN, .text = "Fn", .normal = "", .shift = "", .fn = "", .x = 1.25f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
    { .mod = ALT, .text = "Alt", .normal = "", .shift = "", .fn = "", .x = 2.50f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Space", .normal = " ", .shift = " ", .fn = "", .x = 3.75f, .y = 4.00f, .w = 6.25f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Left", .normal = "\033[D", .shift = "\033[D", .fn = "", .x = 10.00f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Down", .normal = "\033[B", .shift = "\033[B", .fn = "", .x = 11.25f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Up", .normal = "\033[A", .shift = "\033[A", .fn = "", .x = 12.50f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
    { .mod = NOMOD, .text = "Right", .normal = "\033[C", .shift = "\033[C", .fn = "", .x = 13.75f, .y = 4.00f, .w = 1.25f, .h = 1.00f, {}, },
};
