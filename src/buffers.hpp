#pragma once

#include <deque>

struct Buffers {
    std::deque<int> prog_stdout;
    std::deque<int> keyboard_in;
    std::deque<int> vt100_in;
};
