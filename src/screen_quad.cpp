#include "screen_quad.h"

ScreenQuad::ScreenQuad(video::IVideoDriver* driver) : mDriver(driver)
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

void ScreenQuad::Render() {
    const u16 indices[] = {0,1,2,3,1,2};

    mDriver->setMaterial(mat);
    mDriver->setTransform(video::ETS_PROJECTION, core::IdentityMatrix);
    mDriver->setTransform(video::ETS_VIEW, core::IdentityMatrix);
    mDriver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
    mDriver->drawIndexedTriangleList(mVertices,4,indices,2);
}
