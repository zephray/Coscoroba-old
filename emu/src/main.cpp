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

void Mtx_Diagonal(Vec4<float24>* out, float x, float y, float z, float w) {
	out[0] = Vec4FP24(x, 0.0f, 0.0f, 0.0f);
	out[1] = Vec4FP24(0.0f, y, 0.0f, 0.0f);
	out[2] = Vec4FP24(0.0f, 0.0f, z, 0.0f);
	out[3] = Vec4FP24(0.0f, 0.0f, 0.0f, w);
}

void Mtx_Identity(Vec4<float24>* out) {
	Mtx_Diagonal(out, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Mtx_Translate(Vec4<float24>* mtx, float x, float y, float z)
{
	Vec4<float24> v = Vec4FP24(x, y, z, 1.0f);
	for (int i = 0; i < 4; ++i)
		mtx[i].w = Dot(mtx[i], v);
}

void Mtx_RotateX(Vec4<float24>* mtx, float angle)
{
	float a, b;
	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);
	
	for (int i = 0; i < 4; ++i)
	{
		a = mtx[i].y.ToFloat32()*cosAngle + mtx[i].z.ToFloat32()*sinAngle;
		b = mtx[i].z.ToFloat32()*cosAngle - mtx[i].y.ToFloat32()*sinAngle;
		mtx[i].y = float24::FromFloat32(a);
		mtx[i].z = float24::FromFloat32(b);
	}
}

void Mtx_RotateY(Vec4<float24>* mtx, float angle)
{
	float a, b;
	float cosAngle = cosf(angle);
	float sinAngle = sinf(angle);
	
	for (int i = 0; i < 4; ++i)
	{
		a = mtx[i].x.ToFloat32()*cosAngle + mtx[i].z.ToFloat32()*sinAngle;
		b = mtx[i].z.ToFloat32()*cosAngle - mtx[i].x.ToFloat32()*sinAngle;
		mtx[i].x = float24::FromFloat32(a);
		mtx[i].z = float24::FromFloat32(b);
	}
}

void Mtx_PerspTilt(Vec4<float24>* projection, float fovy, float aspect, 
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

void Dbg_PrintVec(Vec4<float24> vec) {
	printf("%.3f, %.3f, %.3f, %.3f\n", 
			vec.x.ToFloat32(), vec.y.ToFloat32(), 
			vec.z.ToFloat32(), vec.w.ToFloat32());
}

int main(int argc, char *argv[]) {
	printf("Coscoroba Emulator\nVersion %s\n", VERSION);
	
	//Frontend::Init();
	auto &frontend = singleton<Frontend>();

	constexpr int VERTEX_COUNT = 36;
	Shader::OutputVertex vertex[VERTEX_COUNT];
	Shader::OutputVertex outputs[VERTEX_COUNT];

	// First face (PZ)
	// First triangle
	vertex[0].pos    = Vec4FP24(-0.5f, -0.5f, +0.5f, 1.0f);
	vertex[0].color  = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	vertex[1].pos    = Vec4FP24(+0.5f, -0.5f, +0.5f, 1.0f);
	vertex[1].color  = Vec4FP24(1.0f, 0.0f, 0.0f, 0.0f);
	vertex[2].pos    = Vec4FP24(+0.5f, +0.5f, +0.5f, 1.0f);
	vertex[2].color  = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	// Second triangle
	vertex[3].pos    = Vec4FP24(+0.5f, +0.5f, +0.5f, 1.0f);
	vertex[3].color  = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	vertex[4].pos    = Vec4FP24(-0.5f, +0.5f, +0.5f, 1.0f);
	vertex[4].color  = Vec4FP24(0.0f, 1.0f, 0.0f, 0.0f);
	vertex[5].pos    = Vec4FP24(-0.5f, -0.5f, +0.5f, 1.0f);
	vertex[5].color  = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	// Second face (MZ)
	// First triangle
	vertex[6].pos    = Vec4FP24(-0.5f, -0.5f, -0.5f, 1.0f);
	vertex[6].color  = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	vertex[7].pos    = Vec4FP24(-0.5f, +0.5f, -0.5f, 1.0f);
	vertex[7].color  = Vec4FP24(1.0f, 0.0f, 0.0f, 0.0f);
	vertex[8].pos    = Vec4FP24(+0.5f, +0.5f, -0.5f, 1.0f);
	vertex[8].color  = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	// Second triangle
	vertex[9].pos    = Vec4FP24(+0.5f, +0.5f, -0.5f, 1.0f);
	vertex[9].color  = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	vertex[10].pos   = Vec4FP24(+0.5f, -0.5f, -0.5f, 1.0f);
	vertex[10].color = Vec4FP24(0.0f, 1.0f, 0.0f, 0.0f);
	vertex[11].pos   = Vec4FP24(-0.5f, -0.5f, -0.5f, 1.0f);
	vertex[11].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	// Third face (PX)
	// First triangle
	vertex[12].pos   = Vec4FP24(+0.5f, -0.5f, -0.5f, 1.0f);
	vertex[12].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	vertex[13].pos   = Vec4FP24(+0.5f, +0.5f, -0.5f, 1.0f);
	vertex[13].color = Vec4FP24(1.0f, 0.0f, 0.0f, 0.0f);
	vertex[14].pos   = Vec4FP24(+0.5f, +0.5f, +0.5f, 1.0f);
	vertex[14].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	// Second triangle
	vertex[15].pos   = Vec4FP24(+0.5f, +0.5f, +0.5f, 1.0f);
	vertex[15].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	vertex[16].pos   = Vec4FP24(+0.5f, -0.5f, +0.5f, 1.0f);
	vertex[16].color = Vec4FP24(0.0f, 1.0f, 0.0f, 0.0f);
	vertex[17].pos   = Vec4FP24(+0.5f, -0.5f, -0.5f, 1.0f);
	vertex[17].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	// Fourth face (MX)
	// First triangle
	vertex[18].pos   = Vec4FP24(-0.5f, -0.5f, -0.5f, 1.0f);
	vertex[18].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	vertex[19].pos   = Vec4FP24(-0.5f, -0.5f, +0.5f, 1.0f);
	vertex[19].color = Vec4FP24(1.0f, 0.0f, 0.0f, 0.0f);
	vertex[20].pos   = Vec4FP24(-0.5f, +0.5f, +0.5f, 1.0f);
	vertex[20].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	// Second triangle
	vertex[21].pos   = Vec4FP24(-0.5f, +0.5f, +0.5f, 1.0f);
	vertex[21].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	vertex[22].pos   = Vec4FP24(-0.5f, +0.5f, -0.5f, 1.0f);
	vertex[22].color = Vec4FP24(0.0f, 1.0f, 0.0f, 0.0f);
	vertex[23].pos   = Vec4FP24(-0.5f, -0.5f, -0.5f, 1.0f);
	vertex[23].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	// Fifth face (PY)
	// First triangle
	vertex[24].pos   = Vec4FP24(-0.5f, +0.5f, -0.5f, 1.0f);
	vertex[24].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	vertex[25].pos   = Vec4FP24(-0.5f, +0.5f, +0.5f, 1.0f);
	vertex[25].color = Vec4FP24(1.0f, 0.0f, 0.0f, 0.0f);
	vertex[26].pos   = Vec4FP24(+0.5f, +0.5f, +0.5f, 1.0f);
	vertex[26].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	// Second triangle
	vertex[27].pos   = Vec4FP24(+0.5f, +0.5f, +0.5f, 1.0f);
	vertex[27].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	vertex[28].pos   = Vec4FP24(+0.5f, +0.5f, -0.5f, 1.0f);
	vertex[28].color = Vec4FP24(0.0f, 1.0f, 0.0f, 0.0f);
	vertex[29].pos   = Vec4FP24(-0.5f, +0.5f, -0.5f, 1.0f);
	vertex[29].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	// Sixth face (MY)
	// First triangle
	vertex[30].pos   = Vec4FP24(-0.5f, -0.5f, -0.5f, 1.0f);
	vertex[30].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);
	vertex[31].pos   = Vec4FP24(+0.5f, -0.5f, -0.5f, 1.0f);
	vertex[31].color = Vec4FP24(1.0f, 0.0f, 0.0f, 0.0f);
	vertex[32].pos   = Vec4FP24(+0.5f, -0.5f, +0.5f, 1.0f);
	vertex[32].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	// Second triangle
	vertex[33].pos   = Vec4FP24(+0.5f, -0.5f, +0.5f, 1.0f);
	vertex[33].color = Vec4FP24(1.0f, 1.0f, 0.0f, 0.0f);
	vertex[34].pos   = Vec4FP24(-0.5f, -0.5f, +0.5f, 1.0f);
	vertex[34].color = Vec4FP24(0.0f, 1.0f, 0.0f, 0.0f);
	vertex[35].pos   = Vec4FP24(-0.5f, -0.5f, -0.5f, 1.0f);
	vertex[35].color = Vec4FP24(0.0f, 0.0f, 0.0f, 0.0f);

	Shader::Uniforms uniform;
	Vec4<float24>* projection = &uniform.f[0];
	Vec4<float24>* modelView = &uniform.f[4];
	// Projection Matrix
	Mtx_PerspTilt(projection, AngleFromDegrees(80.0f), 240.0f/400.0f, 
			0.01f, 1000.0f, false);
	//Mtx_Identity(projection);
	// Constants
	uniform.f[95] = Vec4FP24(0.0f, 1.0f, -1.0f, 0.1f);

	Shader::Setup setup;
	setup.program_code[0]  = 0x4e000000; // mov  r0.xyz_  v0.xyzw
	setup.program_code[1]  = 0x4e07f001; // mov  r0.___w c95.yyyy
	setup.program_code[2]  = 0x0a224802; // dp4  r1.x___  c4.xyzw  r0.xyzw
	setup.program_code[3]  = 0x0a225803; // dp4  r1._y__  c5.xyzw  r0.xyzw
	setup.program_code[4]  = 0x0a226804; // dp4  r1.__z_  c6.xyzw  r0.xyzw
	setup.program_code[5]  = 0x0a227805; // dp4  r1.___w  c7.xyzw  r0.xyzw
	setup.program_code[6]  = 0x08020882; // dp4  o0.x___  c0.xyzw  r1.xyzw
	setup.program_code[7]  = 0x08021883; // dp4  o0._y__  c1.xyzw  r1.xyzw
	setup.program_code[8]  = 0x08022884; // dp4  o0.__z_  c2.xyzw  r1.xyzw
	setup.program_code[9]  = 0x08023885; // dp4  o0.___w  c3.xyzw  r1.xyzw
	setup.program_code[10] = 0x4c201006; // mov  o1.xyzw  v1.xyzw
	setup.program_code[11] = 0x88000000; // end

	setup.swizzle_data[0]  = 0x0000036e; // xyz_, xyzw, xxxx, xxxx
	setup.swizzle_data[1]  = 0x00000aa1; // ___w, yyyy, xxxx, xxxx
	setup.swizzle_data[2]  = 0x0006c368; // x___, xyzw, xyzw, xxxx
	setup.swizzle_data[3]  = 0x0006c364; // _y__, xyzw, xyzw, xxxx
	setup.swizzle_data[4]  = 0x0006c362; // __z_, xyzw, xyzw, xxxx
	setup.swizzle_data[5]  = 0x0006c361; // ___w, xyzw, xyzw, xxxx
	setup.swizzle_data[6]  = 0x0000036f; // xyzw, xyzw, xxxx, xxxx

	setup.entry_point = 0x0000;

	Shader::ShaderEngine shader_engine(setup, uniform);

	Rasterizer rasterizer;
	float angleX = 0.0, angleY = 0.0;

	while (frontend.PollEvent()) {
		frontend.Clear();

		Mtx_Identity(modelView);
		Mtx_Translate(modelView, 0.0, 0.0, -2.0 + 0.5*sinf(angleX));
		Mtx_RotateX(modelView, angleX);
		Mtx_RotateY(modelView, angleY);

		// Rotate the cube each frame
		angleX += M_PI / 180;
		angleY += M_PI / 360;

		for (int i = 0; i < VERTEX_COUNT; i++) {
			shader_engine.LoadInput(*((Shader::AttributeBuffer *)&vertex[i]));
			shader_engine.Run();
			shader_engine.WriteOutput(*((Shader::AttributeBuffer *)&outputs[i]));
		}

		for (int i = 0; i < VERTEX_COUNT; i+=3) {
			rasterizer.AddTriangle(outputs[i], outputs[i + 1], outputs[i + 2]);
		}

		frontend.Flip();
		frontend.Wait();	
	}

	//Frontend::Deinit();
}