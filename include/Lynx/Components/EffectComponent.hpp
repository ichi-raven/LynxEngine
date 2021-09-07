#pragma once

#include "IComponent.hpp"

namespace Lynx
{
    class EffectComponent : public IComponent
    {
    public:
        virtual ~EffectComponent(){}
        virtual void update();
    private:

    };
}