#pragma once

#include "vgfw.hpp"

constexpr auto kRSMResolution = 512;
constexpr auto kNumVPL        = kRSMResolution * kRSMResolution;
constexpr auto kLPVResolution = 32;

constexpr auto kAdditiveBlending = vgfw::renderer::BlendState {
    .enabled   = true,
    .srcColor  = vgfw::renderer::BlendFactor::eOne,
    .destColor = vgfw::renderer::BlendFactor::eOne,
    .colorOp   = vgfw::renderer::BlendOp::eAdd,

    .srcAlpha  = vgfw::renderer::BlendFactor::eOne,
    .destAlpha = vgfw::renderer::BlendFactor::eOne,
    .alphaOp   = vgfw::renderer::BlendOp::eAdd,
};