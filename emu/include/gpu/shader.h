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
// Shader common definition
#include "cos.h"
#include "isa.h"
#include "vec.h"
#include "float.h"

// Size of code storage and swizzle data region, in 32-bit word
constexpr unsigned MAX_PROGRAM_CODE_LENGTH = 512;
constexpr unsigned MAX_SWIZZLE_DATA_LENGTH = 128;

using isa::DestRegister;
using isa::RegisterType;
using isa::SourceRegister;

namespace Shader {
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

    // Register file owned by each hardware thread
    struct Registers {
        Vec4<float24> input[16];
        Vec4<float24> temporary[16];
        Vec4<float24> output[16];

        static std::size_t InputOffset(const SourceRegister& reg) {
            switch (reg.GetRegisterType()) {
            case RegisterType::Input:
                return offsetof(Registers, input) +
                    reg.GetIndex() * sizeof(Vec4<float24>);

            case RegisterType::Temporary:
                return offsetof(Registers, temporary) +
                    reg.GetIndex() * sizeof(Vec4<float24>);

            default:
                UNREACHABLE();
                return 0;
            }
        }

        static std::size_t OutputOffset(const DestRegister& reg) {
            switch (reg.GetRegisterType()) {
            case RegisterType::Output:
                return offsetof(Registers, output) +
                    reg.GetIndex() * sizeof(Vec4<float24>);

            case RegisterType::Temporary:
                return offsetof(Registers, temporary) +
                    reg.GetIndex() * sizeof(Vec4<float24>);

            default:
                UNREACHABLE();
                return 0;
            }
        }
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

    // Shader setup is common among few shaders at the same stage
    struct Setup {
        std::array<uint32_t, MAX_PROGRAM_CODE_LENGTH> program_code;
        std::array<uint32_t, MAX_SWIZZLE_DATA_LENGTH> swizzle_data;
        unsigned int entry_point;
    };

    class ShaderEngine {
    public:
        ShaderEngine(Setup &setup, Uniforms &uniforms) : 
            setup(setup), uniforms(uniforms) {}
        void LoadInput(const AttributeBuffer& input);
        void WriteOutput(AttributeBuffer& output);
        void SetupBatch(unsigned int entry_point);
        void Run();
            
    private:
        // Common among shaders at the same stage
        Setup &setup;
        // Common among all shaders
        Uniforms &uniforms;
        // Specific to one shader
        Registers registers;
        bool conditional_code[2];
        signed int address_registers[3];
    };

    static_assert(sizeof(OutputVertex) == 24 * 4, "OutputVertex has invalid size");

};
