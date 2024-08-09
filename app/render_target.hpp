#pragma once

enum class RenderTarget
{
    eFinal = 0,
    eRSMPosition,
    eRSMNormal,
    eRSMFlux,
    eGNormal,
    eGAlbedo,
    eGEmissive,
    eGMetallicRoughnessAO,
    eHBAO,
    eSceneColorHDR,
    eSceneColorBright,
    eSSR,
    eSceneColorLDR,
};