#pragma once

#include <deque>

struct Buffers {
    std::deque<int> prog_stdout;
    std::deque<int> scancodes;
    std::deque<char> keyboard;
    std::deque<int> vt100_in;
};
