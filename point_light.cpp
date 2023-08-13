#include "point_light.h"

bool PointLight::addPointLight(scene::ISceneManager* smgr, const core::vector3df& spos, const video::SColorf& scolor, const f32& sintensity, const core::aabbox3df& sbbox)
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

void PointLight::setPosition(const core::vector3df& new_pos)
{
    pos = new_pos;
    cam->setPosition(pos);
    is_position_changed = true;
}

void PointLight::setColor(const video::SColorf& new_color)
{
    color = new_color;
}

void PointLight::setIntensity(f32 new_intensity)
{
    intensity = new_intensity;
}

core::vector3df PointLight::getPosition() const
{
    return pos;
}

video::SColorf PointLight::getColor() const
{
    return color;
}

f32 PointLight::getIntensity() const
{
    return intensity;
}

core::aabbox3df PointLight::getBox() const
{
    return bbox;
}

video::ITexture* PointLight::getShadowTexture() const
{
    return shadow;
}

video::ITexture* PointLight::getRaysTexture() const
{
    return rays;
}

void PointLight::setShadowCamera() const
{
    smgr->setActiveCamera(cam);
}

void PointLight::setRenderTexture(video::IRenderTarget* rt, video::E_CUBE_SURFACE surf) const
{
    cam->setTarget(pos+dirs[surf]);
    rt->setTexture(shadow, shadow_depth, surf);
}

