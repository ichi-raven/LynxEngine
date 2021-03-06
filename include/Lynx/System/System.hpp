#pragma once

#include <memory>

#include "Loader.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

namespace Lynx
{
    class System
    {
    public:

        std::unique_ptr<Loader> loader;
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<Input> input;
    };
}