#define OLC_PGE_APPLICATION
#include "app.h"
#include "vec2.h"

static const int MAP_DIM = 5;

static int MAP[MAP_DIM * MAP_DIM] = {
    1, 0, 0, 0, 1,
    1, 1, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 1, 1,
    1, 1, 0, 0, 0
};

static int TILE_SIZE = 1;



static const int TILE_STEPS = 10;
static const int NUM_STEPS = TILE_STEPS * MAP_DIM;
static const int STEP = TILE_SIZE / TILE_STEPS;


static float dist_to_near_plane = 0.2; // equals to half plane width means 45 degree FOV
static float near_plane_width = 0.4;

static const float stop_dist = 0.1;

static const float mov_speed = 1;

static vec2 player = {2, 3};

static vec2 player_look_dir = {0, 1};

bool Application::OnUserCreate()
{
    vec2 v1(10, 12);
    vec2 v2(2 , 3 );

    vec2 sum = v1 + v2;

    std::cout << sum << '\n';

    return true;
}

#define pix_x(x) ((x) / MAP_DIM * COLS)
#define pix_y(y) ((y) / MAP_DIM * ROWS)

#define map_x(x) ((x) / (float)COLS * MAP_DIM)
#define map_y(y) ((y) / (float)ROWS * MAP_DIM)

bool Application::OnUserUpdate(float fElapsedTime)
{
    float delta_x = 0; 
    float delta_y = 0;

    if (GetKey(olc::Key::LEFT).bHeld || GetKey(olc::Key::A).bHeld) {
        delta_x -= mov_speed * fElapsedTime;
    }
	if (GetKey(olc::Key::RIGHT).bHeld || GetKey(olc::Key::D).bHeld) {
        delta_x += mov_speed * fElapsedTime;
    }
    
    if (GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld) {
        delta_y -= mov_speed * fElapsedTime;
    }
	if (GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld) {
        delta_y += mov_speed * fElapsedTime;
    }

    player.x += delta_x;
    player.y += delta_y;

    std::cout << "Mouse X: " << GetMouseX() << '\n';
    std::cout << "Mouse Y: " << GetMouseY() << '\n';

    std::cout << "Player: " << player << '\n';

    vec2 vpy = { pix_x(player.x), pix_x(player.y)};

    player_look_dir = {GetMouseX() - vpy.x, GetMouseY() - vpy.y};
    player_look_dir = player_look_dir.normalized();

    Clear(olc::DARK_BLUE);

    olc::Pixel pcol    = olc::Pixel(255, 0, 0);
    olc::Pixel ldcol   = olc::Pixel(255, 255, 0);
    olc::Pixel plcol   = olc::Pixel(255, 0, 255);
    olc::Pixel raycol  = olc::Pixel(0, 255, 0);
    olc::Pixel tilecol = olc::Pixel(255, 0, 0);

    vec2 plane_dir = player_look_dir.perpendicular();
    // Draw near clipping plane
    int plane_width = 12;
    int plane_dist = 5;

    int supersampling = 16;
    int ray_len = 300;
    float ray_step = 0.1;
    float stop_dist = 0.1;
    for (int i = -plane_width/2; i < plane_width/2+1; ++i) {
        vec2 vpl = vec2(vpy.x, vpy.y) + player_look_dir * (plane_dist) + plane_dir * i;

        Draw(vpl.x, vpl.y, plcol);

        if (i == plane_width/2+1) {
            supersampling = 1;
        }
        for (int ss = 0; ss < supersampling; ++ss) {
            vpl += plane_dir * ((float)ss / supersampling);

            vec2 rayd = vpl - vpy;
            bool hit = false;
            for (int j = 0; j < ray_len && !hit; ++j) {
                vec2 rayp = vpy + rayd * j * ray_step;
                Draw(rayp.x, rayp.y, raycol);

                vec2 tilep = { map_x(rayp.x), map_y(rayp.y) };
                
                int tile = MAP[(int)tilep.x + MAP_DIM * (int)tilep.y];
                hit = tile != 0;
                if (hit) { // hit
                    Draw(rayp.x, rayp.y, tilecol);

                    vec2 dist_to_wall = rayp - vpy;
                    if (dist_to_wall.magnitude() < stop_dist) { // stop
                        player.x -= delta_x;
                        player.y -= delta_y;
                    }
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


#if 0
    for (int c = 0; c < COLS; ++c) {
        vec2 origin = player;
        vec2 target = origin;
        target.y += dist_to_near_plane;
        target.x += c * near_plane_width;

        vec2 dir = target - origin;
        dir = dir.normalized();

        vec2 rayp = origin;
        bool tile_hit = false;
        for (int s = 0; s < NUM_STEPS && !tile_hit; ++s) {
            rayp += dir * STEP;

            int current_tile = MAP[(int)rayp.x + MAP_DIM * (int)rayp.y];
            if (current_tile != 0) {
                tile_hit = true;
                for (int r = 0; r < ROWS; ++r) {
                    Draw(c, r, olc::Pixel(255, 0, 0));
                }
            }
        }
    }
#endif    

    return true;
}
