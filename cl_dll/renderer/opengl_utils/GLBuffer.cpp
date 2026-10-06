#include "glWrapper.h"
#include "glcommon_internal.h"

//////////////////////////////
//////////////////////////////
static void legacybuffersetup(GLBuffer* buffer, uint32_t numbytes, uint8_t* data) {
    GLBuffer* popbuf;
    switch (buffer->GetTarget())
    {
    case GL_ARRAY_BUFFER:
        popbuf = GLContext::GetBoundArrayBuffer();
        GLContext::BindArrayBuffer((GLArrayBuffer*)buffer);
        glBufferData(GL_ARRAY_BUFFER, numbytes, data, GL_DYNAMIC_DRAW);
        GLContext::BindArrayBuffer((GLArrayBuffer*)popbuf);
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        if (GLContext::GetBoundVAO())
        {
            popbuf = GLContext::GetBoundVAO()->GetBoundElementArrayBuffer();
            GLContext::GetBoundVAO()->BindElementArrayBuffer((GLElementArrayBuffer*)buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, numbytes, data, GL_DYNAMIC_DRAW);
            GLContext::GetBoundVAO()->BindElementArrayBuffer((GLElementArrayBuffer*)popbuf);
        }
        break;
    case GL_UNIFORM_BUFFER:
        popbuf = GLContext::GetBoundUniformBuffer();
        GLContext::BindUniformBuffer((GLUniformBuffer*)buffer);
        glBufferData(GL_UNIFORM_BUFFER, numbytes, data, GL_DYNAMIC_DRAW);
        GLContext::BindUniformBuffer((GLUniformBuffer*)popbuf);
        break;
    }
}
static void legacybuffermemcpy(GLBuffer* buffer, uint32_t numbytes, uint8_t* bytes, uint32_t bytesoffset) {
    GLBuffer* popbuf;
    switch (buffer->GetTarget())
    {
    case GL_ARRAY_BUFFER:
        popbuf = GLContext::GetBoundArrayBuffer();
        GLContext::BindArrayBuffer((GLArrayBuffer*)buffer);
        glBufferSubData(GL_ARRAY_BUFFER, bytesoffset, numbytes, bytes);
        GLContext::BindArrayBuffer((GLArrayBuffer*)popbuf);
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        if (GLContext::GetBoundVAO())
        {
            popbuf = GLContext::GetBoundVAO()->GetBoundElementArrayBuffer();
            GLContext::GetBoundVAO()->BindElementArrayBuffer((GLElementArrayBuffer*)buffer);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, bytesoffset, numbytes, bytes);
            GLContext::GetBoundVAO()->BindElementArrayBuffer((GLElementArrayBuffer*)popbuf);
        }
        break;
    case GL_UNIFORM_BUFFER:
        popbuf = GLContext::GetBoundUniformBuffer();
        GLContext::BindUniformBuffer((GLUniformBuffer*)buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, bytesoffset, numbytes, bytes);
        GLContext::BindUniformBuffer((GLUniformBuffer*)popbuf);
        break;
    }
}
//////////////////////////////
//////////////////////////////
static void newbuffercreate(GLBuffer* buffer, uint32_t numbytes, uint8_t* data) {
    glNamedBufferData(buffer->GetHandle(), numbytes, data, GL_DYNAMIC_DRAW);
}
static void newbuffermemcpy(GLBuffer* buffer, uint32_t numbytes, uint8_t* bytes, uint32_t bytesoffset) {
    glNamedBufferSubData(buffer->GetHandle(), bytesoffset, numbytes, bytes);
}
//////////////////////////////
//////////////////////////////
static void (*gl_buffercreatefunc)(GLBuffer*, uint32_t, uint8_t*) = legacybuffersetup;
static void(*gl_buffermemcopyfunc)(GLBuffer*, uint32_t, uint8_t*, uint32_t) = legacybuffermemcpy;
//////////////////////////////
//////////////////////////////

void GLBuffer_initenv()
{
    if (glNamedBufferData != NULL)
    {
        gl_buffercreatefunc = newbuffercreate;
        gl_buffermemcopyfunc = newbuffermemcpy;
    }

}

GLBuffer::GLBuffer(GLuint target, uint32_t numbytes, uint8_t* data)
    : m_eTarget(target), m_uiSize(numbytes)
{
    if (glNamedBufferData != NULL){
        glCreateBuffers(1, &m_hGLHandle);
    }
    else{
        glGenBuffers(1, &m_hGLHandle);
    }
    gl_buffercreatefunc(this, numbytes, data);
}
GLBuffer::~GLBuffer()
{
    glDeleteBuffers(1, &m_hGLHandle);
}
void GLBuffer::MemCpy(uint32_t numbytes, uint8_t* data, uint32_t offset)
{
    _GLASSERT(numbytes + offset <= m_uiSize, "GLBuffer::MemCpy called with more data than this buffer can hold!\n");
    gl_buffermemcopyfunc(this, numbytes, data, offset);
}
GLuint GLBuffer::GetTarget()
{
    return m_eTarget;
}
uint32_t GLBuffer::GetSize()
{
    return m_uiSize;
}

//////////////////////////////////////////////////////////////////////

GLArrayBuffer::GLArrayBuffer(uint32_t numbytes, uint8_t* data)
    : GLBuffer(GL_ARRAY_BUFFER, numbytes, data)
{
}

//////////////////////////////////////////////////////////////////////

GLElementArrayBuffer::GLElementArrayBuffer(uint32_t numindices, uint32_t* data)
    : GLBuffer(GL_ELEMENT_ARRAY_BUFFER, numindices * sizeof(uint32_t), (uint8_t*)data), m_eType(eGL_type::eGL_type_uint32)
{
}
GLElementArrayBuffer::GLElementArrayBuffer(uint32_t numindices, uint16_t* data)
    : GLBuffer(GL_ELEMENT_ARRAY_BUFFER, numindices * sizeof(uint16_t), (uint8_t*)data), m_eType(eGL_type::eGL_type_uint16)
{
}
GLElementArrayBuffer::GLElementArrayBuffer(uint32_t numindices, uint8_t* data)
    : GLBuffer(GL_ELEMENT_ARRAY_BUFFER, numindices * sizeof(uint8_t), data), m_eType(eGL_type::eGL_type_uint8)
{
}


//////////////////////////////////////////////////////////////////////

GLUniformBuffer::GLUniformBuffer(uint32_t numbytes, uint8_t* data)
    : GLBuffer(GL_UNIFORM_BUFFER, numbytes, data)
{
}