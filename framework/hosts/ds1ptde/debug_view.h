#pragma once
#include "debug_observation.h"

// Edition composition: observe once, assemble neutral geometry, submit through
// the renderer. Only the collector knows native addresses/guest probe semantics.
static void debugDraw(SCRenderAPI::Renderer &renderer, SCRenderAPI::Context context) {
    if (!debugVisible())
        return;
    debugGather();
    SCDebug::Lines lines;
    auto drawLines = [&](const SCDebugView::Observations &observations) {
        SCDebugView::geometry(observations, lines);
        renderer.lines({lines.vertices.data(), uint32_t(lines.vertices.size() / 2)}, !debugXray,
                       context);
    };
    if (debugHas(1))
        drawLines(debugTerrain);
    drawLines(debugLines);
    if (debugHas(0))
        drawLines(debugAttack);
    if (!presentationFontCreate())
        return;
    SCDebugView::PanelGeometry panel;
    SCDebugView::panel(debugPanelObservation(), panel);
    renderer.screen({panel.background.data(), uint32_t(panel.background.size() / 3)}, {}, context);
    SCRenderAPI::Material style{};
    style.image = presentationFont();
    style.color = SCRenderAPI::ColorMode::Modulate;
    style.alphaCutoff = 100;
    renderer.screen({panel.text.data(), uint32_t(panel.text.size() / 3)}, style, context);
}
