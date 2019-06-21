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
#include "main.h"
#include "frontend.h"
#include "gpu/cos.h"
#include "gpu/shader.h"
#include "gpu/rasterizer.h"
#include "gpu/texturing.h"

//#include "kitten.h"

#define Vec4FP24(x, y, z, w) MakeVec(\
		float24::FromFloat32(x),\
        float24::FromFloat32(y),\
        float24::FromFloat32(z),\
		float24::FromFloat32(w))

#define M_TAU (2*M_PI)
#define AngleFromDegrees(_angle) ((_angle)*M_TAU/360.0f)

void Mtx_PerspTilt(Vec4<float24> *projection, float fovy, float aspect, 
		float near, float far, bool isLeftHanded) {
	// Notes:
	// Includes adjusting depth range from [-1,1] to [-1,0]

	// Notes:
	// fovx = 2 atan(tan(fovy/2)*w/h)
	// fovy = 2 atan(tan(fovx/2)*h/w)
	// invaspect = h/w

	// a0,0 = h / (w*tan(fovy/2)) =
	//      = h / (w*tan(2 atan(tan(fovx/2)*h/w) / 2)) =
	//      = h / (w*tan( atan(tan(fovx/2)*h/w) )) =
	//      = h / (w * tan(fovx/2)*h/w) =
	//      = 1 / tan(fovx/2)

	// a1,1 = 1 / tan(fovy/2) = (...) = w / (h*tan(fovx/2))

	float fovy_tan = tanf(fovy/2.0f);

	float z = isLeftHanded ? 1.0f : -1.0f;
	float scale = near / (near - far);
	projection[0] = Vec4FP24(1.0f / fovy_tan, 0.0f, 0.0f, 0.0f);
	projection[1] = Vec4FP24(0.0f, 1.0f / (fovy_tan*aspect), 0.0f, 0.0f);
	projection[2] = Vec4FP24(0.0f, 0.0f, -z * scale, far * scale);
	projection[3] = Vec4FP24(0.0f, 0.0f, z, 0.0f);
}

int main(int argc, char *argv[]) {
	printf("Coscoroba Emulator\nVersion %s\n", VERSION);
	
	//Frontend::Init();
	auto &frontend = singleton<Frontend>();

	constexpr int VERTEX_COUNT = 4;
	Shader::OutputVertex vertex[VERTEX_COUNT];

	// Left lower corner
	// color is reused as texcoord
	vertex[0].pos = Vec4FP24(100.0f, 220.0f, 0.5f, 1.0f);
	vertex[0].color = Vec4FP24(0.0f, 1.0f, 0.0f, 1.0f);
	// Right lower corner
	vertex[1].pos = Vec4FP24(300.0f, 220.0f, 0.5f, 1.0f);
	vertex[1].color = Vec4FP24(1.0f, 1.0f, 0.0f, 1.0f);
	// Right upper corner
	vertex[2].pos = Vec4FP24(300.0f, 20.0f, 0.5f, 1.0f);
	vertex[2].color = Vec4FP24(1.0f, 0.0f, 0.0f, 1.0f);
	// Left upper corner
	vertex[3].pos = Vec4FP24(100.0f, 20.0f, 0.5f, 1.0f);
	vertex[3].color = Vec4FP24(0.0f, 0.0f, 0.0f, 1.0f);
	
	Shader::Uniforms uniform;
	// Projection Matrix
	Vec4<float24> projection = 
	uniform.f[0] = Vec4FP24(0.005f, 0.0f, 0.0f, -1.0f);
	uniform.f[1] = Vec4FP24(0.0f, -5.0f/600.0f, 0.0f, 1.0f);
	uniform.f[2] = Vec4FP24(0.0f, 0.0f, 1.0f, -1.0f);
	uniform.f[3] = Vec4FP24(0.0f, 0.0f, 0.0f, -1.0f);
	// Constants
	uniform.f[95] = Vec4FP24(0.0f, 1.0f, -1.0f, 0.1f);

	Shader::Setup setup;
	setup.program_code[0]  = 0x4e000000; // mov  r0.xyz_  v0.xyzw
	setup.program_code[1]  = 0x4e07f001; // mov  r0.___w c95.yyyy
	setup.program_code[2]  = 0x0a224802; // dp4  r1.x___  c4.xyzw  r0.xyzw
	setup.program_code[3]  = 0x0a225803; // dp4  r1._y__  c5.xyzw  r0.xyzw
	setup.program_code[4]  = 0x0a226804; // dp4  r1.__z_  c6.xyzw  r0.xyzw
	setup.program_code[5]  = 0x0a227805; // dp4  r1.___w  c7.xyzw  r0.xyzw
	setup.program_code[2]  = 0x08020882; // dp4  o0.x___  c0.xyzw  r1.xyzw
	setup.program_code[3]  = 0x08021883; // dp4  o0._y__  c1.xyzw  r1.xyzw
	setup.program_code[4]  = 0x08022884; // dp4  o0.__z_  c2.xyzw  r1.xyzw
	setup.program_code[5]  = 0x08023885; // dp4  o0.___w  c3.xyzw  r1.xyzw
	setup.program_code[6]  = 0x4c201006; // mov  o1.xyzw  v1.xyzw
	setup.program_code[7]  = 0x88000000; // end

	setup.swizzle_data[0]  = 0x0000036e; // xyz_, xyzw, xxxx, xxxx
	setup.swizzle_data[1]  = 0x00000aa1; // ___w, yyyy, xxxx, xxxx
	setup.swizzle_data[2]  = 0x0006c368; // x___, xyzw, xyzw, xxxx
	setup.swizzle_data[3]  = 0x0006c364; // _y__, xyzw, xyzw, xxxx
	setup.swizzle_data[4]  = 0x0006c362; // __z_, xyzw, xyzw, xxxx
	setup.swizzle_data[5]  = 0x0006c361; // ___w, xyzw, xyzw, xxxx
	setup.swizzle_data[6]  = 0x0000036f; // xyzw, xyzw, xxxx, xxxx

	setup.entry_point = 0x0000;

	Shader::ShaderEngine shader_engine(setup, uniform);
	for (int i = 0; i < VERTEX_COUNT; i++) {
		shader_engine.LoadInput(*((Shader::AttributeBuffer *)&vertex[i]));
		shader_engine.Run();
		shader_engine.WriteOutput(*((Shader::AttributeBuffer *)&vertex[i]));
	}
	
	Rasterizer rasterizer;

	rasterizer.AddTriangle(vertex[0], vertex[1], vertex[2]);
	rasterizer.AddTriangle(vertex[2], vertex[3], vertex[0]);
	
	/*
	// Fun with texture
	Texturing::TextureInfo textureInfo;
	textureInfo.width = 64;
	textureInfo.height = 64;
	textureInfo.stride = 8 * 8 * 4 * 8;
	textureInfo.format = Texturing::RGBA8;

	for (int x = 0; x < 64; x++)
		for (int y = 0; y < 64; y++) {
			Vec4<uint8_t> color = Texturing::LookupTexture(kitten_raw + 4,
					x, y, textureInfo);
			frontend.DrawPixel(x * 2, y * 2, color.r(), color.g(), color.b());
			frontend.DrawPixel(x * 2 + 1, y * 2, color.r(), color.g(), color.b());
			frontend.DrawPixel(x * 2, y * 2 + 1, color.r(), color.g(), color.b());
			frontend.DrawPixel(x * 2 + 1, y * 2 + 1, color.r(), color.g(), color.b());
		}
	*/

	frontend.Flip();

	while (frontend.PollEvent()) {
		// ?
	}

	//Frontend::Deinit();
}