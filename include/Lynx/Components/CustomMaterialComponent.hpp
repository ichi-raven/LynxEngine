#pragma once
#include "MaterialComponent.hpp"

#include <Cutlass/Cutlass.hpp>

#include <glm/glm.hpp>

namespace Lynx
{
    class CustomMaterialComponent : public MaterialComponent
    {
    public:

        CustomMaterialComponent();
        virtual ~CustomMaterialComponent() override;

        //const std::vector<MaterialSet>& getMaterialSets() const;

        void setVS(const Cutlass::Shader& shader);
        const Cutlass::Shader& getVS() const;

        void setFS(const Cutlass::Shader& shader); 
        const Cutlass::Shader& getFS() const;

        void setColorBlend(Cutlass::ColorBlend colorBlend);
        Cutlass::ColorBlend getColorBlend() const;

        void setMultiSampleState(Cutlass::MultiSampleState multiSampleState);
        Cutlass::MultiSampleState getMultiSampleState() const;

        virtual void update() override;

    private:

        Cutlass::Shader mVS;
        Cutlass::Shader mFS;
        Cutlass::ColorBlend mColorBlend;
        Cutlass::MultiSampleState mMultiSampleState;
    };
}