#include "glWrapper.h"
#include "glcommon_internal.h"


//////////////////////////////
//////////////////////////////
static void legacybindelementbuffer(GLVertexArray* vertarray, GLElementArrayBuffer* buf) {
    GLVertexArray* vaopop = GLContext::GetBoundVAO();
    GLContext::BindVertexArray(vertarray);
    GLuint handle = buf ? buf->GetHandle() : 0;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
    GLContext::BindVertexArray(vaopop);
}
static void legacyvaosetup(GLVertexArray* vertarray,
    GLArrayBuffer* vertbuffer,
    const std::vector<gl_vaoattrib_t>& attribs,
    GLElementArrayBuffer* indexbuffer)
{
    GLVertexArray* popvao = GLContext::GetBoundVAO();
    GLArrayBuffer* popbuffer = GLContext::GetBoundArrayBuffer();
    GLContext::BindVertexArray(vertarray);
    GLContext::BindArrayBuffer(vertbuffer);
    if (indexbuffer)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer->GetHandle());

    for (const auto& i : attribs)
    {
        glEnableVertexAttribArray(i.attribindex);
        glVertexAttribPointer(i.attribindex, 1, gltype_map[i.attribtype], i.normalized ? GL_TRUE : GL_FALSE, i.structsize, (void*)i.memberoffset);
    }

    GLContext::BindVertexArray(popvao);
    GLContext::BindArrayBuffer(popbuffer);
}
//////////////////////////////
//////////////////////////////
static void newbindelementbuffer(GLVertexArray* vertarray, GLElementArrayBuffer* buf) {
    GLuint handle = buf ? buf->GetHandle() : 0;
    glVertexArrayElementBuffer(vertarray->GetHandle(), handle);
}
static void newvaosetup(GLVertexArray* vertarray,
    GLArrayBuffer* vertbuffer,
    const std::vector<gl_vaoattrib_t>& attribs,
    GLElementArrayBuffer* indexbuffer)
{
    glVertexArrayVertexBuffer(vertarray->GetHandle(), 0, vertbuffer->GetHandle(), 0, attribs[0].structsize);
    if (indexbuffer)
        glVertexArrayElementBuffer(vertarray->GetHandle(), indexbuffer->GetHandle());

    for (const auto& i : attribs)
    {
        glEnableVertexArrayAttrib(vertarray->GetHandle(), i.attribindex);
        if (i.attribtype < eGL_type_float && !i.normalized)
			glVertexArrayAttribIFormat(vertarray->GetHandle(), i.attribindex, i.numelements, gltype_map[i.attribtype], i.memberoffset);
		else
            glVertexArrayAttribFormat(vertarray->GetHandle(), i.attribindex, i.numelements, gltype_map[i.attribtype], i.normalized ? GL_TRUE : GL_FALSE, i.memberoffset);
        glVertexArrayAttribBinding(vertarray->GetHandle(), i.attribindex, 0);
    }
}
//////////////////////////////
//////////////////////////////
void(*gl_bindelementbufferfunc)(GLVertexArray*, GLElementArrayBuffer*) = legacybindelementbuffer;
void(*gl_vaosetupfunc)(GLVertexArray*, GLArrayBuffer*, const std::vector<gl_vaoattrib_t>&, GLElementArrayBuffer*) = legacyvaosetup;

void GLVertexArray_initenv()
{
    if (glVertexArrayElementBuffer != NULL)
    {
        gl_bindelementbufferfunc = newbindelementbuffer;
        gl_vaosetupfunc = newvaosetup;
    }
}

GLVertexArray::GLVertexArray(
    GLArrayBuffer* vertbuffer,
    const std::vector<gl_vaoattrib_t>& attribs,
    GLElementArrayBuffer* indexbuffer
)
    : m_pElementArrayBuffer(indexbuffer)
{
    if (glCreateVertexArrays != NULL) {
        glCreateVertexArrays(1, &m_hGLHandle);
    }
    else {
        glGenVertexArrays(1, &m_hGLHandle);
    }
    gl_vaosetupfunc(this, vertbuffer, attribs, indexbuffer);
}
GLVertexArray::~GLVertexArray()
{
    glDeleteVertexArrays(1, &m_hGLHandle);
}


void GLVertexArray::BindElementArrayBuffer(GLElementArrayBuffer* buffer)
{
    if (m_pElementArrayBuffer == buffer)
        return;
    m_pElementArrayBuffer = buffer;
    gl_bindelementbufferfunc(this, buffer);
}

GLElementArrayBuffer* GLVertexArray::GetBoundElementArrayBuffer()
{
    return m_pElementArrayBuffer;
}