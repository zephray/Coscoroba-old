/*
 *  Project Coscoroba
 *
 *  Copyright (C) 2015  Citra Emulator Project
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

#include <cstring>

#include "main.h"
#include "vec.h"

namespace Color {

/// Convert a 1-bit color component to 8 bit
constexpr uint8_t Convert1To8(uint8_t value) {
    return value * 255;
}

/// Convert a 4-bit color component to 8 bit
constexpr uint8_t Convert4To8(uint8_t value) {
    return (value << 4) | value;
}

/// Convert a 5-bit color component to 8 bit
constexpr uint8_t Convert5To8(uint8_t value) {
    return (value << 3) | (value >> 2);
}

/// Convert a 6-bit color component to 8 bit
constexpr uint8_t Convert6To8(uint8_t value) {
    return (value << 2) | (value >> 4);
}

/// Convert a 8-bit color component to 1 bit
constexpr uint8_t Convert8To1(uint8_t value) {
    return value >> 7;
}

/// Convert a 8-bit color component to 4 bit
constexpr uint8_t Convert8To4(uint8_t value) {
    return value >> 4;
}

/// Convert a 8-bit color component to 5 bit
constexpr uint8_t Convert8To5(uint8_t value) {
    return value >> 3;
}

/// Convert a 8-bit color component to 6 bit
constexpr uint8_t Convert8To6(uint8_t value) {
    return value >> 2;
}

/**
 * Decode a color stored in RGBA8 format
 * @param bytes Pointer to encoded source color
 * @return Result color decoded as Vec4<uint8_t>
 */
inline Vec4<uint8_t> DecodeRGBA8(const uint8_t* bytes) {
    return {bytes[3], bytes[2], bytes[1], bytes[0]};
}

/**
 * Decode a color stored in RGB8 format
 * @param bytes Pointer to encoded source color
 * @return Result color decoded as Vec4<uint8_t>
 */
inline Vec4<uint8_t> DecodeRGB8(const uint8_t* bytes) {
    return {bytes[2], bytes[1], bytes[0], 255};
}

/**
 * Decode a color stored in RG8 (aka HILO8) format
 * @param bytes Pointer to encoded source color
 * @return Result color decoded as Vec4<uint8_t>
 */
inline Vec4<uint8_t> DecodeRG8(const uint8_t* bytes) {
    return {bytes[1], bytes[0], 0, 255};
}

/**
 * Decode a color stored in RGB565 format
 * @param bytes Pointer to encoded source color
 * @return Result color decoded as Vec4<uint8_t>
 */
inline Vec4<uint8_t> DecodeRGB565(const uint8_t* bytes) {
    uint16_t pixel;
    std::memcpy(&pixel, bytes, sizeof(pixel));
    return {Convert5To8((pixel >> 11) & 0x1F), Convert6To8((pixel >> 5) & 0x3F),
            Convert5To8(pixel & 0x1F), 255};
}

/**
 * Decode a color stored in RGB5A1 format
 * @param bytes Pointer to encoded source color
 * @return Result color decoded as Vec4<uint8_t>
 */
inline Vec4<uint8_t> DecodeRGB5A1(const uint8_t* bytes) {
    uint16_t pixel;
    std::memcpy(&pixel, bytes, sizeof(pixel));
    return {Convert5To8((pixel >> 11) & 0x1F), Convert5To8((pixel >> 6) & 0x1F),
            Convert5To8((pixel >> 1) & 0x1F), Convert1To8(pixel & 0x1)};
}

/**
 * Decode a color stored in RGBA4 format
 * @param bytes Pointer to encoded source color
 * @return Result color decoded as Vec4<uint8_t>
 */
inline Vec4<uint8_t> DecodeRGBA4(const uint8_t* bytes) {
    uint16_t pixel;
    std::memcpy(&pixel, bytes, sizeof(pixel));
    return {Convert4To8((pixel >> 12) & 0xF), Convert4To8((pixel >> 8) & 0xF),
            Convert4To8((pixel >> 4) & 0xF), Convert4To8(pixel & 0xF)};
}

/**
 * Decode a depth value stored in D16 format
 * @param bytes Pointer to encoded source value
 * @return Depth value as an uint32_t
 */
inline uint32_t DecodeD16(const uint8_t* bytes) {
    uint16_t data;
    std::memcpy(&data, bytes, sizeof(data));
    return data;
}

/**
 * Decode a depth value stored in D24 format
 * @param bytes Pointer to encoded source value
 * @return Depth value as an uint32_t
 */
inline uint32_t DecodeD24(const uint8_t* bytes) {
    return (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

/**
 * Decode a depth value and a stencil value stored in D24S8 format
 * @param bytes Pointer to encoded source values
 * @return Resulting values stored as a Vec2
 */
inline Vec2<uint32_t> DecodeD24S8(const uint8_t* bytes) {
    return {static_cast<uint32_t>((bytes[2] << 16) | (bytes[1] << 8) | bytes[0]), bytes[3]};
}

/**
 * Encode a color as RGBA8 format
 * @param color Source color to encode
 * @param bytes Destination pointer to store encoded color
 */
inline void EncodeRGBA8(const Vec4<uint8_t>& color, uint8_t* bytes) {
    bytes[3] = color.r();
    bytes[2] = color.g();
    bytes[1] = color.b();
    bytes[0] = color.a();
}

/**
 * Encode a color as RGB8 format
 * @param color Source color to encode
 * @param bytes Destination pointer to store encoded color
 */
inline void EncodeRGB8(const Vec4<uint8_t>& color, uint8_t* bytes) {
    bytes[2] = color.r();
    bytes[1] = color.g();
    bytes[0] = color.b();
}

/**
 * Encode a color as RG8 (aka HILO8) format
 * @param color Source color to encode
 * @param bytes Destination pointer to store encoded color
 */
inline void EncodeRG8(const Vec4<uint8_t>& color, uint8_t* bytes) {
    bytes[1] = color.r();
    bytes[0] = color.g();
}
/**
 * Encode a color as RGB565 format
 * @param color Source color to encode
 * @param bytes Destination pointer to store encoded color
 */
inline void EncodeRGB565(const Vec4<uint8_t>& color, uint8_t* bytes) {
    const uint16_t data =
        (Convert8To5(color.r()) << 11) | (Convert8To6(color.g()) << 5) | Convert8To5(color.b());

    std::memcpy(bytes, &data, sizeof(data));
}

/**
 * Encode a color as RGB5A1 format
 * @param color Source color to encode
 * @param bytes Destination pointer to store encoded color
 */
inline void EncodeRGB5A1(const Vec4<uint8_t>& color, uint8_t* bytes) {
    const uint16_t data = (Convert8To5(color.r()) << 11) | (Convert8To5(color.g()) << 6) |
                        (Convert8To5(color.b()) << 1) | Convert8To1(color.a());

    std::memcpy(bytes, &data, sizeof(data));
}

/**
 * Encode a color as RGBA4 format
 * @param color Source color to encode
 * @param bytes Destination pointer to store encoded color
 */
inline void EncodeRGBA4(const Vec4<uint8_t>& color, uint8_t* bytes) {
    const uint16_t data = (Convert8To4(color.r()) << 12) | (Convert8To4(color.g()) << 8) |
                     (Convert8To4(color.b()) << 4) | Convert8To4(color.a());

    std::memcpy(bytes, &data, sizeof(data));
}

/**
 * Encode a 16 bit depth value as D16 format
 * @param value 16 bit source depth value to encode
 * @param bytes Pointer where to store the encoded value
 */
inline void EncodeD16(uint32_t value, uint8_t* bytes) {
    const uint16_t data = static_cast<uint16_t>(value);
    std::memcpy(bytes, &data, sizeof(data));
}

/**
 * Encode a 24 bit depth value as D24 format
 * @param value 24 bit source depth value to encode
 * @param bytes Pointer where to store the encoded value
 */
inline void EncodeD24(uint32_t value, uint8_t* bytes) {
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
}

/**
 * Encode a 24 bit depth and 8 bit stencil values as D24S8 format
 * @param depth 24 bit source depth value to encode
 * @param stencil 8 bit source stencil value to encode
 * @param bytes Pointer where to store the encoded value
 */
inline void EncodeD24S8(uint32_t depth, uint8_t stencil, uint8_t* bytes) {
    bytes[0] = depth & 0xFF;
    bytes[1] = (depth >> 8) & 0xFF;
    bytes[2] = (depth >> 16) & 0xFF;
    bytes[3] = stencil;
}

/**
 * Encode a 24 bit depth value as D24X8 format (32 bits per pixel with 8 bits unused)
 * @param depth 24 bit source depth value to encode
 * @param bytes Pointer where to store the encoded value
 * @note unused bits will not be modified
 */
inline void EncodeD24X8(uint32_t depth, uint8_t* bytes) {
    bytes[0] = depth & 0xFF;
    bytes[1] = (depth >> 8) & 0xFF;
    bytes[2] = (depth >> 16) & 0xFF;
}

/**
 * Encode an 8 bit stencil value as X24S8 format (32 bits per pixel with 24 bits unused)
 * @param stencil 8 bit source stencil value to encode
 * @param bytes Pointer where to store the encoded value
 * @note unused bits will not be modified
 */
inline void EncodeX24S8(uint8_t stencil, uint8_t* bytes) {
    bytes[3] = stencil;
}

} // namespace Color
