from typing import List, Tuple

def load_keymap():
    '''returns {
        'KEY_U': ['press u', 'shift U'],
        'KEY_UP': ['press \\x1b[A'],
        'KEY_V': ['press v', 'shift V'],
        'KEY_W': ['press w', 'shift W'],
    }
    '''
    keymap = {
        '_toggle': [],
        '_track': []
    }
    with open('./keymap') as f:
        for line in f:
            line = line.rstrip()
            if line.lstrip() == line:
                keycode = line[4:] if line.startswith('KEY_') else line
                keymap[keycode] = []
            else:
                action = line.lstrip()
                if action == 'track':
                    keymap['_track'].append(keycode)
                elif action == 'toggle':
                    keymap['_toggle'].append(keycode)
                else:
                    on, then = action.split()
                    keymap[keycode].append((on, then))
    to_delete = []
    for k, v in keymap.items():
        if not v:
            to_delete.append(k)
    for k in to_delete:
        del keymap[k]
    return keymap

keymap = load_keymap()

def p(indent=None, *a):
    if indent is None:
        print()
        return
    if indent == 0:
        print(*a)
    else:
        print(' '*(4*indent-1), *a)

def trackvar(k):
    return f'_is_{k.lower()}_down'

def togglevar(k):
    return f'{k.lower()}_state'

mods = {
    'shift': 'leftshift rightshift',
    'meta': 'leftmeta rightmeta',
    'ctrl': 'capslock leftctrl rightctrl',
    'alt': 'leftalt rightalt'
}

def kbstate(n):
    return {
        'shift': 'is_shift()',
        'meta': 'is_meta()',
        'ctrl': 'is_ctrl()',
        'alt': 'is_alt()',
        'numlock': 'numlock_state',
        'scrolllock': 'scrolllock_state'
    }[n]

def emit(indent, step):
    def esc(s):
        if s == '\xff':
            return "'\\xff'"
        s = repr(s)[1:-1]
        if s == "'":
            s = "\\'"
        return f"'{s}'"
    step = step.encode('utf-8').decode('unicode_escape')
    if len(step) == 1:
        p(indent, f"out.push_back({esc(step)});")
    else:
        p(indent, f"out.insert(out.end(), {{{', '.join(map(esc, step))}}});")
    #for s in step:
    #    s = repr(s)[1:-1]
    #    if s == "'":
    #        s = "\\'"
    #    p(indent, f"out.push_back('{s}');")

p(0, '''
/* autogenerated by keymap.py - do not edit */

#pragma once
#include <deque>
#include <linux/input.h>

class KeycodeTranslation {''')
for k in keymap['_track']:
    p(1, f'int {trackvar(k)};')
for k in keymap['_toggle']:
    p(1, f'int {togglevar(k)};')
p()
p(0, 'public:')
for k, v in mods.items():
    p(1, f'bool is_{k}() {{')
    p(2, f'return {" || ".join(trackvar(vv.upper()) for vv in v.split())};')
    p(1, '}')
    p()

p(0, 'private:')
p(1, 'void _translate_press(int keycode, std::deque<char> & out) {')
p(2, 'switch (keycode) {')
for i, (k, v) in enumerate(keymap.items()):
    if k == '_toggle' or k == '_track':
        continue
    p(3, f'case KEY_{k}:')
    press = None
    for action, step in v:
        if action == 'press':
            press = step
            continue
        p(4, f'if ({kbstate(action)}) {{')
        emit(5, step)
        p(4, '}')
    if press is not None and len(v) > 1:
        p(4, 'else {')
        emit(5, press)
        p(4, '}')
    elif press is not None:
        emit(4, press)
    p(4, 'break;')
p(2, '}')
p(1, '}')
p()

p(0, 'public:')
p(1, 'void press(int keycode, std::deque<char> & out) {')
for k in keymap['_track']:
    p(2, f'if (keycode == KEY_{k})'.ljust(30), f'{trackvar(k)} = 1;')
for k in keymap['_toggle']:
    p(2, f'if (keycode == KEY_{k})'.ljust(30), f'{togglevar(k)} = !{togglevar(k)};')
p(2, '_translate_press(keycode, out);')
p(1, '}')
p()

p(1, 'void release(int keycode, std::deque<char> & out __attribute__((unused))) {')
for k in keymap['_track']:
    p(2, f'if (keycode == KEY_{k})'.ljust(30), f'{trackvar(k)} = 0;')
p(1, '}')
p(0, '};')
