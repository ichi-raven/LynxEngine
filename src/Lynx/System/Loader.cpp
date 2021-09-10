#include <Lynx/System/Loader.hpp>

// #define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION
// #define JSON_NOEXCEPTION
// #define TINYOBJLOADER_IMPLEMENTATION
// #include <Lynx/ThirdParty/tiny_obj_loader.h>
// #include <Lynx/ThirdParty/tiny_gltf.h>

#include <Lynx/Components/MeshComponent.hpp>
#include <Lynx/Components/MaterialComponent.hpp>

#include <iostream>
#include <unordered_map>
#include <regex>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>


//for extract unique vertex
// namespace std 
// {
//     template<> struct hash<Lynx::MeshComponent::Vertex>
//     {
//         size_t operator()(Lynx::MeshComponent::Vertex const& vertex) const 
//         {
//             return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
//         }
//     };
// }

namespace Lynx
{
    //inline const glm::vec4 real3ToVec4(const tinyobj::real_t src[], const float w = 1.f)
    // {
    //     return glm::vec4(src[0], src[1], src[2], w);
    // };

    // inline const glm::vec4 real4ToVec4(const tinyobj::real_t src[])
    // {
    //     return glm::vec4(src[0], src[1], src[2], src[4]);
    // };

    // inline bool ends_with(const std::string& str, const std::string& suffix) 
    // {
    //     size_t len1 = str.size();
    //     size_t len2 = suffix.size();
    //     return len1 >= len2 && str.compare(len1 - len2, len2, suffix) == 0;
    // }

    inline glm::mat4 convert4x4(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //transpose
        for(uint8_t i = 0; i < 4; ++i)
            for(uint8_t j = 0; j < 4; ++j)
                to[i][j] = from[j][i];
        
        return to;
    } 

    inline glm::quat convertQuat(const aiQuaternion& from)
    {
        glm::quat to;
        to.w = from.w;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;
        return to;
    }

    inline glm::vec3 convertVec3(const aiVector3D& from)
    {
        glm::vec3 to;
        to.x = from.x;
        to.y = from.y;
        to.z = from.z;
        return to;
    }

    inline glm::vec2 convertVec2(const aiVector2D& from)
    {
        glm::vec2 to;
        to.x = from.x;
        to.y = from.y;
        return to;
    }

    Loader::Loader(const std::shared_ptr<Cutlass::Context>& context)
    : mContext(context)
    , mBoneNum(0)
    {
        
    }

    Loader::~Loader()
    {
        if(mLoaded)
            unload();
    }
    void Loader::unload()
    {
        mMeshes.clear();
        mTexturesLoaded.clear();
        mSkeleton.bones.clear();
        mSkeleton.boneMap.clear();
        mLoaded = false;
    }

    //Static
    void Loader::load
    (
        const char* modelPath,
        std::weak_ptr<MeshComponent>& mesh_out,
        std::weak_ptr<MaterialComponent>& material_out
    )
    {
        if(mesh_out.expired() || material_out.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        mSkeletal = false;

        mPath = std::string(modelPath);

        mScene = std::shared_ptr<const aiScene>(mImporter.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs));

        if(!mScene || mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mRootNode) 
        {
            std::cerr << "ERROR::ASSIMP::" << mImporter.GetErrorString() << "\n";
            assert(0);
            return;
        }

        mDirectory = mPath.substr(0, mPath.find_last_of("/\\"));

        // mSkeleton.setGlobalInverse(glm::inverse(convert4x4(mScene->mRootNode->mTransformation)));
        // mSkeleton.setAIScene(mScene);

        processNode(mScene->mRootNode);

        //assert(0);

        // std::vector<MeshComponent::Vertex> vertices;
        // std::vector<uint32_t> indices;

        // vertices.reserve(1e5);
        // indices.reserve(1e5);
        // uint32_t checkSumV = 0, checkSumI = 0;
        std::cerr << "mesh count : " << mMeshes.size() << "\n";
        // for(const auto& mesh : mMeshes)
        // {
        //     std::cerr << "mesh node name : " << mesh.nodeName << "\n";
        //     std::cerr << "mesh name : " << mesh.meshName << "\n";
        //     std::cerr << "mesh vertex num : " << mesh.vertices.size() << "\n";
        //     std::cerr << "mesh index num : " << mesh.indices.size() << "\n";

        //     vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        //     indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
        //     //std::copy(mesh.vertices.begin(), mesh.vertices.end(), std::back_inserter(vertices));
        //     //std::copy(mesh.indices.begin(), mesh.indices.end(), std::back_inserter(indices));
        //     checkSumV += mesh.vertices.size();
        //     checkSumI += mesh.indices.size();
        // }

        // std::cerr << "all vertices size : " << vertices.size() << "\n";
        // std::cerr << "check vertices size : " << checkSumV << "\n";
        // std::cerr << "all indices size : " << indices.size() << "\n";
        // std::cerr << "check indices size : " << checkSumI << "\n";
        //mesh_out.lock()->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
        mesh_out.lock()->create(mMeshes);

        material_out.lock()->addTextures(mTexturesLoaded);

        //unload();
    }

    //Skeletal
    void Loader::load
    (
        const char* modelPath,
        std::weak_ptr<SkeletalMeshComponent>& skeletalMesh_out,
        std::weak_ptr<MaterialComponent>& material_out
    )
    {
        if(skeletalMesh_out.expired() || material_out.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        mSkeletal = true;

        mPath = std::string(modelPath);

        mScene = std::shared_ptr<const aiScene>(mImporter.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs));

        if(!mScene || mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mRootNode) 
        {
            std::cerr << "ERROR::ASSIMP::" << mImporter.GetErrorString() << "\n";
            assert(0);
            return;
        }

        mDirectory = mPath.substr(0, mPath.find_last_of("/\\"));

        mSkeleton.setGlobalInverse(glm::inverse(convert4x4(mScene->mRootNode->mTransformation)));
	    mSkeleton.setAIScene(mScene);

        processNode(mScene->mRootNode);

        std::cerr << "bone num : " << mBoneNum << "\n";
        //assert(0);

        // std::vector<MeshComponent::Vertex> vertices;
        // std::vector<uint32_t> indices;

        // vertices.reserve(1e5);
        // indices.reserve(1e5);
        // for(const auto& mesh : mMeshes)
        // {
        //     std::cerr << "mesh vertex num : " << mesh.vertices.size() << "\n";
        //     std::copy(mesh.vertices.begin(), mesh.vertices.end(), std::back_inserter(vertices));
        //     std::copy(mesh.indices.begin(), mesh.indices.end(), std::back_inserter(indices));
        // }

        //skeletalMesh_out.lock()->setRasterizerState(Cutlass::RasterizerState(Cutlass::PolygonMode::eFill, Cutlass::CullMode::eBack, Cutlass::FrontFace::eCounterClockwise));
        //skeletalMesh_out.lock()->create(vertices, indices, mSkeleton);
        skeletalMesh_out.lock()->create(mMeshes, mSkeleton);
        
        material_out.lock()->addTextures(mTexturesLoaded);
        
        unload();
    }

    void Loader::processNode(const aiNode* node)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            if(mesh)
                mMeshes.emplace_back(processMesh(node, mesh));
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) 
        {
            processNode(node->mChildren[i]);
        }

    }

    MeshComponent::Mesh Loader::processMesh(const aiNode* node, const aiMesh* mesh)
    {
        std::cerr << "start process mesh\n";
        // Data to fill
        std::vector<MeshComponent::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MaterialComponent::Texture> textures;

        // Walk through each of the mesh's vertices
        for (uint32_t i = 0; i < mesh->mNumVertices; i++) 
        {
            MeshComponent::Vertex vertex;

            vertex.pos = convertVec3(mesh->mVertices[i]);

            if(mesh->mNormals)
                vertex.normal = convertVec3(mesh->mNormals[i]);
            else
                vertex.normal = glm::vec3(0);


            if (mesh->mTextureCoords[0])
            {
                vertex.uv.x = (float)mesh->mTextureCoords[0][i].x;
                vertex.uv.y = (float)mesh->mTextureCoords[0][i].y;
            }
            else
            {
                assert(!"texture coord nothing!");
                vertex.uv.x = 0;
                vertex.uv.y = 0;
            }

            vertices.push_back(vertex);
        }

        std::vector<VertexBoneData> vbdata;
        mBoneNum += mesh->mNumBones;
        if(mSkeletal)
        {
            loadBones(node, mesh, vbdata);
            //頂点にボーン情報を付加
            for(uint32_t i = 0; i < vertices.size(); ++i)
            {
                vertices[i].joint = glm::make_vec4(vbdata[i].id);
                vertices[i].weight = glm::make_vec4(vbdata[i].weights);
                //std::cerr << to_string(vertices[i].weight0) << "\n"; 
                // float sum = vertices[i].weight.x + vertices[i].weight.y + vertices[i].weight.z + vertices[i].weight.w; 
                // assert(sum <= 1.01f);
                // assert(vertices[i].joint.x < mBoneNum);
                // assert(vertices[i].joint.y < mBoneNum);
                // assert(vertices[i].joint.z < mBoneNum);
                // assert(vertices[i].joint.w < mBoneNum);
                
            }
        }

        if(mesh->mFaces)
        {
            for(unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for(unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
        }

        std::cerr << "indices\n";

        if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < mScene->mNumMaterials) 
        {
            aiMaterial* material = mScene->mMaterials[mesh->mMaterialIndex];
            std::vector<MaterialComponent::Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        }

        std::cerr << "materials\n";

        {
            MeshComponent::Mesh m;
            m.vertices = vertices;
            m.indices = indices;
            m.nodeName = std::string(node->mName.C_Str());
            m.meshName = std::string(mesh->mName.C_Str());

            std::cerr << "end process mesh\n";
            
            return m;
        }
    }

    void Loader::loadBones(const aiNode* node, const aiMesh* mesh, std::vector<VertexBoneData>& vbdata_out)
    {
        vbdata_out.resize(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumBones; i++) 
        {
            uint32_t boneIndex = 0;
            std::string boneName(mesh->mBones[i]->mName.data);

            if (mSkeleton.boneMap.count(boneName) <= 0)//そのボーンは登録されてない
            {
                boneIndex = mSkeleton.bones.size();
                mSkeleton.bones.emplace_back();
            }
            else //あった
                boneIndex = mSkeleton.boneMap[boneName];

            mSkeleton.boneMap[boneName] = boneIndex;
            mSkeleton.bones[boneIndex].offset = convert4x4(mesh->mBones[i]->mOffsetMatrix);
            // for(size_t i = 0; i < 3; ++i)
            //     if(abs(mSkeleton.bones[boneIndex].offset[i][3]) > std::numeric_limits<float>::epsilon())
            //     {
            //         std::cerr << boneIndex << "\n";
            //         std::cerr << glm::to_string(mSkeleton.bones[boneIndex].offset) << "\n";
            //         assert(!"invalid bone offset!");
            //     }
            mSkeleton.bones[boneIndex].transform = glm::mat4(1.f);
            //頂点セット
            for (uint j = 0; j < mesh->mBones[i]->mNumWeights; j++) 
            {
                size_t vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
                float weight = mesh->mBones[i]->mWeights[j].mWeight;
                for (uint k = 0; k < 4; k++)
                {
                    if (vbdata_out[vertexID].weights[k] == 0.0) 
                    {
                        vbdata_out[vertexID].id[k]      = boneIndex;
                        vbdata_out[vertexID].weights[k] = weight;
                        break;
                    }

                    // if(k == 3)//まずい
                    //     assert(!"invalid bone weight!");
                }
            }
        }

    }

    //from sample
    std::vector<MaterialComponent::Texture> Loader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
    {
        std::vector<MaterialComponent::Texture> textures;
        for (uint32_t i = 0; i < mat->GetTextureCount(type); i++) 
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for (uint32_t j = 0; j < mTexturesLoaded.size(); j++) 
            {
                if (std::strcmp(mTexturesLoaded[j].path.c_str(), str.C_Str()) == 0) 
                {
                    textures.push_back(mTexturesLoaded[j]);
                    skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            {   // If texture hasn't been loaded already, load it
                MaterialComponent::Texture texture;

                const aiTexture* embeddedTexture = mScene->GetEmbeddedTexture(str.C_Str());
                if (embeddedTexture != nullptr) 
                {
                    Cutlass::TextureInfo ti;
                    ti.setSRTex2D(embeddedTexture->mWidth, embeddedTexture->mHeight, true);
                    Cutlass::Result res = mContext->createTexture(ti, texture.handle);
                    if (res != Cutlass::Result::eSuccess)
                        assert(!"failed to create embedded texture!");
                    res = mContext->writeTexture(embeddedTexture->pcData, texture.handle);
                    if (res != Cutlass::Result::eSuccess)
                        assert(!"failed to write to embedded texture!");
                } 
                else 
                {
                    std::string filename = std::regex_replace(str.C_Str(), std::regex("\\\\"), "/");
                    filename = mDirectory + '/' + filename;
                    std::wstring filenamews = std::wstring(filename.begin(), filename.end());
                    Cutlass::Result res = mContext->createTextureFromFile(filename.c_str(), texture.handle);
                    if (res != Cutlass::Result::eSuccess)
                    {
                        std::cerr << "failed path : " << filename << "\n";
                        assert(!"failed to create material texture!");
                    }
                }
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                mTexturesLoaded.emplace_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }

    //MaterialTexture
    void Loader::load(const char* path, const char* type, std::weak_ptr<MaterialComponent>& material_out)
    {
        if(material_out.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        material_out.lock()->addTexture(loadTexture(path, type));
    }

    //Sprite
    void Loader::load(const char* path, std::weak_ptr<SpriteComponent>& sprite_out)
    {
        if(sprite_out.expired())
        {
            assert(!"destroyed component!");
            return;
        }
        
        SpriteComponent::Sprite sprite;
        
        Cutlass::HTexture handle;
        if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(path, handle))
            assert(!"failed to load sprite texture!");

        sprite.handles.resize(1);
        sprite.handles[0] = handle;
        
        sprite_out.lock()->create(sprite);          
    }

    //Sprite
    void Loader::load(std::vector<const char*> pathes, std::weak_ptr<SpriteComponent>& sprite_out)
    {
        if(sprite_out.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        SpriteComponent::Sprite sprite;

        sprite.handles.reserve(pathes.size());

        for(const auto& path : pathes)
        {
            Cutlass::HTexture handle;
            if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(path, handle))
                assert(!"failed to load sprite texture!");
            
            sprite.handles.emplace_back(handle);
        }
        
        sprite_out.lock()->create(sprite);            
    }

    MaterialComponent::Texture Loader::loadTexture(const char* path, const char* type)
    {
        MaterialComponent::Texture texture;
        if(!path)
        {
            assert(!"invalid texture path(nullptr)!");
            return texture;
        }
        texture.path = std::string(path);
        if(type)
            texture.type = std::string(type);
        else
            texture.type =  texture.path.substr(texture.path.find_last_of("/\\"), texture.path.size());

        if(Cutlass::Result::eSuccess != mContext->createTextureFromFile(path, texture.handle))
        {
            assert(!"failed to load texture!");
        }

        return texture;
    }
    
    //Font
    void Loader::load(const char* path, std::weak_ptr<TextComponent>& text_out)
    {

    }

}