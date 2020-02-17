/* inkvt - VT100 terminal for E-ink devices
 * Copyright (C) 2020 Lennart Landsmeer <lennart@landsmeer.email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <deque>
#include <linux/input-event-codes.h>
#include <stdio.h>

struct Keymap {
    int shift = 0;
    int ctrl = 0;

    void press(std::deque<int> & keyboard_in, int keycode) {
        if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
            shift = 1;
            return;
        }
        if (keycode == KEY_LEFTCTRL || keycode == KEY_RIGHTCTRL || keycode == KEY_CAPSLOCK) {
            ctrl = 1;
            return;
        }
        if (keycode == KEY_BACKSPACE) {
            keyboard_in.push_back(0x7f);
            return;
        }
        int c = translate(keycode);
        if (c != 0) {
            if (ctrl) {
                if (c > 0x60) c -= 0x20;
                c -= 0x40;
            }
            keyboard_in.push_back(c);
        }
    }

    void release(int keycode) {
        if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
            shift = 0;
        }
        if (keycode == KEY_LEFTCTRL || keycode == KEY_RIGHTCTRL || keycode == KEY_CAPSLOCK) {
            ctrl = 0;
            return;
        }
    }

    int translate(int keycode) {
        switch (keycode) {
            case KEY_MINUS:
                  if (shift) return '_';
                  else return '-';
            case KEY_EQUAL:
                  if (shift) return '+';
                  else return '=';
            case KEY_LEFTBRACE:
                  if (shift) return '{';
                  else return '[';
            case KEY_RIGHTBRACE:
                  if (shift) return '}';
                  else return ']';
            case KEY_SEMICOLON:
                  if (shift) return ':';
                  else return ';';
            case KEY_APOSTROPHE:
                  if (shift) return '"';
                  else return '\'';
            case KEY_BACKSLASH:
                  if (shift) return '|';
                  else return '\\';
            case KEY_COMMA:
                  if (shift) return '<';
                  else return ',';
            case KEY_DOT:
                  if (shift) return '>';
                  else return '.';
            case KEY_SPACE:
                  return ' ';
            case KEY_SLASH:
                  if (shift) return '?';
                  else return '/';
            case KEY_BACKSPACE:
                  return 128;
            case KEY_ENTER:
                  return '\n';
            case KEY_1:
                  if (shift) return '!';
                  else return '1';
            case KEY_2:
                  if (shift) return '@';
                  else return '2';
            case KEY_3:
                  if (shift) return '#';
                  else return '3';
            case KEY_4:
                  if (shift) return '$';
                  else return '4';
            case KEY_5:
                  if (shift) return '%';
                  else return '5';
            case KEY_6:
                  if (shift) return '^';
                  else return '6';
            case KEY_7:
                  if (shift) return '&';
                  else return '7';
            case KEY_8:
                  if (shift) return '*';
                  else return '8';
            case KEY_9:
                  if (shift) return '(';
                  else return '9';
            case KEY_0:
                  if (shift) return ')';
                  else return '0';
            case KEY_Q:
                  if (shift) return 'Q';
                  else return 'q';
            case KEY_W:
                  if (shift) return 'W';
                  else return 'w';
            case KEY_E:
                  if (shift) return 'E';
                  else return 'e';
            case KEY_R:
                  if (shift) return 'R';
                  else return 'r';
            case KEY_T:
                  if (shift) return 'T';
                  else return 't';
            case KEY_Y:
                  if (shift) return 'Y';
                  else return 'y';
            case KEY_U:
                  if (shift) return 'U';
                  else return 'u';
            case KEY_I:
                  if (shift) return 'I';
                  else return 'i';
            case KEY_O:
                  if (shift) return 'O';
                  else return 'o';
            case KEY_P:
                  if (shift) return 'P';
                  else return 'p';
            case KEY_A:
                  if (shift) return 'A';
                  else return 'a';
            case KEY_S:
                  if (shift) return 'S';
                  else return 's';
            case KEY_D:
                  if (shift) return 'D';
                  else return 'd';
            case KEY_F:
                  if (shift) return 'F';
                  else return 'f';
            case KEY_G:
                  if (shift) return 'G';
                  else return 'g';
            case KEY_H:
                  if (shift) return 'H';
                  else return 'h';
            case KEY_J:
                  if (shift) return 'J';
                  else return 'j';
            case KEY_K:
                  if (shift) return 'K';
                  else return 'k';
            case KEY_L:
                  if (shift) return 'L';
                  else return 'l';
            case KEY_Z:
                  if (shift) return 'Z';
                  else return 'z';
            case KEY_X:
                  if (shift) return 'X';
                  else return 'x';
            case KEY_C:
                  if (shift) return 'C';
                  else return 'c';
            case KEY_V:
                  if (shift) return 'V';
                  else return 'v';
            case KEY_B:
                  if (shift) return 'B';
                  else return 'b';
            case KEY_N:
                  if (shift) return 'N';
                  else return 'n';
            case KEY_M:
                  if (shift) return 'M';
                  else return 'm';
              }
        return 0;
    }
};
