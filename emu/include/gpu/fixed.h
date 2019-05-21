/*
 *  Project Coscoroba
 *
 *  Copyright (C) 2015  Citra Emulator Project
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

// NOTE: Assuming that rasterizer coordinates are 12.4 fixed-point values
struct Fix12P4 {
    Fix12P4() {}
    Fix12P4(uint16_t val) : val(val) {}

    static uint16_t FracMask() {
        return 0xF;
    }
    static uint16_t IntMask() {
        return (uint16_t)~0xF;
    }

    operator uint16_t() const {
        return val;
    }

    bool operator<(const Fix12P4& oth) const {
        return (uint16_t) * this < (uint16_t)oth;
    }

private:
    uint16_t val;
};
