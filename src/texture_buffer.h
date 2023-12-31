#ifndef _TEXTURE_BUFFER_H_
#define _TEXTURE_BUFFER_H_

#include "stdbool.h"
#include "stdlib.h"
#include "assert.h"

#include "glad/glad.h"

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pixel;

Pixel pixel_mulf(Pixel p, float f)
{
    p.r *= f;
    p.g *= f;
    p.b *= f;
    return p;
}

typedef struct {
    unsigned int gl_tex_id;
    unsigned int gl_tex_unit;

    bool gl_tex_init;

    bool updated_this_frame;
    
    int width;
    int height;

    Pixel* data;
} TextureBuffer;

void textureBuffer_clear(TextureBuffer* tb, Pixel col);

void textureBuffer_init(TextureBuffer* tb, int width, int height) {
    memset(tb, 0, sizeof(TextureBuffer));
    tb->data = NULL;
    tb->gl_tex_id = 0;
    tb->gl_tex_unit = 0;
    tb->gl_tex_init = false;
    tb->updated_this_frame = false;
    tb->width = width;
    tb->height = height;

    assert(sizeof(Pixel) == 3);

    void* tmp = malloc(tb->width * tb->height * sizeof(Pixel));
    if (tmp == NULL) {
        perror("Fatal error: malloc failed");
        exit(1);
    }
    tb->data = tmp;

    assert(sizeof(Pixel) == 3);

    Pixel magenta = { 255, 0, 255 };

    textureBuffer_clear(tb, magenta);
}

void textureBuffer_clear(TextureBuffer* tb, Pixel col) {
    for (int h = 0; h < tb->height; ++h) {
        for (int w = 0; w < tb->width; ++w) {
            tb->data[w + h * tb->width] = col;
        }
    }
}


void textureBuffer_reset(TextureBuffer* tb, int width, int height) {
    tb->width = width;
    tb->height = height;

    void* tmp = realloc(tb->data, tb->width * tb->height * sizeof(Pixel));
    if (tmp == NULL) {
        perror("Fatal error: realloc failed");
        exit(1);
    }
    tb->data = tmp;

    assert(sizeof(Pixel) == 3);

    Pixel magenta = {255, 0, 255};

    textureBuffer_clear(tb, magenta);

    //https://stackoverflow.com/questions/8866904/differences-and-relationship-between-glactivetexture-and-glbindtexture
    glActiveTexture(GL_TEXTURE0 + tb->gl_tex_unit);
    // Update the texture size on GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tb->width, tb->height, 0, GL_RGB, GL_UNSIGNED_BYTE, tb->data);
}

void textureBuffer_loadTexData(TextureBuffer* tb) {
    assert(tb->gl_tex_init);

    glActiveTexture(GL_TEXTURE0 + tb->gl_tex_unit);
    glBindTexture(GL_TEXTURE_2D, tb->gl_tex_id);

    // Load texture to GPU. Fails if size is different so we game sure glTexImage2D is called before.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tb->width, tb->height, GL_RGB, GL_UNSIGNED_BYTE, tb->data);
}

void textureBuffer_setPixel(TextureBuffer* tb, int x, int y, Pixel p) {
    if (x < 0 || x >= tb->width ||
        y < 0 || y >= tb->height) {
        return;
    }

    assert(x > -1 && x < tb->width);
    assert(y > -1 && y < tb->height);

    tb->data[x + y * tb->width] = p;
    tb->updated_this_frame = true;
}

#endif // _TEXTURE_BUFFER_H_
