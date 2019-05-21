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

static_assert(sizeof(OutputVertex) == 24 * 4, "OutputVertex has invalid size");
