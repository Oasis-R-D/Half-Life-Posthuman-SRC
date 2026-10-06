#include "PlatformHeaders.h"
#include "Platform.h"
#include "hud.h"
#include "cl_util.h"

#include "renderer/rendererdefs.h"
#include "renderer/bsprenderer.h"
#include "glWrapper.h"
#include "GL_ShadowMap.h"

GLFramebuffer* GL_ShadowMap::m_pMainShadowFBO = nullptr;
std::vector<GLRenderbuffer*> GL_ShadowMap::m_pShadowRBOs;


std::vector<GL_ShadowMap*> GL_ShadowMap::m_vShadowMapList;

GL_ShadowMap* GL_ShadowMap::AllocateShadowMap(bool cubemap, eGL_texformat format, int width, int height, eGL_pixelformat pixelformat, eGL_type type, bool canuseblur)
{
	GLenum target = GL_TEXTURE_2D;
	if (cubemap)	target = GL_TEXTURE_CUBE_MAP;
	for (auto& shadowmap : m_vShadowMapList)
	{
		if (!shadowmap->m_bInUse &&
			shadowmap->m_pMainTexture->GetTargetType() == target &&
			shadowmap->m_pMainTexture->GetWidth() == width &&
			shadowmap->m_pMainTexture->GetHeight() == height)
		{
			shadowmap->m_bInUse = true;
			return shadowmap;
		}
	}

	gltexcommon_createinfo_t commoninfo = {
		nullptr,
		0,
		0,
		format,
		pixelformat,
		type,
		eGL_texfilter_linear,
		eGL_texfilter_linear
	};

	if (cubemap)
	{
		gltexcubemap_createinfo_t cubeinfo = {width, height, commoninfo};
		return new GL_ShadowMap(cubeinfo, canuseblur);
	}
	else
	{
		gltex2d_createinfo_t tex2dinfo = {width, height, commoninfo};
		return new GL_ShadowMap(tex2dinfo, canuseblur);
	}
}

void GL_ShadowMap::DeAllocateShadowMap(GL_ShadowMap* pSM)
{
	//	MEMORY LEAK WARNING! need to automatically delete inactive shadowmaps from gpu after a period of time,
	//	can't depend on shadowmaps getting cleared on level load.
	pSM->m_bInUse = false;
}

void GL_ShadowMap::ClearAllShadowMaps()
{
	m_vShadowMapList.clear();
}

void GL_ShadowMap::CheckFBO(GLsizei width, GLsizei height)
{
	if (m_pShadowRBOs.empty())
	{
		GLRenderbuffer* shadowrbo = new GLRenderbuffer(eGL_texformat_depth24, width, height);
		m_pShadowRBOs.push_back(shadowrbo);
	}
	else
	{
		bool bNeedsNewRenderBuffer = true;
		for (auto shadowrbo : m_pShadowRBOs)
		{
			uint32_t m_iwidth, m_iheight;
			shadowrbo->GetWidthHeight(m_iwidth, m_iheight);
			if (width == m_iwidth && height == m_iheight)
			{
				bNeedsNewRenderBuffer = false;
				break;
			}
		}
		if (bNeedsNewRenderBuffer)
		{
			GLRenderbuffer* shadowrbo = new GLRenderbuffer(eGL_texformat_depth16, width, height);
			m_pShadowRBOs.push_back(shadowrbo);
		}
	}
}

GL_ShadowMap::GL_ShadowMap(const gltex2d_createinfo_t& info, bool canuseblur)
{
	CheckFBO(info.width, info.height);

	for (auto shadowrbo : m_pShadowRBOs)
	{
		uint32_t width, height;
		shadowrbo->GetWidthHeight(width, height);
		if (width == info.width && height == info.height)
		{
			m_pShadowRBO = shadowrbo;
			break;
		}
	}

	m_bInUse = true;
	m_bCanBlur = canuseblur;

	m_vShadowMapList.push_back(this);
	m_pDummyTexture = new GLTexture2D(&info);
	m_pMainTexture = new GLTexture2D(&info);
}

GL_ShadowMap::GL_ShadowMap(const gltexcubemap_createinfo_t& info, bool canuseblur)
{
	CheckFBO(info.width, info.height);

	for (auto shadowrbo : m_pShadowRBOs)
	{
		uint32_t width, height;
		shadowrbo->GetWidthHeight(width, height);
		if (width == info.width && height == info.height)
		{
			m_pShadowRBO = shadowrbo;
			break;
		}
	}

	m_bInUse = true;
	m_bCanBlur = canuseblur;

	m_vShadowMapList.push_back(this);
	gltex2d_createinfo_t dummyinfo;
	dummyinfo.width = info.width;
	dummyinfo.height = info.height;
	dummyinfo.common = info.common;

	m_pDummyTexture = new GLTexture2D(&dummyinfo);
	m_pMainTexture = new GLTextureCubeMap(&info);

}

GL_ShadowMap::~GL_ShadowMap()
{
	// empty, automatically calls ~GL_TextureHandler()
}

void GL_ShadowMap::InitRendering(Vector cleancolor, GLsizei layer)
{
	m_iCurrentLayer = layer;

	GLContext::BindFramebuffer(m_pMainShadowFBO);
	m_pMainShadowFBO->AttachTexture(eGL_fboattachment_color0, m_pMainTexture, eGL_fbotarget_draw, m_iCurrentLayer);
	m_pMainShadowFBO->AttachRenderBuffer(eGL_fboattachment_depth, m_pShadowRBO, eGL_fbotarget_draw);

	GLContext::SetViewportSize(m_pMainTexture->GetWidth(), m_pMainTexture->GetHeight());

	// Completely clear everything
	GLContext::SetColorBufferClearValue(cleancolor.x, cleancolor.y, 0, 0);
	GLContext::ClearColorBuffer();
	GLContext::ClearDepthBuffer();
}

void GL_ShadowMap::BlurShadows()
{
	GLContext::BindFramebuffer(m_pMainShadowFBO, eGL_fbotarget_draw);
	
	GLContext::BindShader(gBSPRenderer.m_FilterShader);

	gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("texture_"), 0);
	gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("horizontal"), 1);

	GLContext::BindVertexArray(gBSPRenderer.m_pScreenQuadVAO);

	GLContext::SetDepthTesting(false);
	GLContext::SetFaceCulling(false);

	for (auto& shadowmap : m_vShadowMapList)
	{
		if (!shadowmap->m_bInUse || !shadowmap->m_bCanBlur)
			continue;

		// only blur closest light sources, my poor attempt at optimization
		float distance = (gBSPRenderer.m_vRenderOrigin - shadowmap->position).Length();
		if (distance > 512)
			continue;

		m_pMainShadowFBO->AttachTexture(eGL_fboattachment_color0, shadowmap->m_pDummyTexture, eGL_fbotarget_draw);

		GLTexture* shadowtexture = shadowmap->m_pMainTexture;

		GLContext::SetViewportSize(shadowtexture->GetWidth(), shadowtexture->GetHeight());

		if (shadowtexture->GetTargetType() == GL_TEXTURE_2D)
		{
			GLContext::BindTexture(shadowtexture, 0);
			gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("gaussian_pass"), 1);

			// blur shadow into m_pDummyTexture
			GLContext::DrawPolys(eGL_drawmode_triangles, 6);

			m_pMainShadowFBO->AttachTexture(eGL_fboattachment_color0, shadowtexture, eGL_fbotarget_draw);

			GLContext::BindTexture(shadowmap->m_pDummyTexture, 0);
			gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("gaussian_pass"), 0);

			// now render blurred shadow into the actual shadowmap texture
			GLContext::DrawPolys(eGL_drawmode_triangles, 6);
		}
		else if (shadowtexture->GetTargetType() == GL_TEXTURE_CUBE_MAP)
		{
			GLContext::BindTexture(shadowtexture, 1);
			GLContext::BindTexture(shadowmap->m_pDummyTexture, 0);
			for (int i = 0; i < 6; i++)
			{
				m_pMainShadowFBO->AttachTexture(eGL_fboattachment_color0, shadowmap->m_pDummyTexture, eGL_fbotarget_draw);

				gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("gaussian_pass"), 1);
				gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("cubemap"), 1);
				gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("cubemap_layer"), i);

				// blur shadow into m_pDummyTexture
				GLContext::DrawPolys(eGL_drawmode_triangles, 6);

				m_pMainShadowFBO->AttachTexture(eGL_fboattachment_color0, shadowtexture, eGL_fbotarget_draw, i);

				gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("gaussian_pass"), 0);
				gBSPRenderer.m_FilterShader->SetUniformInt(gBSPRenderer.m_FilterShader->GetUniformLoc("cubemap"), 0);

				// now render blurred shadow into the actual shadowmap texture
				GLContext::DrawPolys(eGL_drawmode_triangles, 6);
			}
		}
	}

	GLContext::SetDepthTesting(true);
	GLContext::SetFaceCulling(true);

	GLContext::BindFramebuffer(GLContext::GetMainFramebuffer());
	GLContext::SetViewportSize(ScreenWidth, ScreenHeight);
}

void GL_ShadowMap::FinishRendering()
{
	m_iCubeMapIteration = 0;
}

void GL_ShadowMap::StartShadowMapping()
{
	if (!m_pMainShadowFBO)
		m_pMainShadowFBO = new GLFramebuffer();

	//m_pMainShadowFBO->AttachTexture(eGL_fboattachment_color0, nullptr, eGL_fbotarget_draw);
}

void GL_ShadowMap::EndShadowMapping()
{
	if (gBSPRenderer.m_pCvarBlurShadows->value > 0)
		BlurShadows();

	GLContext::BindFramebuffer(GLContext::GetMainFramebuffer());
	GLContext::SetViewportSize(ScreenWidth, ScreenHeight);
}