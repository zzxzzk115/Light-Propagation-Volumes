#pragma once

#include <fg/Fwd.hpp>

struct GBufferData
{
    FrameGraphResource normal;
    FrameGraphResource albedo;
    FrameGraphResource emissive;
    FrameGraphResource metallicRoughnessAO;
    FrameGraphResource depth;
};