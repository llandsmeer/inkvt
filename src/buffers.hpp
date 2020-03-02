#pragma once

#include <deque>
#include <vector>

struct Buffers {
    std::deque<int> prog_stdout;
    std::deque<int> scancodes;
#ifdef INPUT_EVDEV
    std::vector<char> serial; // needs a continuous chunk of memory to cast to struct
#else
    std::deque<char> serial;
#endif
    std::deque<char> keyboard;
    std::deque<char> vt100_in;
};
