#define GLAD_GL_IMPLEMENTATION

#include "glWrapper.h"
#include "glcommon_internal.h"

static uint32_t s_glDriverUBOAlignment;

static struct glstates_t
{
    //glPolygonMode
    eGL_face m_polymodeface;
    eGL_polymode m_polymode;

    //blending
    eGL_blendfactor m_rgb_blendsrcfactor;
    eGL_blendfactor m_rgb_blenddstfactor;
    eGL_blendfactor m_alpha_blendsrcfactor;
    eGL_blendfactor m_alpha_blenddstfactor;
    eGL_blendequation m_blendequation;
    bool m_bBlending;

    eGL_frontface m_frontface;

    eGL_comparefunc m_depthfunc;
    bool m_bDepthTest;
    bool m_bDepthWrite;

    bool m_bPolyOffsetFill;

    eGL_face m_cullfacemode;
    bool m_bCullFace;

    //glViewport
    uint16_t m_x;
    uint16_t m_y;
    uint16_t m_width;
    uint16_t m_height;

    float m_fdepthclearvalue;
    float m_fcolorclearvalue[4];

    bool m_bAlphaTest;

    bool m_bDepthClamp;
};
glstates_t s_gl_states;
glstates_t s_gl_default_states;
static struct
{
    GLShader* m_pShader;
    GLArrayBuffer* m_pArrayBuffer;
    GLUniformBuffer* m_pUniformBuffer;
    GLVertexArray* m_pVAO;
    GLFramebuffer* m_pDrawFBO;
    GLFramebuffer* m_pReadFBO;
} s_gl_bindings;

const GLuint gltype_map[] = {
    GL_UNSIGNED_BYTE,
    GL_BYTE,
    GL_UNSIGNED_SHORT,
    GL_SHORT,
    GL_UNSIGNED_INT,
    GL_INT,
    GL_FLOAT
};
const GLuint gltexformat_map[] = {
    GL_DEPTH_COMPONENT,
    GL_DEPTH_COMPONENT16,
    GL_DEPTH_COMPONENT24,
    GL_DEPTH_COMPONENT32,
    GL_DEPTH_COMPONENT32F,
    GL_DEPTH_STENCIL,


    GL_R,
    GL_R8,
    GL_R16,

    GL_R8I,
    GL_R16I,
    GL_R32I,

    GL_R8UI,
    GL_R16UI,
    GL_R32UI,

    GL_R16F,
    GL_R32F,


    GL_RG,
    GL_RG8,
    GL_RG16,

    GL_RG8I,
    GL_RG16I,
    GL_RG32I,

    GL_RG8UI,
    GL_RG16UI,
    GL_RG32UI,

    GL_RG16F,
    GL_RG32F,


    GL_RGB,
    GL_RGB8,
    GL_RGB16,

    GL_RGB8I,
    GL_RGB16I,
    GL_RGB32I,

    GL_RGB8UI,
    GL_RGB16UI,
    GL_RGB32UI,

    GL_RGB16F,
    GL_RGB32F,

    GL_RGBA,
    GL_RGBA8,
    GL_RGBA16,

    GL_RGBA8I,
    GL_RGBA16I,
    GL_RGBA32I,

    GL_RGBA8UI,
    GL_RGBA16UI,
    GL_RGBA32UI,

    GL_RGBA16F,
    GL_RGBA32F,

    GL_COMPRESSED_RGB_S3TC_DXT1_EXT,

    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
};
static constexpr GLuint glblendfactor_map[] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
    GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA
};
static constexpr GLuint glblendequation_map[] = {
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX
};
static constexpr GLuint glfrontface_map[] = {
    GL_CW,
    GL_CCW
};
static constexpr GLuint glfbotarget_map[] = {
    GL_DRAW_FRAMEBUFFER,
    GL_READ_FRAMEBUFFER,
    GL_FRAMEBUFFER
};
static constexpr GLuint gldrawmode_map[] = {
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_QUADS
};
static constexpr GLuint glcomparefunc_map[] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS
};
static constexpr GLuint glface_map[] = {
    GL_FRONT,
    GL_BACK,
    GL_FRONT_AND_BACK
};
static constexpr GLuint glpolymode_map[] = {
    GL_POINT,
    GL_LINE,
    GL_FILL
};

static_assert(sizeof(glblendfactor_map) == (eGL_blendfactor_enummax * 4), "");
static_assert(sizeof(glblendequation_map) == (eGL_blendequation_enummax * 4), "");
static_assert(sizeof(glfrontface_map) == (eGL_frontface_enummax * 4), "");
static_assert(sizeof(glfbotarget_map) == (eGL_fbotarget_enummax * 4), "");
static_assert(sizeof(gldrawmode_map) == (eGL_drawmode_enummax * 4), "");
static_assert(sizeof(glcomparefunc_map) == (eGL_comparefunc_enummax * 4), "");
static_assert(sizeof(glface_map) == (eGL_face_enummax * 4), "");
static_assert(sizeof(glpolymode_map) == (eGL_polymode_enummax * 4), "");




static bool GLIsCore() {
    GLint profile;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    return profile & GL_CONTEXT_CORE_PROFILE_BIT;
}
//////////////////////////////
//////////////////////////////
static void legacytexbind(GLTexture* tex, uint32_t slot) {
    glActiveTexture(slot + GL_TEXTURE0);
    glBindTexture(tex ? tex->GetTargetType() : GL_TEXTURE_2D, tex ? tex->GetHandle() : 0);
}
//////////////////////////////
//////////////////////////////
static void newtexbind(GLTexture* tex, uint32_t slot) {
    glBindTextureUnit(slot, tex ? tex->GetHandle() : 0);
}
//////////////////////////////
//////////////////////////////
static void (*gl_texturebindfunc)(GLTexture*, uint32_t) = legacytexbind;
//////////////////////////////
//////////////////////////////

extern void GLDebugLayer_initialize();

extern void GLTexture_initenv();
extern void GLFramebuffer_initenv();
extern void GLRenderbuffer_initenv();
extern void GLShader_initenv();
extern void GLBuffer_initenv();
extern void GLVertexArray_initenv();

static void dummyErrorFunc(const char* msg)
{
    printf(msg);
}

static void(*_customError)(const char*) = dummyErrorFunc;

void _GLError(const char* msg)
{
    std::string str;
    str = "[OpenGL] ";
    str += msg + '\n';
    _customError(msg);
}


namespace GLContext
{
    class GLinternal_MainFramebuffer : public GLFramebuffer
    {
    public:
        GLinternal_MainFramebuffer(int dummy) { (int)dummy;m_hGLHandle = 0; };

        void _sethandle(GLuint handle) { m_hGLHandle = handle; };
    };
	GLinternal_MainFramebuffer* s_pMainFBO = nullptr;

    void Init_GLContext(const glcontext_initinfo_t& info) {
        if (!gladLoadGL(info.fn))
        {
            _GLError("OpenGL functions could not be loaded!\n");
            return;
        }

        if(info.gl_loggerFn != nullptr)
            _customError = info.gl_loggerFn;
        
        if(info.debuglayer)
            GLDebugLayer_initialize();

        if (glBindTextureUnit != NULL)
            gl_texturebindfunc = newtexbind;

        GLTexture_initenv();
        GLFramebuffer_initenv();
        GLRenderbuffer_initenv();
        GLShader_initenv();
        GLBuffer_initenv();
        GLVertexArray_initenv();

        s_gl_bindings.m_pDrawFBO = s_gl_bindings.m_pReadFBO = s_pMainFBO = new GLinternal_MainFramebuffer(0);

        memset(&s_gl_states, 0, sizeof(s_gl_states));
        s_gl_states.m_polymodeface = eGL_face_frontback;
        s_gl_states.m_polymode = eGL_polymode_fill;

        s_gl_states.m_rgb_blendsrcfactor = eGL_blendfactor_one;
        s_gl_states.m_rgb_blenddstfactor = eGL_blendfactor_zero;
        s_gl_states.m_alpha_blendsrcfactor = eGL_blendfactor_one;
        s_gl_states.m_alpha_blenddstfactor = eGL_blendfactor_zero;
        s_gl_states.m_blendequation = eGL_blendequation_add;
        s_gl_states.m_bBlending = false;

        s_gl_states.m_frontface = eGL_frontface_counterclockwise;

        s_gl_states.m_depthfunc = eGL_comparefunc_less;

        s_gl_states.m_bDepthTest = false;
        s_gl_states.m_bDepthWrite = true;

        s_gl_states.m_bPolyOffsetFill = false;

        s_gl_states.m_cullfacemode = eGL_face_back;
        s_gl_states.m_bCullFace = false;

        s_gl_states.m_bAlphaTest = false;
		s_gl_states.m_bDepthClamp = false;

        memcpy(&s_gl_default_states, &s_gl_states, sizeof(s_gl_states));

        GLint temp;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &temp);
		s_glDriverUBOAlignment = temp;
    }
    void Destroy_GLContext() {
        delete s_pMainFBO;
    }

    void ResetStates() {
		memcpy(&s_gl_states, &s_gl_default_states, sizeof(s_gl_states));

		glPolygonMode(glface_map[s_gl_states.m_polymodeface], glpolymode_map[s_gl_states.m_polymode]);
		glDepthFunc(glcomparefunc_map[s_gl_states.m_depthfunc]);
		glDepthMask(s_gl_states.m_bDepthWrite? GL_TRUE : GL_FALSE);
		glBlendEquation(glblendequation_map[s_gl_states.m_blendequation]);
		glFrontFace(glfrontface_map[s_gl_states.m_frontface]);
        glCullFace(glface_map[s_gl_states.m_cullfacemode]);
        glClearDepth(s_gl_states.m_fdepthclearvalue);
		glClearColor(s_gl_states.m_fcolorclearvalue[0], s_gl_states.m_fcolorclearvalue[1], s_gl_states.m_fcolorclearvalue[2], s_gl_states.m_fcolorclearvalue[3]);
    
        glBlendFuncSeparate(
			glblendfactor_map[s_gl_states.m_rgb_blendsrcfactor],
			glblendfactor_map[s_gl_states.m_rgb_blenddstfactor],
			glblendfactor_map[s_gl_states.m_alpha_blendsrcfactor],
			glblendfactor_map[s_gl_states.m_alpha_blenddstfactor]);
		glViewport(
			s_gl_states.m_x, s_gl_states.m_y,
			s_gl_states.m_width, s_gl_states.m_height);

        if (s_gl_states.m_bDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (s_gl_states.m_bPolyOffsetFill) glEnable(GL_POLYGON_OFFSET_FILL); else glDisable(GL_POLYGON_OFFSET_FILL);
        if (s_gl_states.m_bBlending) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (s_gl_states.m_bCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		if (s_gl_states.m_bAlphaTest) glEnable(GL_ALPHA_TEST); else glDisable(GL_ALPHA_TEST);
        if (s_gl_states.m_bDepthClamp) glEnable(GL_DEPTH_CLAMP); else glDisable(GL_DEPTH_CLAMP);
    }

    void SetMainFBOHandle(GLuint handle) {
		s_pMainFBO->_sethandle(handle);
     }

    void BindShader(GLShader* shader) {
        if (shader == s_gl_bindings.m_pShader)
            return;

        glUseProgram(shader ? shader->GetHandle() : NULL);
        s_gl_bindings.m_pShader = shader;
    }
    void BindTexture(GLTexture* tex, uint32_t slot) {
        gl_texturebindfunc(tex, slot);
    }
    void BindFramebuffer(GLFramebuffer* fbo, eGL_fbotarget target) {
        _GLASSERT(fbo != NULL, "BindFramebuffer called with null fbo!\n");
        glBindFramebuffer(glfbotarget_map[target], fbo->GetHandle());
        switch (target)
        {
        case eGL_fbotarget_draw: s_gl_bindings.m_pDrawFBO = fbo; break;
        case eGL_fbotarget_read: s_gl_bindings.m_pReadFBO = fbo; break;
        case eGL_fbotarget_drawread: s_gl_bindings.m_pDrawFBO = s_gl_bindings.m_pReadFBO = fbo; break;
        }
    }
    void BindVertexArray(GLVertexArray* vao) {
        GLuint handle = vao ? vao->GetHandle() : NULL;
        if (vao == s_gl_bindings.m_pVAO)
            return;
        glBindVertexArray(handle);
        s_gl_bindings.m_pVAO = vao;
    }
    void BindArrayBuffer(GLArrayBuffer* buffer) {
        GLuint handle = buffer ? buffer->GetHandle() : NULL;
        if (buffer == s_gl_bindings.m_pArrayBuffer)
            return;
        glBindBuffer(GL_ARRAY_BUFFER, handle);
        s_gl_bindings.m_pArrayBuffer = buffer;
    }
	void BindUniformBuffer(GLUniformBuffer* buffer, int program_ubobindpoint, uint32_t size, uint32_t offset)
	{
        GLuint handle = buffer ? buffer->GetHandle() : NULL;
        glBindBuffer(GL_UNIFORM_BUFFER, handle);

        if (program_ubobindpoint != -1)
		{
			if (size == 0)
				glBindBufferBase(GL_UNIFORM_BUFFER, program_ubobindpoint, handle);
            else
			    glBindBufferRange(GL_UNIFORM_BUFFER, program_ubobindpoint, handle, offset, size);
		}

        s_gl_bindings.m_pUniformBuffer = buffer;
    }

    void BindTextureLegacy(GLuint texhandle, GLenum targettype, uint32_t slot) {
		if (glBindTextureUnit)
		{
			glBindTextureUnit(slot, texhandle);
		}
        else
        {
			glActiveTexture(slot + GL_TEXTURE0);
			glBindTexture(targettype, texhandle);
        }
    }

    GLShader* GetBoundShader() { return s_gl_bindings.m_pShader; }
    GLFramebuffer* GetBoundDrawFBO() { return s_gl_bindings.m_pDrawFBO; }
    GLFramebuffer* GetBoundReadFBO() { return s_gl_bindings.m_pReadFBO; }
    GLVertexArray* GetBoundVAO() { return s_gl_bindings.m_pVAO; }
    GLArrayBuffer* GetBoundArrayBuffer() { return s_gl_bindings.m_pArrayBuffer; }
    GLUniformBuffer* GetBoundUniformBuffer() { return s_gl_bindings.m_pUniformBuffer; }

    GLFramebuffer* GetMainFramebuffer() { return s_pMainFBO; }

    //states

    void SetPolygonRasterMode(eGL_polymode mode) {
        s_gl_states.m_polymode = mode;
        glPolygonMode(glface_map[s_gl_states.m_polymodeface], glpolymode_map[s_gl_states.m_polymode]);
    }

    void SetDepthCompare(eGL_comparefunc func) {
        s_gl_states.m_depthfunc = func;
        glDepthFunc(glcomparefunc_map[func]);
    }
    void SetDepthTesting(bool enable) {
        s_gl_states.m_bDepthTest = enable;
        if (enable) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }
    void SetDepthWriting(bool enable) {
        s_gl_states.m_bDepthWrite = enable;
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
    }

    void SetPolygonOffsetFill(bool enable) {
		s_gl_states.m_bPolyOffsetFill = enable;
        if (enable) glEnable(GL_POLYGON_OFFSET_FILL); else glDisable(GL_POLYGON_OFFSET_FILL);
    }

    static void UpdateBlendFunc() {
        glBlendFuncSeparate(
            glblendfactor_map[s_gl_states.m_rgb_blendsrcfactor],
            glblendfactor_map[s_gl_states.m_rgb_blenddstfactor],
            glblendfactor_map[s_gl_states.m_alpha_blendsrcfactor],
            glblendfactor_map[s_gl_states.m_alpha_blenddstfactor]);
    }
    void SetSrcBlendFunc_rgb(eGL_blendfactor factor) {
        s_gl_states.m_rgb_blendsrcfactor = factor;
        UpdateBlendFunc();
    }
    void SetDstBlendFunc_rgb(eGL_blendfactor factor) {
        s_gl_states.m_rgb_blenddstfactor = factor;
        UpdateBlendFunc();
    }
    void SetSrcBlendFunc_alpha(eGL_blendfactor factor) {
        s_gl_states.m_alpha_blendsrcfactor = factor;
        UpdateBlendFunc();
    }
    void SetDstBlendFunc_alpha(eGL_blendfactor factor) {
        s_gl_states.m_alpha_blenddstfactor = factor;
        UpdateBlendFunc();
    }
    void SetBlendFunc_rgb(eGL_blendfactor src, eGL_blendfactor dst) {
		s_gl_states.m_rgb_blendsrcfactor = src;
		s_gl_states.m_rgb_blenddstfactor = dst;
		UpdateBlendFunc();
    }
    void SetBlendFunc_alpha(eGL_blendfactor src, eGL_blendfactor dst) {
		s_gl_states.m_alpha_blendsrcfactor = src;
		s_gl_states.m_alpha_blenddstfactor = dst;
		UpdateBlendFunc();
    }
    void SetBlendFunc_rgba(eGL_blendfactor src, eGL_blendfactor dst) {
		s_gl_states.m_rgb_blendsrcfactor = src;
		s_gl_states.m_rgb_blenddstfactor = dst;
		s_gl_states.m_alpha_blendsrcfactor = src;
		s_gl_states.m_alpha_blenddstfactor = dst;
		UpdateBlendFunc();
    }
    void SetBlendEquation(eGL_blendequation eq) {
        s_gl_states.m_blendequation = eq;
        glBlendEquation(glblendequation_map[eq]);
    }
    void SetBlending(bool enable) {
        s_gl_states.m_bBlending = enable;
        if (enable) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    }

    void SetFrontFaceWise(eGL_frontface wise) {
        s_gl_states.m_frontface = wise;
        glFrontFace(glfrontface_map[wise]);
    }

    void SetFaceToCull(eGL_face face) {
        s_gl_states.m_cullfacemode = face;
        glCullFace(glface_map[face]);
    }
    void SetFaceCulling(bool enable) {
        s_gl_states.m_bCullFace = enable;
        if (enable) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    }

    static void UpdateViewport() {
        glViewport(
            s_gl_states.m_x, s_gl_states.m_y,
            s_gl_states.m_width, s_gl_states.m_height
        );
    }
    void SetViewportPos(uint16_t x, uint16_t y) {
        s_gl_states.m_x = x; s_gl_states.m_y = y;
        UpdateViewport();
    }
    void SetViewportSize(uint16_t width, uint16_t height) {
        s_gl_states.m_width = width; s_gl_states.m_height = height;
        UpdateViewport();
    }

    void SetDepthBufferClearValue(float val) {
        s_gl_states.m_fdepthclearvalue = val;
        glClearDepth(val);
    }
    void SetColorBufferClearValue(float r, float g, float b, float a) {
        s_gl_states.m_fcolorclearvalue[0] = r;
        s_gl_states.m_fcolorclearvalue[1] = g;
        s_gl_states.m_fcolorclearvalue[2] = b;
        s_gl_states.m_fcolorclearvalue[3] = a;
        glClearColor(r, g, b, a);
    }

    void SetAlphaTest(bool enable) {
		s_gl_states.m_bAlphaTest = enable;
		if (enable) glEnable(GL_ALPHA_TEST); else glDisable(GL_ALPHA_TEST);
    }

    void SetDepthClamp(bool enable) {
		s_gl_states.m_bDepthClamp = enable;
		if (enable) glEnable(GL_DEPTH_CLAMP); else glDisable(GL_DEPTH_CLAMP);
    }


    //gets
    eGL_polymode GetPolygonRasterMode() { return s_gl_states.m_polymode; }

    eGL_comparefunc SetDepthCompare() { return s_gl_states.m_depthfunc; }
    bool SetDepthTesting() { return s_gl_states.m_bDepthTest; }
    bool SetDepthWriting() { return s_gl_states.m_bDepthWrite; }

    eGL_blendfactor SetSrcBlendFunc_rgb() { return s_gl_states.m_rgb_blendsrcfactor; }
    eGL_blendfactor SetDstBlendFunc_rgb() { return s_gl_states.m_rgb_blenddstfactor; }
    eGL_blendfactor SetSrcBlendFunc_alpha() { return s_gl_states.m_alpha_blendsrcfactor; }
    eGL_blendfactor SetDstBlendFunc_alpha() { return s_gl_states.m_alpha_blenddstfactor; }
    eGL_blendequation SetBlendEquation() { return s_gl_states.m_blendequation; }
    bool SetBlending() { return s_gl_states.m_bBlending; }

    eGL_frontface SetFrontFaceWise() { return s_gl_states.m_frontface; }

    eGL_face SetFaceToCull() { return s_gl_states.m_cullfacemode; }
    bool SetFaceCulling() { return s_gl_states.m_bCullFace; }

    float GetDepthBufferClearValue() { return s_gl_states.m_fdepthclearvalue; }
    void GetColorBufferClearValue(float* buffer) {
        memcpy(buffer, s_gl_states.m_fcolorclearvalue, sizeof(float) * 4);
    }

    uint32_t GetDriverUBOAlignment() { return s_glDriverUBOAlignment; }

    //commands

    void ClearDepthBuffer() {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    void ClearColorBuffer() {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void DrawPolys(eGL_drawmode mode, uint32_t numpoints, uint32_t pointsoffset, bool indexed) {
        if (!s_gl_bindings.m_pVAO)
            return;
        
        GLElementArrayBuffer* element = s_gl_bindings.m_pVAO->GetBoundElementArrayBuffer();
        if (element && indexed) {
            glDrawElements(gldrawmode_map[mode], numpoints, gltype_map[element->GetType()], (void*)pointsoffset);
        }
        else {
            glDrawArrays(gldrawmode_map[mode], pointsoffset, numpoints);
        }
    }

    void MultiDrawPolys(eGL_drawmode mode, const std::vector<glpolydrawcmd_t>& requests, bool indexed) {
        if (!s_gl_bindings.m_pVAO)
            return;
        
        int numdraws = requests.size();
        std::vector<GLsizei> numpointslist(numdraws); 
        std::vector<GLint> pointsoffsetlist(numdraws);
        for(int i = 0; i < numdraws; i++)
        {
            numpointslist[i] = requests[i].numpoints;
            pointsoffsetlist[i] = requests[i].pointsoffset;
        }
        
        GLElementArrayBuffer* element = s_gl_bindings.m_pVAO->GetBoundElementArrayBuffer();
        if (element && indexed) {
            glMultiDrawElements(gldrawmode_map[mode], numpointslist.data(), gltype_map[element->GetType()], (void**)pointsoffsetlist.data(), numdraws);
        }
        else {
            glMultiDrawArrays(gldrawmode_map[mode], pointsoffsetlist.data(), numpointslist.data(), numdraws);
        }
    }
}