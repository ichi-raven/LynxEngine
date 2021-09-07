#pragma once
#include "SpriteComponent.hpp"

#include <stb/stb_truetype.h>

#include <Cutlass/Cutlass.hpp>

#include <memory>

namespace Lynx
{
    class TextComponent : public SpriteComponent
    {
    public:
        TextComponent();

        virtual ~TextComponent();

        void loadFont(const char* fontPath);

        void render(const std::wstring& str, std::shared_ptr<Cutlass::Context>& context, uint32_t width, uint32_t height);

        virtual void update();

    protected:
        struct RGBA
        {
            RGBA()
            : r(0)
            , g(0)
            , b(0)
            , a(255)
            {}

            unsigned char r, g, b, a;
        };

        stbtt_fontinfo mFontInfo;
        std::unique_ptr<unsigned char> mFontBuffer;

    private:

        bool mLoaded;
    };
}