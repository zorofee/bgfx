/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/bounds.h>
#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"
#include "camera.h"

namespace
{

constexpr bgfx::ViewId kRenderPassGeometry     = 1;
constexpr bgfx::ViewId kRenderPassLight        = 2;
constexpr bgfx::ViewId kRenderPassCombine      = 3;
constexpr bgfx::ViewId kRenderPassRaytracing   = 4;

static float s_texelHalf = 0.0f;

struct PosNormalTangentTexcoordVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_normal;
	uint32_t m_tangent;
	int16_t m_u;
	int16_t m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::Tangent,   4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Int16, true, true)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};

struct PosNormalTangentTexcoordVertexRaw
{
	float m_x;
	float m_y;
	float m_z;
	float m_nx;
	float m_ny;
	float m_nz;
	float m_tx;
	float m_ty;
	float m_tz;
	int16_t m_u;
	int16_t m_v;
};

bgfx::VertexLayout PosNormalTangentTexcoordVertex::ms_layout;

struct PosTexCoord0Vertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosTexCoord0Vertex::ms_layout;

struct DebugVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout DebugVertex::ms_layout;

static PosNormalTangentTexcoordVertex s_cubeVertices[24] =
{
	{-1.0f,  1.0f,  1.0f, encodeNormalRgba8( 0.0f,  0.0f,  1.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, encodeNormalRgba8( 0.0f,  0.0f,  1.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f,  1.0f, encodeNormalRgba8( 0.0f,  0.0f,  1.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, encodeNormalRgba8( 0.0f,  0.0f,  1.0f), 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f, -1.0f, encodeNormalRgba8( 0.0f,  0.0f, -1.0f), 0,      0,      0 },
	{ 1.0f,  1.0f, -1.0f, encodeNormalRgba8( 0.0f,  0.0f, -1.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, encodeNormalRgba8( 0.0f,  0.0f, -1.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, encodeNormalRgba8( 0.0f,  0.0f, -1.0f), 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f,  1.0f, encodeNormalRgba8( 0.0f,  1.0f,  0.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, encodeNormalRgba8( 0.0f,  1.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f,  1.0f, -1.0f, encodeNormalRgba8( 0.0f,  1.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, encodeNormalRgba8( 0.0f,  1.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, encodeNormalRgba8( 0.0f, -1.0f,  0.0f), 0,      0,      0 },
	{ 1.0f, -1.0f,  1.0f, encodeNormalRgba8( 0.0f, -1.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, encodeNormalRgba8( 0.0f, -1.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, encodeNormalRgba8( 0.0f, -1.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, encodeNormalRgba8( 1.0f,  0.0f,  0.0f), 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, encodeNormalRgba8( 1.0f,  0.0f,  0.0f), 0, 0x7fff,      0 },
	{ 1.0f, -1.0f, -1.0f, encodeNormalRgba8( 1.0f,  0.0f,  0.0f), 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, encodeNormalRgba8( 1.0f,  0.0f,  0.0f), 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0,      0,      0 },
	{-1.0f,  1.0f,  1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0,      0, 0x7fff },
	{-1.0f,  1.0f, -1.0f, encodeNormalRgba8(-1.0f,  0.0f,  0.0f), 0, 0x7fff, 0x7fff },
};

static PosNormalTangentTexcoordVertexRaw s_cubeVertices2[24] =
{
	{-1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 0, 0, 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 0, 0, 0, 0x7fff,      0 },
	{-1.0f, -1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 0, 0, 0,      0, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, 0.0f,  0.0f,  1.0f, 0, 0, 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0, 0, 0,      0,      0 },
	{ 1.0f,  1.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0, 0, 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0, 0, 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0, 0, 0, 0x7fff, 0x7fff },
	{-1.0f,  1.0f,  1.0f, 0.0f,  1.0f,  0.0f, 0, 0, 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, 0.0f,  1.0f,  0.0f, 0, 0, 0, 0x7fff,      0 },
	{-1.0f,  1.0f, -1.0f, 0.0f,  1.0f,  0.0f, 0, 0, 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, 0.0f,  1.0f,  0.0f, 0, 0, 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, 0.0f, -1.0f,  0.0f, 0, 0, 0,      0,      0 },
	{ 1.0f, -1.0f,  1.0f, 0.0f, -1.0f,  0.0f, 0, 0, 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, 0.0f, -1.0f,  0.0f, 0, 0, 0,      0, 0x7fff },
	{ 1.0f, -1.0f, -1.0f, 0.0f, -1.0f,  0.0f, 0, 0, 0, 0x7fff, 0x7fff },
	{ 1.0f, -1.0f,  1.0f, 1.0f,  0.0f,  0.0f, 0, 0, 0,      0,      0 },
	{ 1.0f,  1.0f,  1.0f, 1.0f,  0.0f,  0.0f, 0, 0, 0, 0x7fff,      0 },
	{ 1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f, 0, 0, 0,      0, 0x7fff },
	{ 1.0f,  1.0f, -1.0f, 1.0f,  0.0f,  0.0f, 0, 0, 0, 0x7fff, 0x7fff },
	{-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0, 0, 0,      0,      0 },
	{-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0, 0, 0, 0x7fff,      0 },
	{-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0, 0, 0,      0, 0x7fff },
	{-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0, 0, 0, 0x7fff, 0x7fff },
};

static const uint16_t s_cubeIndices[36] =
{
	 0,  2,  1,
	 1,  2,  3,
	 4,  5,  6,
	 5,  7,  6,

	 8, 10,  9,
	 9, 10, 11,
	12, 13, 14,
	13, 15, 14,

	16, 18, 17,
	17, 18, 19,
	20, 21, 22,
	21, 23, 22,
};

void screenSpaceQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _width = 1.0f, float _height = 1.0f)
{
	if (3 == bgfx::getAvailTransientVertexBuffer(3, PosTexCoord0Vertex::ms_layout) )
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, PosTexCoord0Vertex::ms_layout);
		PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;

		const float minx = -_width;
		const float maxx =  _width;
		const float miny = 0.0f;
		const float maxy = _height*2.0f;

		const float texelHalfW = _texelHalf/_textureWidth;
		const float texelHalfH = _texelHalf/_textureHeight;
		const float minu = -1.0f + texelHalfW;
		const float maxu =  1.0f + texelHalfH;

		const float zz = 0.0f;

		float minv = texelHalfH;
		float maxv = 2.0f + texelHalfH;

		if (_originBottomLeft)
		{
			float temp = minv;
			minv = maxv;
			maxv = temp;

			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = minx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_u = minu;
		vertex[0].m_v = minv;

		vertex[1].m_x = maxx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_u = maxu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		bgfx::setVertexBuffer(0, &vb);
	}
}

class ExampleDeferred : public entry::AppI
{
public:
	ExampleDeferred(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
	}

	void setup(Args args)
	{
		m_width = 1280;
		m_height = 720;
		m_debug = BGFX_DEBUG_TEXT;
		m_reset = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type = bgfx::RendererType::Vulkan;//args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width = m_width;
		init.resolution.height = m_height;
		init.resolution.reset = m_reset;
		bgfx::init(init);

		// Enable m_debug text.
		bgfx::setDebug(m_debug);

		// Set geometry pass view clear state.
		bgfx::setViewClear(kRenderPassGeometry
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 1.0f
			, 0
			, 1
			, 0
		);

		// Set light pass view clear state.
		bgfx::setViewClear(kRenderPassLight
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 1.0f
			, 0
			, 0
		);

		bgfx::setViewClear(kRenderPassRaytracing
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 1.0f
			, 0
			, 0
		);


	}
	void loadModel()
	{

	}

	void createVertexBuffers()
	{
		// Create static vertex buffer.
		m_vbh = bgfx::createVertexBuffer(
			bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices))
			, PosNormalTangentTexcoordVertex::ms_layout
		);

		// Create static index buffer.
		m_ibh = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices)));

	}

	void createUniforms()
	{
		// Create texture sampler uniforms.
		s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
		s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);

		s_albedo = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
		s_normal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
		s_depth = bgfx::createUniform("s_depth", bgfx::UniformType::Sampler);
		s_light = bgfx::createUniform("s_light", bgfx::UniformType::Sampler);

		u_mtx = bgfx::createUniform("u_mtx", bgfx::UniformType::Mat4);
		u_lightPosRadius = bgfx::createUniform("u_lightPosRadius", bgfx::UniformType::Vec4);
		u_lightRgbInnerR = bgfx::createUniform("u_lightRgbInnerR", bgfx::UniformType::Vec4);
		u_layer = bgfx::createUniform("u_layer", bgfx::UniformType::Vec4);
	}

	void loadPrograms()
	{
		// Create program from shaders.
		m_geomProgram = loadProgram("vs_deferred_geom", "fs_deferred_geom");
		m_lightProgram = loadProgram("vs_deferred_light", "fs_deferred_light");
		m_combineProgram = loadProgram("vs_deferred_combine", "fs_deferred_combine");
		//m_raytracingProgram = loadProgram("","");

	}

	void loadTextures()
	{
		// Load diffuse texture.
		m_textureColor = loadTexture("textures/fieldstone-rgba.dds");

		// Load normal texture.
		m_textureNormal = loadTexture("textures/fieldstone-n.dds");
	}

	void createGraphicsPipeline() {}

	//VKRay
	void initRayTracing()
	{
		const char* DamagedHelmet = "gltfScenes/DamagedHelmet/DamagedHelmet.gltf";
		const char* shirt = "gltfScenes/shirt/shirts_test_v5.gltf";
		bgfx::setupRaytracing();
		bgfx::initRayTracingScene(DamagedHelmet);
		// 调用scene 的 processRawData
		//bgfx::initRayTracingScene(s_cubeVertices2, (void*)s_cubeIndices);
	}

	void updateGUI()
	{
		ImGui::SetNextWindowPos(
			ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f)
			, ImGuiCond_FirstUseEver
		);
		ImGui::SetNextWindowSize(
			ImVec2(m_width / 5.0f, m_height / 3.0f)
			, ImGuiCond_FirstUseEver
		);
		ImGui::Begin("Settings", NULL, 0);
		ImGui::SliderInt("Num lights", &m_numLights, 1, 2048);
		ImGui::End();
	}



	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		setup(args);

		if (_width + _height < 0)
			return;

		// Create vertex stream declaration.
		PosNormalTangentTexcoordVertex::init();
		PosTexCoord0Vertex::init();
		DebugVertex::init();

		calcTangents(s_cubeVertices
				, BX_COUNTOF(s_cubeVertices)
				, PosNormalTangentTexcoordVertex::ms_layout
				, s_cubeIndices
				, BX_COUNTOF(s_cubeIndices)
				);

		createVertexBuffers();

		createUniforms();

		loadPrograms();

		loadTextures();


		m_lightBufferTex.idx = bgfx::kInvalidHandle;

		m_gbufferTex[0].idx = bgfx::kInvalidHandle;
		m_gbufferTex[1].idx = bgfx::kInvalidHandle;
		m_gbufferTex[2].idx = bgfx::kInvalidHandle;
		m_gbuffer.idx       = bgfx::kInvalidHandle;
		m_lightBuffer.idx   = bgfx::kInvalidHandle;

		// Imgui.
		imguiCreate();

		m_timeOffset = bx::getHPCounter();
		const bgfx::RendererType::Enum renderer = bgfx::getRendererType();
		s_texelHalf = 0.0f;

		// Get renderer capabilities info.
		m_caps = bgfx::getCaps();

		m_oldWidth  = 0;
		m_oldHeight = 0;
		m_oldReset  = m_reset;

		m_scrollArea = 0;
		m_numLights = 512;

		cameraCreate();

		cameraSetPosition({ 5.0f, 0.0f, -35.0f });
		cameraSetVerticalAngle(0.0f);

		initRayTracing();
	}

	virtual int shutdown() override
	{
		// Cleanup.
		cameraDestroy();
		imguiDestroy();

		if (bgfx::isValid(m_gbuffer) )
		{
			bgfx::destroy(m_gbuffer);
		}

		if (bgfx::isValid(m_lightBuffer) )
		{
			bgfx::destroy(m_lightBuffer);
		}

		if (bgfx::isValid(m_lightBufferTex) )
		{
			bgfx::destroy(m_lightBufferTex);
		}

		bgfx::destroy(m_ibh);
		bgfx::destroy(m_vbh);

		bgfx::destroy(m_geomProgram);
		bgfx::destroy(m_lightProgram);


		bgfx::destroy(m_combineProgram);

		bgfx::destroy(m_textureColor);
		bgfx::destroy(m_textureNormal);
		bgfx::destroy(s_texColor);
		bgfx::destroy(s_texNormal);

		bgfx::destroy(s_albedo);
		bgfx::destroy(s_normal);
		bgfx::destroy(s_depth);
		bgfx::destroy(s_light);

		bgfx::destroy(u_layer);
		bgfx::destroy(u_lightPosRadius);
		bgfx::destroy(u_lightRgbInnerR);
		bgfx::destroy(u_mtx);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			imguiBeginFrame(m_mouseState.m_mx
				, m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				, m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);


			//updateGUI();


			if (m_oldWidth != m_width)
			{
				// Recreate variable size render targets when resolution changes.
				m_oldWidth = m_width;
				m_oldHeight = m_height;
				m_oldReset = m_reset;

		
				const uint64_t tsFlags = 0
					| BGFX_SAMPLER_MIN_POINT
					| BGFX_SAMPLER_MAG_POINT
					| BGFX_SAMPLER_MIP_POINT
					| BGFX_SAMPLER_U_CLAMP
					| BGFX_SAMPLER_V_CLAMP
					;

				bgfx::Attachment gbufferAt[3];

				m_gbufferTex[0] = bgfx::createTexture2D(uint16_t(m_width), uint16_t(m_height), false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | tsFlags);
				m_gbufferTex[1] = bgfx::createTexture2D(uint16_t(m_width), uint16_t(m_height), false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | tsFlags);
				gbufferAt[0].init(m_gbufferTex[0]);
				gbufferAt[1].init(m_gbufferTex[1]);


				bgfx::TextureFormat::Enum depthFormat =
					bgfx::isTextureValid(0, false, 1, bgfx::TextureFormat::D32F, BGFX_TEXTURE_RT | tsFlags)
					? bgfx::TextureFormat::D32F
					: bgfx::TextureFormat::D24
					;

				m_gbufferTex[2] = bgfx::createTexture2D(uint16_t(m_width), uint16_t(m_height), false, 1, depthFormat, BGFX_TEXTURE_RT | tsFlags);
				gbufferAt[2].init(m_gbufferTex[2]);

				m_gbuffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferAt), gbufferAt, true);

				m_lightBufferTex = bgfx::createTexture2D(uint16_t(m_width), uint16_t(m_height), false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | tsFlags);
				m_lightBuffer = bgfx::createFrameBuffer(1, &m_lightBufferTex, true);
			}

			// Update camera.
			//cameraUpdate(deltaTime, m_mouseState, ImGui::MouseOverArea());

			float view[16];
			cameraGetViewMtx(view);

			// Setup views
			float vp[16];
			float invMvp[16];
			{
				bgfx::setViewRect(kRenderPassGeometry, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewRect(kRenderPassLight, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewRect(kRenderPassCombine, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewRect(kRenderPassRaytracing, 0, 0, uint16_t(m_width), uint16_t(m_height));


				bgfx::setViewFrameBuffer(kRenderPassLight, m_lightBuffer);
				bgfx::setViewFrameBuffer(kRenderPassGeometry, m_gbuffer);
				bgfx::setViewFrameBuffer(kRenderPassRaytracing, m_gbuffer);


				float proj[16];
				bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, m_caps->homogeneousDepth);
				bgfx::setViewTransform(kRenderPassGeometry, view, proj);
				bgfx::setViewTransform(kRenderPassRaytracing, view, proj);

				bx::mtxMul(vp, view, proj);
				bx::mtxInverse(invMvp, vp);

				const bgfx::Caps* caps = bgfx::getCaps();

				bx::mtxOrtho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, caps->homogeneousDepth);
				bgfx::setViewTransform(kRenderPassLight, NULL, proj);
				bgfx::setViewTransform(kRenderPassCombine, NULL, proj);

				const float aspectRatio = float(m_height) / float(m_width);
				const float size = 10.0f;
				bx::mtxOrtho(proj, -size, size, size*aspectRatio, -size * aspectRatio, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
				bx::mtxOrtho(proj, 0.0f, (float)m_width, 0.0f, (float)m_height, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
			}

			const uint32_t dim = 11;
			const float offset = (float(dim - 1) * 3.0f) * 0.5f;

			// Draw into geometry pass.
			for (uint32_t yy = 0; yy < dim; ++yy)
			{
				for (uint32_t xx = 0; xx < dim; ++xx)
				{
					float mtx[16];
					bx::mtxIdentity(mtx);

					mtx[12] = -offset + float(xx)*3.0f;
					mtx[13] = -offset + float(yy)*3.0f;
					mtx[14] = 0.0f;

					// Set transform for draw call.
					bgfx::setTransform(mtx);

					// Set vertex and index buffer.
					bgfx::setVertexBuffer(0, m_vbh);
					bgfx::setIndexBuffer(m_ibh);

					// Bind textures.
					bgfx::setTexture(0, s_texColor, m_textureColor);
					bgfx::setTexture(1, s_texNormal, m_textureNormal);

					// Set render states.
					bgfx::setState(0
						| BGFX_STATE_WRITE_RGB
						| BGFX_STATE_WRITE_A
						| BGFX_STATE_WRITE_Z
						| BGFX_STATE_DEPTH_TEST_LESS
						| BGFX_STATE_MSAA
					);

					// Submit primitive for rendering to view 0.
					//bgfx::submit(kRenderPassGeometry, m_geomProgram);
					bgfx::submit(kRenderPassRaytracing, m_geomProgram);
				}
			}


			// Draw lights into light buffer.
			for (int32_t light = 0; light < m_numLights; ++light)
			{
				bx::Sphere lightPosRadius;

				float lightTime = 0.1;
				lightPosRadius.center.x = bx::sin(((lightTime + light * 0.47f) + bx::kPiHalf*1.37f))*offset;
				lightPosRadius.center.y = bx::cos(((lightTime + light * 0.69f) + bx::kPiHalf*1.49f))*offset;
				lightPosRadius.center.z = bx::sin(((lightTime + light * 0.37f) + bx::kPiHalf*1.57f))*2.0f;
				lightPosRadius.radius = 2.0f;

				bx::Aabb aabb;
				toAabb(aabb, lightPosRadius);

				const bx::Vec3 box[8] =
				{
					{ aabb.min.x, aabb.min.y, aabb.min.z },
					{ aabb.min.x, aabb.min.y, aabb.max.z },
					{ aabb.min.x, aabb.max.y, aabb.min.z },
					{ aabb.min.x, aabb.max.y, aabb.max.z },
					{ aabb.max.x, aabb.min.y, aabb.min.z },
					{ aabb.max.x, aabb.min.y, aabb.max.z },
					{ aabb.max.x, aabb.max.y, aabb.min.z },
					{ aabb.max.x, aabb.max.y, aabb.max.z },
				};

				bx::Vec3 xyz = bx::mulH(box[0], vp);
				bx::Vec3 min = xyz;
				bx::Vec3 max = xyz;

				for (uint32_t ii = 1; ii < 8; ++ii)
				{
					xyz = bx::mulH(box[ii], vp);
					min = bx::min(min, xyz);
					max = bx::max(max, xyz);
				}

				// Cull light if it's fully behind camera.
				if (max.z >= 0.0f)
				{
					const float x0 = bx::clamp((min.x * 0.5f + 0.5f) * m_width, 0.0f, (float)m_width);
					const float y0 = bx::clamp((min.y * 0.5f + 0.5f) * m_height, 0.0f, (float)m_height);
					const float x1 = bx::clamp((max.x * 0.5f + 0.5f) * m_width, 0.0f, (float)m_width);
					const float y1 = bx::clamp((max.y * 0.5f + 0.5f) * m_height, 0.0f, (float)m_height);


					uint8_t val = light & 7;
					float lightRgbInnerR[4] =
					{
						val & 0x1 ? 1.0f : 0.25f,
						val & 0x2 ? 1.0f : 0.25f,
						val & 0x4 ? 1.0f : 0.25f,
						0.8f,
					};

					// Draw light.
					bgfx::setUniform(u_lightPosRadius, &lightPosRadius);
					bgfx::setUniform(u_lightRgbInnerR, lightRgbInnerR);
					bgfx::setUniform(u_mtx, invMvp);
					const uint16_t scissorHeight = uint16_t(y1 - y0);
					bgfx::setScissor(uint16_t(x0), uint16_t(m_height - scissorHeight - y0), uint16_t(x1 - x0), uint16_t(scissorHeight));
					bgfx::setTexture(0, s_normal, bgfx::getTexture(m_gbuffer, 1));
					bgfx::setTexture(1, s_depth, bgfx::getTexture(m_gbuffer, 2));
					bgfx::setState(0
						| BGFX_STATE_WRITE_RGB
						| BGFX_STATE_WRITE_A
						| BGFX_STATE_BLEND_ADD
					);
					screenSpaceQuad((float)m_width, (float)m_height, s_texelHalf, m_caps->originBottomLeft);

					bgfx::submit(kRenderPassLight, m_lightProgram);

				}
			}

			// Combine color and light buffers.
			bgfx::setTexture(0, s_albedo, bgfx::getTexture(m_gbuffer, 0));
			bgfx::setTexture(1, s_light, m_lightBufferTex);
			bgfx::setState(0
				| BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_A
			);
			screenSpaceQuad((float)m_width, (float)m_height, s_texelHalf, m_caps->originBottomLeft);


			bgfx::submit(kRenderPassCombine, m_combineProgram);



			imguiEndFrame();

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;

	bgfx::UniformHandle s_albedo;
	bgfx::UniformHandle s_normal;
	bgfx::UniformHandle s_depth;
	bgfx::UniformHandle s_light;

	bgfx::UniformHandle u_mtx;
	bgfx::UniformHandle u_lightPosRadius;
	bgfx::UniformHandle u_lightRgbInnerR;
	bgfx::UniformHandle u_layer;

	bgfx::ProgramHandle m_geomProgram;
	bgfx::ProgramHandle m_lightProgram;
	bgfx::ProgramHandle m_combineProgram;
	bgfx::ProgramHandle m_raytracingProgram;


	bgfx::TextureHandle m_textureColor;
	bgfx::TextureHandle m_textureNormal;

	bgfx::TextureHandle m_gbufferTex[3];
	bgfx::TextureHandle m_lightBufferTex;
	bgfx::FrameBufferHandle m_gbuffer;
	bgfx::FrameBufferHandle m_lightBuffer;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	uint32_t m_oldWidth;
	uint32_t m_oldHeight;
	uint32_t m_oldReset;

	bool m_useTArray = false;

	bool m_useUav =false;

	int32_t m_scrollArea;
	int32_t m_numLights;

	entry::MouseState m_mouseState;

	const bgfx::Caps* m_caps;
	int64_t m_timeOffset;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	  ExampleDeferred
	, "21-deferred"
	, "MRT rendering and deferred shading."
	, "https://bkaradzic.github.io/bgfx/examples.html#deferred"
	);
