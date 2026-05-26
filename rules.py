# SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

def rules(ctx):
    ctx.rule(':build', [
        'src/:build',
    ])
