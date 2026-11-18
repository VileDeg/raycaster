#ifndef _TEXTURE_BUFFER_H_
#define _TEXTURE_BUFFER_H_

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "glad/glad.h"




typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char _pad; // padding to align to 32-bit
} Pixel;

#pragma warning(push)
#pragma warning(disable: 4244)

Pixel pixel_mulf(Pixel p, float f) {
    f = fmax(fmin(f, 1.f), 0.f);
    p.r *= f;
    p.g *= f;
    p.b *= f;
    return p;
}

Pixel pixel_from_uint(unsigned int v) {
    Pixel p;
    p.r = v >> 16 & 0xFF;
    p.g = v >> 8  & 0xFF;
    p.b = v       & 0xFF;
    return p;
}

unsigned int pixel_to_uint(Pixel p) {
    return (p.r << 16) + (p.g << 8) + p.b;
}

#pragma warning(pop)

typedef struct {
    unsigned int gl_tex_id;

    bool updated_this_frame;

    // TODO: Region that was last updated
    //Vec2i updated_region;
    
    // TODO use Vec2i
    int width;
    int height;

    // TODO: store as uint32_t even with no alpha channel 
    // as a potential optimization for faster memory operations.
    /* 
    * COLUMN-MAJOR BOTTOM-UP to optimize drawing of columns.
    * OpenGL uses ROW-MAJOR so will need to flip U and V texture coordinates axes.
    */
    Pixel* data;
} TextureBuffer;

void textureBuffer_init(TextureBuffer* tb) {
    memset(tb, 0, sizeof(TextureBuffer));
}

/*
void textureBuffer_fill(TextureBuffer* tb, Pixel col) {
    for (int h = 0; h < tb->height; ++h) {
        for (int w = 0; w < tb->width; ++w) {
            tb->data[w + h * tb->width] = col;
        }
    }
    tb->updated_this_frame = true;
}
*/

/**
* width and height must be multiple of 4
*/
void textureBuffer_reset(TextureBuffer* tb, int width, int height) {
    assert(sizeof(Pixel) == 4);

    width -= width % 2;
    height -= height % 2;

    // Round down to even
    tb->width = width;
    tb->height = height;

    if (width < 1 || height < 1) {
        return;
    }

    tb->data = tb->data
        ? realloc(tb->data, tb->width * tb->height * sizeof(Pixel))
        : malloc(tb->width * tb->height * sizeof(Pixel));
    if (tb->data == NULL) {
        perror("Fatal error: realloc failed");
        exit(1);
    }

    // Fill with color black
    memset(tb->data, 0, tb->width * tb->height * sizeof(Pixel));
}

//void textureBuffer_initOpengGL(TextureBuffer* tb) {
//    top_view_tb.gl_tex_unit = 0;
//    glActiveTexture(GL_TEXTURE0);
//
//    glGenTextures(1, &tb->gl_tex_id);
//    glBindTexture(GL_TEXTURE_2D, tb->gl_tex_id);
//
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tb->width, tb->height, 0, GL_RGB, GL_UNSIGNED_BYTE, tb->data);
//}

/**
* Bind texture to unit. Creates lazily.
*/
void textureBuffer_bind(TextureBuffer* tb, int unit) {
    assert(tb->gl_tex_id > 0);

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tb->gl_tex_id);
}

/**
* Leaves texture bound to unit 0.
*/
void textureBuffer_loadTexData(TextureBuffer* tb) {
    glActiveTexture(GL_TEXTURE0);

    if (tb->gl_tex_id < 1) {
        glGenTextures(1, &tb->gl_tex_id);
        glBindTexture(GL_TEXTURE_2D, tb->gl_tex_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tb->width, tb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tb->data);
    } else {
        glBindTexture(GL_TEXTURE_2D, tb->gl_tex_id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tb->width, tb->height, GL_RGBA, GL_UNSIGNED_BYTE, tb->data);
    }
}



/**
* x - left to right
* y - top to bottom
*/
void textureBuffer_setPixel(TextureBuffer* tb, int x, int y, Pixel p) {
    assert(
        x >= 0 && x < tb->width  &&
        y >= 0 && y < tb->height
    );

    // Flip Y axis since the buffer is bottom-up
    tb->data[x * tb->height + tb->height - y] = p;
    tb->updated_this_frame = true;
}

/**
* @param x        - left to right.
* @param y_offset - top to bottom.
* @param height   - length of the desired write area in the selected column.
* @param p        - color to assing.
* 
* Pointer to column data must hold `height` elements.
*/
void textureBuffer_setColumn(TextureBuffer* tb, int x, int y_offset, int height, Pixel* p) {
    assert(
        x        >= 0 && x < tb->width &&
        y_offset >= 0 &&
        height   >= 0 && y_offset + height <= tb->height
    );

    Pixel* row_ptr = 
        tb->data + 
        x * tb->height + // start of column
        tb->height - y_offset; // start of desired area in column (flipped Y offset)

    memcpy(row_ptr, p, height); // TODO: also need to flip pixels in the pointer
    tb->updated_this_frame = true;
}

void textureBuffer_setColumn_color(TextureBuffer* tb, int x, int y_offset, int height, Pixel color) {
    assert(
        x        >= 0 && x < tb->width &&
        y_offset >= 0 &&
        height   >= 0 && y_offset + height <= tb->height
    );

    Pixel* row_ptr = 
        tb->data + 
        x * tb->height + // start of column
        tb->height - y_offset; // start of desired area in column (flipped Y offset)

    int start = tb->height - y_offset - height;
    int end = tb->height - y_offset;
    for(int i = start; i < end; ++i) {
        *(row_ptr + i) = color;
    }
    //memset(row_ptr, color, height);
    tb->updated_this_frame = true;
}

#endif // _TEXTURE_BUFFER_H_
