#ifndef CONSTANT_SETTER_H
#define CONSTANT_SETTER_H

#endif // CONSTANT_SETTER_H

#include "globals.h"

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
