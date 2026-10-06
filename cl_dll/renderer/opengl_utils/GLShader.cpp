#include "glWrapper.h"
#include "glcommon_internal.h"

static const std::vector<uint16_t> s_default_versions = {
    130, 140, 150, 330, 400, 410, 420, 430, 440, 450, 460
};

//////////////////////////////
//////////////////////////////
static void legacyuniformvecfunc(
    GLShader* shader,
    uint32_t index,
    uint32_t vecsize,
    eGL_type type,
    const void* val)
{
    GLShader* popshader = GLContext::GetBoundShader();
    GLContext::BindShader(shader);
    switch (type)
    {
    case eGL_type_float:
        switch (vecsize)
        {
        case 1: glUniform1fv(index, 1, (float*)val); break;
        case 2: glUniform2fv(index, 1, (float*)val); break;
        case 3: glUniform3fv(index, 1, (float*)val); break;
        case 4: glUniform4fv(index, 1, (float*)val); break;
        }
        break;
    case eGL_type_int32:
        switch (vecsize)
        {
        case 1: glUniform1iv(index, 1, (int*)val); break;
        case 2: glUniform2iv(index, 1, (int*)val); break;
        case 3: glUniform3iv(index, 1, (int*)val); break;
        case 4: glUniform4iv(index, 1, (int*)val); break;
        }
        break;
    }
    GLContext::BindShader(popshader);
}
static void legacyuniformmatrix3func(GLShader* shader, uint32_t index, bool trans, const float* val) {
    GLShader* popshader = GLContext::GetBoundShader();
    GLContext::BindShader(shader);
    glUniformMatrix3fv(index, 1, trans ? GL_TRUE : GL_FALSE, val);
    GLContext::BindShader(popshader);
}
static void legacyuniformmatrix3x4func(GLShader* shader, uint32_t index, bool trans, const float* val) {
    GLShader* popshader = GLContext::GetBoundShader();
    GLContext::BindShader(shader);
    glUniformMatrix3x4fv(index, 1, trans ? GL_TRUE : GL_FALSE, val);
    GLContext::BindShader(popshader);
}
static void legacyuniformmatrix4func(GLShader* shader, uint32_t index, bool trans, const float* val) {
    GLShader* popshader = GLContext::GetBoundShader();
    GLContext::BindShader(shader);
    glUniformMatrix4fv(index, 1, trans ? GL_TRUE : GL_FALSE, val);
    GLContext::BindShader(popshader);
}
//////////////////////////////
//////////////////////////////
static void newuniformvecfunc(
    GLShader* shader,
    uint32_t index,
    uint32_t vecsize,
    eGL_type type,
    const void* val)
{
    switch (type)
    {
    case eGL_type_float:
        switch (vecsize)
        {
        case 1: glProgramUniform1fv(shader->GetHandle(), index, 1, (float*)val); break;
        case 2: glProgramUniform2fv(shader->GetHandle(), index, 1, (float*)val); break;
        case 3: glProgramUniform3fv(shader->GetHandle(), index, 1, (float*)val); break;
        case 4: glProgramUniform4fv(shader->GetHandle(), index, 1, (float*)val); break;
        }
        break;
    case eGL_type_int32:
        switch (vecsize)
        {
        case 1: glProgramUniform1iv(shader->GetHandle(), index, 1, (int*)val); break;
        case 2: glProgramUniform2iv(shader->GetHandle(), index, 1, (int*)val); break;
        case 3: glProgramUniform3iv(shader->GetHandle(), index, 1, (int*)val); break;
        case 4: glProgramUniform4iv(shader->GetHandle(), index, 1, (int*)val); break;
        }
        break;
    }
}
static void newuniformmatrix3func(GLShader* shader, uint32_t index, bool trans, const float* val) {
    glProgramUniformMatrix3fv(shader->GetHandle(), index, 1, trans ? GL_TRUE : GL_FALSE, val);
}
static void newuniformmatrix3x4func(GLShader* shader, uint32_t index, bool trans, const float* val) {
    glProgramUniformMatrix3x4fv(shader->GetHandle(), index, 1, trans ? GL_TRUE : GL_FALSE, val);
}
static void newuniformmatrix4func(GLShader* shader, uint32_t index, bool trans, const float* val) {
    glProgramUniformMatrix4fv(shader->GetHandle(), index, 1, trans ? GL_TRUE : GL_FALSE, val);
}
//////////////////////////////
//////////////////////////////
static void(*gl_uniformvecfunc)(GLShader*, uint32_t, uint32_t, eGL_type, const void*) = legacyuniformvecfunc;
static void(*gl_uniformmatrix3func)(GLShader*, uint32_t, bool, const float*) = legacyuniformmatrix3func;
static void(*gl_uniformmatrix3x4func)(GLShader*, uint32_t, bool, const float*) = legacyuniformmatrix3x4func;
static void(*gl_uniformmatrix4func)(GLShader*, uint32_t, bool, const float*) = legacyuniformmatrix4func;

static bool checkCompileError(GLuint shaderhandle)
{
    GLint status;
    glGetShaderiv(shaderhandle, GL_COMPILE_STATUS, &status);
    if (status != GL_FALSE)
        return true;

    char msgbuffer[512];
	GLsizei msgsize = sizeof(msgbuffer);
    glGetShaderInfoLog(shaderhandle, msgsize, NULL, msgbuffer);
    _GLError(msgbuffer);
    return false;
}
static void checkLinkError(GLuint proghandle)
{
    GLint status;
    glGetProgramiv(proghandle, GL_LINK_STATUS, &status);
    if (status != GL_FALSE)
        return;

    char msgbuffer[512];
    GLsizei msgsize = sizeof(msgbuffer);
    glGetProgramInfoLog(proghandle, msgsize, NULL, msgbuffer);
    _GLError(msgbuffer);
}

void GLShader_initenv()
{
    if (glProgramUniform1d != NULL)
    {
        gl_uniformvecfunc = newuniformvecfunc;
        gl_uniformmatrix3func = newuniformmatrix3func;
        gl_uniformmatrix3x4func = newuniformmatrix3x4func;
        gl_uniformmatrix4func = newuniformmatrix4func;
    }
}


GLShader::GLShader(const glshader_createinfo_t& info)
{
    m_hGLHandle = glCreateProgram();

    GLuint vertshader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragshader = glCreateShader(GL_FRAGMENT_SHADER);

    std::vector<uint16_t> versions = s_default_versions;
    if (!info.supported_versions.empty())
        versions = info.supported_versions;

    for (const auto& i : versions)
    {
        std::string vertstring, fragstring;
        if (i < 330)
        {
            vertstring = fragstring =
                "#version " + std::to_string(i) + '\n' +
                "#define GLSL_" + std::to_string(i) + '\n';
        }
        else
        {
            vertstring = fragstring =
                "#version " + std::to_string(i) + "compatibility\n" +
                "#define GLSL_" + std::to_string(i) + '\n';
        }
		vertstring += glsl_engine_defines_vertex + info.vertcode;
		fragstring += glsl_engine_defines_fragment + info.fragcode;
        const char* vertstringcstr = vertstring.c_str();
        const char* fragstringcstr = fragstring.c_str();
        glShaderSource(vertshader, 1, &vertstringcstr, NULL);
        glShaderSource(fragshader, 1, &fragstringcstr, NULL);

        glCompileShader(vertshader);
        if (!checkCompileError(vertshader)) continue;
        glCompileShader(fragshader);
        if (!checkCompileError(fragshader)) continue;

        break;
    }

    glAttachShader(m_hGLHandle, vertshader);
    glAttachShader(m_hGLHandle, fragshader);

    for (const auto& i : info.defineattribs)
        glBindAttribLocation(m_hGLHandle, i.loc, i.name.c_str());

    glLinkProgram(m_hGLHandle);
    checkLinkError(m_hGLHandle);

    //formally index the program's variables
    GLint numuniforms, numuniformblocks, numattribs;
    char* buffer = new char[512];
    glGetProgramiv(m_hGLHandle, GL_ACTIVE_UNIFORMS, &numuniforms);
    glGetProgramiv(m_hGLHandle, GL_ACTIVE_UNIFORM_BLOCKS, &numuniformblocks);
    glGetProgramiv(m_hGLHandle, GL_ACTIVE_ATTRIBUTES, &numattribs);
    m_vUniforms.resize(numuniforms);
    m_vUniformBlocks.resize(numuniformblocks);
    m_vAttribs.resize(numattribs);
    GLsizei nouse[3];
    for (GLint i = 0; i < numuniforms; ++i)
    {
        glGetActiveUniform(m_hGLHandle, i, 512, &nouse[0], &nouse[1], (GLenum*)&nouse[2], buffer);
        m_vUniforms[i].name = buffer;
        m_vUniforms[i].index = glGetUniformLocation(m_hGLHandle, buffer);
    }
    for (GLint i = 0; i < numuniformblocks; ++i)
    {
        glGetActiveUniformBlockName(m_hGLHandle, i, 512, &nouse[0], buffer);
        m_vUniformBlocks[i].name = buffer;
        m_vUniformBlocks[i].index = glGetUniformBlockIndex(m_hGLHandle, buffer);

        for (const auto& j : info.defineubos)
        {
            if (j.name != m_vUniformBlocks[i].name)
                continue;
            glUniformBlockBinding(m_hGLHandle, m_vUniformBlocks[i].index, j.loc);
            break;
        }
    }
    for (GLint i = 0; i < numattribs; ++i)
    {
        glGetActiveAttrib(m_hGLHandle, i, 512, &nouse[0], &nouse[1], (GLenum*)&nouse[2], buffer);
        m_vAttribs[i].name = buffer;
        m_vAttribs[i].index = glGetAttribLocation(m_hGLHandle, buffer);
    }

    //cleanup
    glDeleteShader(vertshader);
    glDeleteShader(fragshader);
    delete[] buffer;

}
GLShader::~GLShader()
{
    glDeleteProgram(m_hGLHandle);
}

uint32_t GLShader::GetUniformLoc(const char* name)
{
    for (const auto& i : m_vUniforms)
    {
        if (!strcmp(name, i.name.c_str()))
            return i.index;
    }
    return -1;
}


void GLShader::SetUniformFloat(uint32_t index, const float val) {
    gl_uniformvecfunc(this, index, 1, eGL_type::eGL_type_float, (void*)&val);
}
void GLShader::SetUniformVec2(uint32_t index, const float* val) {
    gl_uniformvecfunc(this, index, 2, eGL_type::eGL_type_float, (void*)val);
}
void GLShader::SetUniformVec3(uint32_t index, const float* val) {
    gl_uniformvecfunc(this, index, 3, eGL_type::eGL_type_float, (void*)val);
}
void GLShader::SetUniformVec4(uint32_t index, const float* val) {
    gl_uniformvecfunc(this, index, 4, eGL_type::eGL_type_float, (void*)val);
}

void GLShader::SetUniformInt(uint32_t index, const int val) {
    gl_uniformvecfunc(this, index, 1, eGL_type::eGL_type_int32, (void*)&val);
}
void GLShader::SetUniformIVec2(uint32_t index, const int* val) {
    gl_uniformvecfunc(this, index, 2, eGL_type::eGL_type_int32, (void*)val);
}
void GLShader::SetUniformIVec3(uint32_t index, const int* val) {
    gl_uniformvecfunc(this, index, 3, eGL_type::eGL_type_int32, (void*)val);
}
void GLShader::SetUniformIVec4(uint32_t index, const int* val) {
    gl_uniformvecfunc(this, index, 4, eGL_type::eGL_type_int32, (void*)val);
}

void GLShader::SetUniformMatrix3x3(uint32_t index, const float* val, bool transpose) {
    gl_uniformmatrix3func(this, index, transpose, val);
}
void GLShader::SetUniformMatrix3x4(uint32_t index, const float* val, bool transpose) {
    gl_uniformmatrix3x4func(this, index, transpose, val);
}
void GLShader::SetUniformMatrix4x4(uint32_t index, const float* val, bool transpose) {
    gl_uniformmatrix4func(this, index, transpose, val);
}

void GLShader::SetUniformSampler2D(uint32_t index, int texindex) {
    gl_uniformvecfunc(this, index, 1, eGL_type::eGL_type_int32, &texindex);
}