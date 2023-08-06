#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <irrlicht.h>
#include <assert.h>

using namespace std;
using namespace irr;

#define MAX_LIGHTS_COUNT 10

IrrlichtDevice* device;
s32 lighting_mat;
s32 blend_mat;
s32 shadow_mat;
s32 rays_mat;
s32 depth_sort_mat;
//s32 depth_write_mat;
io::path project_path = "/home/andrey/minetest_kitchen";
core::dimension2du wnd_size(1024, 680);
f32 far_plane = 30.0f;
bool move_light = false;
s32 current_rendered_light = -1;

class PointLight;

s32 lights_count = 0;
array<shared_ptr<PointLight>, MAX_LIGHTS_COUNT> lights;
array<core::vector3df, 6> dirs = {
    core::vector3df(-1.0f, 0.0f, 0.0f),      // ECS_POSX
    core::vector3df(1.0f, 0.0f, 0.0f),     // ECS_NEGX
    core::vector3df(0.0f, -1.0f, 0.0f),      // ECS_POSY
    core::vector3df(0.0f, 1.0f, 0.0f),     // ECS_NEGY
    core::vector3df(0.0f, 0.0f, -1.0f),      // ECS_POSZ
    core::vector3df(0.0f, 0.0f, 1.0f)      // ECS_NEGZ
};
vector<scene::IMeshSceneNode*> shadow_casters;

void set_material_for_all(video::E_MATERIAL_TYPE mat_t)
{
    for (auto n_p : shadow_casters)
        n_p->setMaterialType(mat_t);
}

class PointLight
{
public:
    static bool addPointLight(scene::ISceneManager* smgr, const core::vector3df& spos, const video::SColorf& scolor, const f32& sintensity, const core::aabbox3df& sbbox)
    {
        shared_ptr<PointLight> pl(new PointLight);

        pl->pos = spos;
        pl->color = scolor;
        pl->intensity = sintensity;
        pl->bbox = sbbox;
        pl->smgr = smgr;
        pl->vdrv = smgr->getVideoDriver();

        pl->cam = smgr->addCameraSceneNode(0, pl->pos, pl->pos+core::vector3df(0.0f, 0.0f, 1.0f), -1, false);
        pl->cam->setFOV(core::PI/2.0f);
        pl->cam->setNearValue(1.0f);
        pl->cam->setFarValue(far_plane);
        pl->cam->setAspectRatio(1.0f);

        pl->shadow = pl->vdrv->addRenderTargetTextureCubemap(1024, "ShadowRT", video::ECF_A32B32G32R32F);
        pl->shadow_depth = pl->vdrv->addRenderTargetTexture(core::dimension2du(1024, 1024), "ShadowDepthRT", video::ECF_D32);

        string rt_tex_name = "RaysRT" + to_string(lights.size()+1);

        pl->rays = pl->vdrv->addRenderTargetTexture(wnd_size, rt_tex_name.c_str(), video::ECF_A32B32G32R32F);

        pl->is_position_changed = true;


        bool success_add = false;
        array<shared_ptr<PointLight>, MAX_LIGHTS_COUNT>::iterator iter = lights.begin();

        while (iter != lights.end())
        {
            if (!*iter)
            {
                *iter = pl;
                success_add = true;
                lights_count++;
                break;
            }

            iter++;
        }

        return success_add;
    }

    void setPosition(const core::vector3df& new_pos)
    {
        pos = new_pos;
        cam->setPosition(pos);
        is_position_changed = true;
    }
    void setColor(const video::SColorf& new_color)
    {
        color = new_color;
    }
    void setIntensity(f32 new_intensity)
    {
        intensity = new_intensity;
    }
    core::vector3df getPosition() const
    {
        return pos;
    }
    video::SColorf getColor() const
    {
        return color;
    }
    f32 getIntensity() const
    {
        return intensity;
    }
    core::aabbox3df getBox() const
    {
        return bbox;
    }
    video::ITexture* getShadowTexture() const
    {
        return shadow;
    }
    video::ITexture* getRaysTexture() const
    {
        return rays;
    }
    void setShadowCamera() const
    {
        smgr->setActiveCamera(cam);
    }
    void setRenderTexture(video::IRenderTarget* rt, video::E_CUBE_SURFACE surf) const
    {
        cam->setTarget(pos+dirs[surf]);
        rt->setTexture(shadow, shadow_depth, surf);
    }

     bool is_position_changed;
private:
    PointLight() = default;

    core::vector3df pos;
    video::SColorf color;
    f32 intensity;
    core::aabbox3df bbox; // in relative coords

    scene::ICameraSceneNode* cam;
    scene::ISceneManager* smgr;
    video::IVideoDriver* vdrv;

    video::ITexture* shadow;
    video::ITexture* shadow_depth;
    video::ITexture* rays;
};

class ScreenQuad {
public:
    ScreenQuad(video::IVideoDriver* driver) : mDriver(driver)
    {
        mVertices[0] = video::S3DVertex2TCoords(
            -1.0f,-1.0f,0.0f,
            0.0f,0.0f,-1.0f,
            video::SColor(255,255,255,255),
            0.0f,1.0f,
            0.0f,1.0f);

        mVertices[1] = video::S3DVertex2TCoords(
            1.0f,-1.0,0.0f,
            0.0f,0.0f,-1.0f,
            video::SColor(255,255,255,255),
            1.0f,1.0f,
            1.0f,1.0f);

        mVertices[2] = video::S3DVertex2TCoords(
            -1.0f,1.0,0.0f,
            0.0f,0.0f,-1.0f,
            video::SColor(255,255,255,255),
            0.0f,0.0f,
            0.0f,0.0f);

        mVertices[3] = video::S3DVertex2TCoords(
            1.0f,1.0f,0.0f,
            0.0f,0.0f,-1.0f,
            video::SColor(255,255,255,255),
            1.0f,0.0f,
            1.0f,0.0f);

        mat.Lighting = false;
        //mat.BlendOperation = video::EBO_ADD;
        mat.MaterialType = (video::E_MATERIAL_TYPE)blend_mat;
        mat.BackfaceCulling = false;
        mat.setFlag(video::EMF_BILINEAR_FILTER, false);
    }

    void Render() {
        const u16 indices[] = {0,1,2,3,1,2};

        mDriver->setMaterial(mat);
        mDriver->setTransform(video::ETS_PROJECTION, core::IdentityMatrix);
        mDriver->setTransform(video::ETS_VIEW, core::IdentityMatrix);
        mDriver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
        mDriver->drawIndexedTriangleList(mVertices,4,indices,2);
    }

    video::SMaterial mat;
private:
    video::IVideoDriver* mDriver;
    video::S3DVertex2TCoords mVertices[4];
};

scene::ICameraSceneNode* camera = nullptr;


struct PBRMaterial
{
    PBRMaterial() = default;

    PBRMaterial(f32 r, f32 m, f32 ao) : Roughness(r), Metallic(m), AO(ao)
    {}
    f32 Roughness = 0.0f;
    f32 Metallic = 0.0f;
    f32 AO = 0.0f;
};

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

class MyEventReceiver : public IEventReceiver {
public:
    virtual bool OnEvent(const SEvent& e)
    {
        if (e.EventType == EET_KEY_INPUT_EVENT)
        {
            if (e.KeyInput.Key == KEY_ESCAPE)
            {
                device->closeDevice();
                return true;
            }
            else if (e.KeyInput.Key == KEY_SPACE && e.KeyInput.PressedDown)
            {
                move_light = !move_light;
                return true;
            }
            else if (e.KeyInput.Key == KEY_PLUS && e.KeyInput.PressedDown)
            {
                auto pos = lights[0]->getPosition();
                pos.Y += 0.25f;
                lights[0]->setPosition(pos);
                return true;
            }
            else if (e.KeyInput.Key == KEY_MINUS && e.KeyInput.PressedDown)
            {
                auto pos = lights[0]->getPosition();
                pos.Y -= 0.25f;
                lights[0]->setPosition(pos);
                return true;
            }
            else if (e.KeyInput.Key == KEY_KEY_W && e.KeyInput.PressedDown)
            {
                auto pos = lights[0]->getPosition();
                pos.Z += 0.25f;
                lights[0]->setPosition(pos);
                return true;
            }
            else if (e.KeyInput.Key == KEY_KEY_S && e.KeyInput.PressedDown)
            {
                auto pos = lights[0]->getPosition();
                pos.Z -= 0.25f;
                lights[0]->setPosition(pos);
                return true;
            }
            else if (e.KeyInput.Key == KEY_KEY_A && e.KeyInput.PressedDown)
            {
                auto pos = lights[0]->getPosition();
                pos.X -= 0.25f;
                lights[0]->setPosition(pos);
                return true;
            }
            else if (e.KeyInput.Key == KEY_KEY_D && e.KeyInput.PressedDown)
            {
                auto pos = lights[0]->getPosition();
                pos.X += 0.25f;
                lights[0]->setPosition(pos);
                return true;
            }
        }

        return false;
    }
};

class MyShaderConstantCallback : public video::IShaderConstantSetCallBack
{
public:
    const video::SMaterial* mat = nullptr;

    virtual void OnSetMaterial(const video::SMaterial& set_mat) { mat = &set_mat; }

    virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userdata)
    {
        video::IVideoDriver* vdrv = device->getVideoDriver();

        if ((s32)mat->MaterialType == lighting_mat || (s32)mat->MaterialType == shadow_mat ||
                (s32)mat->MaterialType == rays_mat || (s32)mat->MaterialType == depth_sort_mat)
        {
            core::matrix4 model = vdrv->getTransform(video::ETS_WORLD);
            scene::ISceneManager* smgr = device->getSceneManager();
            scene::ICameraSceneNode* cur_cam = smgr->getActiveCamera();
            const core::matrix4& projection = cur_cam->getProjectionMatrix();
            const core::matrix4& view = cur_cam->getViewMatrix();

            services->setVertexShaderConstant("mModel", model.pointer(), 16);
            services->setVertexShaderConstant("mView", view.pointer(), 16);
            services->setVertexShaderConstant("mProjection", projection.pointer(), 16);
        }

        if ((s32)mat->MaterialType == lighting_mat)
        {
            services->setPixelShaderConstant("mLightsCount", &lights_count, 1);
            services->setPixelShaderConstant("mFarPlane", &far_plane, 1);

            scene::ICameraSceneNode* cur_cam = device->getSceneManager()->getActiveCamera();
            f32 curNear = cur_cam->getNearValue();
            f32 curFar = cur_cam->getFarValue();

            services->setPixelShaderConstant("mNear", &curNear, 1);
            services->setPixelShaderConstant("mFar", &curFar, 1);

            array<core::vector3df, MAX_LIGHTS_COUNT> lights_positions;
            array<video::SColorf, MAX_LIGHTS_COUNT> lights_colors;
            array<core::vector3df, MAX_LIGHTS_COUNT> lights_bbox_min;
            array<core::vector3df, MAX_LIGHTS_COUNT> lights_bbox_max;

            for (u16 i = 0; i < lights_count; i++)
            {
                lights_positions[i] = lights[i]->getPosition();
                lights_colors[i] = lights[i]->getColor();
                lights_colors[i].r *= lights[i]->getIntensity();
                lights_colors[i].g *= lights[i]->getIntensity();
                lights_colors[i].b *= lights[i]->getIntensity();

                lights_bbox_min[i] = lights[i]->getBox().MinEdge;
                lights_bbox_max[i] = lights[i]->getBox().MaxEdge;
            }


            services->setPixelShaderConstant("mLightsPositions[0]", reinterpret_cast<f32*>(lights_positions.data()), 3 * lights_count);
            services->setPixelShaderConstant("mLightsColors[0]", reinterpret_cast<f32*>(lights_colors.data()), 4 * lights_count);
            services->setPixelShaderConstant("mLightsBBoxMinEdges[0]", reinterpret_cast<f32*>(lights_bbox_min.data()), 3 * lights_count);
            services->setPixelShaderConstant("mLightsBBoxMaxEdges[0]", reinterpret_cast<f32*>(lights_bbox_max.data()), 3 * lights_count);


            core::vector3df cam_pos = camera->getAbsolutePosition();
            services->setPixelShaderConstant("mViewPos", reinterpret_cast<f32*>(&cam_pos), 3);

            f32 r = 0.0f;
            f32 m = 0.0f;
            f32 a = 0.0f;

            unpack_PBR_properties(mat->MaterialTypeParam, r, m, a);

            services->setPixelShaderConstant("mRoughness", &r, 1);
            services->setPixelShaderConstant("mMetallic", &m, 1);
            services->setPixelShaderConstant("mAO", &a, 1);

            s32 basetex_id = 0;
            s32 normaltex_id = 1;

            services->setPixelShaderConstant("mBaseTex", &basetex_id, 1);
            services->setPixelShaderConstant("mNormalTex", &normaltex_id, 1);

            for (u16 i = 0; i < MAX_LIGHTS_COUNT; i++)
            {
                s32 shadowtex_id = i+2;
                string c_name = "mDepthTex";
                c_name += to_string(shadowtex_id-1);
                services->setPixelShaderConstant(c_name.c_str(), &shadowtex_id, 1);
            }

        }
        else if ((s32)mat->MaterialType == shadow_mat)
        {
            if (current_rendered_light != -1)
            {
                auto pos = lights[current_rendered_light]->getPosition();
                services->setPixelShaderConstant("mLightPos", reinterpret_cast<f32*>(&pos), 3);
                services->setPixelShaderConstant("mFarPlane", &far_plane, 1);
            }
        }
        else if ((s32)mat->MaterialType == blend_mat)
        {
            s32 tex1 = 0;
            s32 tex2 = 1;

            services->setPixelShaderConstant("mBaseTex", &tex1, 1);

            for (u16 i =0; i<MAX_LIGHTS_COUNT;i++)
            {
                s32 raystex_id = i+1;
                string c_name = "mRaysTex";
                c_name += to_string(raystex_id);
                services->setPixelShaderConstant(c_name.c_str(), &raystex_id, 1);
            }
        }
        else if ((s32)mat->MaterialType == rays_mat)
        {
            s32 lights_tex = 0;
            s32 lights_ids_tex = 1;

            services->setPixelShaderConstant("mLightsTex", &lights_tex, 1);
            services->setPixelShaderConstant("mLightsIDsTex", &lights_ids_tex, 1);

            auto pos = lights[current_rendered_light]->getPosition();
            services->setVertexShaderConstant("mLightPos", reinterpret_cast<f32*>(&pos), 3);
        }
        else if ((s32)mat->MaterialType == depth_sort_mat)
        {
            s32 depth_tex = 0;

            services->setPixelShaderConstant("mDepthTex", &depth_tex, 1);
            services->setPixelShaderConstant("mScreenSize", reinterpret_cast<f32*>(&wnd_size), 2);

            video::SColorf lcolor = lights[current_rendered_light]->getColor();
            lcolor.r *= lights[current_rendered_light]->getIntensity();
            lcolor.g *= lights[current_rendered_light]->getIntensity();
            lcolor.b *= lights[current_rendered_light]->getIntensity();

            services->setPixelShaderConstant("mLightColor", reinterpret_cast<f32*>(&lcolor), 4);

            s32 biased_light_id = current_rendered_light+1;
            services->setPixelShaderConstant("mLightID", &biased_light_id, 1);
        }
    }
};

bool compileShaders(video::IShaderConstantSetCallBack* callback=nullptr)
{
    video::IVideoDriver* vdrv = device->getVideoDriver();
    video::IGPUProgrammingServices* gpu = vdrv->getGPUProgrammingServices();

    lighting_mat = gpu->addHighLevelShaderMaterialFromFiles(project_path + "/lighting.vert", "main", video::EVST_VS_1_1,
        project_path + "/lighting.frag", "main", video::EPST_PS_1_1, callback, video::EMT_TRANSPARENT_ALPHA_CHANNEL, 0);

    blend_mat = gpu->addHighLevelShaderMaterialFromFiles(project_path + "/blur.vert", "main", video::EVST_VS_1_1,
        project_path + "/blend.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    shadow_mat = gpu->addHighLevelShaderMaterialFromFiles(project_path + "/lighting.vert", "main", video::EVST_VS_1_1,
        project_path + "/shadow_write.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    rays_mat = gpu->addHighLevelShaderMaterialFromFiles(project_path + "/rays.vert", "main", video::EVST_VS_1_1,
        project_path + "/rays.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

    depth_sort_mat = gpu->addHighLevelShaderMaterialFromFiles(project_path + "/lighting.vert", "main", video::EVST_VS_1_1,
        project_path + "/depth_sort.frag", "main", video::EPST_PS_1_1, callback, video::EMT_SOLID, 0);

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

    scene::IMesh* n_mesh = smgr->getMesh(project_path + "/" + mesh_p);
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

        io::path t_path = io::path("/") + texs_paths[i];
        n_mat_copy.setTexture(0, vdrv->getTexture(project_path + t_path));

        if (use_normalmap[i])
        {
            //cout << "normal map is generating " << i << endl;
            io::path filename;
            io::path ext;
            core::cutFilenameExtension(filename, t_path);
            core::getFileNameExtension(ext, t_path);
            video::ITexture* nm = vdrv->getTexture(project_path + filename + "_nm" + ext);
            vdrv->makeNormalMapTexture(nm, pbrs[i].Roughness);
            n_mat_copy.setTexture(1, nm);
        }

        n->getMaterial(i) = n_mat_copy;
    }

    shadow_casters.push_back(n);
    return n;
}

int main()
{
    device = createDevice(video::EDT_OPENGL, wnd_size, 32, false, true, true);

    MyEventReceiver receiver;
    device->setEventReceiver(&receiver);

    MyShaderConstantCallback callback;

    // Compile all necessary shaders
    bool success = compileShaders(&callback);

    if (!success)
        cerr << "Shaders failed to compile!" << endl;

    video::IVideoDriver* vdrv = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();

    // Add two point lights
    core::aabbox3df light_box(-0.25f, -0.25f, -0.25f, 0.25f, 0.25f, 0.25f);
    PointLight::addPointLight(smgr, core::vector3df(7.0f, 2.5f, 5.0f), video::SColorf(1.0, 1.0, 1.0, 1.0), 1.0f, light_box);
    PointLight::addPointLight(smgr, core::vector3df(5.0f, 1.5f, 9.0f), video::SColorf(1.0, 1.0, 1.0, 1.0), 1.0f, light_box);
    PointLight::addPointLight(smgr, core::vector3df(-8.0f, 7.0f, 7.0f), video::SColorf(1.0, 1.0, 1.0, 1.0), 1.0f, light_box);


    // Add the table node with two "lighting_mat" materials
    auto room = addNode(
        core::vector3df(0.0f),
        {PBRMaterial(1.0f, 1.0f, 0.3f), PBRMaterial(0.2f, 1.0f, 0.7f), PBRMaterial(1.0f, 0.5f, 0.3f)},
        "room.b3d",
        {"wood.png", "multidecor_jungle_linoleum.png", "basic_materials_concrete_block.png"},
        {true, true, true}
    );

    auto table = addNode(
        core::vector3df(0.0f),
        {PBRMaterial(0.05f, 1.0f, 0.2f), PBRMaterial(1.0f, 1.0f, 0.3f)},
        "multidecor_round_metallic_table.b3d",
        {"metal.png", "wood.png"},
        {true, true}
    );

    auto chair = addNode(
        core::vector3df(0.0f, 0.0f, 2.5f),
        {PBRMaterial(1.0f, 1.0f, 0.3f)},
        "chairblend.b3d",
        {"wood.png"},
        {true}
    );

    auto vase = addNode(
        core::vector3df(-0.3f, 2.25f, -0.5f),
        {PBRMaterial(0.1f, 1.0f, 0.1f)},
        "cup.b3d",
        {"glass.png"},
        {false}
    );

    auto plate = addNode(
        core::vector3df(0.0f, 2.25f, 0.5f),
        {PBRMaterial(0.7f, 1.0f, 0.4f), PBRMaterial(0.05f, 1.0f, 0.2f), PBRMaterial(1.0f, 1.0f, 0.3f)},
        "plate_with_knife_and_fork.b3d",
        {"multidecor_porcelain_material.png", "metal.png", "wood.png"},
        {true, true, true}
    );

    auto wardrobe = addNode(
        core::vector3df(-5.0f, 0.0f, 5.0f),
        {
            PBRMaterial(1.0f, 0.5f, 0.3f),
            PBRMaterial(0.05f, 1.0f, 0.2f),
            PBRMaterial(1.0f, 0.5f, 0.3f),
            PBRMaterial(0.1f, 1.0f, 0.1f),
            PBRMaterial(0.05f, 1.0f, 0.2f),
            PBRMaterial(1.0f, 0.5f, 0.3f)
        },
        "wardrobe.b3d",
        {"multidecor_jungle_wood.png", "metal.png", "multidecor_jungle_wood.png", "glass.png", "metal.png", "multidecor_jungle_wood.png"},
        {true, true, true, false, true, true}
    );

    auto painting = addNode(
        core::vector3df(9.0f, 6.5f, -2.5f),
        {PBRMaterial(0.3f, 1.0f, 0.4f), PBRMaterial(0.5f, 0.8f, 0.5f)},
        "painting.b3d",
        {"metal.png", "multidecor_image_tropic.png"},
        {true, true}
    );

    auto armchair = addNode(
        core::vector3df(3.0f, 0.0f, 6.0f),
        {PBRMaterial(1.0f, 1.0f, 0.3f), PBRMaterial(1.0f, 0.5f, 0.4f), PBRMaterial(1.0f, 0.5f, 0.4f)},
        "armchair.b3d",
        {"wood.png", "multidecor_wool_material.png", "multidecor_wool_material.png"},
        {true, true, true}
    );


    // Add skybox and main camera
    vdrv->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

    video::ITexture* tiles = vdrv->getTexture(project_path + "/junglewood.png");
    scene::ISceneNode* skybox = smgr->addSkyBoxSceneNode(tiles, tiles, tiles, tiles, tiles, tiles);
    skybox->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);

    vdrv->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, true);

    camera = smgr->addCameraSceneNodeFPS(0, 100.0f, 0.01f);
    camera->setPosition(core::vector3df(0.0f, 0.0f, -5.0f));
    camera->setTarget(core::vector3df(0.0f));

    video::IRenderTarget* rt = vdrv->addRenderTarget();

    video::ITexture* base_t = vdrv->addRenderTargetTexture(wnd_size, "BaseRT", video::ECF_A16B16G16R16F);
    video::ITexture* base_depth_t = vdrv->addRenderTargetTexture(wnd_size, "BaseDepthRT", video::ECF_A16B16G16R16F);

    core::array<video::ITexture*> base_texs;
    base_texs.push_back(base_t);
    base_texs.push_back(base_depth_t);

    video::ITexture* lights_t = vdrv->addRenderTargetTexture(wnd_size, "LightsRT", video::ECF_A16B16G16R16F);
    video::ITexture* depth_t = vdrv->addRenderTargetTexture(wnd_size, "DepthRT", video::ECF_D16);


    ScreenQuad bloom_quad(vdrv);
    ScreenQuad base_quad(vdrv);

    base_quad.mat.setTexture(0, base_t);

    for (u16 i = 0; i < lights_count; i++)
        base_quad.mat.setTexture(i+1, lights[i]->getRaysTexture());

    f32 passed_dist = 0.0f;
    f32 max_dist = 7.0f;
    f32 speed = 0.1f;
    f32 fixed_time = 0.0f;
    bool is_first_iter = true;
    f32 up = 1.0f;

    while(device->run())
    {
        if (device->isWindowActive())
        {
            std::wstring name = L"Minetest Room App | FPS: ";
            std::wstring str = name + std::to_wstring(vdrv->getFPS()) + L" Dist: " + std::to_wstring(passed_dist);

            device->setWindowCaption(str.c_str());

            if (move_light)
            {
            if (is_first_iter)
            {
                fixed_time = device->getTimer()->getTime();
                is_first_iter = false;
            }
            else
            {
                f32 cur_time = device->getTimer()->getTime();
                f32 delta = cur_time - fixed_time;

                fixed_time = cur_time;

                f32 shift = speed * delta/1000.0f;

                auto pos = lights[0]->getPosition();
                if (passed_dist + shift > max_dist)
                {
                    pos.Y += (max_dist - passed_dist) * up;
                    passed_dist = 0.0f;
                    up = -up;
                }
                else
                {
                    pos.Y += shift * up;
                    passed_dist += shift;
                }

                lights[0]->setPosition(pos);
            }
            }
            else
                is_first_iter = true;

            vdrv->beginScene(video::ECBF_ALL);

            vdrv->setTransform(video::ETS_WORLD, core::IdentityMatrix);

            //smgr->setActiveCamera(lights[0]->getCamera());
            /*video::SMaterial& mat1 = table->getMaterial(0);
            mat1.BackfaceCulling = !mat1.BackfaceCulling;
            mat1.FrontfaceCulling = !mat1.FrontfaceCulling;
            video::SMaterial& mat2 = table->getMaterial(1);
            mat2.BackfaceCulling = !mat2.BackfaceCulling;
            mat2.FrontfaceCulling = !mat2.FrontfaceCulling;*/


            // Render point shadows into the cube shadowmap. It will render only if the light position is changed from last frame.

            set_material_for_all((video::E_MATERIAL_TYPE)shadow_mat);
            skybox->setVisible(false);

            for (u16 l_k = 0; l_k < lights_count; l_k++)
            {
                if (lights[l_k]->is_position_changed)
                {
                    cout << "rendering shadow_map for light source " << l_k << endl;
                    lights[l_k]->is_position_changed = false;
                    current_rendered_light = l_k;
                    lights[l_k]->setShadowCamera();

                    for (u16 i = 0; i < 6; i++)
                    {
                        lights[l_k]->setRenderTexture(rt, (video::E_CUBE_SURFACE)i);

                        vdrv->setRenderTargetEx(rt, video::ECBF_ALL);

                        smgr->drawAll();
                    }
                }
            }

            current_rendered_light = -1;

            // Render non-glowing nodes into the color and then depth textures

            smgr->setActiveCamera(camera);
            rt->setTexture(base_texs, depth_t);
            vdrv->setRenderTargetEx(rt, video::ECBF_ALL);

            set_material_for_all((video::E_MATERIAL_TYPE)lighting_mat);
            skybox->setVisible(true);

            smgr->drawAll();

            //for (u16 i = 0; i < lights_count; i++)
             //   drawCubeLight(lights[i], nullptr, true);

            // Render all glowing nodes culled by other nodes or each other into the lights texture

            bloom_quad.mat.setTexture(0, lights_t);
            bloom_quad.mat.MaterialType = (video::E_MATERIAL_TYPE)rays_mat;

            for (u16 i = 0; i < lights_count; i++)
            {
                current_rendered_light = (s32)i;
                rt->setTexture(lights_t, depth_t);
                vdrv->setRenderTargetEx(rt, video::ECBF_ALL);

                drawCubeLight(lights[i], base_depth_t, false);

                //rt->setTexture(base_texs[0], depth_t);
                //vdrv->draw2DImage(lights_t, core::recti(0, 0, wnd_size.Width, wnd_size.Height), core::recti(0, 0, wnd_size.Width, wnd_size.Height));

                rt->setTexture(lights[i]->getRaysTexture(), depth_t);
                vdrv->setRenderTargetEx(rt, video::ECBF_ALL);
                bloom_quad.Render();
            }

            current_rendered_light = -1;

            vdrv->setRenderTargetEx(0, video::ECBF_NONE);
             //vdrv->draw2DImage(lights[1]->getRaysTexture(), core::recti(0, 0, wnd_size.Width, wnd_size.Height), core::recti(0, 0, wnd_size.Width, wnd_size.Height));
            //vdrv->draw2DImage(light_texs[0], core::recti(0, 0, wnd_size.Width, wnd_size.Height), core::recti(0, 0, wnd_size.Width, wnd_size.Height));

            base_quad.Render();

            vdrv->endScene();
        }
    }


    return 0;
}
