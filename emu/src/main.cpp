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

	Shader::OutputVertex v0;
    Shader::OutputVertex v1;
    Shader::OutputVertex v2;
	Shader::OutputVertex v3;
	Rasterizer rasterizer;

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
		float24::FromFloat32(1.0f));
		
	rasterizer.AddTriangle(v0, v1, v2);
	rasterizer.AddTriangle(v2, v3, v0);

	frontend.Flip();

	while (frontend.PollEvent()) {
		// ?
	}

	//Frontend::Deinit();
}