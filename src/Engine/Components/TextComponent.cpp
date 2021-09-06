#include <Engine/Components/TextComponent.hpp>

#include <Engine/Components/SpriteComponent.hpp>


#include <string>
#include <iostream>
#include <memory>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h> 
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>


namespace Engine
{
    TextComponent::TextComponent()
    : mLoaded(false)
    {

    }

    TextComponent::~TextComponent()
    {

    }

    void TextComponent::loadFont(const char* fontPath)
    {
        /* Load font (. ttf) file */
        long int size = 0;
        //unsigned char *fontBuffer = NULL;

        FILE *fontFile = fopen(fontPath, "rb");
        if (fontFile == NULL)
        {
            assert(!"failed to open font file!");
            return;
        }

        fseek(fontFile, 0, SEEK_END); /* Set the file pointer to the end of the file and offset 0 byte based on the end of the file */
        size = ftell(fontFile);       /* Get the file size (end of file - head of file, in bytes) */
        fseek(fontFile, 0, SEEK_SET); /* Reset the file pointer to the file header */

        mFontBuffer = std::unique_ptr<unsigned char>(new unsigned char[size * sizeof(unsigned char)]);
        fread(mFontBuffer.get(), size, 1, fontFile);
        fclose(fontFile);

        /* Initialize font */
        if (!stbtt_InitFont(&mFontInfo, mFontBuffer.get(), 0))
        {
            assert(!"stb init font failed");
        }
        mLoaded = true;

    }

    void TextComponent::render
    (
        const std::wstring& str,
        std::shared_ptr<Cutlass::Context>& context,
        uint32_t width,
        uint32_t height
    )
    {
        if(!mLoaded)
        {
            assert(!"did not load font yet!");
            return;
        }


        /* create a bitmap */
        const uint32_t bitmap_w = width; /* Width of bitmap */
        const uint32_t bitmap_h = height; /* Height of bitmap */
        unsigned char *bitmap = (unsigned char*)calloc(bitmap_w * bitmap_h, sizeof(unsigned char));

        /* "STB"unicode encoding of */
        //char word[20] = "test image";
        //std::string str = "test string";

        /* Calculate font scaling */
        float pixels = 64.0;                                    /* Font size (font size) */
        float scale = stbtt_ScaleForPixelHeight(&mFontInfo, pixels); /* scale = pixels / (ascent - descent) */

        /** 
         * Get the measurement in the vertical direction 
         * ascent: The height of the font from the baseline to the top;
         * descent: The height from baseline to bottom is usually negative;
         * lineGap: The distance between two fonts;
         * The line spacing is: ascent - descent + lineGap.
        */
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&mFontInfo, &ascent, &descent, &lineGap);

        /* Adjust word height according to zoom */
        ascent = roundf(ascent * scale);
        descent = roundf(descent * scale);

        int x = 0; /*x of bitmap*/

        /* Cyclic loading of each character in word */
        //for (int i = 0; i < strlen(word); ++i)
        for (int i = 0; i < str.length(); ++i)
        {
            /** 
             * Get the measurement in the horizontal direction
             * advanceWidth: Word width;
             * leftSideBearing: Left side position;
            */
            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&mFontInfo, str[i], &advanceWidth, &leftSideBearing);

            /* Gets the border of a character */
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&mFontInfo, str[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            /* Calculate the y of the bitmap (different characters have different heights) */
            int y = ascent + c_y1;

            /* Render character */
            int byteOffset = x + roundf(leftSideBearing * scale) + (y * bitmap_w);
            stbtt_MakeCodepointBitmap(&mFontInfo, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_w, scale, scale, str[i]);

            /* Adjust x */
            x += roundf(advanceWidth * scale);

            /* kerning */
            int kern;
            kern = stbtt_GetCodepointKernAdvance(&mFontInfo, str[i], str[i + 1]);
            x += roundf(kern * scale);
        }

                
        //std::unique_ptr<RGBA[][bitmap_w]> writeData(new(std::nothrow) RGBA[bitmap_h][bitmap_w]);
        RGBA* writeData = (RGBA*)calloc(sizeof(RGBA), bitmap_w * bitmap_h);

        {
            char tmp = 0;
            size_t idx = 0;
            for(size_t r = 0; r < bitmap_h; ++r)
                for(size_t c = 0; c < bitmap_w; ++c)
                {
                    idx = r * bitmap_w + c;
                    tmp = bitmap[idx];
                    writeData[idx].r = tmp;
                    writeData[idx].g = tmp;
                    writeData[idx].b = tmp;
                    //if(writeData[r][c].r || writeData[r][c].g || writeData[r][c].b)
                    if(tmp)
                        writeData[idx].a = 255;
                    else
                        writeData[idx].a = 0;
                }
        }

        //stbi_write_png("fontsprite-test.png", bitmap_w, bitmap_h, 4, writeData.get(), 0);

        if(mSprite.handles.size() == 0)
        {
            Cutlass::TextureInfo ti;
            ti.setSRTex2D(bitmap_w, bitmap_h, true);

            Cutlass::HTexture handle;
            context->createTexture(ti, handle);
            context->writeTexture(writeData, handle);
            mSprite.handles.emplace_back(handle);
        }
        else
        {
            uint32_t w, h, d;
            context->getTextureSize(mSprite.handles[0], w, h, d);
            assert(d == 1);

            if(w != bitmap_w || h != bitmap_h)
            {
                SpriteComponent::Sprite sprite;
                Cutlass::TextureInfo ti;
                ti.setSRTex2D(bitmap_w, bitmap_h, true);

                Cutlass::HTexture handle;
                context->createTexture(ti, handle);
                context->writeTexture(writeData, handle);
                context->destroyTexture(mSprite.handles[0]);
                mSprite.handles[0] = handle;
            }
            else
                context->writeTexture(writeData, mSprite.handles[0]);
        }

        free(bitmap);
        free(writeData);
    }

    void TextComponent::update()
    {
        //mTransform.update();
        SpriteComponent::update();
    }
}