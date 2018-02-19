#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include <bitset>

#include "effect.hpp"

class CircleEffect : public Effect
{
    Point m_center;
    float m_radius;
    Color3 m_color;
    bool m_fill;

public:
    CircleEffect(const Point& center, const float radius, const Color3& color, const bool fill);
    void fill(EffectBuffer& buffer, const EffectState& state) override;
    void set_radius(const float radius) { m_radius = radius; }
    void set_color(const Color3& color) { m_color = color; }
    void set_center(const Point& center) { m_center = center; }
};


class ExtendingCircleEffect : public Effect
{
    float m_radius;
    double m_duration;
    double  m_start;
    Color3 m_color;

    CircleEffect m_circle;

public:
    ExtendingCircleEffect(float radius, const Color3& color, float duration, const double start);
    void fill(EffectBuffer& buffer, const EffectState& state) override;

};


class ExplodingCircleEffect : public Effect
{
    float m_radius;
    double m_duration;
    double  m_start;

    CircleEffect m_circle;

public:
    ExplodingCircleEffect(float radius, float duration, const double start);
    void fill(EffectBuffer& buffer, const EffectState& state) override;

};



class DisolvingCircleFieldEffect : public Effect
{

    struct CircleData {
        CircleData()
            : m_start(0),
              m_lenghth(0),
              m_radius(0),
              m_color(0),
              m_center(0, 0)
        {}
        double m_start;
        double m_lenghth;
        float m_radius;
        Color3 m_color;
        Point m_center;
    };

    unsigned int m_count;

    std::vector<CircleData> circles;
    std::bitset<EffectBuffer::Height * EffectBuffer::Width> occupied;


public:
    DisolvingCircleFieldEffect(unsigned int count);
    void fill(EffectBuffer& buffer, const EffectState& state) override;
};

#endif
