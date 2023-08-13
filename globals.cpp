#include "globals.h"

IrrlichtDevice* device = nullptr;
s32 lighting_mat = -1;
s32 blend_mat = -1;
s32 shadow_mat = -1;
s32 rays_mat = -1;
s32 depth_sort_mat = -1;

io::path project_path = "/home/andrey/minetest_kitchen";
core::dimension2du wnd_size = core::dimension2du(1024, 680);

f32 far_plane = 30.0f;
bool move_light = false;
s32 current_rendered_light = -1;

s32 lights_count = 0;

array<shared_ptr<PointLight>, MAX_LIGHTS_COUNT> lights;

array<core::vector3df, 6> dirs = array<core::vector3df, 6>({
    core::vector3df(-1.0f, 0.0f, 0.0f),      // ECS_POSX
    core::vector3df(1.0f, 0.0f, 0.0f),     // ECS_NEGX
    core::vector3df(0.0f, -1.0f, 0.0f),      // ECS_POSY
    core::vector3df(0.0f, 1.0f, 0.0f),     // ECS_NEGY
    core::vector3df(0.0f, 0.0f, -1.0f),      // ECS_POSZ
    core::vector3df(0.0f, 0.0f, 1.0f)      // ECS_NEGZ
});

vector<scene::IMeshSceneNode*> shadow_casters;

scene::ICameraSceneNode* camera = nullptr;
