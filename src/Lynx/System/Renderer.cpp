#include <Lynx/System/Renderer.hpp>

#include <Lynx/Components/MaterialComponent.hpp>
#include <Lynx/Components/CustomMaterialComponent.hpp>
#include <Lynx/Components/MeshComponent.hpp>
#include <Lynx/Components/SkeletalMeshComponent.hpp>
#include <Lynx/Components/CameraComponent.hpp>
#include <Lynx/Components/LightComponent.hpp>
#include <Lynx/Components/SpriteComponent.hpp>

#include <iostream>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace Cutlass;

namespace Lynx
{

    inline glm::vec3 rotate2D(const glm::vec3& vec, const float cosine, const float sine)
    {
        glm::vec3 out = vec;
        out.x = cosine  * vec.x - sine      * vec.y;
        out.y = sine    * vec.x + cosine    * vec.y;
        return out;
    }


    Renderer::Renderer(std::shared_ptr<Context> context, const std::vector<HWindow>& hwindows, const uint16_t frameBufferNum)
    : mContext(context)
    , mHWindows(hwindows)
    , mFrameCount(frameBufferNum)
    , mSceneBuilded(false)
    , mShadowAdded(false)
    , mGeometryAdded(true)
    , mLightingAdded(false)
    , mForwardAdded(false)
    , mPostEffectAdded(false)
    , mSpriteAdded(false)
    {
        assert(Result::eSuccess == mContext->createTextureFromFile("../resources/textures/texture.png", mDebugTex));
        assert(Result::eSuccess == mContext->createTextureFromFile("../resources/textures/unitysky.png", mDebugSky));

        mRTTexs.reserve(hwindows.size());

        mMaxWidth = mMaxHeight = 0;

        for(const auto& hw : hwindows)
        {
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
            if(mMaxWidth < width)
                mMaxWidth = width;
            if(mMaxHeight < height)
                mMaxHeight = height;
        }

        //デフォルトシェーダ
        mShadowVS           = Shader("../resources/shaders/Shadow/shadow_vert.spv",     "VSMain");
        mShadowFS           = Shader("../resources/shaders/Shadow/shadow_frag.spv",     "PSMain");
        mDefferedSkinVS     = Shader("../resources/shaders/Deferred/GBuffer_vert.spv",  "VSMain");
        mDefferedSkinFS     = Shader("../resources/shaders/Deferred/GBuffer_frag.spv",  "PSMain");
        mLightingVS         = Shader("../resources/shaders/Deferred/Lighting_vert.spv", "VSMain");
        mLightingFS         = Shader("../resources/shaders/Deferred/Lighting_frag.spv", "PSMain");
        mSpriteVS           = Shader("../resources/shaders/Sprite/Sprite_vert.spv",     "VSMain");
        mSpriteFS           = Shader("../resources/shaders/Sprite/Sprite_frag.spv",     "PSMain");

        {//シャドウマップ、パス
            Cutlass::TextureInfo ti;
            constexpr uint32_t coef = 2;
            ti.setRTTex2DColor(mMaxWidth * coef, mMaxHeight * coef);
            if(Result::eSuccess != mContext->createTexture(ti, mShadowMap))
                assert(!"failed to create shadow map!");

             if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mShadowMap), mShadowPass))
                assert(!"failed to create shadow renderpass!");
        }

        {//G-Buffer 構築
            Cutlass::TextureInfo ti;
            constexpr uint32_t coef = 2;
            ti.setRTTex2DColor(mMaxWidth * coef, mMaxHeight * coef);
            mContext->createTexture(ti, mGBuffer.albedoRT);
            ti.format = Cutlass::ResourceType::eF32Vec4;
            mContext->createTexture(ti, mGBuffer.normalRT);
            mContext->createTexture(ti, mGBuffer.worldPosRT);
            ti.setRTTex2DDepth(mMaxWidth * coef, mMaxHeight * coef);
            mContext->createTexture(ti, mGBuffer.depthBuffer);

            if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo({mGBuffer.albedoRT, mGBuffer.normalRT, mGBuffer.worldPosRT}, mGBuffer.depthBuffer), mGBuffer.renderPass))
            {
                assert(!"failed to create G-Buffer renderpass!");
            }

            {
                GraphicsPipelineInfo gpi
                (
                    mDefferedSkinVS,
                    mDefferedSkinFS,
                    mGBuffer.renderPass,
                    DepthStencilState::eDepth,
                    RasterizerState(PolygonMode::eFill, CullMode::eBack, FrontFace::eCounterClockwise),
                    Topology::eTriangleList
                );
        
                if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, mGeometryPipeline))
                    assert(!"failed to create geometry pipeline");
            }
        }

        for(const auto& hw : hwindows)
        {
            HRenderPass rp;
            mContext->createRenderPass(RenderPassInfo(hw), rp);
            GraphicsPipelineInfo gpi
            (
                Shader("../resources/shaders/present/vert.spv", "main"),
                Shader("../resources/shaders/present/frag.spv", "main"),
                rp,
                DepthStencilState::eNone,
                RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise, 1.f),
                Topology::eTriangleStrip,
                ColorBlend::eDefault,
                MultiSampleState::eDefault
            );

            HGraphicsPipeline gp;
            if (Result::eSuccess != mContext->createGraphicsPipeline(gpi, gp))
                assert(!"Failed to create present pipeline!\n");  
            
            mPresentPasses.emplace_back(rp, gp);

            TextureInfo ti;
            uint32_t width, height;
            mContext->getWindowSize(hw, width, height);
                
            ti.setRTTex2DColor(width, height);
            mContext->createTexture(ti, mRTTexs.emplace_back());
            ti.setRTTex2DDepth(width, height);
            mContext->createTexture(ti, mDepthBuffers.emplace_back());

            {//テクスチャの画面表示用コマンドを作成
                ShaderResourceSet SRSet[mFrameCount];
                for(size_t i = 0; i < mFrameCount; ++i)
                    SRSet[i].bind(0, mRTTexs.back());
                    //SRSet[i].bind(0, mGBuffer.normalRT);
                
                for(auto& wp : mPresentPasses)
                {
                    std::vector<Cutlass::CommandList> cls;
                    cls.resize(mFrameCount);

                    Cutlass::ColorClearValue ccv{1.f, 0, 0, 0};
                    Cutlass::DepthClearValue dcv(1.f, 0);

                    for(size_t i = 0; i < cls.size(); ++i)
                    {
                        //for(const auto& target : mRTTexs)
                        cls[i].barrier(mRTTexs.back());
                        cls[i].begin(wp.first, true, ccv, dcv);
                        cls[i].bind(wp.second);
                        cls[i].bind(0, SRSet[i]);
                        cls[i].render(4, 1, 0, 0);
                        cls[i].end();
                    }

                    mPresentCBs.emplace_back();
                    mContext->createCommandBuffer(cls, mPresentCBs.back());
                }
            }
        }


        {//ライティング用パス
            //注意 : 複数ウィンドウ対応を後回しにしている
            if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mRTTexs.back(), true), mLightingPass))
                assert(!"failed to create lighting renderpass!");

            GraphicsPipelineInfo gpi
            (
                mLightingVS,
                mLightingFS,
                mLightingPass,
                DepthStencilState::eNone,
                RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
                Topology::eTriangleStrip
            );

            mContext->createGraphicsPipeline(gpi, mLightingPipeline);
        }

        //デフォルトコマンド構築
        CommandList cl;
        cl.clear();
        mContext->createCommandBuffer(cl, mShadowCB);
        mContext->createCommandBuffer(cl, mGeometryCB);
        mContext->createCommandBuffer(cl, mLightingCB);
        mContext->createCommandBuffer(cl, mForwardCB);
        mContext->createCommandBuffer(cl, mPostEffectCB);
        mContext->createCommandBuffer(cl, mSpriteCB);
        
        {//カメラ用バッファを作成しておく
            BufferInfo bi;
            bi.setUniformBuffer<CameraData>();
            if(Result::eSuccess != mContext->createBuffer(bi, mCameraUB))
                assert(!"failed to create camera UB!");
        }
        
        {//ライト用バッファも作成しておく
            BufferInfo bi;
            bi.setUniformBuffer<LightData>(MAX_LIGHT_NUM);
            if(Result::eSuccess != mContext->createBuffer(bi, mLightUB))
                assert(!"failed to create light UB!");
        }
        
        {//シャドウ用
            BufferInfo bi;
            bi.setUniformBuffer<ShadowData>();
            if(Result::eSuccess != mContext->createBuffer(bi, mShadowUB))
                assert(!"failed to create shadow UB!");

            {
            
                GraphicsPipelineInfo gpi
                (
                    mShadowVS,
                    mShadowFS,
                    mShadowPass,
                    DepthStencilState::eDepth,
                    RasterizerState(PolygonMode::eFill, CullMode::eBack, FrontFace::eCounterClockwise),
                    Topology::eTriangleList
                );

                if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, mShadowPipeline))
                    assert(!"failed to create shadow pipeline");
            }

        }

        // {//シャドウマップ、パス
        //     Cutlass::TextureInfo ti;
        //     ti.setRTTex2DColor(mMaxWidth, mMaxHeight);
        //     if(Result::eSuccess != mContext->createTexture(ti, mShadowMap))
        //         assert(!"failed to create shadow map!");

        //      if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mShadowMap), mShadowPass))
        //         assert(!"failed to create shadow renderpass!");
        // }

        {//ライティング用コマンドバッファ
            ShaderResourceSet bufferSet, textureSet;
            {
                bufferSet.bind(0, mLightUB);
                bufferSet.bind(1, mCameraUB);
                bufferSet.bind(2, mShadowUB);

                textureSet.bind(0, mGBuffer.albedoRT);
                textureSet.bind(1, mGBuffer.normalRT);
                textureSet.bind(2, mGBuffer.worldPosRT);
                textureSet.bind(3, mShadowMap);
            }

            CommandList cl;
            cl.begin(mLightingPass);
            cl.bind(mLightingPipeline);
            cl.bind(0, bufferSet);
            cl.bind(1, textureSet);
            cl.render(4, 1, 0, 0);
            cl.end();
            mContext->updateCommandBuffer(cl, mLightingCB);
        }

        {//スプライト用
            if(Result::eSuccess != mContext->createRenderPass(RenderPassInfo(mRTTexs.back(), true), mSpritePass))
                assert(!"failed to create sprite render pass!");

            std::array<uint32_t, 6> indices = 
            {{
                0, 2, 1, 1, 2, 3
            }};

            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(6);
            mContext->createBuffer(bi, mSpriteIB);
            mContext->writeBuffer(6 * sizeof(uint32_t), indices.data(), mSpriteIB);

            Scene2DData scene2DData;
            scene2DData.proj = 
            {
                2.f / mMaxWidth, 0.f, 0.f, 0,
                0.f, 2.f / mMaxHeight, 0.f, 0,
                0.f, 0.f, 1.f, 0.f,
                -1.f, -1.f, 0.f, 1.f
            };
            
            bi.setUniformBuffer<Scene2DData>();
            mContext->createBuffer(bi, mSpriteUB);
            mContext->writeBuffer(sizeof(Scene2DData), &scene2DData, mSpriteUB);
            
            GraphicsPipelineInfo gpi
            (
                mSpriteVS,
                mSpriteFS,
                mSpritePass,
                DepthStencilState::eNone,
                RasterizerState(PolygonMode::eFill, CullMode::eNone, FrontFace::eClockwise),
                Topology::eTriangleList,
                ColorBlend::eAlphaBlend
            );
    
            if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, mSpritePipeline))
                assert(!"failed to create sprite pipeline");
        }
    }

    Renderer::~Renderer()
    {
        
    }

    //StaticMesh
    void Renderer::add(const std::weak_ptr<MeshComponent>& mesh, const std::weak_ptr<MaterialComponent>& material, bool castShadow, bool receiveShadow, bool lighting)
    {
        if(mesh.expired() || material.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        const std::shared_ptr<MeshComponent>& mesh_ = mesh.lock();
        const std::shared_ptr<MaterialComponent>& material_ = material.lock();

        auto& tmp = mRenderInfos.emplace_back();
        tmp.skeletal = false;
        tmp.receiveShadow = receiveShadow;
        tmp.lighting = lighting;
        tmp.mesh = mesh;
        tmp.material = material;

        //頂点バッファ、インデックスバッファ構築
        {
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<MeshComponent::Vertex>(mesh_->getVertexNum());
            mContext->createBuffer(bi, tmp.VB);
            mContext->writeBuffer(mesh_->getVertices().size() * sizeof(MeshComponent::Vertex), mesh_->getVertices().data(), tmp.VB);
        }

        {
            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(mesh_->getIndexNum());
            mContext->createBuffer(bi, tmp.IB);
            mContext->writeBuffer(mesh_->getIndices().size() * sizeof(uint32_t), mesh_->getIndices().data(), tmp.IB);
        }

        //定数バッファ構築
        {
            Cutlass::BufferInfo bi;
            {//WVP
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, tmp.sceneCB);
            }

            {//ボーン
                //注意 : いまのところおいとく
                BoneData data;

                bi.setUniformBuffer<BoneData>();
                mContext->createBuffer(bi, tmp.boneCB);
                mContext->writeBuffer(sizeof(BoneData), &data, tmp.boneCB);
            }

        }

        // if(castShadow)
        // {//シャドウマップ用パス
        
        //     GraphicsPipelineInfo gpi
        //     (
        //         mShadowVS,
        //         mShadowFS,
        //         mShadowPass,
        //         DepthStencilState::eDepth,
        //         mesh->getRasterizerState(),
        //         mesh->getTopology()
        //     );

        //     if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.shadowPipeline))
        //         assert(!"failed to create shadow pipeline");
        // }

        // {
        //     GraphicsPipelineInfo gpi
        //     (
        //         mDefferedSkinVS,
        //         mDefferedSkinFS,
        //         mGBuffer.renderPass,
        //         DepthStencilState::eDepth,
        //         mesh->getRasterizerState(),
        //         mesh->getTopology()
        //     );
    
        //     if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, tmp.geometryPipeline))
        //         assert(!"failed to create geometry pipeline");
        // }

        {//コマンド作成

            //シャドウパス
            if(castShadow)
            {
                ShaderResourceSet bufferSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    bufferSet.bind(1, mShadowUB);
                    bufferSet.bind(2, tmp.boneCB);
                }

                SubCommandList scl(mShadowPass);
                scl.bind(mShadowPipeline);

                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.renderIndexed(mesh_->getIndexNum(), 1, 0, 0, 0);

                //HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, tmp.shadowSubCB))
                    assert(!"failed to create command buffer!");
                //mShadowSubs.emplace_back(cb);
            }

            //ジオメトリパス
            {
                ShaderResourceSet bufferSet;
                ShaderResourceSet textureSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    // if(!material->getMaterialSets().empty())
                    //     bufferSet.bind(1, material->getMaterialSets().back().paramBuffer);
                    // else
                    // {
                    //     //マテリアル読む
                    // }
                    bufferSet.bind(1, tmp.boneCB);

                    auto&& textures = material_->getTextures();

                    if(textures.empty())
                        textureSet.bind(0, mDebugTex);
                    else
                        textureSet.bind(0, textures[0].handle);
                }

                SubCommandList scl(mGBuffer.renderPass);
                scl.bind(mGeometryPipeline);
                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.bind(1, textureSet);
                scl.renderIndexed(mesh_->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, tmp.geometrySubCB))
                    assert(!"failed to create command buffer!");
                //mGeometrySubs.emplace_back(cb);
            }
        }
        
        //影コントロール実装時注意
        mShadowAdded = true;
        mGeometryAdded = true;
        //std::cerr << "registed\n";
    }

    //Custom
    void Renderer::add(const std::weak_ptr<MeshComponent>& mesh, const std::weak_ptr<CustomMaterialComponent>& material)
    {
        assert(!"TODO");
        mForwardAdded = true;
    }

    //SkeletalMesh
    void Renderer::add(const std::weak_ptr<SkeletalMeshComponent>& skeletalMesh, const std::weak_ptr<MaterialComponent>& material, bool castShadow, bool receiveShadow, bool lighting)
    {
        if(skeletalMesh.expired() || material.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        auto& tmp = mRenderInfos.emplace_back();
        tmp.skeletal = true;
        tmp.receiveShadow = receiveShadow;
        tmp.lighting = lighting;
        tmp.mesh = static_cast<std::shared_ptr<MeshComponent>>(skeletalMesh);
        tmp.skeletalMesh = skeletalMesh;
        tmp.material = material;

        const auto& skeletalMesh_ = skeletalMesh.lock();
        const auto& material_ = material.lock();

        //頂点バッファ、インデックスバッファ構築
        {
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<MeshComponent::Vertex>(skeletalMesh_->getVertexNum());
            mContext->createBuffer(bi, tmp.VB);
            mContext->writeBuffer(skeletalMesh_->getVertices().size() * sizeof(MeshComponent::Vertex), skeletalMesh_->getVertices().data(), tmp.VB);
        }

        {
            Cutlass::BufferInfo bi;
            bi.setIndexBuffer<uint32_t>(skeletalMesh_->getIndexNum());
            mContext->createBuffer(bi, tmp.IB);
            mContext->writeBuffer(skeletalMesh_->getIndices().size() * sizeof(uint32_t), skeletalMesh_->getIndices().data(), tmp.IB);
        }

        //定数バッファ構築
        {
            Cutlass::BufferInfo bi;
            {//WVP
                bi.setUniformBuffer<SceneData>();
                mContext->createBuffer(bi, tmp.sceneCB);
            }

            {//ボーン
                //注意 : いまのところおいとく
                //BoneData data;

                bi.setUniformBuffer<BoneData>();
                mContext->createBuffer(bi, tmp.boneCB);
                //mContext->writeBuffer(sizeof(BoneData), &data, tmp.boneCB);
            }

        }

        // if(castShadow)
        // {//シャドウマップ用パス
        //     GraphicsPipelineInfo gpi
        //     (
        //         mShadowVS,
        //         mShadowFS,
        //         mShadowPass,
        //         DepthStencilState::eDepth,
        //         skeletalMesh->getRasterizerState(),
        //         skeletalMesh->getTopology()
        //     );

        //     if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, mShadowPipeline))
        //         assert(!"failed to create shadow pipeline");
        // }

        // {
        //     GraphicsPipelineInfo gpi
        //     (
        //         mDefferedSkinVS,
        //         mDefferedSkinFS,
        //         mGBuffer.renderPass,
        //         DepthStencilState::eDepth,
        //         skeletalMesh->getRasterizerState(),
        //         skeletalMesh->getTopology()
        //     );
    
        //     if(Cutlass::Result::eSuccess != mContext->createGraphicsPipeline(gpi, mGeometryPipeline))
        //         assert(!"failed to create geometry pipeline");
        // }

        {//コマンド作成

            //シャドウパス
            if(castShadow)
            {
                ShaderResourceSet bufferSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    bufferSet.bind(1, mShadowUB);
                    bufferSet.bind(2, tmp.boneCB);
                }

                SubCommandList scl(mShadowPass);
                scl.bind(mShadowPipeline);

                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.renderIndexed(skeletalMesh_->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, tmp.shadowSubCB))
                    assert(!"failed to create command buffer!");
                //mShadowSubs.emplace_back(cb);
            }

            //ジオメトリパス
            {
                ShaderResourceSet bufferSet;
                ShaderResourceSet textureSet;
                {
                    bufferSet.bind(0, tmp.sceneCB);
                    // if(!material->getMaterialSets().empty())
                    //     bufferSet.bind(1, material->getMaterialSets().back().paramBuffer);
                    // else
                    // {
                    //     //マテリアル読む
                    // }
                    bufferSet.bind(1, tmp.boneCB);

                    auto&& textures = material_->getTextures();

                    if(textures.empty())
                        textureSet.bind(0, mDebugTex);
                    else
                        textureSet.bind(0, textures[0].handle);
                }

                SubCommandList scl(mGBuffer.renderPass);
                scl.bind(mGeometryPipeline);
                scl.bind(tmp.VB, tmp.IB);
                scl.bind(0, bufferSet);
                scl.bind(1, textureSet);
                scl.renderIndexed(skeletalMesh_->getIndexNum(), 1, 0, 0, 0);

                HCommandBuffer cb;
                if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, tmp.geometrySubCB))
                    assert(!"failed to create command buffer!");
                //mGeometrySubs.emplace_back(cb);
            }
        }
        
        //影コントロール実装時注意
        mShadowAdded = castShadow;
        mGeometryAdded = true;
    }

    //Sprite
    void Renderer::add(const std::weak_ptr<SpriteComponent>& sprite)
    {
        if(sprite.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        auto&& tmp = mSpriteInfos.emplace_back();
        tmp.sprite = sprite;

        const auto& sprite_ = sprite.lock();

        constexpr glm::vec2 lb(0.0f, 0.0f);
        constexpr glm::vec2 lt(0.0f, 1.0f);
        constexpr glm::vec2 rb(1.0f, 0.0f);
        constexpr glm::vec2 rt(1.0f, 1.0f);

        {
            uint32_t d;
            if(Result::eSuccess != mContext->getTextureSize(sprite_->getSpriteTexture(), tmp.size.x, tmp.size.y, d))
            {
                assert(!"failed to get sprite texture size!");
                return;
            }
            assert(d == 1);
        }

        std::array<SpriteComponent::Vertex, 4> vertices = 
        {{
            {glm::vec3(0, 0, 0), lb},
            {glm::vec3(tmp.size.x, 0, 0), rb},
            {glm::vec3(0, tmp.size.y, 0), lt},
            {glm::vec3(tmp.size.x, tmp.size.y, 0), rt},
        }};

        //頂点バッファ構築
        {
            Cutlass::BufferInfo bi;
            bi.setVertexBuffer<SpriteComponent::Vertex>(4);
            mContext->createBuffer(bi, tmp.VB);
            mContext->writeBuffer(4 * sizeof(SpriteComponent::Vertex), vertices.data(), tmp.VB);
        }

        {//サブコマンド作成

            ShaderResourceSet bufferSet, textureSet;
            {
                bufferSet.bind(0, mSpriteUB);

                textureSet.bind(0, sprite_->getSpriteTexture());
            }

            SubCommandList scl(mSpritePass);
            scl.bind(mSpritePipeline);
            scl.bind(tmp.VB, mSpriteIB);
            scl.bind(0, bufferSet);
            scl.bind(1, textureSet);
            scl.renderIndexed(6, 1, 0, 0, 0);

            HCommandBuffer cb;
            if(Cutlass::Result::eSuccess != mContext->createSubCommandBuffer(scl, tmp.spriteSubCB))
                assert(!"failed to create command buffer!");
            //mSpriteSubs.emplace_back(cb);
        }

        mSpriteAdded = true;
    }

    //Light
    void Renderer::add(const std::weak_ptr<LightComponent>& light)
    {
        if(light.expired())
        {
            assert(!"destroyed component!");
            return;
        }
        
        mLights.emplace_back(light);

        mLightingAdded = true;
    }

    void Renderer::setCamera(const std::weak_ptr<CameraComponent>& camera)
    {
        if(camera.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        mCamera = camera;
    }
    
    void Renderer::remove(const std::weak_ptr<MeshComponent>& mesh)
    {
        if(mesh.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        mRenderInfos.erase(std::remove_if(mRenderInfos.begin(), mRenderInfos.end(), 
        [&](RenderInfo& ri)
        {
            if(ri.mesh.lock() == mesh.lock())
            {
                // if(ri.mesh)
                //     ri.mesh.reset();
                // if(ri.skeletalMesh)
                //     ri.skeletalMesh.reset();
                // if(ri.material)
                //     ri.material.reset();

                mContext->destroyBuffer(ri.VB);
                mContext->destroyBuffer(ri.IB);
                mContext->destroyBuffer(ri.sceneCB);
                mContext->destroyBuffer(ri.boneCB);

                mContext->destroyCommandBuffer(ri.shadowSubCB);
                mContext->destroyCommandBuffer(ri.geometrySubCB);
                mShadowAdded = mGeometryAdded = true;
                return true;
            }

            return false;
        }), mRenderInfos.end());
    }
    
    void Renderer::remove(const std::weak_ptr<SkeletalMeshComponent>& skeletalMesh)
    {
        if(skeletalMesh.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        mRenderInfos.erase(std::remove_if(mRenderInfos.begin(), mRenderInfos.end(), 
        [&](RenderInfo& ri)
        {
            if(ri.mesh.lock() == skeletalMesh.lock() || ri.skeletalMesh.lock() == skeletalMesh.lock())
            {
                // if(ri.mesh)
                //     ri.mesh.reset();
                // if(ri.skeletalMesh)
                //     ri.skeletalMesh.reset();
                // if(ri.material)
                //     ri.material.reset();

                mContext->destroyBuffer(ri.VB);
                mContext->destroyBuffer(ri.IB);
                mContext->destroyBuffer(ri.sceneCB);
                mContext->destroyBuffer(ri.boneCB);

                mContext->destroyCommandBuffer(ri.shadowSubCB);
                mContext->destroyCommandBuffer(ri.geometrySubCB);
                mShadowAdded = mGeometryAdded = true;
                return true;
            }

            return false;
        }), mRenderInfos.end());
    }
    
    void Renderer::remove(const std::weak_ptr<SpriteComponent>& sprite)
    {
        if(sprite.expired())
        {
            assert(!"destroyed component!");
            return;
        }

        mSpriteInfos.erase(std::remove_if(mSpriteInfos.begin(), mSpriteInfos.end(), 
        [&](SpriteInfo& si)
        {
            if(si.sprite.lock() == sprite.lock())
            {
                si.sprite.reset();

                mContext->destroyBuffer(si.VB);
                mContext->destroyCommandBuffer(si.spriteSubCB);
                mSpriteAdded = true;

                return true;
            }

            return false;
        }), mSpriteInfos.end());
    }

    void Renderer::remove(const std::weak_ptr<LightComponent>& light)
    {
        mLights.erase(std::remove_if(mLights.begin(), mLights.end(), 
        [&](const std::weak_ptr<LightComponent>& l){return l.lock() == light.lock();}), mLights.end());
    }

    void Renderer::clearScene()
    {

        //mCamera.reset();
        mLights.clear();

        for(auto& ri : mRenderInfos)
        {
            // if(ri.mesh)
            //     ri.mesh.reset();
            // if(ri.skeletalMesh)
            //     ri.skeletalMesh.reset();
            // if(ri.material)
            //     ri.material.reset();

            mContext->destroyBuffer(ri.VB);
            mContext->destroyBuffer(ri.IB);
            mContext->destroyBuffer(ri.sceneCB);
            mContext->destroyBuffer(ri.boneCB);
            // mContext->destroyGraphicsPipeline(ri.geometryPipeline);
            // mContext->destroyGraphicsPipeline(ri.shadowPipeline);

            mContext->destroyCommandBuffer(ri.shadowSubCB);
            mContext->destroyCommandBuffer(ri.geometrySubCB);
        }

        for(auto& si : mSpriteInfos)
        {
            //si.sprite.reset();

            mContext->destroyBuffer(si.VB);

            mContext->destroyCommandBuffer(si.spriteSubCB);
        }

        mRenderInfos.clear();
        mSpriteInfos.clear();

        mShadowAdded = false;
        mGeometryAdded = false;
        mLightingAdded = false;
        mSpriteAdded = false;

        // for(const auto& cb : mShadowSubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        // for(const auto& cb : mGeometrySubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        // for(const auto& cb : mLightingSubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        // for(const auto& cb : mForwardSubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        // for(const auto& cb : mPostEffectSubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        // for(const auto& cb : mSpriteSubs)
        // {
        //     mContext->destroyCommandBuffer(cb);
        // }

        // mShadowSubs.clear();
        // mGeometrySubs.clear();
        // //mLightingSubs.clear();
        // mForwardSubs.clear();
        // mPostEffectSubs.clear();
        // mSpriteSubs.clear();
    }

    void Renderer::build()
    {

        if(mCamera.expired() || !mCamera.lock()->getEnable())
        {
           assert(!"no camera!");//カメラがないとなにも映りません
           return;
        }
        
        {//各定数バッファを書き込み
            SceneData sceneData;
            {//各ジオメトリ共通データ
                sceneData.view = mCamera.lock()->getViewMatrix();
                sceneData.proj = mCamera.lock()->getProjectionMatrix();
            }

            //for(auto& ri : mRenderInfos)
            mRenderInfos.erase(std::remove_if(mRenderInfos.begin(), mRenderInfos.end(), 
            [&](RenderInfo& ri)
            {
                if(ri.mesh.expired() && ri.skeletalMesh.expired())
                    return true;
                
                //ジオメトリ固有パラメータセット
                sceneData.world = ri.mesh.lock()->getTransform().getWorldMatrix();
                sceneData.receiveShadow = ri.receiveShadow ? 1.f : 0;
                sceneData.lighting = ri.lighting ? 1.f : 0;
                mContext->writeBuffer(sizeof(SceneData), &sceneData, ri.sceneCB);

                if(!ri.skeletal)
                    return false;

                BoneData data;
                const auto& bones = ri.skeletalMesh.lock()->getBones();
                const auto&& identity = glm::mat4(0.f);
                data.useBone = 1;
                for(size_t i = 0; i < MAX_BONE_NUM; ++i)
                {
                    if(i >= bones.size())
                    {
                        data.boneTransform[i] = identity;
                        continue;
                    }

                    data.boneTransform[i] = bones[i].transform;
                }
                mContext->writeBuffer(sizeof(BoneData), &data, ri.boneCB);
                return false;

            }), mRenderInfos.end());

            {
                Cutlass::BufferInfo bi;
                bi.setVertexBuffer<SpriteComponent::Vertex>(4);

                //float x, y, w, h;

                glm::vec3 lu(0), ld(0), ru(0), rd(0);
                float c, s;//cache of cosine, sine

                //for(auto& si : mSpriteInfos)
                mSpriteInfos.erase(std::remove_if(mSpriteInfos.begin(), mSpriteInfos.end(), 
                [&](SpriteInfo& si)
                {
                    if(si.sprite.expired())
                        return true;

                    const auto& transform = si.sprite.lock()->getTransform();
                    const auto& scale = transform.getScale();
                    //const auto& rotAxis = transform.getRotAxis();
                    const auto& rotAngle = transform.getRotAngle();
                    c = cos(rotAngle);
                    s = sin(rotAngle);
                    const auto& pos = transform.getPos();
                    // x = pos.x;
                    // y = pos.y;
                    // w = si.size.x * scale.x;
                    // h = si.size.y * scale.y;
                    // if(si.sprite->getCenterFlag())
                    // {
                    //     x -= (w / 2.f);
                    //     y -= (h / 2.f);
                    // }

                    if(si.sprite.lock()->getCenterFlag())
                    {
                        rd.x = ru.x = 1.f * scale.x * si.size.x / 2.f;
                        ld.y = rd.y = 1.f * scale.y * si.size.y / 2.f;
                        lu.x = ld.x = -1.f * scale.x * si.size.x / 2.f;
                        lu.y = ru.y = -1.f * scale.y * si.size.y / 2.f;
                        lu = rotate2D(lu, c, s);
                    }
                    else
                    {
                        rd.x = ru.x = si.size.x;
                        rd.y = ld.y = si.size.y;
                    }

                    ld = rotate2D(ld, c, s);
                    ru = rotate2D(ru, c, s);
                    rd = rotate2D(rd, c, s);
                    lu += pos;
                    ld += pos;
                    ru += pos;
                    rd += pos;
                    lu.z = std::min(std::max(0.f, pos.z), 1.f);
                    ld.z = std::min(std::max(0.f, pos.z), 1.f);
                    ru.z = std::min(std::max(0.f, pos.z), 1.f);
                    rd.z = std::min(std::max(0.f, pos.z), 1.f);
                    
                    // std::cerr << "sprite pos : " << glm::to_string(lu) << "\n";
                    // std::cerr << "sprite pos : " << glm::to_string(ld) << "\n";
                    // std::cerr << "sprite pos : " << glm::to_string(ru) << "\n";
                    // std::cerr << "sprite pos : " << glm::to_string(rd) << "\n";

                    //ここでスプライトの頂点位置を更新する
                    std::array<SpriteComponent::Vertex, 4> vertices = 
                    {{
                        {lu, glm::vec2(0,    0   )},
                        {ru, glm::vec2(1.f,  0   )},
                        {ld, glm::vec2(0,    1.f )},
                        {rd, glm::vec2(1.f,  1.f )},
                    }};

                    //頂点バッファ構築
                    {
                        mContext->writeBuffer(4 * sizeof(SpriteComponent::Vertex), vertices.data(), si.VB);
                    }
                    return false;
                }), mSpriteInfos.end());
            }

            {//カメラ
                CameraData data;
                data.cameraPos = mCamera.lock()->getTransform().getPos();
                //std::cerr << "camera pos : " << glm::to_string(data.cameraPos) << "\n";
                mContext->writeBuffer(sizeof(CameraData), &data, mCameraUB);
            }

            {//ライト
                LightData data[MAX_LIGHT_NUM];

                for(uint32_t i = 0; i < MAX_LIGHT_NUM; ++i)
                {
                    if(i >= mLights.size())
                    {
                        data[i].lightType = 0;
                        data[i].lightColor = glm::vec4(0);
                        data[i].lightDir = glm::vec3(0);
                        continue;
                    }
                    
                    if(mLights[i].expired())
                        continue;

                    switch(mLights[i].lock()->getType())
                    {
                        case LightComponent::LightType::eDirectionalLight:
                            data[i].lightType = 0;
                        break;
                        case LightComponent::LightType::ePointLight:
                            data[i].lightType = 1;
                            assert(!"TODO");
                        break;
                        default:
                            assert(!"invalid light type!");
                        break;
                    }
                    data[i].lightColor = mLights[i].lock()->getColor();
                    data[i].lightDir = mLights[i].lock()->getDirection();
                }

                // std::cerr << sizeof(LightData) << "\n";
                // assert(0);

                // for(uint32_t i = 0; i < MAX_LIGHT_NUM; ++i)
                // {
                //     std::cerr << i << "\n";
                //     std::cerr << "color : " << glm::to_string(data[i].lightColor) << "\n";
                //     std::cerr << "dir : " << glm::to_string(data[i].lightDir) << "\n\n";
                // }

                if(Result::eSuccess != mContext->writeBuffer(sizeof(LightData) * MAX_LIGHT_NUM, &data, mLightUB))
                {
                    assert(!"failed to create light buffer!");
                }

                //shadow用
                if(mLights.empty())
                {
                    mShadowAdded = false;
                }
                else
                {
                    ShadowData data;
                    glm::vec3 invDir = mLights[0].lock()->getDirection() * -10.f;
                    auto view = glm::lookAtRH(invDir, glm::vec3(0, 0, 0), glm::vec3(0, 1.f, 0));
                    auto proj = glm::perspective(glm::radians(60.f), 1.f * mMaxWidth / mMaxHeight, 1.f, 1000.f);
                    //auto proj = glm::ortho(-10.f, 10.0f, -5.0f, 30.0f, 0.5f, 50.0f);
                    proj[1][1] *= -1;
                    auto&& matBias =  glm::translate(glm::mat4(1.0f), glm::vec3(0.5f,0.5f,0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
                    data.lightViewProj = proj * view;
                    data.lightViewProjBias = matBias * data.lightViewProj;
                    //data.lightViewProj = glm::perspective(glm::radians(45.f), 1.f * width / height, 1.f, 1000.f) * glm::lookAtRH(glm::vec3(0), glm::vec3(0, 0, -10.f), glm::vec3(0, 1.f, 0));
                    mContext->writeBuffer(sizeof(ShadowData), &data, mShadowUB);
                }
            }

        }

        //サブコマンドバッファ積み込み
        //if(0)
        if(mShadowAdded)
        {
            //std::cerr << "build shadow\n";
            CommandList cl;

            cl.begin(mShadowPass);
            for(const auto& ri : mRenderInfos)
                cl.executeSubCommand(ri.shadowSubCB);
            cl.end();
            mContext->updateCommandBuffer(cl, mShadowCB);
            mShadowAdded = false;
        }
        if(mGeometryAdded)
        {
            //std::cerr << "build geom\n";
            CommandList cl;
            cl.barrier(mGBuffer.albedoRT);
            cl.barrier(mGBuffer.normalRT);
            cl.barrier(mGBuffer.worldPosRT);
            cl.begin(mGBuffer.renderPass);
            for(const auto& ri : mRenderInfos)
            {
                if(ri.skeletal)
                {
                    if(ri.skeletalMesh.expired() || !ri.skeletalMesh.lock()->getEnable() || !ri.skeletalMesh.lock()->getVisible())
                        continue;
                }
                else
                {
                    if(ri.mesh.expired() || !ri.mesh.lock()->getEnable() || !ri.mesh.lock()->getVisible())
                        continue;
                }
                cl.executeSubCommand(ri.geometrySubCB);
            }
            cl.end();
            mContext->updateCommandBuffer(cl, mGeometryCB);
           //mGeometryAdded = false;
        }
        // if(mForwardAdded)
        // {
        //     CommandList cl;
        //     cl.begin();
        //     for(const auto& sub : mLightingSubs)
        //         cl.executeSubCommand(sub);
        //     cl.end();
        //     mContext->updateCommandBuffer(cl, mLightingCB);
        // }
        if(mPostEffectAdded)
        {
            mPostEffectAdded = false;
        }
        if(mSpriteAdded)
        {
            //std::cerr << "build sprite\n";
            CommandList cl;
            cl.begin(mSpritePass);
            for(const auto& si : mSpriteInfos)
            {
                if(!si.sprite.expired() && si.sprite.lock()->getEnable() && si.sprite.lock()->getVisible())
                    cl.executeSubCommand(si.spriteSubCB);
            }
            cl.end();
            mContext->updateCommandBuffer(cl, mSpriteCB);
            //mSpriteAdded = false;
        }

        mSceneBuilded = true;
    }

    void Renderer::render()
    {
        if(!mSceneBuilded)
        {
            assert(!"scene wasn't builded!");
            return;
        }

       // std::cerr << "render start\n";
        if(!mRenderInfos.empty())
        {
            mContext->execute(mShadowCB);
            //std::cerr << "shadow\n";
        }
            mContext->execute(mGeometryCB);
            //std::cerr << "geom\n";
            mContext->execute(mLightingCB);
            //std::cerr << "light\n";
        

        //mContext->execute(mForwardCB);
        //mContext->execute(mPostEffectCB);
        if(!mSpriteInfos.empty())
        {
            mContext->execute(mSpriteCB);
            //std::cerr << "sprite\n";
        }

        for(const auto& cb : mPresentCBs)
            mContext->execute(cb);
        //std::cerr << "present\n";
    }
}