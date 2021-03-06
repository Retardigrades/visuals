
#include "wave_effect.hpp"
#include <algorithm>
#include <cmath>


TopDownWaveEffect::TopDownWaveEffect(float duration, bool inverse)
: m_duration(duration), m_inverse(inverse)
{}


void TopDownWaveEffect::fill(EffectBuffer& buffer, const EffectState& state) {
    float elapsed = state.time - m_start;

    if (elapsed < 0) {
        return;
    }

    if (elapsed > m_duration) {
        m_start = state.time + rand() % 4;
        return;
    }

    float progress = elapsed / m_duration;
    float progress_squared = progress*progress;
    float inverse_progress = 1.0f - progress;
    float inverse_squared_progress = 1.0f - progress_squared;

    float dist = float(buffer.height() * progress);

    Color3 color(inverse_squared_progress,
            inverse_progress * inverse_squared_progress * .5,
            std::max(0.0f, 1.0f - (progress * 5.0f)) * inverse_squared_progress * .3);

    for (int y = 0; y < std::ceil(dist); ++y) {
        if (dist - y < 1.0) {
            color *= dist - y;
        }
        for (int x = 0; x < buffer.width(); ++x) {
            if (m_inverse) {
                buffer.set(x, buffer.height() - y -1, color);
            } else {
                buffer.set(x, y, color);
            }
        }
    }
}
