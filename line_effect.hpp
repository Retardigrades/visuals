#ifndef LINE_EFFECT_HPP
#define LINE_EFFECT_HPP

#include <list>

#include "effect.hpp"
#include "color.hpp"


class LineEffect : public Effect
{

    const Point m_p1;
    const Point m_p2;
    const Color3 m_color;
    float m_rot = 0.0f;
    float m_lastFactor = 0.0f;
    float m_limitLow = 0.1f;
    float m_limitHigh = 0.02f;
    bool m_limitState = false;

public:
    LineEffect(const Point& p1, const Point& p2, const Color3 color);
    void fill(EffectBuffer& buffer, const EffectState& state) override;
};

class FallingLineEffect : public Effect
{

    struct LineState {
        Color3 m_color;
        double m_start;
        float m_dist;
    };

    double m_speed;
    std::list<LineState> m_lines;

public:
    FallingLineEffect(const double speed);
    void fill(EffectBuffer& buffer, const EffectState& state) override;
};


#endif
