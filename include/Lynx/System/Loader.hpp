#pragma once

#include <memory>
#include <variant>
#include <vector>

// #include "../ThirdParty/tiny_obj_loader.h"
// #include "../ThirdParty/tiny_gltf.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Lynx/Components/MeshComponent.hpp>
#include <Lynx/Components/SkeletalMeshComponent.hpp>
#include <Lynx/Components/MaterialComponent.hpp>
#include <Lynx/Components/SpriteComponent.hpp>
#include <Lynx/Components/TextComponent.hpp>


namespace Cutlass
{
    class Context;
}

namespace Lynx
{

    class Loader
    {
    public:

        struct VertexBoneData
        {
            VertexBoneData()
            {
                weights[0] = weights[1] = weights[2] = weights[3] = 0;
                id[0] = id[1] = id[2] = id[3] = 0;
            }

            float weights[4];
            float id[4];
        };

        Loader(const std::shared_ptr<Cutlass::Context>& context);

        virtual ~Loader();

        //virtual void load(const char* path);
        virtual void load
        (
            const char* path,
            std::weak_ptr<MeshComponent>& mesh_out,
            std::weak_ptr<MaterialComponent>& material_out
        );

        virtual void load
        (
            const char* path,
            std::weak_ptr<SkeletalMeshComponent>& skeletalMesh_out,
            std::weak_ptr<MaterialComponent>& material_out
        );

        //if type was nullptr, type will be last section of path
        virtual void load(const char* path, const char* type, std::weak_ptr<MaterialComponent>& material_out);

        virtual void load(const char* path, std::weak_ptr<SpriteComponent>& sprite_out);
        virtual void load(std::vector<const char*> pathes, std::weak_ptr<SpriteComponent>& sprite_out);

        virtual void load(const char* path, std::weak_ptr<TextComponent>& text_out);


    private:
        void unload();
        
        void processNode(const aiNode* node);

        MeshComponent::Mesh processMesh(const aiNode* node, const aiMesh* mesh);

        std::vector<MaterialComponent::Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

        void loadBones(const aiNode* node, const aiMesh* mesh, std::vector<VertexBoneData>& vbdata_out);

        MaterialComponent::Texture loadTexture(const char* path, const char* type);

        
        std::shared_ptr<Cutlass::Context> mContext;
        bool mLoaded;
        bool mSkeletal;
        std::string mDirectory;
        std::string mPath;

        std::vector<MeshComponent::Mesh> mMeshes;
        SkeletalMeshComponent::Skeleton mSkeleton;//単体前提
        std::vector<MaterialComponent::Texture> mTexturesLoaded;

        uint32_t mBoneNum;

        Assimp::Importer mImporter;
        std::shared_ptr<const aiScene> mScene;
              
    };
}