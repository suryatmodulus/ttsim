#!/usr/bin/env python3
# SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

# Strips trailing whitespace and carriage returns from each named file in place.
import sys

for path in sys.argv[1:]:
    with open(path, 'rb') as f:
        data = f.read()
    data = data.replace(b'\r', b'') # swallow all carriage returns
    while True:
        new_data = data.replace(b' \n', b'\n').replace(b'\t\n', b'\n')
        if data == new_data:
            break
        data = new_data
    with open(path, 'wb') as f:
        f.write(data)
