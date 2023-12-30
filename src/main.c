#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <time.h>


#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "vec2.h"
#include "texture_buffer.h"

static int ROWS = 128;
static int COLS = 128;

#define PIX_W 4
#define PIX_H 4

static int SCR_WIDTH;
static int SCR_HEIGHT;


GLFWwindow* window = NULL;
static TextureBuffer tb;

static Pixel CLEAR_COLOR;

void glfw_error_callback(int error, const char* description) 
{
    printf("GLFW Error: %s\n", description);
}


void screen_reset(int width, int height) 
{
    // At least 4, if a texel should be bigger, then round to more
    static int screen_round_to = max(max(PIX_W*2, PIX_H*2), 4);

    // Window dimensions must be aligned to multiple of 4 
    // because OpenGL doesn't correctly display textures with dimensions unaligned to 4
    if (width != SCR_WIDTH && width % screen_round_to != 0) {
        if (width > SCR_WIDTH) {
            width += screen_round_to - width % screen_round_to;
        } else {
            width -= width % screen_round_to;
        }
    }

    if (height != SCR_HEIGHT && height % screen_round_to != 0) {
        if (height > SCR_HEIGHT) {
            height += screen_round_to - height % screen_round_to;
        } else {
            height -= height % screen_round_to;
        }
    }

    SCR_WIDTH  = width;
    SCR_HEIGHT = height;

    COLS = width / PIX_W;
    ROWS = height / PIX_H;

    printf("Screen WIDTH: %d\n", SCR_WIDTH);
    printf("Screen HEIGHT: %d\n", SCR_HEIGHT);
    printf("\n\n");


    textureBuffer_reset(&tb, COLS, ROWS);
}



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

    screen_reset(width, height);
}


static double MOUSE_X_SCREEN_SPACE = 0;
static double MOUSE_Y_SCREEN_SPACE = 0;

static double MOUSE_X_TEX_SPACE = 0;
static double MOUSE_Y_TEX_SPACE = 0;

static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    printf("Position: (%f:%f)\n", xpos, ypos);
    MOUSE_X_SCREEN_SPACE = xpos;
    MOUSE_Y_SCREEN_SPACE = ypos;

    MOUSE_X_TEX_SPACE = xpos / SCR_WIDTH * COLS;
    MOUSE_Y_TEX_SPACE = ypos / SCR_HEIGHT * ROWS;
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

#define MAP_DIM 5

static int MAP[MAP_DIM * MAP_DIM];

#define pix_x(x) ((x) / MAP_DIM * COLS)
#define pix_y(y) ((y) / MAP_DIM * ROWS)

#define map_x(x) ((x) / (float)COLS * MAP_DIM)
#define map_y(y) ((y) / (float)ROWS * MAP_DIM)

#define TILE_SIZE 1

#define TILE_STEPS 10
static const int NUM_STEPS = TILE_STEPS * MAP_DIM;
static const int STEP = TILE_SIZE / TILE_STEPS;

static const float dist_to_near_plane = 0.2; // equals to half plane width means 45 degree FOV
static const float near_plane_width = 0.4;

static const float stop_dist = 0.1;

static const float mov_speed = 1;

static Vec2 player = {2, 3};

static Vec2 player_look_dir = {0, 1};

void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    printf("OpenGL Debug Message:\n");
    

    printf("[");
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         printf("high"); break;
    case GL_DEBUG_SEVERITY_MEDIUM:       printf("medium"); break;
    case GL_DEBUG_SEVERITY_LOW:          printf("low"); break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: printf("notification"); break;
    } printf("] ");

    printf("[");
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               printf("Error"); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: printf("Deprecated Behaviour"); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  printf("Undefined Behaviour"); break;
    case GL_DEBUG_TYPE_PORTABILITY:         printf("Portability"); break;
    case GL_DEBUG_TYPE_PERFORMANCE:         printf("Performance"); break;
    case GL_DEBUG_TYPE_MARKER:              printf("Marker"); break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          printf("Push Group"); break;
    case GL_DEBUG_TYPE_POP_GROUP:           printf("Pop Group"); break;
    case GL_DEBUG_TYPE_OTHER:               printf("Other"); break;
    } printf("] ");

    printf("[");
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             printf("API"); break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   printf("Window System"); break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: printf("Shader Compiler"); break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     printf("Third Party"); break;
        case GL_DEBUG_SOURCE_APPLICATION:     printf("Application"); break;
        case GL_DEBUG_SOURCE_OTHER:           printf("Other"); break;
    } printf("] ");

    printf("(%u): %s\n", id, message);

    
    printf("\n");
}



void processInput(GLFWwindow* window, double delta_time) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}


//#define Draw(x, y, pix) do {\
//    textureBuffer_setPixel(&tb, x, y, pix); \
//    printf("Set pixel at (%d,%d)\n", x, y); \
//} while(0)

#define Draw(x, y, pix) do {\
    textureBuffer_setPixel(&tb, (x), (ROWS-y), (pix)); \
} while(0)

//void Draw(int x, int y, Pixel pix) {
//    textureBuffer_setPixel(&tb, x, y, pix);
//    printf("Set pixel at (%d,%d)\n", x, y);
//}

int game_init() {
    const char* filename = "../maps/00.txt";
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open file\n");
        return -1;
    }

    int i = 0;

    for(int row = 0; row < MAP_DIM; row++) {
        for(int col = 0; col < MAP_DIM; col++) {
            if(fscanf(file, "%d", &MAP[i]) != 1)
            {
                printf("Failed to read number at row %d, column %d\n", row, col);
                return -1;
            }
            i++;
        }
    }

    fclose(file);

    for(int row = 0; row < MAP_DIM; row++) {
        for(int col = 0; col < MAP_DIM; col++) {
            printf("%d ", MAP[col + row * MAP_DIM]);
        }
        printf("\n");
    }
    return 0;
}

void gameLogic(double delta_time) {

    float delta_x = 0; 
    float delta_y = 0;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        delta_y -= mov_speed * delta_time;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        delta_x -= mov_speed * delta_time;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        delta_y += mov_speed * delta_time;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        delta_x += mov_speed * delta_time;
    }

    player.x += delta_x;
    player.y += delta_y;

    Vec2 vpy = { pix_x(player.x), pix_x(player.y)};

    player_look_dir.x = MOUSE_X_TEX_SPACE - vpy.x;
    player_look_dir.y = MOUSE_Y_TEX_SPACE - vpy.y;

    player_look_dir = vec2_normalized(player_look_dir);

    Pixel pcol    = {255, 0, 0};
    Pixel ldcol   = {255, 255, 0};
    Pixel plcol   = {255, 0, 255};
    Pixel raycol  = {0, 255, 0};
    Pixel tilecol = {255, 0, 0};

    Vec2 plane_dir = vec2_perpendicular(player_look_dir);
    // Draw near clipping plane
    int plane_width = 12;
    int plane_dist = 5;

    int supersampling = 4;
    float i_step = 1 / (float)supersampling;
    int ray_len = 300;
    float ray_step = 0.1;
    float stop_dist = 0.1;
    for (float i = -plane_width/2; i < plane_width/2+1; i += i_step) {
        // Vec2 vpl = vpy + player_look_dir * (plane_dist) + plane_dir * i;

        Vec2 plane_center = vec2_muli(player_look_dir, plane_dist);

        Vec2 plane_dir_advanced = vec2_mulf(plane_dir, i);

        Vec2 plane_next = vec2_add(plane_center, plane_dir_advanced);

        Vec2 vpl = vec2_add(vpy, plane_next);

        Draw(vpl.x, vpl.y, plcol);

        Vec2 rayd = vec2_sub(vpl, vpy);
        bool hit = false;
        for (int j = 0; j < ray_len && !hit; ++j) {

            //Vec2 rayp = vpy + rayd * j * ray_step;
            Vec2 ray_dir_advanced = vec2_mulf(rayd, j*ray_step);
            Vec2 rayp = vec2_add(vpy, ray_dir_advanced);

            Draw(rayp.x, rayp.y, raycol);

            Vec2 tilep = { map_x(rayp.x), map_y(rayp.y) };
                
            int tile = MAP[(int)tilep.x + MAP_DIM * (int)tilep.y];
            hit = tile != 0;
            if (hit) { // hit
                Draw(rayp.x, rayp.y, tilecol);

                Vec2 dist_to_wall = vec2_sub(rayp, vpy);
                if (vec2_magnitude(dist_to_wall) < stop_dist) { // stop
                    player.x -= delta_x;
                    player.y -= delta_y;
                }
            }
        }
    }


    // Draw player
    Draw(vpy.x, vpy.y, pcol);

    Draw(vpy.x+1, vpy.y, pcol);
    Draw(vpy.x, vpy.y+1, pcol);
    Draw(vpy.x+1, vpy.y+1, pcol);

    Draw(vpy.x-1, vpy.y, pcol);
    Draw(vpy.x, vpy.y-1, pcol);
    Draw(vpy.x-1, vpy.y-1, pcol);

    Draw(vpy.x+1, vpy.y-1, pcol);
    Draw(vpy.x-1, vpy.y+1, pcol);
    
    // Draw player look dir
    for (int i = 0; i < 10; ++i) {
        float ldx = vpy.x + player_look_dir.x * i;
        float ldy = vpy.y + player_look_dir.y * i;

        Draw(ldx, ldy, ldcol);
    }

}


int openglInit()
{
    glfwSetErrorCallback(glfw_error_callback);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Oh sh*t, here we go again...", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_position_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }


    // Initialize OpenGL debug callback
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    } else {
        printf("Error: debug output was not enabled!\n");
    }

    return 0;
}

int main(void)
{
    SCR_WIDTH = COLS * PIX_W;
    SCR_HEIGHT = ROWS * PIX_H;

    if (openglInit() != 0) {
        return -1;
    }

    textureBuffer_init(&tb, COLS, ROWS);
    textureBuffer_reset(&tb, COLS, ROWS);

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
    glShaderSource(vertexShader, 1, (const char**)&vertex_source, NULL);
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
    glShaderSource(fragmentShader, 1, (const char**)&fragment_source, NULL);
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
        // positions        // texture coords
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
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
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

#if 1
    glGenTextures(1, &tb.gl_tex_id);
    glBindTexture(GL_TEXTURE_2D, tb.gl_tex_id); 
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tb.width, tb.height, 0, GL_RGB, GL_UNSIGNED_BYTE, tb.data);

    tb.gl_tex_init = true;
#endif
    
   
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    glUseProgram(shaderProgram); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

#endif
    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

#if 1
    if (game_init() != 0) {
        return -1;
    }
#endif

    CLEAR_COLOR.r = 0;
    CLEAR_COLOR.g = 0;
    CLEAR_COLOR.b = 0;

    glActiveTexture(GL_TEXTURE0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        static double delta_time;

        clock_t start = clock();

        tb.updated_this_frame = false;
        textureBuffer_clear(&tb, CLEAR_COLOR);
        textureBuffer_loadTexData(&tb);
#if 1
        processInput(window, delta_time);
        gameLogic(delta_time);
#endif

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //Pixel prp = tb.data[(int)MOUSE_X + (int)MOUSE_Y * tb.width];
        //printf("Cursor color: %d, %d, %d\n", prp.r, prp.g, prp.b);

#if 1
        // bind textures on corresponding texture units

    #if 1
        //glActiveTexture(GL_TEXTURE0);
            //glBindTexture(GL_TEXTURE_2D, tb.gl_tex_id);


        if (tb.updated_this_frame) {
            
            textureBuffer_loadTexData(&tb);
        }
    #endif

        // draw
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time 
#endif 

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        clock_t end = clock();

        delta_time = ((double) (end - start)) / CLOCKS_PER_SEC;
        //printf("Delta time: %f\n", delta_time);
    }

#if 1
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