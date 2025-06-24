#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <FreeImage/FreeImage.h>

inline void load_png_to_texture(const char* fname,
    GLuint tex_name,
    GLenum filter = GL_LINEAR)
{
    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(fname, 0);
    FIBITMAP* bmp = FreeImage_Load(fmt, fname);
    if (!bmp) { fprintf(stderr, "[TEXTURE] cannot load %s\n", fname); exit(EXIT_FAILURE); }

    FIBITMAP* bmp32 = FreeImage_ConvertTo32Bits(bmp);
    int w = FreeImage_GetWidth(bmp32);
    int h = FreeImage_GetHeight(bmp32);
    void* pixels = FreeImage_GetBits(bmp32);

    glBindTexture(GL_TEXTURE_2D, tex_name);

    /* ¹Ó¸Ê OFF ¡æ ·¹º§ 0 ÇÏ³ª¸¸ »ç¿ë */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, pixels);

    FreeImage_Unload(bmp32);
    if (bmp32 != bmp) FreeImage_Unload(bmp);

    glBindTexture(GL_TEXTURE_2D, 0);
}
