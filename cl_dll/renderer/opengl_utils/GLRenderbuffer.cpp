#include "glWrapper.h"
#include "glcommon_internal.h"


//////////////////////////////
//////////////////////////////
static GLuint legacyrbocreate(eGL_texformat format, uint32_t width, uint32_t height) {
    GLuint handle;
    glGenRenderbuffers(1, &handle);
    glBindRenderbuffer(GL_RENDERBUFFER, handle);
    glRenderbufferStorage(GL_RENDERBUFFER, gltexformat_map[format], width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return handle;
}
//////////////////////////////
//////////////////////////////
static GLuint newrbocreate(eGL_texformat format, uint32_t width, uint32_t height) {
    GLuint handle;
    glCreateRenderbuffers(1, &handle);
    glNamedRenderbufferStorage(handle, gltexformat_map[format], width, height);
    return handle;
}
//////////////////////////////
//////////////////////////////
static GLuint(*gl_rbocreatefunc)(eGL_texformat, uint32_t, uint32_t) = legacyrbocreate;
//////////////////////////////
//////////////////////////////


void GLRenderbuffer_initenv()
{
    if (glNamedRenderbufferStorage != NULL)
    {
        gl_rbocreatefunc = newrbocreate;
    }
}

GLRenderbuffer::GLRenderbuffer(eGL_texformat format, uint32_t width, uint32_t height)
	: m_pAttachedTo(nullptr), m_width(width), m_height(height)
{
    m_hGLHandle = gl_rbocreatefunc(format, width, height);
}
GLRenderbuffer::~GLRenderbuffer() 
{
	glDeleteRenderbuffers(1, &m_hGLHandle);
    if (m_pAttachedTo)
        m_pAttachedTo->WarnDeletedRenderbuffer(this);
}

void GLRenderbuffer::SetAttachedTo(GLFramebuffer* fbo) 
{
	m_pAttachedTo = fbo;
}