#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <vector>
#include <memory>
#include <array>
#include "color.hpp"
#include "effect_data.hpp"

template <int Height, int Width>
class EffectBufferTemplate {
public:
    EffectBufferTemplate() { clear(); }
    void set(int x, int y, const Color3& color) { m_buffer[y * Width + x] = color; }
    const Color3& get(int x, int y) const       { return m_buffer[y * Width + x]; }
    std::vector<Color3>& get()                  { return m_buffer; }
    size_t size() const                         { return Width * Height; }
    int width() const                           { return Width; }
    int height() const                          { return Height; }
    void clear()                                { for (auto& i: m_buffer) i = Color3(0); }

    Color3& operator [](size_t i)               { return m_buffer[i]; }
    const Color3& operator [](size_t i) const   { return m_buffer[i]; }

    EffectBufferTemplate<Height, Width>& operator =(EffectBufferTemplate<Height, Width>&& other)
    {
        if (this != &other)
            m_buffer = std::move(other.m_buffer);
        return *this;
    }

    EffectBufferTemplate<Height, Width>& operator =(const EffectBufferTemplate<Height, Width>& other)
    {
        if (this != &other)
            std::copy(other.m_buffer.begin(), other.m_buffer.end(), m_buffer.begin());
        return *this;
    }
private:
    std::array<Color3, Height*Width> m_buffer;
};

typedef EffectBufferTemplate<20, 25> EffectBuffer;

struct Point
{
    float x;
    float y;

    Point(unsigned int x, unsigned int y)
    : x(x), y(y)
    {}
};

struct Rect
{
    Point top_left;
    Point bottom_right;

    Rect(const Point& top_left, const Point bottom_right)
    : top_left(top_left), bottom_right(bottom_right)
    {}

    float width() const { return bottom_right.x - top_left.x; }
    float height() const { return bottom_right.y - top_left.y; }
};


class Effect {
public:
    virtual ~Effect() {}
    virtual void fill(EffectBuffer& buffer, const EffectState& state) = 0;
};


class AddEffect : public Effect
{
    std::shared_ptr<Effect> m_base;
    float m_coeff;

public:
    AddEffect(std::shared_ptr<Effect> base, const float coeff);
    void fill(EffectBuffer& buffer, const EffectState& state);

    ~AddEffect();
};


#endif
