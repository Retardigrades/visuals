#include "raindrop_effect.hpp"
#include "color_utils.hpp"
#include <cmath>



Raindrop::Raindrop(unsigned int col, double start, Color3 color)
    : m_col(col), m_speed(1 + (rand() % 7) ), m_start(start), m_color(color)
{
}

bool Raindrop::draw(EffectBuffer& buffer, const unsigned int height, const unsigned int width, const double time) const
{
    double elapsed = time - m_start;

    if (0 > elapsed) {
        return true;
    }

    double step_time = m_speed / height;
    unsigned int length = height / std::min(2.0, m_speed*0.5f);

    double offset = (time - m_start) / step_time;
    double step = std::floor(offset);

    if ((step - length) > height) {
        return false;
    }

    for (int pos = length - 1; pos >= 0; --pos) {
        int position = offset - pos;
        if (position < 0 || position >= height) {
            continue;
        }
        buffer[width * position + m_col] = m_color * float(((float(length - pos)) / length) * std::min(1.0f, (float(length) /  (1.5f * pos))));
    }
    return true;
}

void Raindrop::reset(const double time)
{
    m_start = time;
    m_speed = 1 + rand() % 7;
}




EffectRaindrops::EffectRaindrops(unsigned int height, unsigned int width, double time)
: m_height(height), m_width(width)
{
    srand(0);
    for (unsigned int col = 0; col < width; ++col) {
        Color3 color = HSVtoRGB(Color3(col / 50.0f, 1.0f, 1.0f));
        drops.push_back(Raindrop(col, time + (rand() % 25) / 10.0f, color));
    }
}

void EffectRaindrops::fill(EffectBuffer& buffer, const EffectState& state)
{
    for (auto& drop : drops) {
        if (!drop.draw(buffer, m_height, m_width, state.time)) {
            drop.reset(state.time + (rand() % 50) / 10.0f);
        }
    }
}
