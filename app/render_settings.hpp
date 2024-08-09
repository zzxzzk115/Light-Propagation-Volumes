#pragma once

#include "passes/hbao_pass.hpp"
#include "render_target.hpp"
#include "visual_mode.hpp"

struct RenderSettings
{
    // Render target
    RenderTarget renderTarget = RenderTarget::eFinal;

    // Visual mode
    VisualMode visualMode = VisualMode::eDefault;

    // Render features
    bool enableHBAO  = true;
    bool enableSSR   = true;
    bool enableFXAA  = true;
    bool enableBloom = true;

    // LPV settings
    int lpvIteration = 12;

    // HBAO properties
    HBAOProperties hbaoProperties {};

    // SSR settings
    float reflectionFactor = 1.0f;
};