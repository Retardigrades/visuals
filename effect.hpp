#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <vector>
#include <memory>


typedef std::vector<unsigned int>  EffectBuffer;


struct EffectState
{
    const double time;
    const double rotation;

    EffectState(const double time, const double rotation)
    : time(time), rotation(rotation)
    {}
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

    ~AddEffect() {}
};

#endif
