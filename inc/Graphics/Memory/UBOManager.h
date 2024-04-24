//
// Created by Monika on 10.06.2022.
//

#ifndef SR_ENGINE_UBO_MANAGER_H
#define SR_ENGINE_UBO_MANAGER_H

#include <Utils/Common/Singleton.h>
#include <Utils/Types/Map.h>
#include <Utils/Types/ObjectPool.h>
#include <Utils/Types/SharedPtr.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class Pipeline;
}

namespace SR_GRAPH_NS::Memory {
    struct SR_DLL_EXPORT VirtualUBOInfo : public SR_UTILS_NS::NonCopyable {
        using Descriptor = int32_t;
        using UBO = int32_t;

        struct ShaderInfo {
            SR_GTYPES_NS::Shader* pShader;
            uint16_t samples;
            uint16_t uboSize;
            int32_t shaderProgram;
        };

        struct Data {
            void* pIdentifier;
            Descriptor descriptor;
            UBO ubo;
            ShaderInfo shaderInfo;

            void Validate() const;
        };

        VirtualUBOInfo() = default;
        ~VirtualUBOInfo() override = default;

        VirtualUBOInfo(VirtualUBOInfo&& ref) noexcept {
            m_data = SR_UTILS_NS::Exchange(ref.m_data, {});
        }

        VirtualUBOInfo& operator=(VirtualUBOInfo&& ref) noexcept {
            m_data = SR_UTILS_NS::Exchange(ref.m_data, {});
            return *this;
        }

        void Reset() noexcept {
            m_data.clear();
        }

        SR_NODISCARD bool Valid() const noexcept {
            return !m_data.empty();
        }

        std::vector<Data> m_data;

    };

    /**
     * Класс реализует возможность рендера в несколько камер с нескольких ракурсов
    */
    class SR_DLL_EXPORT UBOManager : public SR_UTILS_NS::Singleton<UBOManager> {
        SR_REGISTER_SINGLETON(UBOManager)
        using Super = SR_UTILS_NS::Singleton<UBOManager>;
        using VirtualUBO = int32_t;
        using Descriptor = int32_t;
        using UBO = int32_t;
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<Pipeline>;
    public:
        enum class BindResult : uint8_t {
            None,
            Success,
            Duplicated,
            Failed
        };
    private:
        UBOManager();
        ~UBOManager() override = default;

    public:
        void SetPipeline(PipelinePtr pPipeline) { m_pipeline = std::move(pPipeline); }
        void SetIdentifier(void* pIdentifier);
        void* GetIdentifier() const noexcept;

    public:
        SR_NODISCARD VirtualUBO ReAllocateUBO(VirtualUBO virtualUbo, uint32_t uboSize, uint32_t samples);
        SR_NODISCARD VirtualUBO AllocateUBO(uint32_t uboSize, uint32_t samples);
        bool FreeUBO(VirtualUBO* ubo);
        BindResult BindUBO(VirtualUBO ubo) noexcept;

    private:
        SR_NODISCARD bool AllocMemory(UBO* ubo, Descriptor* descriptor, uint32_t uboSize, uint32_t samples, int32_t shader);
        void FreeMemory(UBO* ubo, Descriptor* descriptor);

    private:
        PipelinePtr m_pipeline;
        SR_HTYPES_NS::ObjectPool<VirtualUBOInfo, VirtualUBO> m_uboPool;

        void* m_identifier = nullptr;

    };
}

#endif //SR_ENGINE_UBOMANAGER_H
