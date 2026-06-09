// SPDX-FileCopyrightText: (c) 2025-2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// RISC-V floating-point implementation helpers, currently for babyrisc FP32 RNE only.
#pragma once
#include <stdint.h>

static inline bool fp32_is_nan(uint32_t x) {
    return (x & 0x7FFFFFFF) > 0x7F800000;
}

static inline uint32_t fp32_classify(uint32_t x) {
    bool sign = x >> 31;
    uint32_t exp = (x >> 23) & 255;
    uint32_t man = x & 0x7FFFFF;
    if (exp == 255) {
        if (!man) {
            return sign ? (1u << 0) : (1u << 7); // -inf/+inf
        }
        return (man & 0x400000) ? (1u << 9) : (1u << 8); // qnan/snan
    } else if (exp == 0) {
        if (!man) {
            return sign ? (1u << 3) : (1u << 4); // -0/+0
        }
        return sign ? (1u << 2) : (1u << 5); // -subnormal/+subnormal
    } else {
        return sign ? (1u << 1) : (1u << 6); // -normal/+normal
    }
}

static inline bool fp32_eq_ordered(uint32_t a, uint32_t b) {
    return (a == b) || !((a | b) & 0x7FFFFFFF);
}

static inline bool fp32_lt_ordered(uint32_t a, uint32_t b) {
    if (fp32_eq_ordered(a, b)) {
        return false;
    }
    bool sign_a = a >> 31;
    bool sign_b = b >> 31;
    if (sign_a != sign_b) {
        return sign_a;
    }
    return sign_a ? ((a & 0x7FFFFFFF) > (b & 0x7FFFFFFF)) : (a < b);
}

// fmin.s/fmax.s: both NaN -> canonical NaN; one NaN -> the other operand; -0 compares below +0.
static inline uint32_t fp32_min_max(uint32_t a, uint32_t b, bool is_max) {
    if (fp32_is_nan(a)) {
        return fp32_is_nan(b) ? 0x7FC00000 : b;
    }
    if (fp32_is_nan(b)) {
        return a;
    }
    if (fp32_eq_ordered(a, b)) {
        // Only +0/-0 reach here with differing bits: OR the signs to pick -0 for min, AND for max.
        return is_max ? (a & b) : (a | b);
    }
    return (fp32_lt_ordered(a, b) != is_max) ? a : b;
}

static inline bool round_nearest_even_increment(uint64_t value, uint64_t half, uint64_t rem) {
    return (rem > half) || ((rem == half) && (value & 1));
}

static inline uint64_t fp32_to_integer_magnitude(uint32_t x) {
    uint32_t exp = (x >> 23) & 255;
    if (exp >= 159) { // note: caller must have excluded NaN
        return 0x100000000ull; // >= 2^32 (incl. inf): overflows both int32 and uint32
    } else if (exp == 0) {
        return 0; // zero or subnormal: magnitude < 1, rounds to 0
    }
    uint64_t sig = (1u << 23) | (x & 0x7FFFFF);
    int32_t shift = int32_t(exp) - 150;
    if (shift >= 0) {
        return sig << shift; // exp <= 158 here, so shift <= 8: no overflow
    }
    uint32_t rshift = uint32_t(-shift);
    if (rshift >= 64) {
        return 0; // magnitude rounds down to 0
    }
    uint64_t value = sig >> rshift;
    uint64_t rem = sig & ((1ull << rshift) - 1);
    return value + round_nearest_even_increment(value, 1ull << (rshift - 1), rem);
}

static inline uint32_t fp32_to_i32(uint32_t x) {
    if (fp32_is_nan(x)) {
        return 0x7FFFFFFF;
    }
    bool sign = x >> 31;
    uint64_t mag = fp32_to_integer_magnitude(x);
    if (sign) {
        if (mag >= 0x80000000ull) {
            return 0x80000000;
        }
        return uint32_t(0 - mag);
    }
    if (mag >= 0x80000000ull) {
        return 0x7FFFFFFF;
    }
    return uint32_t(mag);
}

static inline uint32_t fp32_to_u32(uint32_t x) {
    if (fp32_is_nan(x)) {
        return 0xFFFFFFFF;
    }
    if (x & 0x80000000) {
        return 0;
    }
    uint64_t mag = fp32_to_integer_magnitude(x);
    return (mag >= 0x100000000ull) ? 0xFFFFFFFF : uint32_t(mag);
}

static inline uint32_t integer_magnitude_to_fp32(uint32_t mag, bool sign) {
    if (!mag) {
        return 0; // integer 0 -> +0.0 (mag == 0 only for non-negative input, so sign is never set here)
    }
    uint32_t exp = 31 - __builtin_clz(mag);
    uint32_t sig = mag;
    if (exp <= 23) {
        sig <<= 23 - exp;
    } else {
        uint32_t shift = exp - 23;
        uint32_t rem = sig & ((1u << shift) - 1);
        sig >>= shift;
        if (round_nearest_even_increment(sig, 1u << (shift - 1), rem)) {
            sig++;
            if (sig == (1u << 24)) {
                sig >>= 1;
                exp++;
            }
        }
    }
    return (uint32_t(sign) << 31) | ((exp + 127) << 23) | (sig & 0x7FFFFF);
}

static inline uint32_t i32_to_fp32(uint32_t x) {
    bool sign = int32_t(x) < 0;
    uint32_t mag = sign ? uint32_t(0 - x) : x;
    return integer_magnitude_to_fp32(mag, sign);
}

static inline uint32_t u32_to_fp32(uint32_t x) {
    return integer_magnitude_to_fp32(x, false);
}
