#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <FreeImage/FreeImage.h>

inline void load_png_to_texture(const char* fname,
    GLuint tex_name,
    GLenum filter = GL_LINEAR) {
    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(fname, 0);
    FIBITMAP* bmp = FreeImage_Load(fmt, fname);
    if (!bmp) { fprintf(stderr, "[TEXTURE] %s 열기 실패\n", fname); exit(EXIT_FAILURE); }

    FIBITMAP* bmp32 = FreeImage_ConvertTo32Bits(bmp);
    const int w = FreeImage_GetWidth(bmp32);
    const int h = FreeImage_GetHeight(bmp32);
    void* pixels = FreeImage_GetBits(bmp32);

    glBindTexture(GL_TEXTURE_2D, tex_name);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    FreeImage_Unload(bmp32);
    if (bmp32 != bmp) FreeImage_Unload(bmp);
}
