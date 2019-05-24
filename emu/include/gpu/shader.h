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
// Shader common definition
#include "cos.h"
#include "vec.h"
#include "float.h"

// General purpose attribute buffer, used as VS input and output
struct AttributeBuffer {
    Vec4<float24> attr[16];
};

// TODO: Actually, when programmable FS is used, these shouldn't be predefined.
struct OutputVertex {
    Vec4<float24> pos;
    Vec4<float24> quat;
    Vec4<float24> color;
    Vec2<float24> tc0;
    Vec2<float24> tc1;
    float24 tc0_w;
    INSERT_PADDING_WORDS(1);
    Vec3<float24> view;
    INSERT_PADDING_WORDS(1);
    Vec2<float24> tc2;
};

struct Uniforms {
    // The float uniforms are accessed by the shader JIT using SSE instructions, and are
    // therefore required to be 16-byte aligned.
    Vec4<float24> f[96];

    std::array<bool, 16> b;
    std::array<Vec4<uint8_t>, 4> i;

    static std::size_t GetFloatUniformOffset(unsigned index) {
        return offsetof(Uniforms, f) + index * sizeof(Vec4<float24>);
    }

    static std::size_t GetBoolUniformOffset(unsigned index) {
        return offsetof(Uniforms, b) + index * sizeof(bool);
    }

    static std::size_t GetIntUniformOffset(unsigned index) {
        return offsetof(Uniforms, i) + index * sizeof(Vec4<uint8_t>);
    }
};

struct ShaderSetup {
    Uniforms uniforms;
};

class Shader {
public:
    Shader(ShaderSetup &shader_setup) : shader_setup(shader_setup) {}
    void Run();
private:
    ShaderSetup &shader_setup;
};

static_assert(sizeof(OutputVertex) == 24 * 4, "OutputVertex has invalid size");
