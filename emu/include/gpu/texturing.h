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
// Texture Mapping Unit
#include "cos.h"
#include "isa.h"
#include "vec.h"
#include "float.h"

namespace Texturing {
    enum TextureType {
        Texture2D = 0,
        TextureCube,
        Shadow2D,
        Projection2D,
        ShadowCube,
        Disabled
    };

    enum WrapMode {
        ClampToEdge = 0,
        ClampToBorder,
        Repeat,
        MirroredRepeat
    };

    enum TextureFilter {
        Nearest = 0,
        Linear = 1
    };

    enum TextureFormat {
        RGBA8 = 0,
        RGB8,
        RGB5A1,
        RGB565,
        RGBA4,
        IA8,
        RG8,
        I8,
        A8,
        IA4,
        I4,
        A4,
        ETC1,
        ETC1A4
    };

    struct TextureInfo {
        uint32_t physical_address;
        unsigned int width;
        unsigned int height;
        uint32_t stride;
        TextureFormat format;
    };

    int GetWrappedTexCoord(int val, unsigned size);

    // Returns the byte size of a 8*8 tile of the specified texture format.
    uint32_t CalculateTileSize(TextureFormat format);

    /**
    * Lookup texel located at the given coordinates and return an RGBA vector of
    * its color.
    * @param source Source pointer to read data from
    * @param x,y Texture coordinates to read from
    * @param info TextureInfo object describing the texture setup
    */
    Vec4<uint8_t> LookupTexture(const uint8_t* source, uint16_t x, uint16_t y,
            const TextureInfo& info);

    /**
    * Looks up a texel from a single 8x8 texture tile.
    *
    * @param source Pointer to the beginning of the tile.
    * @param x, y In-tile coordinates to read from. Must be < 8.
    * @param info TextureInfo describing the texture format.
    */
    Vec4<uint8_t> LookupTexelInTile(const uint8_t* source, uint16_t x, 
            uint16_t y, const TextureInfo& info);

    Vec3<uint8_t> SampleETC1Subtile(uint64_t value, unsigned int x, unsigned int y);
    
}