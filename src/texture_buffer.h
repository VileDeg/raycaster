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

typedef struct {
    unsigned int gl_tex_id;
    bool gl_tex_init;

    int width;
    int height;

    Pixel* data;
} TextureBuffer;

void textureBuffer_init(TextureBuffer* tb, int width, int height) {
    tb->data = NULL;
    tb->gl_tex_init = false;
    tb->width = width;
    tb->height = height;
}

void textureBuffer_reset(TextureBuffer* tb, int width, int height) {
    tb->width = width;
    tb->height = height;

    free(tb->data);
    tb->data = malloc(tb->width * tb->height * sizeof(Pixel));
    if (tb->data == NULL) {
        perror("Fatal error: malloc failed");
        exit(1);
    }

    assert(sizeof(Pixel) == 3);

    Pixel magenta = {255, 0, 255};

    for (int h = 0; h < tb->height; ++h) {
        for (int w = 0; w < tb->width; ++w) {
            tb->data[w + h * tb->width] = magenta;
        }
    }
}

void textureBuffer_loadTexData(TextureBuffer* tb) {
    if (!tb->gl_tex_init) {
        printf("Warning: Texture not yet initialized!\n");
        return;
    }

    glBindTexture(GL_TEXTURE_2D, tb->gl_tex_id);

    // render tex data on texture 
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tb->width, tb->height, 0, GL_RGB, GL_UNSIGNED_BYTE, tb->data);
    // glGenerateMipmap(GL_TEXTURE_2D);

    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tb->width, tb->height, GL_RGB, GL_UNSIGNED_BYTE, tb->data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tb->width, tb->height, 0, GL_RGB, GL_UNSIGNED_BYTE, tb->data);
}

void textureBuffer_setPixel(TextureBuffer* tb, int x, int y, Pixel p) {
    if (x < 0 || x >= tb->width ||
        y < 0 || y >= tb->height) {
        return ;
    }

    assert(x > -1 && x < tb->width);
    assert(y > -1 && y < tb->height);

    tb->data[x + y * tb->width] = p;
}

#endif // _TEXTURE_BUFFER_H_