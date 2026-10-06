#include "glWrapper.h"
#include "glcommon_internal.h"

static constexpr GLuint glfbotarget_map[] = {
    GL_DRAW_FRAMEBUFFER,
    GL_READ_FRAMEBUFFER,
    GL_FRAMEBUFFER
};
static constexpr GLuint glfboattach_map[] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3,
    GL_DEPTH_ATTACHMENT
};

//////////////////////////////
//////////////////////////////
static GLuint legacyfbocreate() {
    GLuint handle; glGenFramebuffers(1, &handle); return handle;
}
static void legacyfbotextureattach(GLFramebuffer* fbo, eGL_fbotarget target, eGL_fboattachment attach, GLTexture* tex, int depth) {
    GLFramebuffer* popdrawfbo = GLContext::GetBoundDrawFBO();
    GLFramebuffer* popreadfbo = GLContext::GetBoundReadFBO();

    GLContext::BindFramebuffer(fbo, target);
    switch (tex->GetTargetType())
    {
    case GL_TEXTURE_1D: glFramebufferTexture1D(glfbotarget_map[target], glfboattach_map[attach], GL_TEXTURE_1D, tex->GetHandle(), 0); break;
    case GL_TEXTURE_2D: glFramebufferTexture2D(glfbotarget_map[target], glfboattach_map[attach], GL_TEXTURE_2D, tex->GetHandle(), 0); break;
    case GL_TEXTURE_3D: glFramebufferTexture3D(glfbotarget_map[target], glfboattach_map[attach], GL_TEXTURE_3D, tex->GetHandle(), 0, 0); break;
	case GL_TEXTURE_CUBE_MAP: glFramebufferTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + depth, glfboattach_map[attach], GL_TEXTURE_2D, tex->GetHandle(), 0); break;
    }
    if(target==eGL_fbotarget_draw){
        GLContext::BindFramebuffer(popdrawfbo, eGL_fbotarget_draw);
    }
    else if (target == eGL_fbotarget_read){
        GLContext::BindFramebuffer(popreadfbo, eGL_fbotarget_read);
    }
    else{
        GLContext::BindFramebuffer(popdrawfbo, eGL_fbotarget_draw);
        GLContext::BindFramebuffer(popreadfbo, eGL_fbotarget_read);
    }
}
static void legacyfborenderbufferattach(GLFramebuffer* fbo, eGL_fbotarget target, eGL_fboattachment attach, GLRenderbuffer* rbo) {
    GLFramebuffer* popdrawfbo = GLContext::GetBoundDrawFBO();
    GLFramebuffer* popreadfbo = GLContext::GetBoundReadFBO();

    GLContext::BindFramebuffer(fbo, target);

    glFramebufferRenderbuffer(GL_RENDERBUFFER, glfboattach_map[attach], GL_RENDERBUFFER, rbo->GetHandle());
    
    if (target == eGL_fbotarget_draw) {
        GLContext::BindFramebuffer(popdrawfbo, eGL_fbotarget_draw);
    }
    else if (target == eGL_fbotarget_read) {
        GLContext::BindFramebuffer(popreadfbo, eGL_fbotarget_read);
    }
    else {
        GLContext::BindFramebuffer(popdrawfbo, eGL_fbotarget_draw);
        GLContext::BindFramebuffer(popreadfbo, eGL_fbotarget_read);
    }
}
static void legacyfboblit(GLFramebuffer* src, GLFramebuffer* dst, uint32_t w, uint32_t h, bool color, bool depth) {
    GLuint buffermask = 0;
    if (color)  buffermask |= GL_COLOR_BUFFER_BIT;
    if (depth)  buffermask |= GL_DEPTH_BUFFER_BIT;

    GLFramebuffer* popdrawfbo = GLContext::GetBoundDrawFBO();
    GLFramebuffer* popreadfbo = GLContext::GetBoundReadFBO();
    
    GLContext::BindFramebuffer(src, eGL_fbotarget_read);
    GLContext::BindFramebuffer(dst, eGL_fbotarget_draw);

    glBlitFramebuffer(0, 0, w, h,
                      0, 0, w, h,
                      buffermask,
                      GL_NEAREST);

    GLContext::BindFramebuffer(popreadfbo, eGL_fbotarget_read);
    GLContext::BindFramebuffer(popdrawfbo, eGL_fbotarget_draw);
}
//////////////////////////////
//////////////////////////////
static GLuint newfbocreate(){
    GLuint handle; glCreateFramebuffers(1, &handle); return handle;
}
static void newfbotextureattach(GLFramebuffer* fbo, eGL_fbotarget target, eGL_fboattachment attach, GLTexture* tex, int depth) {
	if (tex->GetTargetType() != GL_TEXTURE_CUBE_MAP)
        glNamedFramebufferTexture(fbo->GetHandle(), glfboattach_map[attach], tex->GetHandle(), 0);
	else
		glNamedFramebufferTextureLayer(fbo->GetHandle(), glfboattach_map[attach], tex->GetHandle(), 0, depth);
}
static void newfborenderbufferattach(GLFramebuffer* fbo, eGL_fbotarget target, eGL_fboattachment attach, GLRenderbuffer* rbo) {
    glNamedFramebufferRenderbuffer(fbo->GetHandle(), glfboattach_map[attach], GL_RENDERBUFFER, rbo->GetHandle());
}
static void newfboblit(GLFramebuffer* src, GLFramebuffer* dst, uint32_t w, uint32_t h, bool color, bool depth) {
    GLuint buffermask = 0;
    if (color)  buffermask |= GL_COLOR_BUFFER_BIT;
    if (depth)  buffermask |= GL_DEPTH_BUFFER_BIT;
    glBlitNamedFramebuffer(src->GetHandle(), dst->GetHandle(),
        0, 0, w, h,
        0, 0, w, h,
        buffermask,
        GL_NEAREST);
}
//////////////////////////////
//////////////////////////////
static GLuint(*gl_fbocreatefunc)() = legacyfbocreate;
static void(*gl_fboattachtexturefunc)(GLFramebuffer*, eGL_fbotarget, eGL_fboattachment, GLTexture*, int) = legacyfbotextureattach;
static void(*gl_fboattachrenderbufferfunc)(GLFramebuffer*, eGL_fbotarget, eGL_fboattachment, GLRenderbuffer*) = legacyfborenderbufferattach;
static void(*gl_fboblitfunc)(GLFramebuffer*, GLFramebuffer*, uint32_t, uint32_t, bool, bool) = legacyfboblit;
//////////////////////////////
//////////////////////////////


void GLFramebuffer_initenv()
{
    if (glNamedFramebufferTexture != NULL)
    {
        gl_fbocreatefunc = newfbocreate;
        gl_fboattachtexturefunc = newfbotextureattach;
        gl_fboattachrenderbufferfunc = newfborenderbufferattach;
        gl_fboblitfunc = newfboblit;
    }
}

GLFramebuffer::GLFramebuffer()
{
    memset(&m_pattachedtextures, 0, sizeof(m_pattachedtextures));
    memset(&m_pattachedRBOs, 0, sizeof(m_pattachedRBOs));
    m_hGLHandle = gl_fbocreatefunc();
}
GLFramebuffer::~GLFramebuffer()
{
    if (m_hGLHandle != 0)
        glDeleteFramebuffers(1, &m_hGLHandle);
}

void GLFramebuffer::BlitToFBO(GLFramebuffer* dst, uint32_t width, uint32_t height,
    bool color, bool depth)
{
    gl_fboblitfunc(this, dst, width, height, color, depth);
}


void GLFramebuffer::AttachTexture(eGL_fboattachment attach, GLTexture* tex, eGL_fbotarget fbotarget, int depth)
{
    GLuint handle = tex ? tex->GetHandle() : 0;
    if (m_pattachedtextures[attach])m_pattachedtextures[attach]->SetAttachedTo(nullptr);
    if (m_pattachedRBOs[attach])    m_pattachedRBOs[attach]->SetAttachedTo(nullptr);
    
    m_pattachedtextures[attach] = tex;
    m_pattachedRBOs[attach] = nullptr;

    gl_fboattachtexturefunc(this, fbotarget, attach, tex, depth);

    m_pattachedtextures[attach]->SetAttachedTo(this);
}

void GLFramebuffer::AttachRenderBuffer(eGL_fboattachment attach, GLRenderbuffer* rbo, eGL_fbotarget fbotarget) 
{
    GLuint handle = rbo ? rbo->GetHandle() : 0;
    if (m_pattachedtextures[attach])m_pattachedtextures[attach]->SetAttachedTo(nullptr);
    if (m_pattachedRBOs[attach])    m_pattachedRBOs[attach]->SetAttachedTo(nullptr);
    
    m_pattachedtextures[attach] = nullptr;
    m_pattachedRBOs[attach] = rbo;

    gl_fboattachrenderbufferfunc(this, fbotarget, attach, rbo);

    m_pattachedRBOs[attach]->SetAttachedTo(this);
}

void GLFramebuffer::WarnDeletedTex(GLTexture* tex)
{
    for (int i = 0; i < eGL_fboattachment_enummax; i++)
    {
        if (m_pattachedtextures[i] != tex)
            continue;
        m_pattachedtextures[i] = nullptr;
        break;
    }
}
void GLFramebuffer::WarnDeletedRenderbuffer(GLRenderbuffer* rbo)
{
    for (int i = 0; i < eGL_fboattachment_enummax; i++)
    {
        if (m_pattachedRBOs[i] != rbo)
            continue;
        m_pattachedRBOs[i] = nullptr;
        break;
    }
}