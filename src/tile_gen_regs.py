#!/usr/bin/env python3
# SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

# Emits _out/{chip}/tile_regs.h from data/{chip}/tile_regs.json.
# The JSON files are produced (and maintained) by scripts/parse_tile_regs.py.
import argparse
import json

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--chip', action='store', required=True)
    parser.add_argument('--out', action='store', required=True)
    args = parser.parse_args()

    with open(f'../data/{args.chip}/tile_regs.json') as f:
        data = json.load(f)

    with open(args.out, 'w') as f:
        for entry in data['address_map']:
            base = int(entry['base'], 0)
            limit = int(entry['limit'], 0)
            name = entry['name']
            assert base <= limit, (hex(base), hex(limit))
            f.write(f'#define {name.upper()}_BASE 0x{base:08X}\n')
            f.write(f'#define {name.upper()}_LIMIT 0x{limit:08X}\n')
        f.write('\n')

        for module in data['regs']:
            module_name = module['name']
            for reg in module['regs']:
                offset = int(reg['offset'], 0)
                reg_name = reg['name']
                array_size = reg.get('array_size')
                reset_value = int(reg['reset_value'], 0) if reg['reset_value'] is not None else None
                if array_size is not None:
                    array_stride = int(reg['array_stride'], 0)
                    f.write(f'#define {module_name.upper()}_{reg_name.upper()}(i) (0x{offset:X} + {array_stride}*(i))\n')
                else:
                    f.write(f'#define {module_name.upper()}_{reg_name.upper()} 0x{offset:X}\n')
                if reset_value is not None:
                    f.write(f'#define {module_name.upper()}_{reg_name.upper()}_RESET_VALUE 0x{reset_value:X}\n')
            f.write('\n')

if __name__ == '__main__':
    main()
