/*
 *  Project Coscoroba
 *
 *  Copyright (C) 2019  Wenting Zhang <zephray@outlook.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms and conditions of the GNU General Public License,
 *  version 2, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <functional>
#include <type_traits>
#include <string>
#include <sstream>
#include <map>
#include <numeric>

// Common functions

/// Textually concatenates two tokens. The double-expansion is required by the 
//  C preprocessor.
#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y

// helper macro to properly align structure members.
// Calling INSERT_PADDING_BYTES will add a new member variable with a name like "pad121",
// depending on the current source line to make sure variable names are unique.
#define INSERT_PADDING_BYTES(num_bytes) uint8_t CONCAT2(pad, __LINE__)[(num_bytes)]
#define INSERT_PADDING_WORDS(num_words) uint32_t CONCAT2(pad, __LINE__)[(num_words)]

#define ASSERT(_a_, ...)                                                 \
    do                                                                   \
        if (!(_a_)) {                                                    \
            fprintf(stderr, "Assertion failed!\n" __VA_ARGS__); exit(1); \
        }                                                                \
    while (0)

#define UNREACHABLE() { ASSERT(false, "Unreachable code!"); }
#define UNIMPLEMENTED() { fprintf(stderr, "Unimplemented function!"); }
