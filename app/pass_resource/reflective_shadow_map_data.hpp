#pragma once

#include <fg/Fwd.hpp>

struct ReflectiveShadowMapData
{
    FrameGraphResource position;
    FrameGraphResource normal;
    FrameGraphResource flux;
    FrameGraphResource depth;
};