#pragma once

#include "PlatformHeaders.h"
#include "Platform.h"
#include "hud.h"
#include "cl_util.h"

#include "renderer/rendererdefs.h"

#include "glWrapper.h"

////////////////////////////////////
//	shadow map class. can be of 2 types: 2d texture and cubemap. a cubemap shadowmap can render all 6 faces
//	by calling InitRendering 6 times.
//
//	it must be correctly reset with FinishRendering after youre done rendering to it.

class GL_ShadowMap
{
public:
	GL_ShadowMap(const gltex2d_createinfo_t& texinfo, bool canuseblur);
	GL_ShadowMap(const gltexcubemap_createinfo_t& texinfo, bool canuseblur);
	~GL_ShadowMap();

	void InitRendering(Vector cleancolor = Vector(0, 0, 0), GLsizei layer = 0);
	void FinishRendering();

	__forceinline void SetPosition(Vector pos) { position = pos; }

	static void BlurShadows();

	static void StartShadowMapping();
	static void EndShadowMapping();

	static GL_ShadowMap* AllocateShadowMap(bool cubemap, eGL_texformat format, int width, int height, eGL_pixelformat pixelformat, eGL_type type, bool canuseblur = true);
	static void DeAllocateShadowMap(GL_ShadowMap* pSM);
	static void ClearAllShadowMaps();

	GLTexture* GetTexture() { return m_pMainTexture; }

private:
	static void CheckFBO(GLsizei width, GLsizei height);
	bool m_bInUse = true;

	GLuint m_iCubeMapIteration = 0; // to render the 6 faces, gets reset in FinishRendering
	GLuint m_iCurrentLayer = 0;		// for _2DTexture_Array shadowmap

	Vector position; // kinda not nice but whatever, cant blur all shadows in the entire damn world

	GLTexture* m_pMainTexture = nullptr;

	GLTexture* m_pDummyTexture = nullptr;
	GLRenderbuffer* m_pShadowRBO = nullptr;
	bool m_bCanBlur = false;

	static GLFramebuffer* m_pMainShadowFBO;
	static std::vector<GLRenderbuffer*> m_pShadowRBOs;

	static std::vector<GL_ShadowMap*> m_vShadowMapList;
};