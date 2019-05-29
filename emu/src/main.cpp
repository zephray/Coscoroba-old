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

int main(int argc, char *argv[]) {
	printf("Coscoroba Emulator\nVersion %s\n", VERSION);
	
	//Frontend::Init();
	auto &frontend = singleton<Frontend>();

	/*Shader::OutputVertex v0;
    Shader::OutputVertex v1;
    Shader::OutputVertex v2;
	Shader::OutputVertex v3;

	// Left up corner
	v0.color = MakeVec(
		float24::FromFloat32(1.0f),
        float24::FromFloat32(0.0f),
        float24::FromFloat32(0.0f),
		float24::FromFloat32(1.0f));
	v0.pos = MakeVec(
		float24::FromFloat32(-0.45f),
        float24::FromFloat32(-0.75f),
        float24::FromFloat32(0.2f),
		float24::FromFloat32(1.0f));
	// Right up corner
	v1.color = MakeVec(
		float24::FromFloat32(1.0f),
        float24::FromFloat32(1.0f),
        float24::FromFloat32(0.0f),
		float24::FromFloat32(1.0f));
	v1.pos = MakeVec(
		float24::FromFloat32(0.45f),
        float24::FromFloat32(-.75f),
        float24::FromFloat32(0.2f),
		float24::FromFloat32(1.0f));
	// Right lower corner
	v2.color = MakeVec(
		float24::FromFloat32(0.0f),
        float24::FromFloat32(1.0f),
        float24::FromFloat32(1.0f),
		float24::FromFloat32(1.0f));
	v2.pos = MakeVec(
		float24::FromFloat32(0.45f),
        float24::FromFloat32(0.75f),
        float24::FromFloat32(0.2f),
		float24::FromFloat32(1.0f));
	// Left lower corner
	v3.color = MakeVec(
		float24::FromFloat32(0.0f),
        float24::FromFloat32(0.0f),
        float24::FromFloat32(1.0f),
		float24::FromFloat32(1.0f));
	v3.pos = MakeVec(
		float24::FromFloat32(-.45f),
        float24::FromFloat32(0.75f),
        float24::FromFloat32(0.2f),
		float24::FromFloat32(1.0f));*/

	Shader::Uniforms uniform;
	uniform.f[0] = {
		float24::FromFloat32(0.0), 
		float24::FromFloat32(5.0/600.0),
		float24::FromFloat32(0.0),
		float24::FromFloat32(-1.0)};
	uniform.f[1] = {
		float24::FromFloat32(-0.005), 
		float24::FromFloat32(0.0),
		float24::FromFloat32(0.0),
		float24::FromFloat32(1.0)};
	uniform.f[2] = {
		float24::FromFloat32(0.0), 
		float24::FromFloat32(0.0),
		float24::FromFloat32(1.0),
		float24::FromFloat32(-1.0)};
	uniform.f[3] = {
		float24::FromFloat32(0.0), 
		float24::FromFloat32(0.0),
		float24::FromFloat32(0.0),
		float24::FromFloat32(-1.0)};
	uniform.f[95] = {
		float24::FromFloat32(0.0), 
		float24::FromFloat32(1.0),
		float24::FromFloat32(-1.0),
		float24::FromFloat32(0.1)};

	Shader::Setup setup;
	setup.program_code[0]  = 0x4e000000; // mov  r0.xyz_  v0.xyzw
	setup.program_code[1]  = 0x4e07f001; // mov  r0.___w c95.yyyy
	setup.program_code[2]  = 0x08020802; // dp4  o0.x___  c0.xyzw  r0.xyzw
	setup.program_code[3]  = 0x08021803; // dp4  o0._y__  c1.xyzw  r0.xyzw
	setup.program_code[4]  = 0x08022804; // dp4  o0.__z_  c2.xyzw  r0.xyzw
	setup.program_code[5]  = 0x08023805; // dp4  o0.___w  c3.xyzw  r0.xyzw
	setup.program_code[6]  = 0x4c201006; // mov  o1.xyzw  v1.xyzw
	setup.program_code[7]  = 0x88000000; // end

	setup.swizzle_data[0]  = 0x0000036e;
	setup.swizzle_data[1]  = 0x00000aa1;
	setup.swizzle_data[2]  = 0x0006c368;
	setup.swizzle_data[3]  = 0x0006c364;
	setup.swizzle_data[4]  = 0x0006c362;
	setup.swizzle_data[5]  = 0x0006c361;
	setup.swizzle_data[6]  = 0x0000036f;

	setup.entry_point = 0x0000;

	Shader::AttributeBuffer input_attr;
	input_attr.attr[0] = {  // position
		float24::FromFloat32(100.0), 
		float24::FromFloat32(220.0),
		float24::FromFloat32(0.5),
		float24::FromFloat32(1.0)};
	input_attr.attr[1] = {  // color
		float24::FromFloat32(1.0), 
		float24::FromFloat32(0.0),
		float24::FromFloat32(1.0),
		float24::FromFloat32(1.0)};
	Shader::AttributeBuffer output_attr;

	Shader::ShaderEngine shader_engine(setup, uniform);
	shader_engine.LoadInput(input_attr);
	shader_engine.Run();
	shader_engine.WriteOutput(output_attr);
	
	printf("o0: %f, %f, %f, %f\n", 
		output_attr.attr[0].x.ToFloat32(),
		output_attr.attr[0].y.ToFloat32(),
		output_attr.attr[0].z.ToFloat32(),
		output_attr.attr[0].w.ToFloat32());
	printf("o1: %f, %f, %f, %f\n", 
		output_attr.attr[1].x.ToFloat32(),
		output_attr.attr[1].y.ToFloat32(),
		output_attr.attr[1].z.ToFloat32(),
		output_attr.attr[1].w.ToFloat32());


	/*Rasterizer rasterizer;

	rasterizer.AddTriangle(v0, v1, v2);
	rasterizer.AddTriangle(v2, v3, v0);*/

	frontend.Flip();

	while (frontend.PollEvent()) {
		// ?
	}

	//Frontend::Deinit();
}