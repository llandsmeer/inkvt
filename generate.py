import string
# remember: only on linux, not kobo!

digits = '1234567890'
upperd = '!@#$%^&*()'

class Key:
    def __init__(self, code, low, shift=None):
        self.code = code
        self.low = low
        self.shift = shift

keys = [Key('KEY_MINUS', '-', '_'),
        Key('KEY_EQUAL', '=', '+'),
        Key('KEY_LEFTBRACE', '[', '{'),
        Key('KEY_RIGHTBRACE', ']', '}'),
        Key('KEY_SEMICOLON', ';', ':'),
        Key('KEY_APOSTROPHE', "'", '"'),
        Key('KEY_BACKSLASH', '\\', '|'),
        Key('KEY_COMMA', ',', '<'),
        Key('KEY_DOT', '.', '>'),
        Key('KEY_SPACE', ' '),
        Key('KEY_SLASH', '/', '?'),

        Key('KEY_LEFTSHIFT', '?'),
        Key('KEY_BACKSPACE', 128),
        Key('KEY_ENTER', '\n'),
        Key('KEY_LEFTCTRL', '?', True),
        Key('KEY_RIGHTSHIFT', '?', True),
        Key('KEY_CAPSLOCK', '?', True),
]


with open('/usr/include/linux/input-event-codes.h') as f:
    for line in f:
        if 'KEY_' not in line:
            continue
        a, b, c, *d = line.split()
        if not c.startswith('0x') and not c.isdigit():
            continue
        name = b[4:]
        if len(name) == 1 and name in string.ascii_letters:
            keys.append(Key(b, name.lower(), name.upper()))
        if len(name) == 1 and name in digits:
            keys.append(Key(b, name, upperd[digits.index(name)]))
        name

def r(s):
    c = repr(s)
    if c[0] == '"':
        return f"'\\{s}'"
    return c
for key in keys:
    if key.shift is True:
        pass
    elif key.shift:
        print(f'''      case {key.code}:
            if (shift) return {r(key.shift)};
            else return {r(key.low)};''')
    else:
        print(f'''      case {key.code}:
            return {r(key.low)};''')

for key in keys:
    if key.shift is True:
        print(key.code)
