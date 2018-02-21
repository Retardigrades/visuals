#include <cmath>
#include "line_effect.hpp"
#include "color_utils.hpp"

LineEffect::LineEffect(const Point& p1, const Point& p2, const Color3 color)
: m_p1(p1), m_p2(p2), m_color(color)
{}


void LineEffect::fill(EffectBuffer& buffer, const EffectState& state)
{
    float dx = float(m_p2.x-m_p1.x)/float(m_p2.y-m_p1.y);
    float dy = float(m_p2.y-m_p1.y)/float(m_p2.x-m_p1.x);
    float len = length(m_p2.x-m_p1.x, m_p2.y-m_p1.y);

    float factor = std::fabs(m_rot - state.rotation.x) * 5.0f;
    factor = factor * 0.8f + m_lastFactor * 0.2f;
    if (factor > 20)
        factor = m_lastFactor;
    if (factor < m_limitHigh)
        m_limitState = false;
    if (factor > m_limitLow)
        m_limitState = true;
    if (!m_limitState)
        factor = 0.0f;
    if (dx < dy) {
        float x = m_p1.x;
        for (int y=m_p1.y; y<=m_p2.y; y++) {
            x += dx;
            buffer.set(x, y, m_color * factor);
        }
    } else {
        float y = m_p1.y;
        for (int x=m_p1.x; x<=m_p2.x; x++) {
            y += dy;
            buffer.set(x, y, m_color * factor);
        }
    }
    m_rot = state.rotation.x;
    m_lastFactor = factor;
}


FallingLineEffect::FallingLineEffect(const double speed) : m_speed(speed)
{ }


void FallingLineEffect::fill(EffectBuffer& buffer, const EffectState& state)
{
    m_lines.remove_if([&](const LineState& l){ return state.time - l.m_start > m_speed; });

    float start = 0.1f;

    for (auto &line : m_lines) {
        double elapsed = state.time - line.m_start;

        if (elapsed > m_speed) {
            continue;
        }

        float progress = elapsed / m_speed;

        float pos = pow(progress, 1.7) * buffer.height();

        float lower = std::ceil(pos);
        float middle = std::floor(pos);
        float upper = middle - 1.0f;

        float coeff = pow((pos - upper), 2);

        for (int i = 0; i < buffer.width(); ++i) {
            buffer.set(i, std::min(int(lower), buffer.height() -1), line.m_color * coeff);
            buffer.set(i, std::max(0, int(upper)), line.m_color * (1.0f - coeff));
            buffer.set(i, std::min(int(middle), buffer.height() -1), line.m_color);
        }

        start = std::min(start, pos - line.m_dist);
    }

    if (start > 0.0f) {
        LineState new_line;
        new_line.m_color = HSVtoRGB(Color3(rand() / (float)RAND_MAX, 1.0f, 1.0f));
        new_line.m_start = state.time;
        new_line.m_dist = 2.0 + float(rand() % 30) / 10.0f;

        m_lines.push_back(new_line);
    }
}
