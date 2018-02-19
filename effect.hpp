#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <vector>
#include <array>
#include <memory>
#include "color.hpp"
#include "effect_data.hpp"


template <int _Height, int _Width>
class EffectBufferBase
{
    using EffectBufferType = EffectBufferBase<_Height, _Width>;

public:
    using PixBuff = std::array<Color3, _Height * _Width>;
    static const int Height = _Height;
    static const int Width = _Width;

    EffectBufferBase() = default;
    void set(int x, int y, const Color3& color) { m_buffer[y * _Width + x] = color; }
    const Color3& get(int x, int y) const       { return m_buffer[y * _Width + x]; }
    PixBuff& get()                              { return m_buffer; }
    size_t size() const                         { return m_buffer.size(); }
    static constexpr int width()                { return _Width; }
    static constexpr int height()               { return Height; }
    void clear()                                { for (auto& i: m_buffer) i = Color3(0); }

    Color3& operator [](size_t i)               { return m_buffer[i]; }
    const Color3& operator [](size_t i) const   { return m_buffer[i]; }

    EffectBufferType& operator =(EffectBufferType&& other)
    {
        if (this != &other)
            m_buffer = std::move(other.m_buffer);
        return *this;
    }

    EffectBufferType& operator =(const EffectBufferType& other)
    {
        if (this != &other)
            std::copy(other.m_buffer.begin(), other.m_buffer.end(), m_buffer.begin());
        return *this;
    }
private:
    PixBuff m_buffer;
};

using EffectBuffer = EffectBufferBase<20, 25>;


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
