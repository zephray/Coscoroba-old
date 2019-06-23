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
#include "rasterizer.h"
// TODO: remove these.
#include "frontend.h"
#include "texturing.h"
#include "kitten.h"

struct ClippingEdge {
public:
    ClippingEdge(Vec4<float24> coeffs, Vec4<float24> bias = 
            Vec4<float24>(float24::FromFloat32(0), float24::FromFloat32(0),
            float24::FromFloat32(0), float24::FromFloat32(0)))
        : coeffs(coeffs), bias(bias) {}

    bool IsInside(const RasterizerVertex& vertex) const {
        return Dot(vertex.pos + bias, coeffs) >= float24::FromFloat32(0);
    }

    bool IsOutSide(const RasterizerVertex& vertex) const {
        return !IsInside(vertex);
    }

    RasterizerVertex GetIntersection(const RasterizerVertex& v0, 
            const RasterizerVertex& v1) const {
        float24 dp = Dot(v0.pos + bias, coeffs);
        float24 dp_prev = Dot(v1.pos + bias, coeffs);
        float24 factor = dp_prev / (dp_prev - dp);

        return RasterizerVertex::Lerp(factor, v0, v1);
    }

private:
    float24 pos;
    Vec4<float24> coeffs;
    Vec4<float24> bias;
};

static void InitScreenCoordinates(RasterizerVertex& vtx) {
    struct {
        float24 halfsize_x;
        float24 offset_x;
        float24 halfsize_y;
        float24 offset_y;
        float24 zscale;
        float24 offset_z;
    } viewport;

    // TODO: Read these from register
    viewport.halfsize_x = float24::FromFloat32(200.0);
    viewport.halfsize_y = float24::FromFloat32(120.0);
    viewport.offset_x = float24::FromFloat32(0.0f);
    viewport.offset_y = float24::FromFloat32(0.0f);

    float24 inv_w = float24::FromFloat32(1.f) / vtx.pos.w;
    vtx.pos.w = inv_w;
    vtx.color *= inv_w;
    /*vtx.quat *= inv_w;
    vtx.tc0 *= inv_w;
    vtx.tc1 *= inv_w;
    vtx.tc0_w *= inv_w;
    vtx.view *= inv_w;
    vtx.tc2 *= inv_w;*/

    vtx.screen_position[0] =
        (vtx.pos.x * inv_w + float24::FromFloat32(1.0)) * viewport.halfsize_x + viewport.offset_x;
    vtx.screen_position[1] =
        (vtx.pos.y * inv_w + float24::FromFloat32(1.0)) * viewport.halfsize_y + viewport.offset_y;
    vtx.screen_position[2] = vtx.pos.z * inv_w;
}

void Rasterizer::AddTriangle(
            const Shader::OutputVertex& v0,
            const Shader::OutputVertex& v1,
            const Shader::OutputVertex& v2) {
    
    // Clipping a planar n-gon against a plane will remove at least 1 vertex and introduces 2 at
    // the new edge (or less in degenerate cases). As such, we can say that each clipping plane
    // introduces at most 1 new vertex to the polygon. Since we start with a triangle and have a
    // fixed 6 clipping planes, the maximum number of vertices of the clipped polygon is 3 + 6 = 9.
    static const std::size_t MAX_VERTICES = 9;
    std::vector<Shader::OutputVertex> buffer_a = {v0, v1, v2};
    std::vector<Shader::OutputVertex> buffer_b;

    auto* output_list = &buffer_a;
    auto* input_list = &buffer_b;

    // NOTE: We clip against a w=epsilon plane to guarantee that the output has a positive w value.
    // TODO: Not sure if this is a valid approach. Also should probably instead use the smallest
    //       epsilon possible within float24 accuracy.
    static const float24 EPSILON = float24::FromFloat32(0.00001f);
    static const float24 f0 = float24::FromFloat32(0.0);
    static const float24 f1 = float24::FromFloat32(1.0);
    static const std::array<ClippingEdge, 7> clipping_edges = {{
        {MakeVec(-f1, f0, f0, f1)}, // x = +w
        {MakeVec(f1, f0, f0, f1)},  // x = -w
        {MakeVec(f0, -f1, f0, f1)}, // y = +w
        {MakeVec(f0, f1, f0, f1)},  // y = -w
        {MakeVec(f0, f0, -f1, f0)}, // z =  0
        {MakeVec(f0, f0, f1, f1)},  // z = -w
        {MakeVec(f0, f0, f0, f1),
         Vec4<float24>(f0, f0, f0, EPSILON)}, // w = EPSILON
    }};

    // Simple implementation of the Sutherland-Hodgman clipping algorithm.
    // TODO: Make this less inefficient (currently lots of useless buffering overhead happens here)
    auto Clip = [&](const ClippingEdge& edge) {
        std::swap(input_list, output_list);
        output_list->clear();

        const Shader::OutputVertex* reference_vertex = &input_list->back();

        for (const auto& vertex : *input_list) {
            // NOTE: This algorithm changes vertex order in some cases!
            if (edge.IsInside(vertex)) {
                if (edge.IsOutSide(*reference_vertex)) {
                    output_list->push_back(edge.GetIntersection(vertex, *reference_vertex));
                }

                output_list->push_back(vertex);
            } else if (edge.IsInside(*reference_vertex)) {
                output_list->push_back(edge.GetIntersection(vertex, *reference_vertex));
            }
            reference_vertex = &vertex;
        }
    };

    for (auto edge : clipping_edges) {
        Clip(edge);

        // Need to have at least a full triangle to continue...
        if (output_list->size() < 3)
            return;
    }

    for (std::size_t i = 0; i < output_list->size() - 2; i++) {
        RasterizerVertex vtx0((*output_list)[0]);
        RasterizerVertex vtx1((*output_list)[i + 1]);
        RasterizerVertex vtx2((*output_list)[i + 2]);

        InitScreenCoordinates(vtx0);
        InitScreenCoordinates(vtx1);
        InitScreenCoordinates(vtx2);

        /*printf(
                "Triangle at position (%.3f, %.3f, %.3f, %.3f), "
                "(%.3f, %.3f, %.3f, %.3f), (%.3f, %.3f, %.3f, %.3f) and "
                "screen position (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)\n",
                vtx0.pos.x.ToFloat32(), vtx0.pos.y.ToFloat32(), vtx0.pos.z.ToFloat32(), vtx0.pos.w.ToFloat32(), 
                vtx1.pos.x.ToFloat32(), vtx1.pos.y.ToFloat32(), vtx1.pos.z.ToFloat32(), vtx1.pos.w.ToFloat32(),
                vtx2.pos.x.ToFloat32(), vtx2.pos.y.ToFloat32(), vtx2.pos.z.ToFloat32(), vtx2.pos.w.ToFloat32(), 
                vtx0.screen_position.x.ToFloat32(), vtx0.screen_position.y.ToFloat32(), vtx0.screen_position.z.ToFloat32(), 
                vtx1.screen_position.x.ToFloat32(), vtx1.screen_position.y.ToFloat32(), vtx1.screen_position.z.ToFloat32(),
                vtx2.screen_position.x.ToFloat32(), vtx2.screen_position.y.ToFloat32(), vtx2.screen_position.z.ToFloat32());*/

        Rasterizer::ProcessTriangle(vtx0, vtx1, vtx2);
    }
}


Fix12P4 FloatToFix(float24 flt) {
    // TODO: Rounding here is necessary to prevent garbage pixels at
    //       triangle borders. Is it that the correct solution, though?
    return Fix12P4(static_cast<unsigned short>(round(flt.ToFloat32() * 16.0f)));
};

Vec3<Fix12P4> ScreenToRasterizerCoordinates(const Vec3<float24>& vec) {
    return Vec3<Fix12P4>{FloatToFix(vec.x), FloatToFix(vec.y), FloatToFix(vec.z)};
};

// Triangle filling rules: Pixels on the right-sided edge or on flat bottom edges are not
    // drawn. Pixels on any other triangle border are drawn. This is implemented with three bias
    // values which are added to the barycentric coordinates w0, w1 and w2, respectively.
    // NOTE: These are the PSP filling rules. Not sure if the 3DS uses the same ones...
bool IsRightSideOrFlatBottomEdge(const Vec2<Fix12P4>& vtx,
        const Vec2<Fix12P4>& line1,
        const Vec2<Fix12P4>& line2) {
    if (line1.y == line2.y) {
        // just check if vertex is above us => bottom line parallel to x-axis
        return vtx.y < line1.y;
    } else {
        // check if vertex is on our left => right side
        // TODO: Not sure how likely this is to overflow
        return (int)vtx.x < (int)line1.x + ((int)line2.x - (int)line1.x) *
                                            ((int)vtx.y - (int)line1.y) /
                                            ((int)line2.y - (int)line1.y);
    }
};

/**
 * Calculate signed area of the triangle spanned by the three argument vertices.
 * The sign denotes an orientation.
 *
 * @todo define orientation concretely.
 */
static int SignedArea(const Vec2<Fix12P4>& vtx1, const Vec2<Fix12P4>& vtx2,
                      const Vec2<Fix12P4>& vtx3) {
    const auto vec1 = MakeVec(vtx2 - vtx1, 0);
    const auto vec2 = MakeVec(vtx3 - vtx1, 0);
    // TODO: There is a very small chance this will overflow for sizeof(int) == 4
    return Cross(vec1, vec2).z;
};

void Rasterizer::ProcessTriangle(
            const RasterizerVertex& v0,
            const RasterizerVertex& v1,
            const RasterizerVertex& v2) {

    Vec3<Fix12P4> vtxpos[3]{
            ScreenToRasterizerCoordinates(v0.screen_position),
            ScreenToRasterizerCoordinates(v1.screen_position),
            ScreenToRasterizerCoordinates(v2.screen_position)};

    /*
        // Cull away triangles which are wound clockwise.
        if (SignedArea(vtxpos[0].xy(), vtxpos[1].xy(), vtxpos[2].xy()) <= 0)
            return;
    */

    uint16_t min_x = std::min({vtxpos[0].x, vtxpos[1].x, vtxpos[2].x});
    uint16_t min_y = std::min({vtxpos[0].y, vtxpos[1].y, vtxpos[2].y});
    uint16_t max_x = std::max({vtxpos[0].x, vtxpos[1].x, vtxpos[2].x});
    uint16_t max_y = std::max({vtxpos[0].y, vtxpos[1].y, vtxpos[2].y});

    /*
    // Convert the scissor box coordinates to 12.4 fixed point
    u16 scissor_x1 = (u16)(regs.rasterizer.scissor_test.x1 << 4);
    u16 scissor_y1 = (u16)(regs.rasterizer.scissor_test.y1 << 4);
    // x2,y2 have +1 added to cover the entire sub-pixel area
    u16 scissor_x2 = (u16)((regs.rasterizer.scissor_test.x2 + 1) << 4);
    u16 scissor_y2 = (u16)((regs.rasterizer.scissor_test.y2 + 1) << 4);

    if (regs.rasterizer.scissor_test.mode == RasterizerRegs::ScissorMode::Include) {
        // Calculate the new bounds
        min_x = std::max(min_x, scissor_x1);
        min_y = std::max(min_y, scissor_y1);
        max_x = std::min(max_x, scissor_x2);
        max_y = std::min(max_y, scissor_y2);
    }*/

    min_x &= Fix12P4::IntMask(); // Round down
    min_y &= Fix12P4::IntMask();
    max_x = ((max_x + Fix12P4::FracMask()) & Fix12P4::IntMask()); // Round up
    max_y = ((max_y + Fix12P4::FracMask()) & Fix12P4::IntMask());

    int bias0 =
        IsRightSideOrFlatBottomEdge(vtxpos[0].xy(), vtxpos[1].xy(), vtxpos[2].xy()) ? -1 : 0;
    int bias1 =
        IsRightSideOrFlatBottomEdge(vtxpos[1].xy(), vtxpos[2].xy(), vtxpos[0].xy()) ? -1 : 0;
    int bias2 =
        IsRightSideOrFlatBottomEdge(vtxpos[2].xy(), vtxpos[0].xy(), vtxpos[1].xy()) ? -1 : 0;

    auto w_inverse = MakeVec(v0.pos.w, v1.pos.w, v2.pos.w);

    /*printf("(%d, %d) ", vtxpos[0].x, vtxpos[0].y);
    printf("(%d, %d) ", vtxpos[1].x, vtxpos[1].y);
    printf("(%d, %d) \n", vtxpos[2].x, vtxpos[2].y);
    printf("Min X %d, Min Y %d, Max X %d, Max Y %d\n", min_x, min_y, max_x, max_y);*/

    // Enter rasterization loop, starting at the center of the topleft bounding box corner.
    // TODO: Not sure if looping through x first might be faster
    for (uint16_t y = min_y + 8; y < max_y; y += 0x10) {
        for (uint16_t x = min_x + 8; x < max_x; x += 0x10) {

            /*
            // Do not process the pixel if it's inside the scissor box and the scissor mode is set
            // to Exclude
            if (regs.rasterizer.scissor_test.mode == RasterizerRegs::ScissorMode::Exclude) {
                if (x >= scissor_x1 && x < scissor_x2 && y >= scissor_y1 && y < scissor_y2)
                    continue;
            }*/

            // Calculate the barycentric coordinates w0, w1 and w2
            int w0 = bias0 + SignedArea(vtxpos[1].xy(), vtxpos[2].xy(), {x, y});
            int w1 = bias1 + SignedArea(vtxpos[2].xy(), vtxpos[0].xy(), {x, y});
            int w2 = bias2 + SignedArea(vtxpos[0].xy(), vtxpos[1].xy(), {x, y});
            int wsum = w0 + w1 + w2;

            //printf("(%d, %d, %d, %d, %d) ", x, y, w0, w1, w2);

            // If current pixel is not covered by the current primitive
            if (w0 < 0 || w1 < 0 || w2 < 0)
                continue;

            auto baricentric_coordinates =
                MakeVec(float24::FromFloat32(static_cast<float>(w0)),
                        float24::FromFloat32(static_cast<float>(w1)),
                        float24::FromFloat32(static_cast<float>(w2)));
            float24 interpolated_w_inverse =
                float24::FromFloat32(1.0f) / Dot(w_inverse, baricentric_coordinates);

            // interpolated_z = z / w
            float interpolated_z_over_w =
                (v0.screen_position[2].ToFloat32() * w0 + v1.screen_position[2].ToFloat32() * w1 +
                 v2.screen_position[2].ToFloat32() * w2) /
                wsum;

            // Not fully accurate. About 3 bits in precision are missing.
            // Z-Buffer (z / w * scale + offset)
            /*float depth_scale = float24::FromRaw(regs.rasterizer.viewport_depth_range).ToFloat32();
            float depth_offset =
                float24::FromRaw(regs.rasterizer.viewport_depth_near_plane).ToFloat32();*/
            float depth_scale = 1.0;
            float depth_offset = 0.1;
            float depth = interpolated_z_over_w * depth_scale + depth_offset;

            // Potentially switch to W-Buffer
            /*if (regs.rasterizer.depthmap_enable ==
                Pica::RasterizerRegs::DepthBuffering::WBuffering) {
                // W-Buffer (z * scale + w * offset = (z / w * scale + offset) * w)
                depth *= interpolated_w_inverse.ToFloat32() * wsum;
            }*/

            // Clamp the result
            depth = std::clamp(depth, 0.0f, 1.0f);

            frontend.DrawPixel((x >> 4) + VIDEO_WIDTH / 2, y >> 4, 
                depth * 255.0, depth * 255.0, depth * 255.0);

            // Perspective correct attribute interpolation:
            // Attribute values cannot be calculated by simple linear interpolation since
            // they are not linear in screen space. For example, when interpolating a
            // texture coordinate across two vertices, something simple like
            //     u = (u0*w0 + u1*w1)/(w0+w1)
            // will not work. However, the attribute value divided by the
            // clipspace w-coordinate (u/w) and and the inverse w-coordinate (1/w) are linear
            // in screenspace. Hence, we can linearly interpolate these two independently and
            // calculate the interpolated attribute by dividing the results.
            // I.e.
            //     u_over_w   = ((u0/v0.pos.w)*w0 + (u1/v1.pos.w)*w1)/(w0+w1)
            //     one_over_w = (( 1/v0.pos.w)*w0 + ( 1/v1.pos.w)*w1)/(w0+w1)
            //     u = u_over_w / one_over_w
            //
            // The generalization to three vertices is straightforward in baricentric coordinates.
            auto GetInterpolatedAttribute = [&](
                    float24 attr0, float24 attr1, float24 attr2) {
                auto attr_over_w = MakeVec(attr0, attr1, attr2);
                float24 interpolated_attr_over_w =
                        Dot(attr_over_w, baricentric_coordinates);
                return interpolated_attr_over_w * interpolated_w_inverse;
            };

            Vec4<uint8_t> primary_color{
                static_cast<uint8_t>(round(
                    GetInterpolatedAttribute(v0.color.r(), v1.color.r(), v2.color.r()).ToFloat32() *
                    64)),
                static_cast<uint8_t>(round(
                    GetInterpolatedAttribute(v0.color.g(), v1.color.g(), v2.color.g()).ToFloat32() *
                    64)),
                static_cast<uint8_t>(round(
                    GetInterpolatedAttribute(v0.color.b(), v1.color.b(), v2.color.b()).ToFloat32() *
                    64)),
                static_cast<uint8_t>(round(
                    GetInterpolatedAttribute(v0.color.a(), v1.color.a(), v2.color.a()).ToFloat32() *
                    64)),
            };

            // These need eventually be moved into Fragment Shader
            Texturing::TextureInfo textureInfo;
	        textureInfo.width = 64;
	        textureInfo.height = 64;
	        textureInfo.stride = 8 * 8 * 4 * 8;
	        textureInfo.format = Texturing::RGBA8;
            Vec4<uint8_t> color = Texturing::LookupTexture(kitten_raw + 4,
					primary_color.x, primary_color.y, textureInfo);

            frontend.DrawPixel(x >> 4, y >> 4, color.r(), color.g(), color.b());
        }
    }
}