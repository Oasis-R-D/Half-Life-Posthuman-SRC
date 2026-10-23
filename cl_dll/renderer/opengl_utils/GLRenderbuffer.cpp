#include "glWrapper.h"
#include "glcommon_internal.h"


//////////////////////////////
//////////////////////////////
static GLuint legacyrbocreate(eGL_texformat format, uint32_t width, uint32_t height, int numsamples) {
    GLuint handle;
    glGenRenderbuffers(1, &handle);
    glBindRenderbuffer(GL_RENDERBUFFER, handle);
	if (numsamples <= 1)
        glRenderbufferStorage(GL_RENDERBUFFER, gltexformat_map[format], width, height);
	else
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, numsamples, gltexformat_map[format], width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return handle;
}
//////////////////////////////
//////////////////////////////
static GLuint newrbocreate(eGL_texformat format, uint32_t width, uint32_t height, int numsamples)
{
    GLuint handle;
    glCreateRenderbuffers(1, &handle);
	if (numsamples <= 1)
        glNamedRenderbufferStorage(handle, gltexformat_map[format], width, height);
	else
		glNamedRenderbufferStorageMultisample(handle, numsamples, gltexformat_map[format], width, height);
    return handle;
}
//////////////////////////////
//////////////////////////////
static GLuint(*gl_rbocreatefunc)(eGL_texformat, uint32_t, uint32_t, int) = legacyrbocreate;
//////////////////////////////
//////////////////////////////


void GLRenderbuffer_initenv()
{
    if (glNamedRenderbufferStorage != NULL)
    {
        gl_rbocreatefunc = newrbocreate;
    }
}

GLRenderbuffer::GLRenderbuffer(eGL_texformat format, uint32_t width, uint32_t height, int numsamples)
	: m_pAttachedTo(nullptr), m_width(width), m_height(height)
{
    m_hGLHandle = gl_rbocreatefunc(format, width, height, numsamples);
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