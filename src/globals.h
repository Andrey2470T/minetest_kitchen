#pragma once

#define MAX_LIGHTS_COUNT 10

#include <irrlicht.h>
#include <iostream>
#include <memory>
#include <array>
#include <vector>
#include <assert.h>

using namespace std;
using namespace irr;

class PointLight;

extern IrrlichtDevice* device;
extern s32 lighting_mat;
extern s32 blend_mat;
extern s32 shadow_mat;
extern s32 rays_mat;
extern s32 depth_sort_mat;

extern io::path project_path;
extern core::dimension2du wnd_size;
extern f32 far_plane;
extern bool move_light;
extern s32 current_rendered_light;
extern s32 lights_count;

extern array<shared_ptr<PointLight>, MAX_LIGHTS_COUNT> lights;
extern array<core::vector3df, 6> dirs;
extern vector<scene::IMeshSceneNode*> shadow_casters;

extern scene::ICameraSceneNode* camera;
