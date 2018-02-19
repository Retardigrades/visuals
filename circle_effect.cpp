#include "circle_effect.hpp"
#include "color_utils.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <functional>

using DrawPixel = std::function<void(int x, int y, const Color3& color)>;

static inline void draw_circle(const DrawPixel& draw, const Color3& color, const Point& center, const float& radius, const bool fill)
{
    const int x_min = std::floor(center.x - radius) - 1;
    const int x_max = std::ceil(center.x + radius) + 1;

    const int y_min = std::max(0, int(std::floor(center.y - radius) - 1));
    const int y_max = std::min(EffectBuffer::height(), int(std::ceil(center.y + radius) + 1));

    for (int y = y_min; y < y_max; ++y) {
        float b = float(y) - center.y;
        for (int x = x_min; x < x_max; ++x) {
            float a = float(x) - center.x;
            float d = std::sqrt(a*a + b*b) - radius;

            if (fill) {
                if (d < 1.0) {
                    float coeff = float(std::min(1.0f, 1.0f - d));
                    draw(fmod2(x, EffectBuffer::Width), y,  color * coeff);
                }
            } else {
                if (d < 1.0 && d > -1.5) {
                    float coeff = (d > 0 ? 1.0f - d : 1.0f + d);
                    draw(fmod2(x, EffectBuffer::Width), y, color * float(std::min(1.0f, coeff)));
                }
            }
        }
    }
}

CircleEffect::CircleEffect(const Point& center, const float radius, const Color3& color, const bool fill)
    : m_center(center), m_radius(radius), m_color(color), m_fill(fill)
{}

void CircleEffect::fill(EffectBuffer& buffer, const EffectState& state)
{
    (void) state;

    using namespace std::placeholders;
    draw_circle(std::bind(&EffectBuffer::set, &buffer, _1, _2, _3), m_color, m_center, m_radius, m_fill);
}

ExtendingCircleEffect::ExtendingCircleEffect(float radius, const Color3& color, float duration, double time)
    : m_radius(radius), m_duration(duration), m_start(time-duration), m_color(color), m_circle(Point(0, 0), 0, color, false)
{}

void ExtendingCircleEffect::fill(EffectBuffer& buffer, const EffectState& state)
{
    double elapsed = state.time - m_start;

    if (elapsed < 0) {
        return;
    }

    if (elapsed > m_duration) {
        m_start = state.time + ((rand() % 500) / 100.0f);
        float x = rand() % buffer.width();
        float y = rand() % buffer.height();
        m_circle.set_center(Point(x, y));
        m_color = HSVtoRGB(Color3(rand() / (float)RAND_MAX, 1.0f, 1.0f));
        return;
    }

    float half_duration = m_duration / 2;
    float progress = (half_duration - std::abs(elapsed - half_duration)) / half_duration;
    float radius = pow(progress, 0.25f) * m_radius;
    float sqrt_progress = std::pow(progress, 0.3);

    if (radius < 1.1f) {
	    return;
    }

    m_circle.set_color(m_color * progress);
    m_circle.set_radius(radius * sqrt_progress);
    m_circle.fill(buffer, state);
}

ExplodingCircleEffect::ExplodingCircleEffect(float radius, float duration, double time)
    : m_radius(radius), m_duration(duration), m_start(time - duration), m_circle(Point(0, 0), 0, Color3(1, 1, 1), true)
{}

void ExplodingCircleEffect::fill(EffectBuffer& buffer, const EffectState& state)
{
    double elapsed = state.time - m_start;

    if (elapsed <= 0.0f) {
        return;
    }

    if (elapsed > m_duration) {
        m_start = state.time + ((rand() % 400) / 100.0f);
        float x = rand() % buffer.width();
        float y = rand() % buffer.height();
        m_circle.set_center(Point(x, y));
        return;
    }

    float progress = elapsed / m_duration;
    float progress_squared = progress*progress;
    float inverse_progress = 1.0f - progress;
    float inverse_squared_progress = 1.0f - progress_squared;

    m_circle.set_radius(std::pow(progress, 0.3) * m_radius);
    m_circle.set_color(Color3(inverse_squared_progress,
            inverse_progress * inverse_squared_progress * .5,
            std::max(0.0f, 1.0f - (progress * 5.0f)) * inverse_squared_progress * .3));
    m_circle.fill(buffer, state);
}


DisolvingCircleFieldEffect::DisolvingCircleFieldEffect(unsigned int count)
    : m_count(count)
{
    for (unsigned i = 0; i < count; ++i) {
        circles.emplace_back();
    }

}


void DisolvingCircleFieldEffect::fill(EffectBuffer& buffer, const EffectState& state)
{
    for (auto& cd : circles) {
        double elapsed = state.time - cd.m_start;

        if (elapsed < 0.0) {
            continue;
        }

        if (elapsed > cd.m_lenghth) {
           if (0.0 != cd.m_start) {
               occupied.set( cd.m_center.y * buffer.width() + cd.m_center.x , false);
           }

           cd.m_start = state.time + 1.0 + 3.0 * double(rand()) / double(RAND_MAX) * 0.3;
           cd.m_lenghth = (rand() % 3) + 2;
           cd.m_radius = (rand() % 4) + 2;
           cd.m_color = HSVtoRGB(Color3(rand() / (float)RAND_MAX, 1.0f, 1.0f));

           unsigned int pos;
           do {
               pos = rand() % (EffectBuffer::Height * EffectBuffer::Width);
           } while (occupied[pos]);

           occupied.set(pos);

           unsigned int x = pos % EffectBuffer::Width;
           unsigned int y = (pos - x) / EffectBuffer::Width;
           cd.m_center = Point(x, y);

           continue;
        }

        float progress(elapsed / cd.m_lenghth);
        float radius = -1.0 * std::pow(progress - 1.0f, 2) + 1.0f;

        draw_circle(
                [&](int x, int y, const Color3& color) {
                    int pos = y * EffectBuffer::Width + x;
                    buffer[pos] += color * float(1.0f - std::pow(progress, 2));
                },
                cd.m_color,  cd.m_center, cd.m_radius * radius, true);

    }

}
