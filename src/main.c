#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "vec2.h"

static const int ROWS = 128;
static const int COLS = 128;

// static const int PIX_W = 4;
// static const int PIX_H = 4;



// const char *vertexShaderSource = "#version 330 core\n"
//     "layout (location = 0) in vec3 aPos;\n"
//     "void main()\n"
//     "{\n"
//     "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
//     "}\0";
// const char *fragmentShaderSource = "#version 330 core\n"
//     "out vec4 FragColor;\n"
//     "void main()\n"
//     "{\n"
//     "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
//     "}\n\0";

static int SCR_WIDTH;
static int SCR_HEIGHT;

typedef struct {
    GLubyte r;
    GLubyte g;
    GLubyte b;
} Pixel;

Pixel* TEX_DATA = NULL;


#define CHECK_IMAGE_WIDTH 64
#define CHECK_IMAGE_HEIGHT 64
static GLubyte checkImage[CHECK_IMAGE_WIDTH][CHECK_IMAGE_HEIGHT][3];

void makeCheckImage(void) {

  int i, j, c;

  for (i = 0; i < CHECK_IMAGE_HEIGHT; i++)

    for (j = 0; j < CHECK_IMAGE_WIDTH; j++) {

      checkImage[i][j][0] = (GLubyte) 255;
      checkImage[i][j][1] = (GLubyte) i == j ? 255 : 0;
      checkImage[i][j][2] = (GLubyte) 255;

    }

}

void glfw_error_callback(int error, const char* description) 
{
    printf("GLFW Error: %s\n", description);
}



void update_tex_data(int width, int height) 
{
    // Window dimensions must be aligned to multiple of 4 
    // because OpenGL doesn't correctly display textures with dimensions unaligned to 4
    int round_to = 4;
    if (width != SCR_WIDTH && width % round_to != 0) {
        if (width > SCR_WIDTH) {
            width += round_to - width % round_to;
        } else {
            width -= width % round_to;
        }
    }

    if (height != SCR_HEIGHT && height % round_to != 0) {
        if (height > SCR_HEIGHT) {
            height += round_to - height % round_to;
        } else {
            height -= height % round_to;
        }
    }

    SCR_WIDTH  = width;
    SCR_HEIGHT = height;

    printf("Screen WIDTH: %d\n", SCR_WIDTH);
    printf("Screen HEIGHT: %d\n", SCR_HEIGHT);
    printf("\n\n");

    assert(sizeof(Pixel) == 3);

    free(TEX_DATA);
    TEX_DATA = malloc(SCR_WIDTH * SCR_HEIGHT * sizeof(Pixel));
    assert(TEX_DATA != NULL);

    Pixel magenta = {255, 0, 255};
    // unsigned char magenta = 0b0;
    // magenta |= 0b11; // blue
    // magenta |= 0b000 << 2; // green
    // magenta |= 0b111 << 5; // red

    for (int h = 0; h < SCR_HEIGHT; ++h) {
        for (int w = 0; w < SCR_WIDTH; ++w) {
            TEX_DATA[w + h * SCR_WIDTH] = magenta;
        }
    }
}



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

    update_tex_data(width, height);
}


void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

char* readShaderSource(const char* filepath) {
    
    FILE* fd = fopen(filepath, "r");
    if (fd == NULL) {
        printf("Failed to open file: %s\n", filepath);
        return NULL;
    }

    // Get the size of the file
    fseek(fd, 0, SEEK_END);
    long filesize = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    // Allocate memory for the buffer
    char* buffer = malloc(filesize + 1);
    if (buffer == NULL)
    {
        printf("Failed to allocate memory for the buffer\n");
        return NULL;
    }

    // Read the file into the buffer
    size_t result = fread(buffer, 1, filesize, fd);
    if (result != filesize)
    {
        printf("Failed to read file\n");
        return NULL;
    }

    // Null-terminate the buffer
    buffer[filesize] = '\0';

    fclose(fd);

    return buffer;
}


int main(void)
{
    update_tex_data(COLS, ROWS);
    //makeCheckImage();

    glfwSetErrorCallback(glfw_error_callback);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Oh sh*t, here we go again...", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

#if 1
    const char* vs = "../shaders/shader.vs";
    const char* fs = "../shaders/shader.fs";

    char* vertex_source = readShaderSource(vs);
    if (vertex_source == NULL) {
        return -1;
    }
    char* fragment_source = readShaderSource(fs);
    if (fragment_source == NULL) {
        return -1;
    }

    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_source, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }

        // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment_source, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    free(vertex_source);
    free(fragment_source);

  
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
         1.f,  1.f, 0.0f,   1.0f, 1.0f, // top right
         1.f, -1.f, 0.0f,   1.0f, 0.0f, // bottom right
        -1.f, -1.f, 0.0f,   0.0f, 0.0f, // bottom left
        -1.f,  1.f, 0.0f,   0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };


    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    int stride = 5 * sizeof(float);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Tex coords
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); 



    // load and create a texture 
    // -------------------------
    unsigned int texture1;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1); 
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    
   
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    glUseProgram(shaderProgram); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
#else
    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#endif
    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

#if 1
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        // render tex data on texture 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, TEX_DATA);
        glGenerateMipmap(GL_TEXTURE_2D);



        // draw our first triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time 

#else
        if (glGet(GL_CURRENT_RASTER_POSITION_VALID) == 0) {
            printf("Error: current raster positions not valid.\n");
            return -1;
        }

        glViewport(0, 0, CHECK_IMAGE_WIDTH, CHECK_IMAGE_HEIGHT);
        glRasterPos2i(0, 0);
        //glDrawPixels(SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, TEX_DATA);
        glDrawPixels(CHECK_IMAGE_WIDTH, CHECK_IMAGE_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, checkImage);
        glFlush();
#endif 

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

#if 0
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
#endif    

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}