#include <stdio.h>
#include<stdarg.h>
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

VEC2_DEF(f, float)
VEC2_DEF(i, int)

VEC2_IMPL(f, float, i, int, "(%.2f, %.2f)")
VEC2_IMPL(i, int, f, float,  "(%d, %d)")


#define M_PI   3.14159265358979323846264338327950288

#define degToRad(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / M_PI)


/*
* tv - top view (TV), left
* pov - first person view (POV), right
* map - integer coordinates in map
*/

#define map_to_tv_x(x) ((float)(x) / GAME_MAP.dim * scr.TOP_VIEW_COLS)
#define map_to_tv_y(y) ((float)(y) / GAME_MAP.dim * scr.TOP_VIEW_ROWS)

#define tv_to_map_x(x) ((x) / (float)scr.TOP_VIEW_COLS * GAME_MAP.dim)
#define tv_to_map_y(y) ((y) / (float)scr.TOP_VIEW_ROWS * GAME_MAP.dim)


// Size of a minimap pixel in screen pixels
static Vec2i minimap_pix_dim = { 4, 4 };

static Vec2i pov_pix_dim = { 4, 4 };

typedef struct {
    // Size of a display pixel in actual screen pixels
    Vec2i pix_dim;

    // Holds the GL texture and resolution
    TextureBuffer tex_buf; // TODO: hold a pointer?

    Pixel clear_color;
} Display;

static void display_update(Display* display, Vec2i screen_resolution);

static Display display_create(Vec2i pix_dim, Vec2i screen_resolution, Pixel clear_color) {
    Display d;
    d.pix_dim = pix_dim;
    d.clear_color = clear_color;
    textureBuffer_init(&d.tex_buf);

    display_update(&d, screen_resolution);
    return d;
}

static Vec2i display_get_real_resolution(Display* display, Vec2i screen_resolution) {
    return (Vec2i){ 
        screen_resolution.x / display->pix_dim.x,
        screen_resolution.y / display->pix_dim.y 
    };
}

static float display_get_aspect_ratio(Display* display, Vec2i screen_resolution) {
    Vec2i resolution = display_get_real_resolution(display, screen_resolution);
    return (float)resolution.x / resolution.y;
}

static Display s_Minimap_display;
static Display s_Pov_display;

static bool s_Is_display_minimap;


// Assume the tile is square on the ground
#define TILE_WIDTH 1.f

// Wall width / height
#define TILE_ASPECT 1.f
#define TILE_HEIGHT (TILE_WIDTH / TILE_ASPECT)


#define MAX_TEXTURES 32

#define TEX_WIDTH  32
#define TEX_HEIGHT 32


static Vec2f map_to_world(int x, int y) {
    return (Vec2f) { 
        x * TILE_WIDTH + TILE_WIDTH / 2.f, 
        y * TILE_WIDTH + TILE_WIDTH / 2.f
    };
}

static Vec2f map_to_world_v(Vec2i v) {
    return (Vec2f) { 
        v.x * TILE_WIDTH + TILE_WIDTH / 2.f, 
        v.y * TILE_WIDTH + TILE_WIDTH / 2.f
    };
}

static Vec2i world_to_map(Vec2f v) {
    return (Vec2i) {
        v.x / TILE_WIDTH,
        v.y / TILE_WIDTH
    };
}

// Actual screen resolution in pixels
static Vec2i s_Screen_resolution;

#define DEFAULT_FOV 66.f

static float s_Horizontal_fov = DEFAULT_FOV; // degrees
static float s_Vertical_fov = DEFAULT_FOV;

// TODO: use log level supplied from CLI arguments
#define LOG(_Type, _Format, ...) printf("[%s]: %s: " _Format "\n", __func__, _Type, __VA_ARGS__)

#define LOG_WARN(_Format, ...) LOG("WARNING", _Format, __VA_ARGS__)
#define LOG_ERR(_Format, ...)  LOG("ERROR",   _Format, __VA_ARGS__)
#define LOG_INFO(_Format, ...) LOG("INFO",    _Format, __VA_ARGS__)


/* 
* @param screen_resolution should be already aligned to pixel dim, 
*                           otherwise display may not take the whole screen space
* */
static void display_update(Display* display, Vec2i screen_resolution)
{
    assert(!(display->pix_dim.x % 2) && !(display->pix_dim.y % 2) && "Pix dim must be even");

    textureBuffer_reset(
        &display->tex_buf,
        screen_resolution.x / display->pix_dim.x,
        screen_resolution.y / display->pix_dim.y
    );
}

static GLFWwindow* s_Window = NULL;

static GameMap GAME_MAP;

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




static int round_to(int src, int val, bool up) {
    return up 
        ? src + val - src % val 
        : src - src % val;
}




// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // TODO: update both pixel dim or do it on minimap toggle
    Vec2i pixel_dim = s_Is_display_minimap ? s_Minimap_display.pix_dim : s_Pov_display.pix_dim;
    assert(!(pixel_dim.x % 2) && !(pixel_dim.y % 2) && "Pix dim must be even");

    // Round up/down to pix
    s_Screen_resolution = (Vec2i){ 
        round_to(width , pixel_dim.x, s_Screen_resolution.x < width),
        round_to(height, pixel_dim.y, s_Screen_resolution.y < height)
    };

    glViewport(0, 0, s_Screen_resolution.x, s_Screen_resolution.y);

    if (s_Is_display_minimap) {
        display_update(&s_Minimap_display, s_Screen_resolution);
    } else {
        display_update(&s_Pov_display, s_Screen_resolution);
    }
}

static void toggle_minimap(bool minimap) {
    if (minimap) {

    }
}


static float MOUSE_DELTA_X = 0;
static float MOUSE_DELTA_Y = 0;

static double theta = 0;

static bool must_update_player_look_dir = false;

static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    static float prev_x = 0;
    static float prev_y = 0;

    must_update_player_look_dir = true;
    
    //printf("Position: (%f:%f)\n", xpos, ypos);

    MOUSE_DELTA_X = (float)xpos - prev_x;
    MOUSE_DELTA_Y = (float)ypos - prev_y;

    //printf("Delta_X: (%lf)\n", MOUSE_DELTA_X);

    prev_x = (float)xpos;
    prev_y = (float)ypos;
}


char* readShaderSource(const char* filepath) {
    
    FILE* fd = fopen(filepath, "rb");
    if (fd == NULL) {
        printf("Failed to open file: %s\n", filepath);
        return NULL;
    }

    // Get the size of the file
    fseek(fd, 0, SEEK_END);
    long filesize = ftell(fd);
    if (filesize < 0) {
        perror("ftell(...) failed");
        exit(EXIT_FAILURE);
    }
    fseek(fd, 0, SEEK_SET);

    // Allocate memory for the buffer
    char* buffer = malloc(filesize + 1);
    if (buffer == NULL)
    {
        printf("Failed to allocate memory for the buffer\n");
        return NULL;
    }

    // Read the file into the buffer
    size_t result = fread(buffer, sizeof(*buffer), filesize, fd);
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


static int NUM_STEPS;

static float plane_width;

/* 
* Array of textures.
* Each texture stored as ROW-MAJOR, TOP_DOWN array of bytes.
*/
static unsigned int* s_Textures[MAX_TEXTURES];

static const float stop_dist = 0.1f;
static const float mov_speed = 3.f;

// Step angle diff between two rays in radians
static float RAY_ANGLE_STEP;

static Vec2f player_world;
// Coordinates of player in map
static Vec2i player_map = { 2, 3 };

static Vec2f player_look_dir = {0, 1};

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
    textureBuffer_setPixel(&s_Pov_display.tex_buf, (x), (y), (pix)); \
} while(0)

unsigned int get_texture_pixel(int texture, int x, int y) {
    return s_Textures[texture][x + y * TEX_WIDTH];
}

static void globals_Init()
{
    PLAYER_COLOR = RED;
    NEAREST_PLANE_COLOR = MAGENTA;

    // TODO: can scale the plane_width by tile width if want to fit N tiles in screen by width
    plane_width = (float)tan(degToRad(s_Horizontal_fov / 2.0));
    LOG_INFO("View plane width %f, tile size w, h [%f, %f]", plane_width, TILE_WIDTH, TILE_HEIGHT);
    player_world = map_to_world_v(player_map);
    LOG_INFO("Player map, world: %s, %s", vec2i_str(player_map), vec2f_str(player_world));

    s_Screen_resolution = (Vec2i){ 1024, 1024 };

#ifdef MINIMAP
    s_Minimap_display = display_create((Vec2i){ 4, 4 }, s_Screen_resolution, BLACK);
#endif
    s_Pov_display = display_create((Vec2i) { 4, 4 }, s_Screen_resolution, BLACK);

    // TODO: display_get_aspect_ratio
    Vec2i resolution = display_get_real_resolution(&s_Pov_display, s_Screen_resolution);
    float aspect_ratio = (float)resolution.x / resolution.y;

    s_Vertical_fov = s_Horizontal_fov / aspect_ratio;
}

#ifdef MINIMAP
static void drawNearestPlane(Vec2i player_pos_pixel_space, Vec2f player_look_dir)
{
    Vec2f plane_dir = vec2f_perpendicular(player_look_dir);
    Vec2f plane_center = { (float)player_pos_pixel_space.x, (float)player_pos_pixel_space.y };

    for (float i = -plane_width / 2; i < plane_width / 2 + 1; i += 1) {
        // Vec2f vpl = vpy + player_look_dir * (plane_dist) + plane_dir * i;

        Vec2f plane_dir_advanced = vec2f_muls(plane_dir, i);

        Vec2f plane_next = vec2f_add(plane_center, plane_dir_advanced);

        Vec2f vpl = vec2f_add(plane_center, plane_next);

        draw_TV((int)vpl.x, (int)vpl.y, NEAREST_PLANE_COLOR);
    }
}

static void drawPlayer(Vec2f player_pos_pixel_space)
{
    for (int i = -1; i < 2; ++i) {
        for (int j = -1; j < 2; ++j) {
            draw_TV(player_pos_pixel_space.x + i, player_pos_pixel_space.y + j, PLAYER_COLOR);
        }
    }
}
#endif

static void gameLogic(float delta_time, bool check_any_input) {
    if (glfwGetKey(s_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(s_Window, true);
        return;
    }

    double delta_x = 0; 
    double delta_y = 0;

    if (glfwGetKey(s_Window, GLFW_KEY_W) == GLFW_PRESS) {
        delta_y -= mov_speed * delta_time;
    }

    if (glfwGetKey(s_Window, GLFW_KEY_A) == GLFW_PRESS) {
        delta_x -= mov_speed * delta_time;
    }

    if (glfwGetKey(s_Window, GLFW_KEY_S) == GLFW_PRESS) {
        delta_y += mov_speed * delta_time;
    }

    if (glfwGetKey(s_Window, GLFW_KEY_D) == GLFW_PRESS) {
        delta_x += mov_speed * delta_time;
    }

    bool was_any_input = !check_any_input || delta_x || delta_y || must_update_player_look_dir;
    if (!was_any_input) {
      return;
    }

    if (must_update_player_look_dir) {
        float rotSpeed = MOUSE_DELTA_X * delta_time;
        float oldDirX = player_look_dir.x;
        player_look_dir.x = player_look_dir.x * cosf(rotSpeed) - player_look_dir.y * sinf(rotSpeed);
        player_look_dir.y = oldDirX * sinf(rotSpeed) + player_look_dir.y * cosf(rotSpeed);

        must_update_player_look_dir = false;
    }

    Vec2f plane_dir = vec2f_perpendicular(player_look_dir);

    /*
    printf("\n", player_look_dir.x, player_look_dir.y);
    printf("player_look_dir: %f, %f\n", player_look_dir.x, player_look_dir.y);
    printf("plane_dir: %f, %f\n", plane_dir.x, plane_dir.y);
    */

    if(delta_x || delta_y) {
        Vec2f addx = vec2f_muls(plane_dir, (float)delta_x);
        Vec2f addy = vec2f_muls(player_look_dir, (float)-delta_y);

        Vec2f player_delta_pos = vec2f_divs(vec2f_add(addx, addy), TILE_WIDTH);

        Vec2f player_world_next = vec2f_add(player_world, player_delta_pos);

        Vec2i player_map_next = world_to_map(player_world_next);

        if (
          !(gameMap_get_tile(&GAME_MAP, player_map_next.x, player_map_next.y) & GM_NON_WALKABLE_MASK)
        ) {
           player_map = player_map_next;
           player_world = player_world_next;
        } 
    }

#ifdef MINIMAP
    Vec2i player_pos_minimap = { map_to_tv_x(player_map.x), map_to_tv_x(player_map.y) };
#endif
    //Pixel ceilcol = YELLOW;
    //Pixel floorcol = GREEN;
    
    int pov_rows = s_Pov_display.tex_buf.height;
    int pov_cols = s_Pov_display.tex_buf.width;

    float aspect = display_get_aspect_ratio(&s_Pov_display, s_Screen_resolution);
    float H = 2 * tanf(degToRad(s_Horizontal_fov / 2.f)) + plane_width;
    H /= aspect;

    float tile_height = TILE_WIDTH / H * 1.5f; // TODO: why does 1.5f work for walls? Experimental
    
    for (int i = 0; i < pov_cols; ++i) {
       
        double dirX = player_look_dir.x;
        double dirY = player_look_dir.y;

        double planeX = plane_dir.x * plane_width;
        double planeY = plane_dir.y * plane_width;

        //calculate ray position and direction
        double cameraX = 2 * i / (double)pov_cols - 1; //x-coordinate in camera space
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirY + planeY * cameraX;

        // Draw floor and ceiling
        {
            // + 1 row because we want to get the angle when ray is in the middle of a pixel, not after the pixel
            float degrees_per_pixel = s_Vertical_fov / (pov_rows + 1);
            int half_r = pov_rows / 2;

            Vec2f ray_dir = (Vec2f){ rayDirX, rayDirY };

            float height = tile_height * 0.5f; // TODO: 0.5f is experimental doesn tw ork
            

            // Ceiling & floor
            for (int y = 0; y < pov_rows; ++y) {

                bool is_ceiling = y < half_r;
                int iter = (is_ceiling ? half_r - y : y - half_r + 1);
                float fov_angle = iter * degrees_per_pixel;
                float sample_point_z = height / tanf(degToRad(fov_angle)); // TODO: precompute tanf
                Vec2f sample_point_world = vec2f_add(
                    player_world, 
                    vec2f_muls(ray_dir, sample_point_z)
                );
                // Sample the selected repeating texture
                int texX = fmod(sample_point_world.x, TILE_WIDTH) / TILE_WIDTH * TEX_WIDTH;
                int texY = fmod(sample_point_world.y, TILE_WIDTH) / TILE_WIDTH * TEX_HEIGHT;

                texX = texX < 0 ? TEX_WIDTH + texX : texX;
                texY = texY > 0 ? TEX_HEIGHT - texY : texY;

                Pixel color = pixel_from_uint(get_texture_pixel(is_ceiling ? 2 : 5, texX, texY));

                draw_POV(i, y, pixel_mulf(color, 1));
            }
            // Floor
            /*for (int y = half_r + 1; y < pov_rows; ++y) {
            draw_POV(i, y, pixel_mulf(floorcol, 1));
            }*/
        }


        //printf("rayDir: %f, %f\n", rayDirX, rayDirY);

        float posX = player_world.x;
        float posY = player_world.y;

        //which box of the map we're in
        int mapX = player_map.x;
        int mapY = player_map.y;

        //length of ray from current position to next x or y-side
        double sideDistX;
        double sideDistY;

        //length of ray from one x or y-side to next x or y-side
        //these are derived as:
        //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
        //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
        //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
        //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
        //unlike (dirX, dirY) is not 1, however this does not matter, only the
        //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
        //stepping further below works. So the values can be computed as below.
        // Division through zero is prevented, even though technically that's not
        // needed in C++ with IEEE 754 floating point values.
        double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1 / rayDirY);

        //double rayDirX = rayd.x;
        //double rayDirY = rayd.y;

        double perpWallDist;

        

        //what direction to step in x or y-direction (either +1 or -1)
        int stepX;
        int stepY;

        int side = 0; //was a NS or a EW wall hit?
        //calculate step and initial sideDist
        if (rayDirX < 0)
        {
          stepX = -1;
          sideDistX = (posX - mapX) * deltaDistX;
        } else
        {
          stepX = 1;
          sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (rayDirY < 0)
        {
          stepY = -1;
          sideDistY = (posY - mapY) * deltaDistY;
        } else
        {
          stepY = 1;
          sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        int tile = 0;
        //perform DDA
        while (tile == 0)
        {
          //jump to next map square, either in x-direction, or in y-direction
          if (sideDistX < sideDistY)
          {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
          } else
          {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
          }

          // Don't go out of map bounds
          if (mapX < 0 || mapY < 0 ||
            mapX >= GAME_MAP.dim || mapY >= GAME_MAP.dim) {
            break;
          }

          assert(mapX >= 0 && mapX < GAME_MAP.dim);
          assert(mapY >= 0 && mapY < GAME_MAP.dim);

          tile = GAME_MAP.map[mapX + GAME_MAP.dim * mapY];
          assert(tile < 9);


          // TODO: draw ray in top view somehow
          //draw_TV(rayp.x, rayp.y, raycol);

        }

        assert(tile < 9);

        if (!tile) {
          continue;
        }
        //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
        //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
        //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
        //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
        //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
        //steps, but we subtract deltaDist once because one step more into the wall was taken above.
        if (side == 0) {
          perpWallDist = (sideDistX - deltaDistX);
        } else {
          perpWallDist = (sideDistY - deltaDistY);
        }

        //int height = POV_ROWS * (1 - factor);
               //*/
        //int wall_height = (int)(pov_rows / perpWallDist);

        // How much of the tile fits in the screen by width
        float tile_screen_scale = TILE_WIDTH / plane_width;


        //float tile_screen_scale = 1.f;

      

        //float wall_height_scale = TILE_HEIGHT * tile_screen_scale / (1 + perpWallDist);
        //float wall_height_scale = TILE_HEIGHT / tile_screen_scale / perpWallDist;
        
        //int wall_height_rows = pov_rows * wall_height_scale;
        int wall_height_rows = pov_rows * tile_height / perpWallDist;
        //wall_height_rows = min(wall_height_rows, pov_rows);


        /*/

        int height = 1 / dist_to_wall * scr.POV_ROWS ;

        //*/

        

        //float brightness = wall_height_rows / (float)pov_rows;
        float brightness = 1;

        //wallcol = side > 0 ? RED : BLUE;
        
        float wallX;
        if(side == 0) {
            wallX = posY + rayDirY * perpWallDist;
        } else {
            wallX = posX + rayDirX * perpWallDist;
        }
        // Get the range [0, 1] inside the tile. Account for tile width.
        wallX = fmod(wallX, TILE_WIDTH) / TILE_WIDTH;

        // Local X coordinate in the texture
        int texX = wallX * TEX_WIDTH;

        if(
            side == 0 && rayDirX > 0 && 
            side == 1 && rayDirY < 0
        ) {
            texX = TEX_WIDTH - texX - 1;
        }

        

        // Draw wall

        /*textureBuffer_setColumn_color(
            &s_Pov_display.tex_buf, 
            i, 
            max(half_r - half_h, 0), 
            min(wall_height, pov_rows), 
            pixel_mulf(wallcol, brightness)
        );*/

        assert(tile > 0);

        // texture id starts from 0 while tile 0 means empty
        int texture = tile - 1;

        float wall_pov_difference_half = (wall_height_rows - pov_rows) / 2.f;

        /* Two cases:
        * 1. Wall is heigher or equal than the screen
        * 2. Wall is lower than the screen
        */
        float step = 1.f / wall_height_rows * TEX_WIDTH;


        if(wall_height_rows >= pov_rows) {
            for(int y = 0; y < pov_rows; ++y) {
                int texY = y + (int)wall_pov_difference_half;
                // Map to texture space
                texY = (float)texY * step;
                assert(texY >= 0 && texY < TEX_WIDTH);
                Pixel wallcol = pixel_from_uint(get_texture_pixel(texture, texX, texY));
                draw_POV(i, y, pixel_mulf(wallcol, brightness));
            }
        }

        if(pov_rows > wall_height_rows) {
            for(int y = -(int)wall_pov_difference_half; y < pov_rows - (int)ceilf(-wall_pov_difference_half); ++y) {
                int texY = y + wall_pov_difference_half;
                // Map to texture space
                texY = (float)texY * step;
                assert(texY >= 0 && texY < TEX_WIDTH);
                Pixel wallcol = pixel_from_uint(get_texture_pixel(texture, texX, texY));
                draw_POV(i, y, pixel_mulf(wallcol, brightness));
            }
        }

}


    // Cast a ray in the direction the player is moving to detect collision with wall
#if 0
    Vec2f player_move_dir = vec2f_normalized(player_delta_pos);

    Pixel collision_ray_col = CYAN;
    int collision_ray_len = stop_dist * 10;

    Vec2f rayd_coll = player_move_dir;

   

    //printf("Moving in dir (%f, %f)   %f\n", rayd_coll.x, rayd_coll.y, vec2f_magnitude(rayd_coll));

    bool hit = false;
    for (int j = 0; j < collision_ray_len && !hit; ++j) {

        Vec2f ray_dir_advanced = vec2f_muls(rayd_coll, j * ray_step);
        Vec2f rayp = vec2f_add(player_pos_minimap, ray_dir_advanced);

        if (rayp.x < 0.f || rayp.y < 0.f) {
            break;
        }

        draw_TV(rayp.x, rayp.y, collision_ray_col);


        Vec2f tilep = { tv_to_map_x(rayp.x), tv_to_map_y(rayp.y) };
        assert(tilep.x < 11 && tilep.y < 11);
        assert(tilep.x > -1 && tilep.y > -1);

        int tile_x = tilep.x;
        int tile_y = tilep.y;

        int tile = gameMap_get_tile(&GAME_MAP, tile_x, tile_y);
        hit = tile != 0;
        if (hit) {
            //drawTopView(rayp.x, rayp.y, wallcol);

            //int pov_y = POV_ROWS / 2;


            Vec2f player_to_wall = vec2f_sub(rayp, player_pos_minimap);
            float dist_to_wall = vec2f_magnitude(player_to_wall);
            if (dist_to_wall < stop_dist) { // stop
                player_stop = true;
            }
        }
    }

    if (player_stop) { // hit
        player_map = vec2f_sub(player_map, vec2f_muls(player_delta_pos, 1.1));
    }

#endif

#ifdef MINIMAP
    // Draw player
    drawPlayer(player_pos_minimap);

    drawNearestPlane(player_pos_minimap, player_look_dir);

    // Draw player look dir
    for (int i = 0; i < 10; ++i) {
        float ldx = player_pos_minimap.x + player_look_dir.x * i;
        float ldy = player_pos_minimap.y + player_look_dir.y * i;

        draw_TV(ldx, ldy, ldcol);
    }

#endif
}

static unsigned int VAO, VBO, EBO;
static unsigned int shaderProgram;

static int opengl_init()
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
    s_Window = glfwCreateWindow(s_Screen_resolution.x, s_Screen_resolution.y, "Raycaster", NULL, NULL);
    if (s_Window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(s_Window);
    glfwSetFramebufferSizeCallback(s_Window, glfw_framebuffer_size_callback);
    glfwSetCursorPosCallback(s_Window, glfw_cursor_position_callback);

    glfwSetInputMode(s_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    // top right
    // bottom right
    // bottom left
    // top left 

    // Flip U and V axes since the texture buffer hold values in column major top-to-bottom
    // bottom right becomes top left and vice versa

    float vertices[] = {
        // positions        // texture coords
         1.f,  1.f, 0.0f,   1.0f, 1.0f, 
         1.f, -1.f, 0.0f,   0.0f, 1.0f, // 1.0f, 0.0f, 
        -1.f, -1.f, 0.0f,   0.0f, 0.0f, 
        -1.f,  1.f, 0.0f,   1.0f, 0.0f  
    };

    unsigned int indices[] = {
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


void update(double delta_time, bool update_check)
{
    {
        s_Pov_display.tex_buf.updated_this_frame = false;
        s_Minimap_display.tex_buf.updated_this_frame = false;
    }

    gameLogic((float)delta_time, update_check);

    if (
        !update_check || 
        !s_Is_display_minimap && 
        s_Pov_display.tex_buf.updated_this_frame
    ) {
        textureBuffer_loadTexData(&s_Pov_display.tex_buf);
    } else if (
        //!update_check || 
        s_Is_display_minimap && 
        s_Minimap_display.tex_buf.updated_this_frame
    ) {
        textureBuffer_loadTexData(&s_Minimap_display.tex_buf);
    }

    // draw
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO); 
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int main(void)
{
    globals_Init();

    if (opengl_init() != 0) {
        return -1;
    }

    if (gameMap_init(&GAME_MAP, "maps/00.txt") != 0) {
        return -1;
    }


    for(int i = 0; i < GAME_MAP.num_textures; ++i) {
        if(i >= MAX_TEXTURES) {
            LOG_ERR("Max num textures exceeded %d >= %d", i, MAX_TEXTURES);
            break;
        }
        s_Textures[i] = malloc(TEX_WIDTH * TEX_HEIGHT * sizeof(*s_Textures));
    }

#define BRICK_hehe (x % 8 && y % 8)

    //generate some textures
    for(int y = 0; y < TEX_HEIGHT; ++y) {
        for(int x = 0; x < TEX_WIDTH; ++x) {
            int xorcolor = (x * 256 / TEX_WIDTH) ^ (y * 256 / TEX_HEIGHT);
            int ycolor = y * 256 / TEX_HEIGHT;
            int xycolor = y * 128 / TEX_HEIGHT + x * 128 / TEX_WIDTH;
            if(GAME_MAP.num_textures >= 8) {
                s_Textures[0][TEX_WIDTH * y + x] = 65536 * 254 * (x != y && x != TEX_WIDTH - y); //flat red texture with black cross
                s_Textures[1][TEX_WIDTH * y + x] = xycolor + 256 * xycolor + 65536 * xycolor; //sloped greyscale
                s_Textures[2][TEX_WIDTH * y + x] = 256 * xycolor + 65536 * xycolor; //sloped yellow gradient
                s_Textures[3][TEX_WIDTH * y + x] = xorcolor + 256 * xorcolor + 65536 * xorcolor; //xor greyscale
                s_Textures[4][TEX_WIDTH * y + x] = 256 * xorcolor; //xor green
                s_Textures[5][TEX_WIDTH * y + x] = 65536 * 192 * BRICK_hehe; //red bricks
                s_Textures[6][TEX_WIDTH * y + x] = 65536 * ycolor; //red gradient
                s_Textures[7][TEX_WIDTH * y + x] = 128 + 256 * 128 + 65536 * 128; //flat grey texture
                //s_Textures[8][TEX_WIDTH * y + x] = BRICK_hehe + 256 * BRICK_hehe + 65536 * BRICK_hehe; //grey bricks
                s_Textures[8][TEX_WIDTH * y + x] = 128 * BRICK_hehe + 256 * 128 * BRICK_hehe + 65536 * 128 * BRICK_hehe; //flat grey texture

            }
            for(int i = 8; i < GAME_MAP.num_textures; ++i) {
                s_Textures[i][TEX_WIDTH * y + x] = pixel_to_uint(MAGENTA);
            }
        }
    }

    update(0, false);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(s_Window))
    {
        static double delta_time;

        clock_t start = clock();

        update(delta_time, true);
        // glBindVertexArray(0); // no need to unbind it every time 

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(s_Window);
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
