#pragma once

#include "globals.h"

void set_material_for_all(video::E_MATERIAL_TYPE mat_t);


struct PBRMaterial
{
    PBRMaterial() = default;

    PBRMaterial(f32 r, f32 m, f32 ao) : Roughness(r), Metallic(m), AO(ao)
    {}
    f32 Roughness = 0.0f;
    f32 Metallic = 0.0f;
    f32 AO = 0.0f;
};

f32 pack_PBR_properties(const PBRMaterial& pbr);

void unpack_PBR_properties(f32 res, f32& r, f32& m, f32& ao);

bool compileShaders(video::IShaderConstantSetCallBack* callback=nullptr);

void drawCubeLight(shared_ptr<PointLight> pl, video::ITexture* depth_t, bool solid);

scene::IMeshSceneNode* addNode(
        const core::vector3df& pos,
        const std::vector<PBRMaterial>& pbrs,
        io::path mesh_p,
        const std::vector<io::path>& texs_paths,
        std::vector<bool> use_normalmap
);
