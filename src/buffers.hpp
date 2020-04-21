#pragma once

#include <deque>
#include <vector>

struct Buffers {
    std::deque<int> scancodes;
    std::deque<char> serial;
    std::deque<char> keyboard;
    std::deque<char> vt100_in;
};
