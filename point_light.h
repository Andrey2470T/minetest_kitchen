#pragma once

#include "globals.h"

using namespace std;
using namespace irr;

class PointLight
{
public:
    static bool addPointLight(scene::ISceneManager* smgr, const core::vector3df& spos, const video::SColorf& scolor, const f32& sintensity, const core::aabbox3df& sbbox);

    void setPosition(const core::vector3df& new_pos);

    void setColor(const video::SColorf& new_color);

    void setIntensity(f32 new_intensity);

    core::vector3df getPosition() const;

    video::SColorf getColor() const;

    f32 getIntensity() const;

    core::aabbox3df getBox() const;

    video::ITexture* getShadowTexture() const;

    video::ITexture* getRaysTexture() const;

    void setShadowCamera() const;

    void setRenderTexture(video::IRenderTarget* rt, video::E_CUBE_SURFACE surf) const;


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
