#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include "vshader_shbin.h"

typedef struct
{
	u32 offset;
	u32 flags[2];
} GPU_BufCfg;

typedef struct
{
	u32 base_paddr;
	int bufCount;
	GPU_BufCfg buffers[12];
} GPU_BufInfo;

#define BUFFER_BASE_PADDR 0x18000000
#define GPU_CMDBUF_SIZE 0x40000

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct {
	float position[3];
	float color[3];
} vertex;

static const vertex vertex_list[] =
{
	{ {100.0f, 220.0f, 0.5f}, {1.0f, 0.0f, 0.0f} },
	{ {100.0f, 20.0f, 0.5f }, {0.0f, 0.0f, 1.0f} },
	{ {300.0f, 20.0f, 0.5f }, {0.0f, 1.0f, 1.0f} },
	{ {100.0f, 220.0f, 0.5f}, {1.0f, 0.0f, 0.0f} },
	{ {300.0f, 20.0f, 0.5f }, {0.0f, 1.0f, 1.0f} },
	{ {300.0f, 220.0f, 0.5f}, {1.0f, 1.0f, 0.0f} },
};

#define vertex_list_count (sizeof(vertex_list)/sizeof(vertex_list[0]))

#define STAGE_HAS_TRANSFER(n)   BIT(0+(n))
#define STAGE_HAS_ANY_TRANSFER  (7<<0)
#define STAGE_NEED_TRANSFER(n)  BIT(3+(n))
#define STAGE_NEED_TOP_TRANSFER (STAGE_NEED_TRANSFER(0)|STAGE_NEED_TRANSFER(1))
#define STAGE_NEED_BOT_TRANSFER STAGE_NEED_TRANSFER(2)
#define STAGE_WAIT_TRANSFER     BIT(6)

gxCmdQueue_s gxQueue;
uint32_t cmdBufSize = GPU_CMDBUF_SIZE;
uint32_t *cmdBuf;
void* colorBuf;
void* depthBuf;
static u8 frameStage = 0;
volatile uint32_t frameCount = 0;
static void* vbo_data;
static GPU_BufInfo bufInfo;

static void onVBlank0(__attribute__((unused)) void* unused)
{
	if (frameStage & STAGE_NEED_TOP_TRANSFER)
	{
		frameStage &= ~STAGE_NEED_TOP_TRANSFER;
		frameStage |= STAGE_WAIT_TRANSFER;
		uint32_t* outputFrameBuf = (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		uint32_t dim = GX_BUFFER_DIM(240, 400);
		GX_DisplayTransfer(colorBuf, dim, outputFrameBuf, dim, DISPLAY_TRANSFER_FLAGS);
		gfxConfigScreen(GFX_TOP, false);
	}
	frameCount ++;
}

static void onQueueFinish(gxCmdQueue_s* queue)
{
	if (frameStage & STAGE_WAIT_TRANSFER)
		frameStage &= ~STAGE_WAIT_TRANSFER;
	else
	{
		u8 needs = frameStage & STAGE_HAS_ANY_TRANSFER;
		frameStage = (frameStage&~STAGE_HAS_ANY_TRANSFER) | (needs<<3);
	}
}

int main()
{
	// Initialize graphics
	svcOutputDebugString("Hello", 5);

	gfxInitDefault();

	// Initialize buffer info list
	memset(&bufInfo, 0, sizeof(bufInfo));
	bufInfo.base_paddr = BUFFER_BASE_PADDR;

	// Allocate VBO
	vbo_data = linearAlloc(sizeof(vertex_list));
	memcpy(vbo_data, vertex_list, sizeof(vertex_list));

	// Add VBO to the buffer list
	int id = bufInfo.bufCount++;
	u32 pa = osConvertVirtToPhys(vbo_data);
	//if (pa < info->base_paddr) return -2;
	GPU_BufCfg* bufCfg = &bufInfo.buffers[id];
	bufCfg->offset = pa - bufInfo.base_paddr;
	//                 permutation
	bufCfg->flags[0] = 0x10 & 0xFFFFFFFF;
	//                 permutation (Hi)  stride              attrib count
	bufCfg->flags[1] = (0x10 >> 32) | (sizeof(vertex) << 16) | (2 << 28);

	cmdBuf = (u32*)linearAlloc(cmdBufSize);
	GPUCMD_SetBuffer(cmdBuf, cmdBufSize, 0);

	gxQueue.maxEntries = 32;
	gxQueue.entries = (gxCmdEntry_s*)malloc(gxQueue.maxEntries*sizeof(gxCmdEntry_s));
	GX_BindQueue(&gxQueue);
	gxCmdQueueRun(&gxQueue);

	gspSetEventCallback(GSPGPU_EVENT_VBlank0, onVBlank0, NULL, false);
	gxCmdQueueSetCallback(&gxQueue, onQueueFinish, NULL);

	colorBuf = vramAlloc(240 * 400 * 4);
	depthBuf = vramAlloc(240 * 400 * 4);

	gxCmdQueueStop(&gxQueue);
	gxCmdQueueClear(&gxQueue);

	u32 cfs = 2;
	u32 dfs = 2;
	void* colorBufEnd = (u8*)colorBuf + 240*400*(2+cfs);
	void* depthBufEnd = (u8*)depthBuf + 240*400*(2+dfs);
	GX_MemoryFill(
		colorBuf, CLEAR_COLOR, (u32*)colorBufEnd, BIT(0) | (cfs << 8),
		depthBuf, 0, (u32*)depthBufEnd, BIT(0) | (dfs << 8));

	// Framebuffer
	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1);

	uint32_t dim = 0x01000000 | (((u32)(400-1) & 0xFFF) << 12) | (240 & 0xFFF);
	GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_LOC, osConvertVirtToPhys(depthBuf) >> 3);
	GPUCMD_AddWrite(GPUREG_COLORBUFFER_LOC, osConvertVirtToPhys(colorBuf) >> 3);
	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_DIM, dim);
	GPUCMD_AddWrite(GPUREG_RENDERBUF_DIM,   dim);
	GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_FORMAT,  3); // Fmt 3
	GPUCMD_AddWrite(GPUREG_COLORBUFFER_FORMAT,  2 | (0u << 16)); // Size 2 Fmt 0
	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_BLOCK32, 0);

	GPUCMD_AddWrite(GPUREG_COLORBUFFER_READ,  0xf);
	GPUCMD_AddWrite(GPUREG_COLORBUFFER_WRITE, 0xf);
	GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_READ,  0x3);
	GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_WRITE, 0x3);

	// Viewpoint
	// TODO: actually calculate this
	GPUCMD_AddWrite(GPUREG_VIEWPORT_WIDTH,    0x0045e000);
	GPUCMD_AddWrite(GPUREG_VIEWPORT_INVW,     0x38111110);
	GPUCMD_AddWrite(GPUREG_VIEWPORT_HEIGHT,   0x00469000);
	GPUCMD_AddWrite(GPUREG_VIEWPORT_INVH,     0x3747ae14);
	GPUCMD_AddWrite(GPUREG_VIEWPORT_XY,       0x00000000);

	// Scissor
	GPUCMD_AddWrite(GPUREG_SCISSORTEST_MODE,  0x00000000);
	GPUCMD_AddWrite(GPUREG_SCISSORTEST_POS,   0x00000000);
	GPUCMD_AddWrite(GPUREG_SCISSORTEST_DIM,   0x00000000);

	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG,  0x3, 0x00000000);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x3, 0x00000000);
	GPUCMD_AddMaskedWrite(GPUREG_VSH_COM_MODE,     0x1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_CONFIG, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x4e000000);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x4e07f001);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x08020802);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x08021803);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x08022804);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x08023805);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x4c201006);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_DATA, 0x88000000);
	GPUCMD_AddWrite(GPUREG_VSH_CODETRANSFER_END, 0x00000001);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_CONFIG, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x0000036e);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x00000aa1);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x0006c368);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x0006c364);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x0006c362);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x0006c361);
	GPUCMD_AddWrite(GPUREG_VSH_OPDESCS_DATA, 0x0000036f);
	GPUCMD_AddWrite(GPUREG_VSH_ENTRYPOINT, 0x7fff0000);
	GPUCMD_AddWrite(GPUREG_VSH_OUTMAP_MASK, 0x00000003);
	GPUCMD_AddWrite(GPUREG_VSH_OUTMAP_TOTAL1, 0x00000001);
	GPUCMD_AddWrite(GPUREG_VSH_OUTMAP_TOTAL2, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x1, 0x00000001);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_TOTAL, 0x00000002);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O0, 0x03020100);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O1, 0x0b0a0908);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O2, 0x1f1f1f1f);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O3, 0x1f1f1f1f);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O4, 0x1f1f1f1f);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O5, 0x1f1f1f1f);
	GPUCMD_AddWrite(GPUREG_SH_OUTMAP_O6, 0x1f1f1f1f);
	GPUCMD_AddWrite(GPUREG_SH_OUTATTR_MODE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_SH_OUTATTR_CLOCK, 0x00000003);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0xa, 0x00000000);
	GPUCMD_AddWrite(GPUREG_GSH_MISC0, 0x00000000);
	GPUCMD_AddWrite(GPUREG_GSH_MISC1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_GSH_INPUTBUFFER_CONFIG, 0xa0000000);

	// Attribute
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_FORMAT_LOW, 0x000000bb);
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_FORMAT_HIGH, 0x1ffc0000);
	// Attribute count = 2
	GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0xb, 0xa0000001);
	GPUCMD_AddWrite(GPUREG_VSH_NUM_ATTR, 0x00000001);

	GPUCMD_AddWrite(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, 0x00000010);
	GPUCMD_AddWrite(GPUREG_VSH_ATTRIBUTES_PERMUTATION_HIGH, 0x00000000);
	
	// Buffer List
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_LOC, bufInfo.base_paddr >> 3);
	GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFER0_OFFSET, 
			(u32*)bufInfo.buffers, sizeof(bufInfo.buffers)/sizeof(u32));

	GPUCMD_AddWrite(GPUREG_DEPTHMAP_ENABLE, 0x00000001);
	GPUCMD_AddWrite(GPUREG_FACECULLING_CONFIG, 0x00000002);
	GPUCMD_AddWrite(GPUREG_DEPTHMAP_SCALE, 0x00bf0000);
	GPUCMD_AddWrite(GPUREG_DEPTHMAP_OFFSET, 0x00000000);
	GPUCMD_AddWrite(GPUREG_FRAGOP_ALPHA_TEST, 0x00000010);
	GPUCMD_AddWrite(GPUREG_STENCIL_TEST, 0xff000010);
	GPUCMD_AddWrite(GPUREG_STENCIL_OP, 0x00000000);
	GPUCMD_AddWrite(GPUREG_DEPTH_COLOR_MASK, 0x00001f61);
	GPUCMD_AddMaskedWrite(GPUREG_GAS_DELTAZ_DEPTH, 0x8, 0x02000000);
	GPUCMD_AddWrite(GPUREG_BLEND_COLOR, 0x00000000);
	GPUCMD_AddWrite(GPUREG_BLEND_FUNC, 0x76760000);
	GPUCMD_AddWrite(GPUREG_LOGIC_OP, 0x00000000);
	GPUCMD_AddMaskedWrite(GPUREG_COLOR_OPERATION, 0x7, 0x00e40100);
	GPUCMD_AddWrite(GPUREG_FRAGOP_SHADOW, 0x80003c00);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, 0x00000000);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_FUNC, 0x1, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_DATA, 0x7, 0x00000000);
	GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG, 0xb, 0x00001000);
	GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG, 0x4, 0x00011000);
	GPUCMD_AddWrite(GPUREG_TEXUNIT0_SHADOW, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_TEXENV_UPDATE_BUFFER, 0x7, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV_BUFFER_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_FOG_COLOR, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV0_SOURCE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV0_OPERAND, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV0_COMBINER, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV0_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_TEXENV0_SCALE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV1_SOURCE, 0x000f000f);
	GPUCMD_AddWrite(GPUREG_TEXENV1_OPERAND, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV1_COMBINER, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV1_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_TEXENV1_SCALE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV2_SOURCE, 0x000f000f);
	GPUCMD_AddWrite(GPUREG_TEXENV2_OPERAND, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV2_COMBINER, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV2_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_TEXENV2_SCALE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV3_SOURCE, 0x000f000f);
	GPUCMD_AddWrite(GPUREG_TEXENV3_OPERAND, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV3_COMBINER, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV3_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_TEXENV3_SCALE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV4_SOURCE, 0x000f000f);
	GPUCMD_AddWrite(GPUREG_TEXENV4_OPERAND, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV4_COMBINER, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV4_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_TEXENV4_SCALE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV5_SOURCE, 0x000f000f);
	GPUCMD_AddWrite(GPUREG_TEXENV5_OPERAND, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV5_COMBINER, 0x00000000);
	GPUCMD_AddWrite(GPUREG_TEXENV5_COLOR, 0xffffffff);
	GPUCMD_AddWrite(GPUREG_TEXENV5_SCALE, 0x00000000);
	GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_INDEX, 0x00000001);
	GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_DATA0, 0x3f00003f);
	GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_DATA1, 0x00003f00);
	GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_DATA2, 0x003f0000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, 0x0000005f);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3b9999bf);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA + 1, 0x00003f00);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA + 2, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, 0x0000005e);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA + 1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA + 2, 0x003d3333);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, 0x80000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0xbf800000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3c088888);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3f800000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0xbba3d70a);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0xbf800000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3f800000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3f800000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VSH_BOOLUNIFORM, 0x7fff0000);
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x2, 0x00000001);
	GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 0x00000001);
	GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, 0x80000000);

	GPUCMD_AddWrite(GPUREG_NUMVERTICES, vertex_list_count);
	
	GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0x00000000);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x1, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 0x1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_DRAWARRAYS, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 0x1, 0x00000001);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x1, 0x00000000);
	GPUCMD_AddWrite(GPUREG_VTX_FUNC, 0x00000001);
	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 0x00000001);
	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 0x00000001);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 0x00000001);
	GPUCMD_AddWrite(GPUREG_FINALIZE, 0x12345678);

	uint32_t *actBuf;
	uint32_t actSize;
	GPUCMD_Split(&actBuf, &actSize);
	GX_ProcessCommandList(actBuf, actSize*4, 0x0);

	extern u32 __ctru_linear_heap;
	extern u32 __ctru_linear_heap_size;
	GSPGPU_FlushDataCache((void*)__ctru_linear_heap, __ctru_linear_heap_size);

	frameStage |= STAGE_NEED_TOP_TRANSFER;

	GPUCMD_SetBuffer(cmdBuf, cmdBufSize, 0);

	gxCmdQueueRun(&gxQueue);
	
	while(1) {
		uint32_t startFrame = frameCount;
		do {
			gspWaitForAnyEvent();
		} while (frameCount == startFrame);

		gxCmdQueueWait(&gxQueue, -1);
		
		while (frameStage) {
			gspWaitForAnyEvent();
		}
		
		gxCmdQueueStop(&gxQueue);
		gxCmdQueueClear(&gxQueue);

		GX_MemoryFill(
			colorBuf, CLEAR_COLOR, (u32*)colorBufEnd, BIT(0) | (cfs << 8),
			depthBuf, 0, (u32*)depthBufEnd, BIT(0) | (dfs << 8));

		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1);

		GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_LOC, osConvertVirtToPhys(depthBuf) >> 3);
		GPUCMD_AddWrite(GPUREG_COLORBUFFER_LOC, osConvertVirtToPhys(colorBuf) >> 3);
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_DIM, dim);
		GPUCMD_AddWrite(GPUREG_RENDERBUF_DIM,   dim);
		GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_FORMAT,  3); // Fmt 3
		GPUCMD_AddWrite(GPUREG_COLORBUFFER_FORMAT,  2 | (0u << 16)); // Size 2 Fmt 0
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_BLOCK32, 0);

		GPUCMD_AddWrite(GPUREG_COLORBUFFER_READ,  0xf);
		GPUCMD_AddWrite(GPUREG_COLORBUFFER_WRITE, 0xf);
		GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_READ,  0x3);
		GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_WRITE, 0x3);

		// Viewpoint
		GPUCMD_AddWrite(GPUREG_VIEWPORT_WIDTH,    0x0045e000);
		GPUCMD_AddWrite(GPUREG_VIEWPORT_INVW,     0x38111110);
		GPUCMD_AddWrite(GPUREG_VIEWPORT_HEIGHT,   0x00469000);
		GPUCMD_AddWrite(GPUREG_VIEWPORT_INVH,     0x3747ae14);
		GPUCMD_AddWrite(GPUREG_VIEWPORT_XY,       0x00000000);

		// Scissor
		GPUCMD_AddWrite(GPUREG_SCISSORTEST_MODE,  0x00000000);
		GPUCMD_AddWrite(GPUREG_SCISSORTEST_POS,   0x00000000);
		GPUCMD_AddWrite(GPUREG_SCISSORTEST_DIM,   0x00000000);

		// Uniform
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, 0x80000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0xbf800000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3c088888);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3f800000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0xbba3d70a);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0xbf800000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3f800000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x3f800000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_DATA, 0x00000000);

		GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x2, 0x00000001);
		GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 0x00000001);
		GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, 0x80000000);

		GPUCMD_AddWrite(GPUREG_NUMVERTICES, vertex_list_count);
		
		GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0x00000000);
		GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x1, 0x00000001);
		GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 0x1, 0x00000000);
		GPUCMD_AddWrite(GPUREG_DRAWARRAYS, 0x00000001);
		GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 0x1, 0x00000001);
		GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x1, 0x00000000);
		GPUCMD_AddWrite(GPUREG_VTX_FUNC, 0x00000001);
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 0x00000001);
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 0x00000001);
		GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 0x00000001);

		GPUCMD_Split(&actBuf, &actSize);
		GX_ProcessCommandList(actBuf, actSize*4, 0x0);

		GSPGPU_FlushDataCache((void*)__ctru_linear_heap, __ctru_linear_heap_size);

		frameStage |= STAGE_HAS_TRANSFER(0);

		GPUCMD_SetBuffer(cmdBuf, cmdBufSize, 0);

		gxCmdQueueRun(&gxQueue);
	}

	gfxExit();
	return 0;
}