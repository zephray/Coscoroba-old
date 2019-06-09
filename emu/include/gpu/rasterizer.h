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
// Rasterizer Interface
#include "main.h"
#include "frontend.h"
#include "shader.h"
#include "fixed.h"

// The rasterizer accepts output vertex from either VS or GS, rasterize the 
// triangle, and output fragment to the FS.

struct RasterizerVertex : Shader::OutputVertex {
    RasterizerVertex(const OutputVertex& v) : OutputVertex(v) {}

    Vec3<float24> screen_position;

    // Linear interpolation
    // factor: 0=this, 1=vtx
    // Note: This function cannot be called after perspective divide
    void Lerp(float24 factor, const RasterizerVertex& vtx) {
        pos = pos * factor + vtx.pos * (float24::FromFloat32(1) - factor);
        color = color * factor + vtx.color * (float24::FromFloat32(1) - factor);
        /*quat = quat * factor + vtx.quat * (float24::FromFloat32(1) - factor);
        tc0 = tc0 * factor + vtx.tc0 * (float24::FromFloat32(1) - factor);
        tc1 = tc1 * factor + vtx.tc1 * (float24::FromFloat32(1) - factor);
        tc0_w = tc0_w * factor + vtx.tc0_w * (float24::FromFloat32(1) - factor);
        view = view * factor + vtx.view * (float24::FromFloat32(1) - factor);
        tc2 = tc2 * factor + vtx.tc2 * (float24::FromFloat32(1) - factor);*/
    }

    // Linear interpolation
    // factor: 0=v0, 1=v1
    // Note: This function cannot be called after perspective divide
    static RasterizerVertex Lerp(float24 factor, const RasterizerVertex& v0, 
            const RasterizerVertex& v1) {
        RasterizerVertex ret = v0;
        ret.Lerp(factor, v1);
        return ret;
    }
};

class RasterizerInterface {
public:
    virtual ~RasterizerInterface() {};

    // Add a primitive to the queue
    virtual void AddTriangle(
            const Shader::OutputVertex& v0,
            const Shader::OutputVertex& v1,
            const Shader::OutputVertex& v2) = 0;

    // Draw the current queue
    virtual void DrawTriangles() = 0;
};

// Software based rasterizer
class Rasterizer : public RasterizerInterface {
public:
    Rasterizer() : frontend(singleton<Frontend>()) {}
    void AddTriangle(
            const Shader::OutputVertex& v0,
            const Shader::OutputVertex& v1,
            const Shader::OutputVertex& v2) override;
    // Well, actually there is no queue. For a hardware rasterizer one would
    // need one, though.
    // But this emulator would probably never get a hardware rasterizer...
    void DrawTriangles() override {}
    
private:
    Frontend &frontend;
    void ProcessTriangle(
            const RasterizerVertex& v0,
            const RasterizerVertex& v1,
            const RasterizerVertex& v2);
};