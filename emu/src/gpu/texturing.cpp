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
#include "texturing.h"
#include "color.h"

namespace Texturing {

    constexpr size_t TILE_SIZE = 8*8;
    constexpr size_t ETC1_SUBTILES = 2*2;

    // 8x8 Z-Order coordinate from 2D coordinates
    static constexpr uint32_t MortonInterleave(uint32_t x, uint32_t y) {
        constexpr uint32_t xlut[] = {0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15};
        constexpr uint32_t ylut[] = {0x00, 0x02, 0x08, 0x0a, 0x20, 0x22, 0x28, 0x2a};
        return xlut[x % 8] + ylut[y % 8];
    }

    int GetWrappedTexCoord(int val, unsigned size) {
        /*switch (mode) {
        case TexturingRegs::TextureConfig::ClampToEdge2:
            // For negative coordinate, ClampToEdge2 behaves the same as Repeat
            if (val < 0) {
                return static_cast<int>(static_cast<unsigned>(val) % size);
            }
        // [[fallthrough]]
        case TexturingRegs::TextureConfig::ClampToEdge:
            val = std::max(val, 0);
            val = std::min(val, static_cast<int>(size) - 1);
            return val;

        case TexturingRegs::TextureConfig::ClampToBorder:
            return val;

        case TexturingRegs::TextureConfig::ClampToBorder2:
        // For ClampToBorder2, the case of positive coordinate beyond the texture size is already
        // handled outside. Here we only handle the negative coordinate in the same way as Repeat.
        case TexturingRegs::TextureConfig::Repeat2:
        case TexturingRegs::TextureConfig::Repeat3:
        case TexturingRegs::TextureConfig::Repeat:
            return static_cast<int>(static_cast<unsigned>(val) % size);

        case TexturingRegs::TextureConfig::MirroredRepeat: {
            unsigned int coord = (static_cast<unsigned>(val) % (2 * size));
            if (coord >= size)
                coord = 2 * size - 1 - coord;
            return static_cast<int>(coord);
        }

        default:
            LOG_ERROR(HW_GPU, "Unknown texture coordinate wrapping mode {:x}", (int)mode);
            UNIMPLEMENTED();
            return 0;
        }*/
        return val;
    }

    uint32_t CalculateTileSize(TextureFormat format) {
        switch (format) {
        case TextureFormat::RGBA8:
            return 4 * TILE_SIZE;

        case TextureFormat::RGB8:
            return 3 * TILE_SIZE;

        case TextureFormat::RGB5A1:
        case TextureFormat::RGB565:
        case TextureFormat::RGBA4:
        case TextureFormat::IA8:
        case TextureFormat::RG8:
            return 2 * TILE_SIZE;

        case TextureFormat::I8:
        case TextureFormat::A8:
        case TextureFormat::IA4:
            return 1 * TILE_SIZE;

        case TextureFormat::I4:
        case TextureFormat::A4:
            return TILE_SIZE / 2;

        case TextureFormat::ETC1:
            return ETC1_SUBTILES * 8;

        case TextureFormat::ETC1A4:
            return ETC1_SUBTILES * 16;

        default: // placeholder for yet unknown formats
            UNIMPLEMENTED();
            return 0;
        }
    }

    Vec4<uint8_t> LookupTexture(const uint8_t* source, uint16_t x, uint16_t y,
            const TextureInfo& info) {
        // Coordinate in tiles
        const uint16_t coarse_x = x / 8;
        const uint16_t coarse_y = y / 8;

        // Coordinate inside the tile
        const uint16_t fine_x = x % 8;
        const uint16_t fine_y = y % 8;

        const uint8_t* line = source + coarse_y * info.stride;
        const uint8_t* tile = line + coarse_x * CalculateTileSize(info.format);
        return LookupTexelInTile(tile, fine_x, fine_y, info);
    }

    Vec4<uint8_t> LookupTexelInTile(const uint8_t* source, uint16_t x, 
            uint16_t y, const TextureInfo& info) {
        switch (info.format) {
        case TextureFormat::RGBA8: {
            auto res = Color::DecodeRGBA8(source + MortonInterleave(x, y) * 4);
            return {res.r(), res.g(), res.b(), res.a()};
        }

        case TextureFormat::RGB8: {
            auto res = Color::DecodeRGB8(source + MortonInterleave(x, y) * 3);
            return {res.r(), res.g(), res.b(), 255};
        }

        case TextureFormat::RGB5A1: {
            auto res = Color::DecodeRGB5A1(source + MortonInterleave(x, y) * 2);
            return {res.r(), res.g(), res.b(), res.a()};
        }

        case TextureFormat::RGB565: {
            auto res = Color::DecodeRGB565(source + MortonInterleave(x, y) * 2);
            return {res.r(), res.g(), res.b(), 255};
        }

        case TextureFormat::RGBA4: {
            auto res = Color::DecodeRGBA4(source + MortonInterleave(x, y) * 2);
            return {res.r(), res.g(), res.b(), res.a()};
        }

        case TextureFormat::IA8: {
            const uint8_t* source_ptr = source + MortonInterleave(x, y) * 2;
            return {source_ptr[1], source_ptr[1], source_ptr[1], source_ptr[0]};
        }

        case TextureFormat::RG8: {
            auto res = Color::DecodeRG8(source + MortonInterleave(x, y) * 2);
            return {res.r(), res.g(), 0, 255};
        }

        case TextureFormat::I8: {
            const uint8_t* source_ptr = source + MortonInterleave(x, y);
            return {*source_ptr, *source_ptr, *source_ptr, 255};
        }

        case TextureFormat::A8: {
            const uint8_t* source_ptr = source + MortonInterleave(x, y);
            return {0, 0, 0, *source_ptr};
        }

        case TextureFormat::IA4: {
            const uint8_t* source_ptr = source + MortonInterleave(x, y);

            uint8_t i = Color::Convert4To8(((*source_ptr) & 0xF0) >> 4);
            uint8_t a = Color::Convert4To8((*source_ptr) & 0xF);
            
            return {i, i, i, a};
        }

        case TextureFormat::I4: {
            uint32_t morton_offset = MortonInterleave(x, y);
            const uint8_t* source_ptr = source + morton_offset / 2;

            uint8_t i = (morton_offset % 2) ? ((*source_ptr & 0xF0) >> 4) : (*source_ptr & 0xF);
            i = Color::Convert4To8(i);

            return {i, i, i, 255};
        }

        case TextureFormat::A4: {
            uint32_t morton_offset = MortonInterleave(x, y);
            const uint8_t* source_ptr = source + morton_offset / 2;

            uint8_t a = (morton_offset % 2) ? ((*source_ptr & 0xF0) >> 4) : (*source_ptr & 0xF);
            a = Color::Convert4To8(a);

            return {0, 0, 0, a};
        }

        // Does not support ETC for now
        /*case TextureFormat::ETC1:
        case TextureFormat::ETC1A4: {
            bool has_alpha = (info.format == TextureFormat::ETC1A4);
            std::size_t subtile_size = has_alpha ? 16 : 8;

            // ETC1 further subdivides each 8x8 tile into four 4x4 subtiles
            constexpr unsigned int subtile_width = 4;
            constexpr unsigned int subtile_height = 4;

            unsigned int subtile_index = (x / subtile_width) + 2 * (y / subtile_height);
            x %= subtile_width;
            y %= subtile_height;

            const uint8_t* subtile_ptr = source + subtile_index * subtile_size;

            uint8_t alpha = 255;

            uint64_t packed_alpha;
            memcpy(&packed_alpha, subtile_ptr, sizeof(uint64_t));
            subtile_ptr += sizeof(uint64_t);

            alpha = Color::Convert4To8((packed_alpha >> (4 * (x * subtile_width + y))) & 0xF);

            uint64_t subtile_data;
            memcpy(&subtile_data, subtile_ptr, sizeof(uint64_t));

            return MakeVec(SampleETC1Subtile(subtile_data, x, y), alpha);
        }*/

        default:
            fprintf(stderr, "TMU: Unknown texture format: %u", (uint32_t)info.format);
            return {};
        }
    }
}