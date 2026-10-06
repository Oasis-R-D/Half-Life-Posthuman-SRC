#pragma once
#include "stdint.h"
#include <string>
#include <vector>
#include <assert.h>
#include "glad.h"

class GLObject
{
public:
    GLuint GetHandle() { return m_hGLHandle; }

protected:
    GLuint m_hGLHandle;
};

enum eGL_type : uint8_t
{
    eGL_type_uint8 = 0,
    eGL_type_int8,
    eGL_type_uint16,
    eGL_type_int16,
    eGL_type_uint32,
    eGL_type_int32,
    eGL_type_float,

    eGL_type_enummax
};
enum eGL_texformat : uint8_t
{
    eGL_texformat_depth = 0,
    eGL_texformat_depth16,
    eGL_texformat_depth24,
    eGL_texformat_depth32,
    eGL_texformat_depth32f,
    eGL_texformat_depthstencil,


    eGL_texformat_r,
    eGL_texformat_r8,
    eGL_texformat_r16,

    eGL_texformat_r8i,
    eGL_texformat_r16i,
    eGL_texformat_r32i,

    eGL_texformat_r8ui,
    eGL_texformat_r16ui,
    eGL_texformat_r32ui,

    eGL_texformat_r16f,
    eGL_texformat_r32f,


    eGL_texformat_rg,
    eGL_texformat_rg8,
    eGL_texformat_rg16,

    eGL_texformat_rg8i,
    eGL_texformat_rg16i,
    eGL_texformat_rg32i,

    eGL_texformat_rg8ui,
    eGL_texformat_rg16ui,
    eGL_texformat_rg32ui,

    eGL_texformat_rg16f,
    eGL_texformat_rg32f,


    eGL_texformat_rgb,
    eGL_texformat_rgb8,
    eGL_texformat_rgb16,

    eGL_texformat_rgb8i,
    eGL_texformat_rgb16i,
    eGL_texformat_rgb32i,

    eGL_texformat_rgb8ui,
    eGL_texformat_rgb16ui,
    eGL_texformat_rgb32ui,

    eGL_texformat_rgb16f,
    eGL_texformat_rgb32f,

    eGL_texformat_rgba,
    eGL_texformat_rgba8,
    eGL_texformat_rgba16,

    eGL_texformat_rgba8i,
    eGL_texformat_rgba16i,
    eGL_texformat_rgba32i,

    eGL_texformat_rgba8ui,
    eGL_texformat_rgba16ui,
    eGL_texformat_rgba32ui,

    eGL_texformat_rgba16f,
    eGL_texformat_rgba32f,

    eGL_texformat_compressedRGB_DXT1,

    eGL_texformat_compressedRGBA_DXT1,
    eGL_texformat_compressedRGBA_DXT3,
    eGL_texformat_compressedRGBA_DXT5,

    eGL_texformat_enummax
};
extern const GLuint gltype_map[];
extern const GLuint gltexformat_map[];