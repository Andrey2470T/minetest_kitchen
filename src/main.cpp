#include "point_light.h"
#include "screen_quad.h"
#include "functions.h"

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
            s32 base_tex = 0;
            s32 light_tex = 1;

            services->setPixelShaderConstant("mBaseTex", &base_tex, 1);
            services->setPixelShaderConstant("mLightTex", &light_tex, 1);

            auto light = lights[current_rendered_light];
            auto pos = light->getPosition();
            auto color = light->getColor();

            services->setPixelShaderConstant("mLightColor", reinterpret_cast<f32*>(&color), 4);
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



int main()
{
    device = createDevice(video::EDT_OPENGL, wnd_size, 32, false, true, true);

    MyEventReceiver receiver;
    device->setEventReceiver(&receiver);

    MyShaderConstantCallback callback;

    project_path = device->getFileSystem()->getAbsolutePath("minetest_kitchen");
    project_path = project_path.subString(0, project_path.findLast('/'));
    project_path = project_path.subString(0, project_path.findLast('/')) + "/minetest_kitchen";

    cout << "project_path: " << project_path.c_str() << endl;

    // Compile all necessary shaders
    bool success = compileShaders(&callback);

    if (!success)
        cerr << "Shaders failed to compile!" << endl;

    video::IVideoDriver* vdrv = device->getVideoDriver();
    scene::ISceneManager* smgr = device->getSceneManager();

    // Add two point lights
    core::aabbox3df light_box(-0.25f, -0.25f, -0.25f, 0.25f, 0.25f, 0.25f);
    PointLight::addPointLight(smgr, core::vector3df(7.0f, 2.5f, 5.0f), video::SColorf(1.0, 0.0, 0.0, 1.0), 1.0f, light_box);
    PointLight::addPointLight(smgr, core::vector3df(5.0f, 1.5f, 9.0f), video::SColorf(0.0, 1.0, 0.0, 1.0), 1.0f, light_box);
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

    video::ITexture* tiles = vdrv->getTexture(project_path + "/media/junglewood.png");
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

    video::ITexture* final_t = vdrv->addRenderTargetTexture(wnd_size, "FinalRT", video::ECF_A16B16G16R16F);


    ScreenQuad bloom_quad(vdrv);
    ScreenQuad base_quad(vdrv);

    //bloom_quad.mat.setTexture(0, base_t);
    bloom_quad.mat.setTexture(1, lights_t);

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


            bloom_quad.mat.MaterialType = (video::E_MATERIAL_TYPE)rays_mat;

            bool render_to_final_t = true;

            for (u16 i = 0; i < lights_count; i++)
            {
                bloom_quad.mat.setTexture(0, render_to_final_t ? base_t : final_t);

                current_rendered_light = (s32)i;
                rt->setTexture(lights_t, depth_t);
                vdrv->setRenderTargetEx(rt, video::ECBF_ALL);

                drawCubeLight(lights[i], base_depth_t, false);

                rt->setTexture(render_to_final_t ? final_t : base_t, depth_t);

                vdrv->setRenderTargetEx(rt, video::ECBF_ALL);
                bloom_quad.Render();

                render_to_final_t = !render_to_final_t;
            }

            current_rendered_light = -1;

            vdrv->setRenderTargetEx(0, video::ECBF_NONE);
            vdrv->draw2DImage(lights_count % 2 == 0 ? base_t : final_t, core::recti(0, 0, wnd_size.Width, wnd_size.Height), core::recti(0, 0, wnd_size.Width, wnd_size.Height));

            //base_quad.Render();

            vdrv->endScene();
        }
    }


    return 0;
}
