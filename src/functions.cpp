#include "functions.h"
#include "point_light.h"


void set_material_for_all(video::E_MATERIAL_TYPE mat_t)
{
    for (auto n_p : shadow_casters)
        n_p->setMaterialType(mat_t);
}


f32 pack_PBR_properties(const PBRMaterial& pbr)
{
    u32 r = pbr.Roughness * 100;
    u32 m = pbr.Metallic * 100;
    u32 ao = pbr.AO * 100;

    u32 res = 0;
    res |= r;
    res <<= 7;
    res |= m;
    res <<= 7;
    res |= ao;

    return f32(res);
}

void unpack_PBR_properties(f32 res, f32& r, f32& m, f32& ao)
{
    u32 res_u = u32(res);

    ao = f32(res_u & 0b00000000000000000000000001111111) / 100;
    res_u >>= 7;
    m = f32(res_u & 0b00000000000000000000000001111111) / 100;
    res_u >>= 7;
    r = f32(res_u & 0b00000000000000000000000001111111) / 100;
}

bool compileShaders(video::IShaderConstantSetCallBack* callback)
{
    video::IVideoDriver* vdrv = device->getVideoDriver();
    video::IGPUProgrammingServices* gpu = vdrv->getGPUProgrammingServices();

    io::path shaders_path = project_path + "/shaders";

    lighting_mat = gpu->addHighLevelShaderMaterialFromFiles(shaders_path + "/lighting.vert", "main", video::EVST_VS_1_1,
        shaders_path + "/lighting.frag", "main", video::EPST_PS_1_1, callback, video::EMT_TRANSPARENT_ALPHA_CHANNEL, 0);

    blend_mat = gpu->addHighLevelShaderMaterialFromFiles(shaders_path + "/blur.vert", "main", video::EVST_VS_1_1,
        shaders_path + "/blend.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    shadow_mat = gpu->addHighLevelShaderMaterialFromFiles(shaders_path + "/lighting.vert", "main", video::EVST_VS_1_1,
        shaders_path + "/shadow_write.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    rays_mat = gpu->addHighLevelShaderMaterialFromFiles(shaders_path + "/rays.vert", "main", video::EVST_VS_1_1,
        shaders_path + "/rays.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    depth_sort_mat = gpu->addHighLevelShaderMaterialFromFiles(shaders_path + "/lighting.vert", "main", video::EVST_VS_1_1,
        shaders_path + "/depth_sort.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    return lighting_mat != 0 && blend_mat != 0 && shadow_mat != 0 && rays_mat != 0 && depth_sort_mat != 0;
}

void drawCubeLight(shared_ptr<PointLight> pl, video::ITexture* depth_t, bool solid)
{
    // light_id starts from 0 !

    video::IVideoDriver* vdrv = device->getVideoDriver();

    core::vector3df normal(1.0f, 0.0f, 0.0f);
    core::vector2df texcoords(0.0f, 0.0f);

    auto box = pl->getBox();
    box.repair();

    video::SColor u32_color = pl->getColor().toSColor();
    video::S3DVertex vertices[8] = {
        video::S3DVertex(box.MinEdge, normal, u32_color, texcoords),
        video::S3DVertex(core::vector3df(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z), normal, u32_color, texcoords),
        video::S3DVertex(core::vector3df(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), normal, u32_color, texcoords),
        video::S3DVertex(core::vector3df(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z), normal, u32_color, texcoords),
        video::S3DVertex(core::vector3df(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z), normal, u32_color, texcoords),
        video::S3DVertex(core::vector3df(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), normal, u32_color, texcoords),
        video::S3DVertex(box.MaxEdge, normal, u32_color, texcoords),
        video::S3DVertex(core::vector3df(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), normal, u32_color, texcoords)
    };

    u16 indices[] = {1, 0, 3, 2,   0, 1, 5, 4,   1, 2, 6, 5,   2, 3, 7, 6,   3, 0, 4, 7,   4, 5, 6, 7};

    core::matrix4 trans_mat;
    trans_mat.setTranslation(pl->getPosition());

    vdrv->setTransform(video::ETS_WORLD, trans_mat);

    video::SMaterial def_mat;
    def_mat.Lighting = false;
    def_mat.MaterialType = solid ?  video::EMT_SOLID : (video::E_MATERIAL_TYPE)depth_sort_mat;
    def_mat.setFlag(video::EMF_BILINEAR_FILTER, false);
    def_mat.setTexture(0, depth_t);
    vdrv->setMaterial(def_mat);

    vdrv->drawVertexPrimitiveList(vertices, 8, indices, 6, video::EVT_STANDARD, scene::EPT_QUADS, video::EIT_16BIT);
}

scene::IMeshSceneNode* addNode(
        const core::vector3df& pos,
        const std::vector<PBRMaterial>& pbrs,
        io::path mesh_p,
        const std::vector<io::path>& texs_paths,
        std::vector<bool> use_normalmap)
{
    scene::ISceneManager* smgr = device->getSceneManager();
    video::IVideoDriver* vdrv = smgr->getVideoDriver();

    io::path media_path = project_path + "/media/";

    scene::IMesh* n_mesh = smgr->getMesh(media_path + mesh_p);
    n_mesh = smgr->getMeshManipulator()->createMeshWithTangents(n_mesh);

    scene::IMeshSceneNode* n = smgr->addMeshSceneNode(n_mesh);
    n->setPosition(pos);

    assert(pbrs.size() >= n->getMaterialCount() || texs_paths.size() >= n->getMaterialCount());

    video::SMaterial n_mat;
    n_mat.Lighting = false;
    n_mat.NormalizeNormals = true;
    n_mat.BackfaceCulling = false;
    n_mat.setFlag(video::EMF_BILINEAR_FILTER, false);
    n_mat.setFlag(video::EMF_TEXTURE_WRAP, 0);
    n_mat.MaterialType = (video::E_MATERIAL_TYPE)lighting_mat;

    for (u16 i = 0; i < lights_count; i++)
        n_mat.setTexture(i+2, lights[i]->getShadowTexture());

    for (u16 i = 0; i < n->getMaterialCount(); i++)
    {
        auto n_mat_copy(n_mat);

        n_mat_copy.MaterialTypeParam = pack_PBR_properties(pbrs[i]);

        io::path t_path = texs_paths[i];
        n_mat_copy.setTexture(0, vdrv->getTexture(media_path + t_path));

        if (use_normalmap[i])
        {
            //cout << "normal map is generating " << i << endl;
            io::path filename;
            io::path ext;
            core::cutFilenameExtension(filename, t_path);
            core::getFileNameExtension(ext, t_path);
            video::ITexture* nm = vdrv->getTexture(media_path + filename + "_nm" + ext);
            vdrv->makeNormalMapTexture(nm, pbrs[i].Roughness);
            n_mat_copy.setTexture(1, nm);
        }

        n->getMaterial(i) = n_mat_copy;
    }

    shadow_casters.push_back(n);
    return n;
}
