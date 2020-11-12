# http://www.keyboard-layout-editor.com/#/
W = 15 # no. of width==1 keys in a row
H = 5  # no. of width==1 keys in a column

# Requirements:
#  - No empty keys
#  - MOD key: Left top name, right top MOD
#  - Other keys: Left bottom normal, left top shift, right top Fn
#            or: Right top normal

w = 'width'
rows = [
    ["~\n`","!\n1\nF1","@\n2\nF2","#\n3\nF3","$\n4\nF4","%\n5\nF5","^\n6\nF6","&\n7\nF7","*\n8\nF8","(\n9\nF9",")\n0\nF10","_\n-\nF11","+\n=\nF12",{w:2},"\nBackspace"],
    [{w:1.5},"Tab","Q\nq","W\nw","E\ne","R\nr","T\nt","Y\ny","U\nu","I\ni","O\no","P\np","{\n[","}\n]",{w:1.5},"|\n\\"],
    [{w:1.75},"Escape","A\na","S\ns","D\nd","F\nf","G\ng","H\nh","J\nj","K\nk","L\nl",":\n;","\"\n'",{w:2.25},"Enter"],
    [{w:2.25},"Shift\n\nMOD","Z\nz","X\nx","C\nc","V\nv","B\nb","N\nn","M\nm","<\n,",">\n.","?\n/",{w:2.75},"Shift\n\nMOD"],
    [{w:1.25},"Ctrl\n\nMOD",{w:1.25},"Fn\n\nMOD",{w:1.25},"Alt\n\nMOD",{w:6.25},"Space",{w:1.25},"\nLeft",{w:1.25},"\nDown",{w:1.25},"\nUp",{w:1.25},"\nRight"]
]

special = {
    'Escape': '\x1b',
    'Tab': ['\t', '\x1b[Z'],
    'Backspace': '\x7f',
    'Enter': '\n',
    'Left': '\x1b[D',
    'Right': '\x1b[C',
    'Up': '\x1b[A',
    'Down': '\x1b[B',
    'Space': ' '
}


DEFAULT_CONFIG = {w: 1, 'mod': 'NOMOD', 'text': '<->', 'normal': '', 'shift': '', 'fn': ''}
allkeys = []
for y, row in enumerate(rows):
    config = dict(DEFAULT_CONFIG)
    x = 0
    for cell in row:
        if isinstance(cell, dict):
            config = dict(DEFAULT_CONFIG)
            config.update(cell)
        elif isinstance(cell, str):
            parts = cell.split('\n')
            if len(parts) > 2 and parts[2] == 'MOD':
                config['mod'] = parts[0].upper()
                config['text'] = parts[0]
            else:
                if len(parts) == 1:
                    config['normal'] = parts[0]
                if len(parts) > 1:
                    config['shift'] = parts[0]
                    config['normal'] = parts[1]
                if len(parts) > 2:
                    config['fn'] = parts[2]
                config['text'] = config['normal']
                if config['normal'] in special:
                    replace = special[config['normal']]
                    if isinstance(replace, list):
                        config['normal'], config['shift'] = replace
                    else:
                        config['normal'] = config['shift'] = replace
            config['x'] = x
            config['y'] = y
            config['h'] = 1
            allkeys.append(config)
            x += config[w]
            config = dict(DEFAULT_CONFIG)


DEF = '''
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
'''

fmt = ' '.join(map(str.strip, '''{{
    .mod = {mod},
    .text = {text},
    .normal = {normal},
    .shift = {shift},
    .fn = {fn},
    .x = {x},
    .y = {y},
    .w = {width},
    .h = {h},
    {{}},
}}'''.splitlines()))

def cstr(s, encoding='ascii'):
    #s = s.encode(encoding)
    result = ''
    for c in s:
        if not (32 <= ord(c) < 127) or c in ('\\', '"'):
            result += '\\%03o' % ord(c)
        else:
            result += c
    return '"' + result + '"'

print(DEF)
print(f'#define OSK_NKEYS {len(allkeys)}')
print(f'#define OSK_W {W}')
print(f'#define OSK_H {H}')
print(f'struct kbkey osk_keys[{len(allkeys)}] = {{')
for key in allkeys:
    for k in 'text', 'normal', 'shift', 'fn':
        key[k] = cstr(key[k])
    print('    ' + fmt.format(**key) + ',')
print('};')
