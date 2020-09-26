import json

def load():
    with open('./src/kblayout.js') as f:
        lines = ''
        for line in f:
            if line.startswith('//'):
                continue
            lines += line
        lines = lines.replace('{w:', '{"W":')


