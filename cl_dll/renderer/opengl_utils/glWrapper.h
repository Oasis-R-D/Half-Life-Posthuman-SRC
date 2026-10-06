#pragma once
#include "glcommon.h"

class GLTexture;
class GLFramebuffer;
class GLRenderbuffer;
class GLVertexArray;
class GLBuffer;
class GLArrayBuffer;
class GLElementArrayBuffer;
class GLUniformBuffer;
class GLShader;

enum eGL_fbotarget : uint8_t{
    eGL_fbotarget_draw = 0,
    eGL_fbotarget_read,
    eGL_fbotarget_drawread,

    eGL_fbotarget_enummax
};

//blend factors (rgba):
//  GL_ZERO                 (0, 0, 0, 0)
//  GL_ONE                  (1, 1, 1, 1)
//  GL_SRC_COLOR            (sR, sG, sB, sA)
//  GL_ONE_MINUS_SRC_COLOR  (1-sR, 1-sG, 1-sB, 1-sA)
//  GL_DST_COLOR            (dR, dG, dB, dA)
//  GL_ONE_MINUS_DST_COLOR  (1-dR, 1-dG, 1-dB, 1-dA)
//  GL_SRC_ALPHA            (sA, sA, sA, sA)
//  GL_ONE_MINUS_SRC_ALPHA  (1-sA, 1-sA, 1-sA, 1-sA)
//  GL_DST_ALPHA            (dA, dA, dA, dA)
//  GL_ONE_MINUS_DST_ALPHA  (1-dA, 1-dA, 1-dA, 1-dA)

enum eGL_blendfactor : uint8_t{
    eGL_blendfactor_zero = 0, eGL_blendfactor_one,
    eGL_blendfactor_srccolor, eGL_blendfactor_1_minus_srccolor,
    eGL_blendfactor_dstcolor, eGL_blendfactor_1_minus_dstcolor,
    eGL_blendfactor_srcalpha, eGL_blendfactor_1_minus_srcalpha,
    eGL_blendfactor_dstalpha, eGL_blendfactor_1_minus_dstalpha,

    eGL_blendfactor_enummax
};
enum eGL_blendequation : uint8_t{
    eGL_blendequation_add = 0,
    eGL_blendequation_sub,
    eGL_blendequation_reverse_sub,
    eGL_blendequation_min,
    eGL_blendequation_max,

    eGL_blendequation_enummax
};
enum eGL_comparefunc : uint8_t{
    eGL_comparefunc_never = 0,
    eGL_comparefunc_less,
    eGL_comparefunc_equal,
    eGL_comparefunc_less_or_equal,
    eGL_comparefunc_greater,
    eGL_comparefunc_not_equal,
    eGL_comparefunc_greater_or_equal,
    eGL_comparefunc_always,

    eGL_comparefunc_enummax
};
enum eGL_face : uint8_t{
    eGL_face_front = 0,
    eGL_face_back,
    eGL_face_frontback,

    eGL_face_enummax
};
enum eGL_polymode : uint8_t{
    eGL_polymode_point = 0,
    eGL_polymode_line,
    eGL_polymode_fill,

    eGL_polymode_enummax
};
enum eGL_frontface : uint8_t{
    eGL_frontface_clockwise = 0,
    eGL_frontface_counterclockwise,

    eGL_frontface_enummax
};
enum eGL_drawmode : uint8_t{
    eGL_drawmode_points = 0,

    eGL_drawmode_lines,
    eGL_drawmode_linestrip,

    eGL_drawmode_triangles,
    eGL_drawmode_trianglestrip,
    eGL_drawmode_trianglefan,

    eGL_drawmode_quads,

    eGL_drawmode_enummax
};
enum eGL_fboattachment : uint8_t{
    eGL_fboattachment_color0 = 0,
    eGL_fboattachment_color1,
    eGL_fboattachment_color2,
    eGL_fboattachment_color3,
    eGL_fboattachment_depth,

    eGL_fboattachment_enummax
};
enum eGL_pixelformat : uint8_t { //format of the incoming pixel data
    eGL_pixelformat_r = 0,
    eGL_pixelformat_rg,
    eGL_pixelformat_rgb,
    eGL_pixelformat_bgr,
    eGL_pixelformat_rgba,
    eGL_pixelformat_bgra,
    eGL_pixelformat_depth,
    eGL_pixelformat_depthstencil,

    eGL_pixelformat_enummax
};
enum eGL_texfilter : uint8_t{
    //min&max
    eGL_texfilter_nearest = 0,
    eGL_texfilter_linear,

    //minifying only
    eGL_texfilter_nearest_mipmap_nearest,
    eGL_texfilter_linear_mipmap_nearest,
    eGL_texfilter_nearest_mipmap_linear,
    eGL_texfilter_linear_mipmap_linear,

    eGL_texfilter_enummax
};
enum eGL_texwrap : uint8_t{
	eGL_texwrap_clamptoedge = 0,
	eGL_texwrap_clamptoborder,
    eGL_texwrap_mirrorrepeat,
    eGL_texwrap_repeat,

    eGL_texwrap_enummax
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class GLFramebuffer : public GLObject
{
    friend GLTexture;
    friend GLRenderbuffer;
public:
    GLFramebuffer();
    ~GLFramebuffer();

    void BlitToFBO(GLFramebuffer* dst, uint32_t width, uint32_t height,
            bool color = true, bool depth = true);

    void AttachTexture(eGL_fboattachment attach, GLTexture* tex = nullptr, eGL_fbotarget fbotarget = eGL_fbotarget_drawread, int depth = 0 /*for cubemaps too*/);
    void AttachRenderBuffer(eGL_fboattachment attach, GLRenderbuffer* rbo = nullptr, eGL_fbotarget fbotarget = eGL_fbotarget_drawread);


protected:
    void WarnDeletedTex(GLTexture* tex);
    void WarnDeletedRenderbuffer(GLRenderbuffer* rbo);

private:
    GLTexture* m_pattachedtextures[eGL_fboattachment_enummax];
    GLRenderbuffer* m_pattachedRBOs[eGL_fboattachment_enummax];
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class GLRenderbuffer : public GLObject
{
    friend GLFramebuffer;
public:
    GLRenderbuffer(eGL_texformat format, uint32_t width, uint32_t height);
    ~GLRenderbuffer();

    void GetWidthHeight(uint32_t& width, uint32_t& height) {
		width = m_width; height = m_height;
    }

protected:
    void SetAttachedTo(GLFramebuffer* fbo);

    GLFramebuffer* m_pAttachedTo;
	uint32_t m_width, m_height;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class GLTexture : public GLObject
{
    friend GLFramebuffer;
public:
    ~GLTexture();

    GLuint GetTargetType() const;
	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }

protected:
    GLTexture(GLuint target, uint32_t width, uint32_t height);

    void SetAttachedTo(GLFramebuffer* fbo);

    GLuint m_Target;
    GLFramebuffer* m_pAttachedTo;

    uint32_t m_width, m_height;
};

struct gltexcommon_createinfo_t{
    void* pixeldata;
    uint32_t numbytes;
    uint32_t bytesperpixel;
    eGL_texformat texformat;
    eGL_pixelformat pixelformat;
    eGL_type pixeltype; // data type of every channel of the incoming pixel data
    eGL_texfilter minfilter;
    eGL_texfilter maxfilter;
	eGL_texwrap wrapmode;
    float bordercolor[4];
};

////////////////////////////////////////////

struct gltex1d_createinfo_t{
    uint32_t width;
    gltexcommon_createinfo_t common;
};
class GLTexture1D : public GLTexture
{
public: GLTexture1D(const gltex1d_createinfo_t* info);
};

////////////////////////////////////////////

struct gltex2d_createinfo_t{
    uint32_t width;
    uint32_t height;
    gltexcommon_createinfo_t common;
};
class GLTexture2D : public GLTexture
{
public: GLTexture2D(const gltex2d_createinfo_t* info);
};

////////////////////////////////////////////

struct gltex3d_createinfo_t{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    gltexcommon_createinfo_t common;
};
class GLTexture3D : public GLTexture
{
public: GLTexture3D(const gltex3d_createinfo_t* info);
};

////////////////////////////////////////////

struct gltexcubemap_createinfo_t
{
	uint32_t width;
	uint32_t height;
	gltexcommon_createinfo_t common;
};
class GLTextureCubeMap : public GLTexture
{
public:
	GLTextureCubeMap(const gltexcubemap_createinfo_t* info);
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct gl_vaoattrib_t
{
    uint32_t attribindex;
    uint32_t memberoffset; //offsetof(vertstruct, member)
    uint32_t structsize; //sizeof(vertstruct)
    uint16_t numelements; //ex: numelements = 4 for vec4, numelements = 2 for vec2
    bool normalized;
    eGL_type attribtype;
};
class GLVertexArray : public GLObject
{
public:
    GLVertexArray(
        GLArrayBuffer* vertbuffer, 
        const std::vector<gl_vaoattrib_t>& attribs,
        GLElementArrayBuffer* indexbuffer = nullptr
    );
    ~GLVertexArray();

    void BindElementArrayBuffer(GLElementArrayBuffer* buffer = nullptr);
    GLElementArrayBuffer* GetBoundElementArrayBuffer();

private:
    GLElementArrayBuffer* m_pElementArrayBuffer;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class GLBuffer : public GLObject
{
public:
    ~GLBuffer();

    void MemCpy(uint32_t numbytes, uint8_t* data, uint32_t offset = 0);

    GLuint GetTarget();
    uint32_t GetSize();

protected:

    GLBuffer(GLuint target, uint32_t numbytes, uint8_t* data = nullptr);

    GLuint m_eTarget;
    uint32_t m_uiSize;
};

//////////////////////////////////////////////////////////////////////

class GLArrayBuffer : public GLBuffer
{
public:
    GLArrayBuffer(uint32_t numbytes, uint8_t* data = nullptr);
};

//////////////////////////////////////////////////////////////////////

class GLElementArrayBuffer : public GLBuffer
{
public:
    GLElementArrayBuffer(uint32_t numindices, uint32_t* data = nullptr);
    GLElementArrayBuffer(uint32_t numindices, uint16_t* data = nullptr);
    GLElementArrayBuffer(uint32_t numindices, uint8_t* data = nullptr);

    eGL_type GetType() { return m_eType; };

private:
    eGL_type m_eType;
};

//////////////////////////////////////////////////////////////////////

class GLUniformBuffer : public GLBuffer
{
public:
    GLUniformBuffer(uint32_t numbytes, uint8_t* data = nullptr);
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


struct glshader_clientvariable
{
    std::string name; //name of the variable (attribute, uniform, ubo, etc)
    uint32_t loc; //bind point of the ubo / location of the variable, which the client will use
};

struct glshader_servervariable
{
    std::string name; //name of the variable (attribute, uniform, ubo, etc)
    uint32_t index; //location of the variable
    uint32_t bindpoint; //bind point of the variable. this is only used by ubos
};

struct glshader_createinfo_t
{
    const char* vertcode;
    const char* fragcode;
    std::vector<glshader_clientvariable> defineattribs; // formally define the locations of any existing attributes
    std::vector<glshader_clientvariable> defineubos; // formally define the binding point of any existing ubos

    //glsl versions that we'll attempt to compile under, in order. ex: supported_versions = {120, 430, 140}
    //a macro will be created with the version that succeeds, ex: #define GLSL_120, #define GLSL_430, GLSL_140
    //this field is optional.
    std::vector<uint16_t> supported_versions;

    std::string vertdefines; //optional. definitions that are included before vertcode
    std::string fragdefines; //optional. definitions that are included before fragcode
};
class GLShader : public GLObject
{
public:
    GLShader(const glshader_createinfo_t& info);
    ~GLShader();

    uint32_t GetUniformLoc(const char* name);

    void SetUniformFloat(uint32_t index, const float val);
    void SetUniformVec2(uint32_t index, const float* val);
    void SetUniformVec3(uint32_t index, const float* val);
    void SetUniformVec4(uint32_t index, const float* val);

    void SetUniformInt(uint32_t index, const int val);
    void SetUniformIVec2(uint32_t index, const int* val);
    void SetUniformIVec3(uint32_t index, const int* val);
    void SetUniformIVec4(uint32_t index, const int* val);

    void SetUniformMatrix3x3(uint32_t index, const float* val, bool transpose = false);
    void SetUniformMatrix3x4(uint32_t index, const float* val, bool transpose = false);

    void SetUniformMatrix4x4(uint32_t index, const float* val, bool transpose = false);

    void SetUniformSampler2D(uint32_t index, int texindex);


private:
    std::vector<glshader_servervariable> m_vUniforms;
    std::vector<glshader_servervariable> m_vUniformBlocks;
    std::vector<glshader_servervariable> m_vAttribs;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

namespace GLContext
{
    struct glcontext_initinfo_t
    {
        GLADloadfunc fn; // used to query the opengl functions. ex: SDL_GL_GetProcAddress
        void (*gl_loggerFn)(const char*); // optional. used to output any warnings/errors
        bool debuglayer; //if the opengl debug layer should be enabled. 
    };
    void Init_GLContext(const glcontext_initinfo_t& info);
    void Destroy_GLContext();

    void ResetStates();

    void SetMainFBOHandle(GLuint handle);

    //objects
    void BindShader(GLShader* shader);
    void BindTexture(GLTexture* tex, uint32_t slot = 0);
    void BindFramebuffer(GLFramebuffer* fbo, eGL_fbotarget target = eGL_fbotarget_drawread);
    void BindVertexArray(GLVertexArray* vao);
    void BindArrayBuffer(GLArrayBuffer* buffer);
	void BindUniformBuffer(GLUniformBuffer* buffer, int program_ubobindpoint = -1, uint32_t size = 0, uint32_t offset = 0);

    void BindTextureLegacy(GLuint texhandle, GLenum targettype, uint32_t slot = 0);

    GLShader* GetBoundShader();
    GLFramebuffer* GetBoundDrawFBO();
    GLFramebuffer* GetBoundReadFBO();
    GLVertexArray* GetBoundVAO();
    GLArrayBuffer* GetBoundArrayBuffer();
    GLUniformBuffer* GetBoundUniformBuffer();

    GLFramebuffer* GetMainFramebuffer();

    //states
    //sets
    void SetPolygonRasterMode(eGL_polymode mode);

    void SetDepthCompare(eGL_comparefunc func);
    void SetDepthTesting(bool enable);
    void SetDepthWriting(bool enable);

    void SetPolygonOffsetFill(bool enable);

    void SetSrcBlendFunc_rgb(eGL_blendfactor factor);
    void SetDstBlendFunc_rgb(eGL_blendfactor factor);
    void SetSrcBlendFunc_alpha(eGL_blendfactor factor);
    void SetDstBlendFunc_alpha(eGL_blendfactor factor);
    void SetBlendFunc_rgb(eGL_blendfactor src, eGL_blendfactor dst);
	void SetBlendFunc_alpha(eGL_blendfactor src, eGL_blendfactor dst);
	void SetBlendFunc_rgba(eGL_blendfactor src, eGL_blendfactor dst);
    void SetBlendEquation(eGL_blendequation eq); //ex: src*srcfactor + dst*dstfactor; src*srcfactor - dst*dstfactor
    void SetBlending(bool enable);

    void SetFrontFaceWise(eGL_frontface wise);

    void SetFaceToCull(eGL_face face);
    void SetFaceCulling(bool enable);

    void SetViewportPos(uint16_t x, uint16_t y);
    void SetViewportSize(uint16_t width, uint16_t height);

    void SetDepthBufferClearValue(float val);
    void SetColorBufferClearValue(float r, float g, float b, float a);

    void SetAlphaTest(bool enable);

    void SetDepthClamp(bool enable);

    //gets
    eGL_polymode GetPolygonRasterMode();

    eGL_comparefunc GetDepthCompare();
    bool GetDepthTesting();
    bool GetDepthWriting();

    eGL_blendfactor GetSrcBlendFunc_rgb();
    eGL_blendfactor GetDstBlendFunc_rgb();
    eGL_blendfactor GetSrcBlendFunc_alpha();
    eGL_blendfactor GetDstBlendFunc_alpha();
    eGL_blendequation GetBlendEquation();
    bool GetBlending();

    void GetFrontFaceWise(eGL_frontface wise);

    void GetFaceToCull(eGL_face face);
    bool GetFaceCulling(bool enable);

    void GetViewportPos(uint16_t x, uint16_t y);
    void GetViewportSize(uint16_t width, uint16_t height);

    float GetDepthBufferClearValue();
    void GetColorBufferClearValue(float* buffer);

    uint32_t GetDriverUBOAlignment();



    //commands

    void ClearDepthBuffer();
    void ClearColorBuffer();

    struct glpolydrawcmd_t{
        uint32_t numpoints;
        uint32_t pointsoffset;
    };
    void DrawPolys(eGL_drawmode mode, uint32_t numpoints, uint32_t pointsoffset = 0, bool indexed = false);
    void MultiDrawPolys(eGL_drawmode mode, const std::vector<glpolydrawcmd_t>& requests, bool indexed = false);
};



//////////////////////////custom//////////////////////////////////////

const std::string glsl_engine_defines_vertex = R"(
	#define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h

	#define M_PI2 6.28318530718

	#extension GL_ARB_shading_language_420pack : enable // supported by +/- 76.86% of gpus
	#extension GL_ARB_uniform_buffer_object : enable // supported by +/- 78.22% of gpus
	#extension GL_ARB_enhanced_layouts : enable // supported by +/- 67.05% of gpus (yikes)


	in vec3 aPosition;
	in vec3 aNormal;
	in vec2 aTexCoord;
	in vec2 aTexCoordLM;
	in vec2 aTexCoordDetail;
	in vec4 aColor;
	in int aBoneID;

	//math utilities

	float sqr(float x){ return x * x; }

	#if defined(GLSL_130)	//add used functions that glsl 130 doesnt have	

	float det(mat2 matrix) {
		return matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x;
	}

	mat3 inverse(mat3 matrix) {
	    vec3 row0 = matrix[0];
	    vec3 row1 = matrix[1];
	    vec3 row2 = matrix[2];
	
	    vec3 minors0 = vec3(
	        det(mat2(row1.y, row1.z, row2.y, row2.z)),
	        det(mat2(row1.z, row1.x, row2.z, row2.x)),
	        det(mat2(row1.x, row1.y, row2.x, row2.y))
	    );
	    vec3 minors1 = vec3(
	        det(mat2(row2.y, row2.z, row0.y, row0.z)),
	        det(mat2(row2.z, row2.x, row0.z, row0.x)),
	        det(mat2(row2.x, row2.y, row0.x, row0.y))
	    );
	    vec3 minors2 = vec3(
	        det(mat2(row0.y, row0.z, row1.y, row1.z)),
	        det(mat2(row0.z, row0.x, row1.z, row1.x)),
	        det(mat2(row0.x, row0.y, row1.x, row1.y))
	    );
	
	    mat3 adj = transpose(mat3(minors0, minors1, minors2));
	
	    return (1.0 / dot(row0, minors0)) * adj;
	}
	#endif
)";

const std::string glsl_engine_defines_fragment = R"(
	#extension GL_ARB_shading_language_420pack : enable // supported by +/- 76.86% of gpus
	#extension GL_ARB_uniform_buffer_object : enable // supported by +/- 78.22% of gpus
	#extension GL_ARB_enhanced_layouts : enable // supported by +/- 67.05% of gpus (yikes)

	//math utilities

	float sqr(float x){ return x * x; }

	#if defined(GLSL_130)	//add used functions that glsl 130 doesnt have	

	float det(mat2 matrix) {
		return matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x;
	}
	mat3 inverse(mat3 matrix) {
	    vec3 row0 = matrix[0];
	    vec3 row1 = matrix[1];
	    vec3 row2 = matrix[2];
	
	    vec3 minors0 = vec3(
	        det(mat2(row1.y, row1.z, row2.y, row2.z)),
	        det(mat2(row1.z, row1.x, row2.z, row2.x)),
	        det(mat2(row1.x, row1.y, row2.x, row2.y))
	    );
	    vec3 minors1 = vec3(
	        det(mat2(row2.y, row2.z, row0.y, row0.z)),
	        det(mat2(row2.z, row2.x, row0.z, row0.x)),
	        det(mat2(row2.x, row2.y, row0.x, row0.y))
	    );
	    vec3 minors2 = vec3(
	        det(mat2(row0.y, row0.z, row1.y, row1.z)),
	        det(mat2(row0.z, row0.x, row1.z, row1.x)),
	        det(mat2(row0.x, row0.y, row1.x, row1.y))
	    );
	
	    mat3 adj = transpose(mat3(minors0, minors1, minors2));
	
	    return (1.0 / dot(row0, minors0)) * adj;
	}
	#endif
)";

#define VERTPOS_LOC 0
#define NORMAL_LOC 1
#define TEXCOORD_LOC 2
#define LM_TEXCOORD_LOC 3
#define DETAIL_TEXCOORD_LOC 4
#define COLOR_LOC 5
#define STUDIOMDL_BONEID_LOC 6

const std::vector<glshader_clientvariable> s_CommonAttribs = {
	{"aPosition", VERTPOS_LOC},
	{"aNormal", NORMAL_LOC},
	{"aTexCoord", TEXCOORD_LOC},
	{"aTexCoordLM", LM_TEXCOORD_LOC},
	{"aTexCoordDetail", DETAIL_TEXCOORD_LOC},
	{"aColor", COLOR_LOC},
	{"aBoneID", STUDIOMDL_BONEID_LOC}
};



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////