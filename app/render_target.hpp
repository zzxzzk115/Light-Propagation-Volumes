#pragma once

enum class RenderTarget
{
    eFinal = 0,
    eRSMPosition,
    eRSMNormal,
    eRSMFlux,
    eGPosition,
    eGNormal,
    eGAlbedo,
    eGEmissive,
    eGMetallicRoughnessAO,
    eSceneColorHDR,
    eSceneColorLDR,
};