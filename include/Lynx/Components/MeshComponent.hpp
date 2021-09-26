#pragma once

#include <Cutlass/Cutlass.hpp>
#include <glm/glm.hpp>

#include <typeinfo>

#include "IComponent.hpp"

#include "../Utility/Transform.hpp"


namespace Lynx
{
    class MeshComponent : public IComponent
    {
    public:
        //頂点構造, これをいじるとRendererとかも変わります
        #define EQ(MEMBER) (MEMBER == another.MEMBER)//便利
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 uv;
            glm::vec4 joint;
            glm::vec4 weight;

            bool operator==(const Vertex& another) const 
            {
                return EQ(pos) && EQ(normal) && EQ(uv) && EQ(joint) && EQ(weight);
            }
        };

        struct Mesh 
	    {
            std::vector<MeshComponent::Vertex> vertices;
            std::vector<uint32_t> indices;
            std::string nodeName;
            std::string meshName;
        };

        // struct CPUVertex
        // {
        //     glm::vec3 pos;

        //     bool operator==(const CPUVertex& another) const 
        //     {
        //         return EQ(pos);
        //     }
        // };

        MeshComponent();
        virtual ~MeshComponent();

        //メッシュを構築する
        void create
        (
            const std::vector<Vertex>& vertices,
            const std::vector<uint32_t>& indices
        );

        void create(const std::vector<Mesh>& meshes);

        //基本スタティックメッシュ構築
        void createCube(const double& edgeLength);
        void createPlane(const double& xSize, const double& zSize);
        
        void setVisible(bool flag);
        bool getVisible() const;

        void setEnable(bool flag);
        bool getEnable() const;

        void setTransform(const Transform& transform);
        Transform& getTransform();
        //const Transform& getTransform() const;

        void setTopology(Cutlass::Topology topology);
        Cutlass::Topology getTopology() const;

        void setRasterizerState(const Cutlass::RasterizerState& rasterizerState);
        const Cutlass::RasterizerState& getRasterizerState() const;

        const std::vector<Mesh>& getMeshes() const;

        // const std::vector<Vertex>& getVertices() const;
        // const std::vector<uint32_t>& getIndices() const; 

        // const Cutlass::HBuffer& getVB() const;

        // const Cutlass::HBuffer& getIB() const;

        virtual void update() override;

    protected:

        bool mVisible;
        bool mEnabled;
        Transform mTransform;

        std::vector<Mesh> mMeshes;

        Cutlass::Topology mTopology;
        Cutlass::RasterizerState mRasterizerState;

        
    };
};