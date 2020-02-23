#pragma once

#include <deque>
#include <vector>

struct Buffers {
    std::deque<int> prog_stdout;
    std::deque<int> scancodes;
    std::vector<char> serial;
    std::deque<char> keyboard;
    std::deque<int> vt100_in;
};
