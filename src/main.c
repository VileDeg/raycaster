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
#include "game_map.h"


#define TOP_VIEW_PIX_W 4
#define TOP_VIEW_PIX_H 4


#define M_PI   3.14159265358979323846264338327950288

#define degToRad(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / M_PI)


/*
* tv - top view (TV), left
* pov - first person view (POV), right
* map - integer coordinates in map
*/

#define map_to_tv_x(x) ((x) / gm.dim * scr.TOP_VIEW_COLS)
#define map_to_tv_y(y) ((y) / gm.dim * scr.TOP_VIEW_ROWS)

#define tv_to_map_x(x) ((x) / (float)scr.TOP_VIEW_COLS * gm.dim)
#define tv_to_map_y(y) ((y) / (float)scr.TOP_VIEW_ROWS * gm.dim)


typedef struct {
    int POV_PIX_W;
    int POV_PIX_H;

    int TOP_VIEW_ROWS;
    int TOP_VIEW_COLS;

    int POV_ROWS;
    int POV_COLS;

    int SCR_WIDTH;
    int SCR_HEIGHT;

    float PLAYER_POV;

    // Step angle diff between two rays in radians
    float RAY_ANGLE_STEP; // theta
    
} Screen;

static float RAY_STEP_COS;
static float RAY_STEP_SIN;

static void screen_set_pov_cols(Screen* scr, int pov_cols) {
    scr->POV_COLS = pov_cols;
    scr->RAY_ANGLE_STEP = degToRad( scr->PLAYER_POV / scr->POV_COLS );

    RAY_STEP_COS = cos(scr->RAY_ANGLE_STEP);
    RAY_STEP_SIN = sin(scr->RAY_ANGLE_STEP);
}

static void screen_reset(Screen* scr, int width, int height)
{
    // At least 4, if a texel should be bigger, then round to more
    // Mutpily by 2 to avoid having texture of uneven dimensions (probably bad for OpenGL?)
    static int screen_round_to = max(max(TOP_VIEW_PIX_W * 2, TOP_VIEW_PIX_H * 2), 4);

    // Window dimensions must be aligned to multiple of 4 
    // because OpenGL doesn't correctly display textures with dimensions unaligned to 4
    if (width != scr->SCR_WIDTH && width % screen_round_to != 0) {
        if (width > scr->SCR_WIDTH) {
            width += screen_round_to - width % screen_round_to;
        } else {
            width -= width % screen_round_to;
        }
    }

    if (height != scr->SCR_HEIGHT && height % screen_round_to != 0) {
        if (height > scr->SCR_HEIGHT) {
            height += screen_round_to - height % screen_round_to;
        } else {
            height -= height % screen_round_to;
        }
    }

    scr->SCR_WIDTH = width * 2;
    scr->SCR_HEIGHT = height;

    scr->TOP_VIEW_COLS = width / TOP_VIEW_PIX_W;
    scr->TOP_VIEW_ROWS = height / TOP_VIEW_PIX_H;

    float new_pov_cols = width / TOP_VIEW_PIX_W;
    screen_set_pov_cols(scr, new_pov_cols);
    //scr->POV_COLS = width / TOP_VIEW_PIX_W;
    scr->POV_ROWS = height / TOP_VIEW_PIX_H;

    printf("Screen WIDTH: %d\n", scr->SCR_WIDTH);
    printf("Screen HEIGHT: %d\n", scr->SCR_HEIGHT);
    printf("\n\n");


    /*textureBuffer_reset(&top_view_tb, scr->TOP_VIEW_COLS, scr->TOP_VIEW_ROWS);
    textureBuffer_reset(&pov_tb, scr->POV_COLS, scr->POV_ROWS);*/
}

Screen scr = {
    .TOP_VIEW_ROWS = 128,
    .TOP_VIEW_COLS = 128,
    .PLAYER_POV = 120,
};


GLFWwindow* window = NULL;
static TextureBuffer top_view_tb; // top view (left side of the screen)
static TextureBuffer pov_tb; // first-person view (right side of the screen)

static GameMap gm;

static Pixel TOP_CLEAR_COLOR;
static Pixel POV_CLEAR_COLOR;

void glfw_error_callback(int error, const char* description) 
{
    printf("GLFW Error: %s\n", description);
}

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









// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

    screen_reset(&scr, width, height);
    textureBuffer_reset(&top_view_tb, scr.TOP_VIEW_COLS, scr.TOP_VIEW_ROWS);
    textureBuffer_reset(&pov_tb, scr.POV_COLS, scr.POV_ROWS);
}


static double MOUSE_X_SCREEN_SPACE = 0;
static double MOUSE_Y_SCREEN_SPACE = 0;

static double MOUSE_X_TEX_SPACE = 0;
static double MOUSE_Y_TEX_SPACE = 0;

static double MOUSE_DELTA_X = 0;
static double MOUSE_DELTA_Y = 0;

static double theta = 0;

static bool must_update_player_look_dir = false;

static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double prev_x = 0;
    static double prev_y = 0;

    must_update_player_look_dir = true;
    
    //printf("Position: (%f:%f)\n", xpos, ypos);
    //MOUSE_X_SCREEN_SPACE = xpos * 2; // Mult by 2 to anchor mouse pos relative to top view
    MOUSE_X_SCREEN_SPACE = xpos; // Mult by 2 to anchor mouse pos relative to top view
    MOUSE_Y_SCREEN_SPACE = ypos;

    MOUSE_X_TEX_SPACE = MOUSE_X_SCREEN_SPACE / scr.SCR_WIDTH * scr.TOP_VIEW_COLS;
    MOUSE_Y_TEX_SPACE = MOUSE_Y_SCREEN_SPACE / scr.SCR_HEIGHT * scr.TOP_VIEW_ROWS;

    MOUSE_DELTA_X = xpos - prev_x;
    MOUSE_DELTA_Y = ypos - prev_y;

    //printf("Delta_X: (%lf)\n", MOUSE_DELTA_X);

    

    prev_x = xpos;
    prev_y = ypos;
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

#define TILE_SIZE 1

#define TILE_STEPS 10

static int NUM_STEPS;

//static const int STEP = TILE_SIZE / TILE_STEPS;
//
//static const float dist_to_near_plane = 0.2; // equals to half plane width means 45 degree FOV
//static const float near_plane_width = 0.4;


static float plane_width;
static float plane_dist = 4;

//static const int supersampling = 32;

static const float stop_dist = 0.1;

static const float mov_speed = 3;

// Step angle diff between two rays in radians
static float RAY_ANGLE_STEP;

// Coordinates of player in map
static Vec2 player = {2, 3};

static Vec2 player_look_dir = {0, 1};

static Pixel BLACK = { 0,  0,  0 };
static Pixel RED   = { 255, 0, 0 };
static Pixel GREEN = { 0, 255, 0 };
static Pixel BLUE  = { 0, 0, 255 };

static Pixel YELLOW = { 255, 255, 0 };
static Pixel MAGENTA = { 255, 0, 255 };
static Pixel CYAN = { 0, 255, 255 };

static Pixel PLAYER_COLOR;
static Pixel NEAREST_PLANE_COLOR;


#define draw_TV(x, y, pix) do {\
    textureBuffer_setPixel(&top_view_tb, (x), (scr.TOP_VIEW_ROWS-y), (pix)); \
} while(0)

#define draw_POV(x, y, pix) do {\
    textureBuffer_setPixel(&pov_tb, (x), (y), (pix)); \
} while(0)


void globals_Init()
{
    PLAYER_COLOR = RED;
    NEAREST_PLANE_COLOR = MAGENTA;


    plane_width = plane_dist * tan(degToRad(scr.PLAYER_POV / 2.0)) * 2.0;


    //POV_COLS = plane_width * supersampling;
    screen_set_pov_cols(&scr, 128);
    scr.POV_ROWS = scr.POV_COLS;

    int half_screen_w = scr.TOP_VIEW_COLS * TOP_VIEW_PIX_W;

    scr.POV_PIX_W = half_screen_w / scr.POV_COLS;
    scr.POV_PIX_H = scr.POV_PIX_W;

    scr.SCR_WIDTH = scr.TOP_VIEW_COLS * TOP_VIEW_PIX_W * 2;
    scr.SCR_HEIGHT = scr.TOP_VIEW_ROWS * TOP_VIEW_PIX_H;

    NUM_STEPS = TILE_STEPS * gm.dim;

    TOP_CLEAR_COLOR = BLACK;
    POV_CLEAR_COLOR = BLACK;
}



void processInput(GLFWwindow* window, double delta_time) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}




void drawNearestPlane(Vec2 player_pos_pixel_space, Vec2 player_look_dir)
{
    Vec2 plane_dir = vec2_perpendicular(player_look_dir);
    Vec2 plane_center = vec2_muli(player_look_dir, plane_dist);

    for (float i = -plane_width / 2; i < plane_width / 2 + 1; i += 1) {
        // Vec2 vpl = vpy + player_look_dir * (plane_dist) + plane_dir * i;

        Vec2 plane_dir_advanced = vec2_mulf(plane_dir, i);

        Vec2 plane_next = vec2_add(plane_center, plane_dir_advanced);

        Vec2 vpl = vec2_add(player_pos_pixel_space, plane_next);

        draw_TV(vpl.x, vpl.y, NEAREST_PLANE_COLOR);
    }
}

void drawPlayer(Vec2 player_pos_pixel_space)
{
    for (int i = -1; i < 2; ++i) {
        for (int j = -1; j < 2; ++j) {
            draw_TV(player_pos_pixel_space.x + i, player_pos_pixel_space.y + j, PLAYER_COLOR);
        }
    }
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

    Vec2 player_pos_pixel_space = { map_to_tv_x(player.x), map_to_tv_x(player.y)};

    /*player_look_dir.x = MOUSE_X_TEX_SPACE - player_pos_pixel_space.x;
    player_look_dir.y = MOUSE_Y_TEX_SPACE - player_pos_pixel_space.y;*/
    if (must_update_player_look_dir) {
        theta += MOUSE_DELTA_X * delta_time;

        player_look_dir.x = cos(theta);
        player_look_dir.y = sin(theta);

        player_look_dir = vec2_normalized(player_look_dir);

        must_update_player_look_dir = false;
    }

    Vec2 plane_dir = vec2_perpendicular(player_look_dir);

    Vec2 addx = vec2_mulf(plane_dir, delta_x);
    Vec2 addy = vec2_mulf(player_look_dir, -delta_y);

    Vec2 player_delta_pos = vec2_add(addx, addy);

    player = vec2_add(player, player_delta_pos);

    /*player.x += ;
    player.y += delta_y;*/
    
    Pixel ldcol   = YELLOW;
    
    Pixel raycol  = GREEN;
    Pixel wallcol = RED;

    Pixel ceilcol = GREEN;
    Pixel floorcol = YELLOW;

    
    // Draw near clipping plane
    

    //int num_rays = plane_width * supersampling;
    int num_rays = scr.POV_COLS;
    
    //float i_step = 1 / (float)supersampling;
    //int ray_len = 50;
    float ray_step = 0.1;
    int num_ray_steps = scr.TOP_VIEW_ROWS / ray_step;

    int player_view_dist = scr.TOP_VIEW_ROWS;
    float stop_dist = 5;

    bool player_stop = false;


    // Rotate fully to the left of pov 
    float half_pov = scr.PLAYER_POV / 2.f;
    float cos_start = cos(degToRad(-half_pov));
    float sin_start = sin(degToRad(-half_pov));

    float ray_x = player_look_dir.x * cos_start - player_look_dir.y * sin_start;
    float ray_y = player_look_dir.x * sin_start + player_look_dir.y * cos_start;

    Vec2 rayd = { ray_x, ray_y };

    Vec2 ray_origin = player_pos_pixel_space;

    //for (float i = -plane_width/2; i < plane_width/2+1; i += i_step) {
    for (int i = 0; i < num_rays; ++i) {
        // Vec2 vpl = vpy + player_look_dir * (plane_dist) + plane_dir * i;

        /*Vec2 plane_center = vec2_mulf(player_look_dir, plane_dist);

        float half_pw = plane_width / 2.f;
        float adv = -half_pw + i / (float)num_rays * plane_width;

        Vec2 plane_dir_advanced = vec2_mulf(plane_dir, adv);

        Vec2 plane_next = vec2_add(plane_center, plane_dir_advanced);

        Vec2 ray_origin = vec2_add(player_pos_pixel_space, plane_next);*/

        //Draw(vpl.x, vpl.y, nearest_plane_col);

        

        
        if (i > 0) {
            // Rotate ray to next step
            ray_x = rayd.x * RAY_STEP_COS - rayd.y * RAY_STEP_SIN;
            ray_y = rayd.x * RAY_STEP_SIN + rayd.y * RAY_STEP_COS;

            rayd.x = ray_x;
            rayd.y = ray_y;

            // TODO: is already normalized?
            rayd = vec2_normalized(rayd);
        }

        
        
        
        // Draw floor and ceiling
        {
            int half_r = scr.POV_ROWS / 2;

            // Floor
            for (int y = 0; y < half_r + 1; ++y) {
                draw_POV(i, y, pixel_mulf(floorcol, 1));
            }
            // Ceilling
            for (int y = half_r + 1; y < scr.POV_ROWS; ++y) {
                draw_POV(i, y, pixel_mulf(ceilcol, 1));
            }
        }

        bool hit = false;
        int j = 0;
        for (; j < num_ray_steps && !hit; ++j) {

            //Vec2 rayp = vpy + rayd * j * ray_step;
            Vec2 ray_dir_advanced = vec2_mulf(rayd, j*ray_step);
            //Vec2 rayp = vec2_add(player_pos_pixel_space, ray_dir_advanced);
            Vec2 rayp = vec2_add(ray_origin, ray_dir_advanced);

            

            draw_TV(rayp.x, rayp.y, raycol);


            Vec2 tilep = { tv_to_map_x(rayp.x), tv_to_map_y(rayp.y) };


            int tile_x = tilep.x;
            int tile_y = tilep.y;

            // Don't go out of map bounds
            if (tile_x < 0.f    || tile_y < 0.f     ||
                tile_x > gm.dim || tile_y > gm.dim) {
                break;
            }

            assert(tile_x >= 0 && tile_x <= gm.dim);
            assert(tile_y >= 0 && tile_y <= gm.dim);
                
            int tile = gm.map[tile_x + gm.dim * tile_y];
            hit = tile != 0;
            if (hit) {
                draw_TV(rayp.x, rayp.y, wallcol);

                //int pov_y = POV_ROWS / 2;
                

                //Vec2 player_to_wall = vec2_sub(rayp, player_pos_pixel_space);
                Vec2  player_to_wall = vec2_sub(rayp, ray_origin);
                float dist_to_wall   = vec2_magnitude(player_to_wall);
                //if (dist_to_wall < stop_dist) { // stop
                //    player_stop = true;
                //}

                

                float ratio = dist_to_wall / player_view_dist;
                //float pw = 2.5;
                //float factor = pow(pw, 1+ratio) - pw;
                float factor = ratio;

                //float pw = 2.5;
                //float ratio = pow(pw, dist_to_wall) / player_view_dist;
                ////float factor = pow(pw, 1 + ratio) - pw;
                //float factor = ratio;

                //int height = POV_ROWS * (1 - factor);
                int height = scr.POV_ROWS - scr.POV_ROWS * (dist_to_wall / player_view_dist);
                int half_h = height / 2;

                int half_r = scr.POV_ROWS / 2;

                float brightness = height / (float)scr.POV_ROWS;
                // Draw wall
                for (int y = half_r - half_h; y < half_r + half_h + 1; ++y) {
                    draw_POV(i, y, pixel_mulf(wallcol, brightness));
                }
            }
        }
    }


    // Cast a ray in the direction the player is moving to detect collision with wall
#if 1
    Vec2 player_move_dir = vec2_normalized(player_delta_pos);

    Pixel collision_ray_col = CYAN;
    int collision_ray_len = stop_dist * 10;

    Vec2 rayd_coll = player_move_dir;

   

    //printf("Moving in dir (%f, %f)   %f\n", rayd_coll.x, rayd_coll.y, vec2_magnitude(rayd_coll));

    bool hit = false;
    for (int j = 0; j < collision_ray_len && !hit; ++j) {

        Vec2 ray_dir_advanced = vec2_mulf(rayd_coll, j * ray_step);
        Vec2 rayp = vec2_add(player_pos_pixel_space, ray_dir_advanced);

        if (rayp.x < 0.f || rayp.y < 0.f) {
            break;
        }

        draw_TV(rayp.x, rayp.y, collision_ray_col);


        Vec2 tilep = { tv_to_map_x(rayp.x), tv_to_map_y(rayp.y) };
        assert(tilep.x < 11 && tilep.y < 11);
        assert(tilep.x > -1 && tilep.y > -1);

        int tile_x = tilep.x;
        int tile_y = tilep.y;

        int tile = gm.map[tile_x + gm.dim * tile_y];
        hit = tile != 0;
        if (hit) {
            //drawTopView(rayp.x, rayp.y, wallcol);

            //int pov_y = POV_ROWS / 2;


            Vec2 player_to_wall = vec2_sub(rayp, player_pos_pixel_space);
            float dist_to_wall = vec2_magnitude(player_to_wall);
            if (dist_to_wall < stop_dist) { // stop
                player_stop = true;
            }
        }
    }

#endif

    if (player_stop) { // hit
        player = vec2_sub(player, vec2_mulf(player_delta_pos, 1.1));
    }


    // Draw player
    drawPlayer(player_pos_pixel_space);

    drawNearestPlane(player_pos_pixel_space, player_look_dir);

    // Draw player look dir
    for (int i = 0; i < 10; ++i) {
        float ldx = player_pos_pixel_space.x + player_look_dir.x * i;
        float ldy = player_pos_pixel_space.y + player_look_dir.y * i;

        draw_TV(ldx, ldy, ldcol);
    }


}

static unsigned int VAO, VBO, EBO;
static unsigned int shaderProgram;

int opengl_init()
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
    window = glfwCreateWindow(scr.SCR_WIDTH, scr.SCR_HEIGHT, "Raycaster", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_position_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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


    // ----------------------------

    const char* vs = "shaders/shader.vert";
    const char* fs = "shaders/shader.frag";

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
    shaderProgram = glCreateProgram();
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
    //float vertices[] = {
    //    // positions        // texture coords
    //     1.f,  1.f, 0.0f,   1.0f, 1.0f, // top right
    //     1.f, -1.f, 0.0f,   1.0f, 0.0f, // bottom right
    //    -1.f, -1.f, 0.0f,   0.0f, 0.0f, // bottom left
    //    -1.f,  1.f, 0.0f,   0.0f, 1.0f  // top left 
    //};

    // Set u coordinate to 0 - 2 to make the texture repeat twice horizontally
    float vertices[] = {
        // positions        // texture coords
         1.f,  1.f, 0.0f,   2.0f, 1.0f, // top right
         1.f, -1.f, 0.0f,   2.0f, 0.0f, // bottom right
        -1.f, -1.f, 0.0f,   0.0f, 0.0f, // bottom left
        -1.f,  1.f, 0.0f,   0.0f, 1.0f  // top left 
    };


    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };


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

    {
        top_view_tb.gl_tex_unit = 0;
        glActiveTexture(GL_TEXTURE0);

        glGenTextures(1, &top_view_tb.gl_tex_id);
        glBindTexture(GL_TEXTURE_2D, top_view_tb.gl_tex_id);
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, top_view_tb.width, top_view_tb.height, 0, GL_RGB, GL_UNSIGNED_BYTE, top_view_tb.data);

        top_view_tb.gl_tex_init = true;
    }
    {
        pov_tb.gl_tex_unit = 1;
        glActiveTexture(GL_TEXTURE0 + 1);

        glGenTextures(1, &pov_tb.gl_tex_id);
        glBindTexture(GL_TEXTURE_2D, pov_tb.gl_tex_id);
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NEVER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NEVER);*/
        // set texture filtering parameters
        /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pov_tb.width, pov_tb.height, 0, GL_RGB, GL_UNSIGNED_BYTE, pov_tb.data);

        pov_tb.gl_tex_init = true;
    }


    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    glUseProgram(shaderProgram); // don't forget to activate/use the shader before setting uniforms!
    glUniform1i(glGetUniformLocation(shaderProgram, "topViewTex"), 0); // Texture unit 0
    glUniform1i(glGetUniformLocation(shaderProgram, "povTex"), 1); // Texture unit 1

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    return 0;
}

void opengl_cleanup()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}


void game_loop(double delta_time)
{
    // render
    // ------
    /*glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);*/
    {
        top_view_tb.updated_this_frame = false;
        textureBuffer_clear(&top_view_tb, TOP_CLEAR_COLOR);
    }
    {
        pov_tb.updated_this_frame = false;
        textureBuffer_clear(&pov_tb, POV_CLEAR_COLOR);
    }

    processInput(window, delta_time);
    gameLogic(delta_time);

    if (top_view_tb.updated_this_frame) {
        textureBuffer_loadTexData(&top_view_tb);
    }

    if (pov_tb.updated_this_frame) {
        textureBuffer_loadTexData(&pov_tb);
    }

    // draw
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int main(void)
{
    globals_Init();


    textureBuffer_init(&top_view_tb, scr.TOP_VIEW_COLS, scr.TOP_VIEW_ROWS);
    textureBuffer_init(&pov_tb, scr.POV_COLS, scr.POV_ROWS);

    if (opengl_init() != 0) {
        return -1;
    }


    if (gameMap_init(&gm, "maps/00.txt") != 0) {
        return -1;
    }

    

    

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        static double delta_time;

        clock_t start = clock();

        game_loop(delta_time);
        // glBindVertexArray(0); // no need to unbind it every time 

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        clock_t end = clock();

        delta_time = ((double) (end - start)) / CLOCKS_PER_SEC;
        //printf("Delta time: %f\n", delta_time);
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    opengl_cleanup();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}