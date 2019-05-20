#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <math.h>
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"
#include "vshader_shbin.h"

#undef DEMO_TEAPOT
#define DEMO_SPONZA

#define CLEAR_COLOR 0x332211FF

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static float angleX = 0.0, angleY = 0.0;

typedef struct {
    float position[3];
    float color[3];
} vertex;

static const vertex fallback[] =
{
    { {100.0f, 220.0f, 0.5f}, {1.0f, 0.0f, 0.0f} },
    { {100.0f, 20.0f, 0.5f }, {0.0f, 0.0f, 1.0f} },
    { {300.0f, 20.0f, 0.5f }, {0.0f, 1.0f, 1.0f} },
    { {100.0f, 220.0f, 0.5f}, {1.0f, 0.0f, 0.0f} },
    { {300.0f, 20.0f, 0.5f }, {0.0f, 1.0f, 1.0f} },
    { {300.0f, 220.0f, 0.5f}, {1.0f, 1.0f, 0.0f} },
};

#define fallback_list_count (sizeof(fallback)/sizeof(fallback[0]))

static int vertex_list_count;

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static C3D_Mtx projection;

static void* vbo_data;

char* readFile(size_t* len, const char* fname) {
    char* buf = NULL;
    size_t fileSize;
    FILE* f = fopen(fname, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        fileSize = ftell(f);
        *len = fileSize;
        fseek(f, 0, SEEK_SET);
        buf = malloc(fileSize);
        if (buf) {
            fread(buf, fileSize, 1, f);
        }
        fclose(f);
    }
    return buf;
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
    float v10[3];
    float v20[3];
    float len2;

    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];

    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];

    N[0] = v20[1] * v10[2] - v20[2] * v10[1];
    N[1] = v20[2] * v10[0] - v20[0] * v10[2];
    N[2] = v20[0] * v10[1] - v20[1] * v10[0];

    len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
        float len = (float)sqrt((double)len2);

        N[0] /= len;
        N[1] /= len;
    }
}

float* loadObject(size_t* vertexListCount) {
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = NULL;
    size_t numShapes;
    tinyobj_material_t* materials = NULL;
    size_t numMaterials;
    float bmin[3];
    float bmax[3];

    size_t dataLen = 0;
    #ifdef DEMO_SPONZA
    char* data = readFile(&dataLen, "romfs:/sponza.obj");
    #elif defined (DEMO_TEAPOT)
    char* data = readFile(&dataLen, "romfs:/teapot.obj");
    #endif

    if (!data) {
        printf("Unable to open object file!\n");
        return NULL;
    }

    printf("filesize: %d\n", (int)dataLen);
    
    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret = tinyobj_parse_obj(&attrib, &shapes, &numShapes, &materials,
                                &numMaterials, data, dataLen, flags);
    if (ret != TINYOBJ_SUCCESS) {
        printf("Unable to parse object file!\n");
        return NULL;
    }

    printf("# of shapes    = %d\n", (int)numShapes);
    printf("# of materials = %d\n", (int)numMaterials);

    size_t num_triangles = attrib.num_face_num_verts;
    size_t stride = 6; /* 6 = pos(3float), color(3float) */

    size_t face_offset = 0;
    size_t i;

    float* vb = (float*)malloc(sizeof(float) * stride * num_triangles * 3);

    for (i = 0; i < attrib.num_face_num_verts; i++) {
        size_t f;
        assert(attrib.face_num_verts[i] % 3 == 0); /* assume all triangle faces. */
        for (f = 0; f < (size_t)attrib.face_num_verts[i] / 3; f++) {
            size_t k;
            float v[3][3];
            float n[3][3];
            float c[3];
            float len2;

            tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * f + 0];
            tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * f + 1];
            tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * f + 2];

            for (k = 0; k < 3; k++) {
                int f0 = idx0.v_idx;
                int f1 = idx1.v_idx;
                int f2 = idx2.v_idx;
                assert(f0 >= 0);
                assert(f1 >= 0);
                assert(f2 >= 0);

                v[0][k] = attrib.vertices[3 * (size_t)f0 + k];
                v[1][k] = attrib.vertices[3 * (size_t)f1 + k];
                v[2][k] = attrib.vertices[3 * (size_t)f2 + k];
                bmin[k] = (v[0][k] < bmin[k]) ? v[0][k] : bmin[k];
                bmin[k] = (v[1][k] < bmin[k]) ? v[1][k] : bmin[k];
                bmin[k] = (v[2][k] < bmin[k]) ? v[2][k] : bmin[k];
                bmax[k] = (v[0][k] > bmax[k]) ? v[0][k] : bmax[k];
                bmax[k] = (v[1][k] > bmax[k]) ? v[1][k] : bmax[k];
                bmax[k] = (v[2][k] > bmax[k]) ? v[2][k] : bmax[k];
            }

            if (attrib.num_normals > 0) {
                int f0 = idx0.vn_idx;
                int f1 = idx1.vn_idx;
                int f2 = idx2.vn_idx;
                if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                    assert(f0 < (int)attrib.num_normals);
                    assert(f1 < (int)attrib.num_normals);
                    assert(f2 < (int)attrib.num_normals);
                    for (k = 0; k < 3; k++) {
                    n[0][k] = attrib.normals[3 * (size_t)f0 + k];
                    n[1][k] = attrib.normals[3 * (size_t)f1 + k];
                    n[2][k] = attrib.normals[3 * (size_t)f2 + k];
                    }
                } 
                else { /* normal index is not defined for this face */
                    /* compute geometric normal */
                    CalcNormal(n[0], v[0], v[1], v[2]);
                    n[1][0] = n[0][0];
                    n[1][1] = n[0][1];
                    n[1][2] = n[0][2];
                    n[2][0] = n[0][0];
                    n[2][1] = n[0][1];
                    n[2][2] = n[0][2];
                }
            } 
            else {
                /* compute geometric normal */
                CalcNormal(n[0], v[0], v[1], v[2]);
                n[1][0] = n[0][0];
                n[1][1] = n[0][1];
                n[1][2] = n[0][2];
                n[2][0] = n[0][0];
                n[2][1] = n[0][1];
                n[2][2] = n[0][2];
            }

            for (k = 0; k < 3; k++) {
                vb[(3 * i + k) * stride + 0] = v[k][0];
                vb[(3 * i + k) * stride + 1] = v[k][1];
                vb[(3 * i + k) * stride + 2] = v[k][2];

                /* Use normal as color. */
                c[0] = n[k][0];
                c[1] = n[k][1];
                c[2] = n[k][2];
                len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
                if (len2 > 0.0f) {
                    float len = (float)sqrt((double)len2);

                    c[0] /= len;
                    c[1] /= len;
                    c[2] /= len;
                }

                vb[(3 * i + k) * stride + 3] = (c[0] * 0.5f + 0.5f);
                vb[(3 * i + k) * stride + 4] = (c[1] * 0.5f + 0.5f);
                vb[(3 * i + k) * stride + 5] = (c[2] * 0.5f + 0.5f);
            }
        }
        face_offset += (size_t)attrib.face_num_verts[i];
    }

    printf("bmin = %f, %f, %f\n", (double)bmin[0], (double)bmin[1],
            (double)bmin[2]);
    printf("bmax = %f, %f, %f\n", (double)bmax[0], (double)bmax[1],
            (double)bmax[2]);
    printf("Number of triangles: %d\n", num_triangles);

    *vertexListCount = num_triangles * 3;

    return vb;
}

static void sceneInit(void)
{
    // Load the vertex shader, create a shader program and bind it
    vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    // Get the location of the uniforms
    uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
    uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

    // Configure attributes for use with the vertex shader
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // v1=color
    //AttrInfo_AddFixed(attrInfo, 1); // v1=color

    // Set the fixed attribute (color) to solid white
    //C3D_FixedAttribSet(1, 1.0, 1.0, 1.0, 1.0);

    // Compute the projection matrix
    #ifdef DEMO_SPONZA
	Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(70.0f), C3D_AspectRatioTop, 0.01f, 100.0f, false);
    #elif defined (DEMO_TEAPOT)
    Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 100.0f, false);
    #endif
    
    //Mtx_OrthoTilt(&projection, -15, 15.0, -1.0, 8.0, 0.0, 10.0, true);

    // Create the VBO (vertex buffer object)
    float* vb = loadObject(&vertex_list_count);
    if (!vb) {
        vb = (float *)&fallback;
        vertex_list_count = fallback_list_count;
    }

    vbo_data = linearAlloc(vertex_list_count * sizeof(vertex));
    memcpy(vbo_data, vb, vertex_list_count * sizeof(vertex));

    // Configure buffers
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

    // Configure the first fragment shading substage to just pass through the vertex color
    // See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

static void sceneRender(void)
{
    // Calculate the modelView matrix
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
    #ifdef DEMO_SPONZA
    //Mtx_Scale(&modelView, 1, 1, 1);
	Mtx_Translate(&modelView, 0.0, -2.0, -6.5 + 0.5*sinf(angleX), true);
    #elif defined (DEMO_TEAPOT)
    Mtx_Scale(&modelView, 0.01, 0.01, 0.01);
	Mtx_Translate(&modelView, 0.0, -40.0, -100.0 + 0.5*sinf(angleX), true);
    #endif
    
	Mtx_RotateX(&modelView, angleX, true);
	Mtx_RotateY(&modelView, angleY, true);

	// Rotate the cube each frame
	//angleX += M_PI / 180;
	angleY += M_PI / 360;

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView,  &modelView);

    // Draw the VBO
    C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_list_count);
}

static void sceneExit(void)
{
    // Free the VBO
    linearFree(vbo_data);

    // Free the shader program
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
}

int main()
{
    // Initialize graphics
    gfxInitDefault();

    consoleInit(GFX_BOTTOM, NULL);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Initialize the render target
    C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    Result rc = romfsInit();
	if (rc)
		printf("romfsInit: %08lX\n", rc);

    // Initialize the scene
    sceneInit();

    //C3D_CullFace(GPU_CULL_NONE);
    //C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_ALL);

    // Main loop
    while (aptMainLoop())
    {
    //	hidScanInput();

        // Respond to user input
    //	u32 kDown = hidKeysDown();
    //	if (kDown & KEY_START)
    //		break; // break in order to return to hbmenu

        // Render the scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
            C3D_FrameDrawOn(target);
            sceneRender();
        C3D_FrameEnd(0);
    }

    while(1);

    // Deinitialize the scene
    sceneExit();

    // Deinitialize graphics
    C3D_Fini();
    gfxExit();
    return 0;
}
