#include "glWrapper.h"
#include "glcommon_internal.h"

#define NUM_MIPMAPLEVELS 1

static constexpr GLuint glpixelformat_map[] = {
    GL_RED,
    GL_RG,
    GL_RGB,
    GL_BGR,
    GL_RGBA,
    GL_BGRA,
    GL_DEPTH_COMPONENT,
    GL_DEPTH_STENCIL
};
static_assert(sizeof(glpixelformat_map) == (eGL_pixelformat_enummax * 4), "");

static constexpr GLuint gltexwrap_map[] = {
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,
	GL_MIRRORED_REPEAT,
	GL_REPEAT
};
static_assert(sizeof(gltexwrap_map) == (eGL_texwrap_enummax * 4), "");

static constexpr GLuint gltexfilter_map[] = {
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_LINEAR,
};
static_assert(sizeof(gltexfilter_map) == (eGL_texfilter_enummax * 4), "");

static constexpr bool isCompressed(eGL_texformat format){
    return format >= eGL_texformat_compressedRGB_DXT1;
}

//////////////////////////////
//////////////////////////////
static GLuint legacytexturecreate(GLuint target) {
    GLuint handle; glGenTextures(1, &handle); return handle;
}
static void legacytexture1dcreate(GLTexture1D* tex, const gltex1d_createinfo_t* info) {
    GLContext::BindTexture(tex);
    if (isCompressed(info->common.texformat)){
        glCompressedTexImage1D(GL_TEXTURE_1D,
            NUM_MIPMAPLEVELS,
            gltexformat_map[info->common.texformat],
            info->width,
            0,
            info->common.numbytes,
            info->common.pixeldata);
    }
    else{
        glTexImage1D(GL_TEXTURE_1D,
            NUM_MIPMAPLEVELS,
            gltexformat_map[info->common.texformat],
            info->width,
            0,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	glTexParameterfv(GL_TEXTURE_1D, GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);
}
static void legacytexture2dcreate(GLTexture2D* tex, const gltex2d_createinfo_t* info) {
    GLContext::BindTexture(tex);
    if (isCompressed(info->common.texformat)) {
        glCompressedTexImage2D(GL_TEXTURE_2D,
            NUM_MIPMAPLEVELS,
            gltexformat_map[info->common.texformat],
            info->width, info->height,
            0,
            info->common.numbytes,
            info->common.pixeldata);
    }
    else {
        glTexImage2D(GL_TEXTURE_2D,
            NUM_MIPMAPLEVELS,
            gltexformat_map[info->common.texformat],
            info->width, info->height,
            0,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);
}
static void legacytexture3dcreate(GLTexture3D* tex, const gltex3d_createinfo_t* info) {
    GLContext::BindTexture(tex);
    if (isCompressed(info->common.texformat)) {
        glCompressedTexImage3D(GL_TEXTURE_3D,
            NUM_MIPMAPLEVELS,
            gltexformat_map[info->common.texformat],
            info->width, info->height, info->depth,
            0,
            info->common.numbytes,
            info->common.pixeldata);
    }
    else {
        glTexImage3D(GL_TEXTURE_3D,
            NUM_MIPMAPLEVELS,
            gltexformat_map[info->common.texformat],
            info->width, info->height, info->depth,
            0,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);
}
static void legacytexturecubemapcreate(GLTextureCubeMap* tex, const gltexcubemap_createinfo_t* info)
{
	GLContext::BindTexture(tex);
	_GLASSERT(!isCompressed(info->common.texformat), "cubemap with compressed textures not supported.");
	for (GLuint i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			NUM_MIPMAPLEVELS,
			gltexformat_map[info->common.texformat],
			info->width, info->height,
			0,
			glpixelformat_map[info->common.pixelformat],
			gltype_map[info->common.pixeltype],
			info->common.pixeldata);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
		glTexParameterfv(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);
	}
}
//////////////////////////////
//////////////////////////////
static GLuint newtexturecreate(GLuint target) {
    GLuint handle; glCreateTextures(target, 1, &handle); return handle;
}
static void newtexture1dcreate(GLTexture1D* tex, const gltex1d_createinfo_t* info) {
    glTextureStorage1D(tex->GetHandle(), NUM_MIPMAPLEVELS, gltexformat_map[info->common.texformat],
        info->width);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	glTextureParameterfv(tex->GetHandle(), GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);

    if (!info->common.pixeldata)
        return;

    if (isCompressed(info->common.texformat)){
        glCompressedTextureSubImage1D(tex->GetHandle(), 0,
            0, info->width,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
    else{
        glTextureSubImage1D(tex->GetHandle(), 0,
            0, info->width,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
}
static void newtexture2dcreate(GLTexture2D* tex, const gltex2d_createinfo_t* info) {
    glTextureStorage2D(tex->GetHandle(), NUM_MIPMAPLEVELS, gltexformat_map[info->common.texformat],
        info->width, info->height);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	glTextureParameterfv(tex->GetHandle(), GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);

    if (!info->common.pixeldata)
        return;

    if (isCompressed(info->common.texformat)) {
        glCompressedTextureSubImage2D(tex->GetHandle(), 0,
            0, 0, info->width, info->height,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
    else {
        glTextureSubImage2D(tex->GetHandle(), 0,
            0, 0, info->width, info->height,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
}
static void newtexture3dcreate(GLTexture3D* tex, const gltex3d_createinfo_t* info) {
    glTextureStorage2D(tex->GetHandle(), NUM_MIPMAPLEVELS, gltexformat_map[info->common.texformat],
        info->width, info->height);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	glTextureParameterfv(tex->GetHandle(), GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);

    if (!info->common.pixeldata)
        return;

    if (isCompressed(info->common.texformat)) {
        glCompressedTextureSubImage3D(tex->GetHandle(), 0,
            0, 0, 0, info->width, info->height, info->depth,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
    else {
        glTextureSubImage3D(tex->GetHandle(), 0,
            0, 0, 0, info->width, info->height, info->depth,
            glpixelformat_map[info->common.pixelformat],
            gltype_map[info->common.pixeltype],
            info->common.pixeldata);
    }
}
static void newtexturecubemapcreate(GLTextureCubeMap* tex, const gltexcubemap_createinfo_t* info)
{
	glTextureStorage2D(tex->GetHandle(), NUM_MIPMAPLEVELS, gltexformat_map[info->common.texformat],
		info->width, info->height);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MIN_FILTER, gltexfilter_map[info->common.minfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_MAG_FILTER, gltexfilter_map[info->common.maxfilter]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_S, gltexwrap_map[info->common.wrapmode]);
	glTextureParameteri(tex->GetHandle(), GL_TEXTURE_WRAP_T, gltexwrap_map[info->common.wrapmode]);
	glTextureParameterfv(tex->GetHandle(), GL_TEXTURE_BORDER_COLOR, info->common.bordercolor);

	if (!info->common.pixeldata)
		return;

    _GLASSERT(!isCompressed(info->common.texformat), "cubemap with compressed textures not supported.");

	for (GLuint i = 0; i < 6; ++i)
	{
		glTextureSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			NUM_MIPMAPLEVELS,
			gltexformat_map[info->common.texformat],
			info->width, info->height,
			0,
			glpixelformat_map[info->common.pixelformat],
			gltype_map[info->common.pixeltype],
			info->common.pixeldata);
	}
}
//////////////////////////////
//////////////////////////////
static GLuint(*gl_texturecreatefunc)(GLuint target) = legacytexturecreate;
static void (*gl_texture1dcreatefunc)(GLTexture1D* tex, const gltex1d_createinfo_t* info) = legacytexture1dcreate;
static void (*gl_texture2dcreatefunc)(GLTexture2D* tex, const gltex2d_createinfo_t* info) = legacytexture2dcreate;
static void (*gl_texture3dcreatefunc)(GLTexture3D* tex, const gltex3d_createinfo_t* info) = legacytexture3dcreate;
static void (*gl_texturecubemapcreatefunc)(GLTextureCubeMap* tex, const gltexcubemap_createinfo_t* info) = legacytexturecubemapcreate;
//////////////////////////////
//////////////////////////////

void GLTexture_initenv()
{
    if (glTextureStorage1D != NULL)
    {
        gl_texturecreatefunc = newtexturecreate;
        gl_texture1dcreatefunc = newtexture1dcreate;
        gl_texture2dcreatefunc = newtexture2dcreate;
        gl_texture3dcreatefunc = newtexture3dcreate;
		gl_texturecubemapcreatefunc = newtexturecubemapcreate;
    }
}

GLTexture::GLTexture(GLuint target, uint32_t width, uint32_t height)
	: m_pAttachedTo(nullptr), m_width(width), m_height(height)
{
    m_Target = target;
    m_hGLHandle = gl_texturecreatefunc(target);
}
GLTexture::~GLTexture()
{
    glDeleteTextures(1, &m_hGLHandle);
    if (m_pAttachedTo)
        m_pAttachedTo->WarnDeletedTex(this);
}

GLuint GLTexture::GetTargetType() const
{
    return m_Target;
}
void GLTexture::SetAttachedTo(GLFramebuffer* fbo)
{
    m_pAttachedTo = fbo;
}

////////////////////////////////////////////

GLTexture1D::GLTexture1D(const gltex1d_createinfo_t* info)
    : GLTexture(GL_TEXTURE_1D, info->width, 0)
{
    gl_texture1dcreatefunc(this, info);
}

////////////////////////////////////////////

GLTexture2D::GLTexture2D(const gltex2d_createinfo_t* info)
    : GLTexture(GL_TEXTURE_2D, info->width, info->height)
{
    gl_texture2dcreatefunc(this, info);
}

////////////////////////////////////////////

GLTexture3D::GLTexture3D(const gltex3d_createinfo_t* info)
	: GLTexture(GL_TEXTURE_3D, info->width, info->height)
{
    gl_texture3dcreatefunc(this, info);
}

////////////////////////////////////////////

GLTextureCubeMap::GLTextureCubeMap(const gltexcubemap_createinfo_t* info)
	: GLTexture(GL_TEXTURE_CUBE_MAP, info->width, info->height)
{
	gl_texturecubemapcreatefunc(this, info);
}

////////////////////////////////////////////