#pragma once

#include <Cutlass/Cutlass.hpp>

#include <glm/glm.hpp>

#include <vector>

#include "../Actors/IActor.hpp"

namespace Engine
{
    class MeshComponent;
    class SkeletalMeshComponent;
    class MaterialComponent;
    class CustomMaterialComponent;
    class LightComponent;
    class CameraComponent;
    class SpriteComponent;

    class Renderer
    {
    public:
        Renderer() = delete;

        Renderer(std::shared_ptr<Cutlass::Context> context, const std::vector<Cutlass::HWindow>& hwindows, const uint16_t frameCount = 3);

        //Noncopyable, Nonmoveable
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        virtual ~Renderer();

        virtual void add(const std::weak_ptr<MeshComponent>& mesh, const std::weak_ptr<MaterialComponent>& material, bool castShadow = true, bool receiveShadow = true, bool lighting = true);
        virtual void add(const std::weak_ptr<MeshComponent>& mesh, const std::weak_ptr<CustomMaterialComponent>& material);
        virtual void add(const std::weak_ptr<SkeletalMeshComponent>& skeletalMesh, const std::weak_ptr<MaterialComponent>& material, bool castShadow = true, bool receiveShadow = false, bool lighting = true);
        virtual void add(const std::weak_ptr<SpriteComponent>& sprite);
        virtual void add(const std::weak_ptr<LightComponent>& light);

        virtual void remove(const std::weak_ptr<MeshComponent>& mesh);
        virtual void remove(const std::weak_ptr<SkeletalMeshComponent>& skeletalmesh);
        virtual void remove(const std::weak_ptr<SpriteComponent>& sprite);
        virtual void remove(const std::weak_ptr<LightComponent>& light);

        virtual void clearScene();//Scene変更時に情報をアンロードする

        //TODO
        //virtual void addPostEffect();

        //このカメラになる
        virtual void setCamera(const std::weak_ptr<CameraComponent>& camera);

        //現在設定されている情報から描画用シーンをビルドする
        virtual void build();

        //描画コマンド実行
        virtual void render();

    protected:
        std::shared_ptr<Cutlass::Context> mContext;
        std::vector<Cutlass::HWindow> mHWindows;

    private:
        struct GBuffer//G-Buffer
        {
            Cutlass::HTexture albedoRT;
            Cutlass::HTexture normalRT;
            Cutlass::HTexture worldPosRT;
            Cutlass::HTexture depthBuffer;

            Cutlass::HRenderPass renderPass;
        };

        struct SceneData
        {
            glm::mat4 world;
            glm::mat4 view;
            glm::mat4 proj;
            float receiveShadow;
            float lighting;
            glm::vec2 padding;
        };

        struct Scene2DData
        {
            glm::mat4 proj;   
        };

        struct CameraData
        {
            glm::vec3 cameraPos;
        };

        #define MAX_LIGHT_NUM (16)
        struct LightData
        {
            uint32_t lightType;
            glm::vec3 lightDir;
            glm::vec4 lightColor;
        };

        struct ShadowData
        {
            glm::mat4 lightViewProj;
            glm::mat4 lightViewProjBias;
        };

        #define MAX_BONE_NUM (128)
        struct BoneData
        {
            BoneData()
            : useBone(0)
            {

            }
            uint32_t useBone;
            glm::vec3 padding;
            glm::mat4 boneTransform[MAX_BONE_NUM];
        };

        struct RenderInfo
        {
            bool skeletal;
            bool receiveShadow;
            bool lighting;
            std::weak_ptr<MeshComponent> mesh;
            std::weak_ptr<SkeletalMeshComponent> skeletalMesh;
            std::weak_ptr<MaterialComponent> material;

            Cutlass::HBuffer VB;
            Cutlass::HBuffer IB;

            Cutlass::HBuffer sceneCB;
            Cutlass::HBuffer boneCB;
            
            //Cutlass::HGraphicsPipeline geometryPipeline;
            //Cutlass::HGraphicsPipeline shadowPipeline;

            Cutlass::HCommandBuffer shadowSubCB;
            Cutlass::HCommandBuffer geometrySubCB;
        };

        struct SpriteInfo
        {
            std::weak_ptr<SpriteComponent> sprite;

            glm::uvec2 size;

            Cutlass::HBuffer VB;

            Cutlass::HCommandBuffer spriteSubCB;
        };

        const uint16_t mFrameCount;
        uint32_t mMaxWidth;
        uint32_t mMaxHeight;

        std::vector<Cutlass::HTexture> mDepthBuffers;

        std::vector<Cutlass::HTexture> mRTTexs;
        std::vector<std::pair<Cutlass::HRenderPass, Cutlass::HGraphicsPipeline>> mPresentPasses;

        GBuffer mGBuffer;
        Cutlass::Shader mShadowVS;
        Cutlass::Shader mShadowFS;
        Cutlass::Shader mDefferedSkinVS;
        Cutlass::Shader mDefferedSkinFS;
        Cutlass::Shader mLightingVS;
        Cutlass::Shader mLightingFS;
        Cutlass::Shader mSpriteVS;
        Cutlass::Shader mSpriteFS;


        Cutlass::HRenderPass mShadowPass;
        Cutlass::HTexture mShadowMap;
        Cutlass::HBuffer mShadowUB;
        Cutlass::HGraphicsPipeline mShadowPipeline;
    
        Cutlass::HRenderPass mLightingPass;
        Cutlass::HGraphicsPipeline mLightingPipeline;

        std::weak_ptr<CameraComponent> mCamera;
        Cutlass::HBuffer mCameraUB;
        std::vector<std::weak_ptr<LightComponent>> mLights;
        Cutlass::HBuffer mLightUB;

        Cutlass::HGraphicsPipeline mGeometryPipeline;
        std::vector<RenderInfo> mRenderInfos;

        Cutlass::HRenderPass mSpritePass;
        std::vector<SpriteInfo> mSpriteInfos;
        Cutlass::HBuffer mSpriteIB;
        Cutlass::HBuffer mSpriteUB;
        Cutlass::HGraphicsPipeline mSpritePipeline;

        Cutlass::HCommandBuffer mShadowCB;
        Cutlass::HCommandBuffer mGeometryCB;
        Cutlass::HCommandBuffer mLightingCB;
        Cutlass::HCommandBuffer mForwardCB;
        Cutlass::HCommandBuffer mPostEffectCB;
        Cutlass::HCommandBuffer mSpriteCB;
        std::vector<Cutlass::HCommandBuffer> mPresentCBs;

        //サブコマンドバッファ
        //std::vector<Cutlass::HCommandBuffer> mShadowSubs;
        //std::vector<Cutlass::HCommandBuffer> mGeometrySubs;
        // std::vector<Cutlass::HCommandBuffer> mLightingSubs;
        // std::vector<Cutlass::HCommandBuffer> mForwardSubs;
        // std::vector<Cutlass::HCommandBuffer> mPostEffectSubs;
        // std::vector<Cutlass::HCommandBuffer> mSpriteSubs;
 
        bool mShadowAdded;
        bool mGeometryAdded;
        bool mLightingAdded;
        bool mForwardAdded;
        bool mPostEffectAdded;
        bool mSpriteAdded;

        Cutlass::HTexture mDebugTex;//!
        Cutlass::HTexture mDebugSky;

        bool mSceneBuilded;
    };
};