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
        using UBO = int32_t;

        struct Data {
            UBO ubo = SR_ID_INVALID;
            void* pShaderHandle = nullptr;
            uint16_t uboSize = 0;

            void Validate() const {
                SRAssert(ubo != SR_ID_INVALID);
                SRAssert(pShaderHandle);
                SRAssert(uboSize != 0);
            }
        };

        VirtualUBOInfo() = default;
        ~VirtualUBOInfo() override = default;

        VirtualUBOInfo(VirtualUBOInfo&& ref) noexcept {
            data = SR_UTILS_NS::Exchange(ref.data, {});
            shared = ref.shared;
        }

        VirtualUBOInfo& operator=(VirtualUBOInfo&& ref) noexcept {
            data = SR_UTILS_NS::Exchange(ref.data, {});
            shared = ref.shared;
            return *this;
        }

        void Reset() noexcept {
            data.clear();
        }

        SR_NODISCARD bool Valid() const noexcept {
            return !data.empty();
        }

        std::vector<Data> data;

        /// UBO используется в нескольких шейдерах. Если выключен, то UBO будет создан для каждого шейдера
        bool shared = false;

    };

    /**
     * Класс реализует возможность рендера в несколько камер с нескольких ракурсов
    */
    class SR_DLL_EXPORT UBOManager : public SR_UTILS_NS::Singleton<UBOManager> {
        SR_REGISTER_SINGLETON(UBOManager)
        using Super = SR_UTILS_NS::Singleton<UBOManager>;
        using VirtualUBO = int32_t;
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
        ~UBOManager() override;

    public:
        void SetPipeline(PipelinePtr pPipeline);

    public:
        SR_NODISCARD VirtualUBO AllocateUBO(VirtualUBO virtualUbo, uint32_t uboSize, bool shared);
        SR_NODISCARD VirtualUBO AllocateUBO(VirtualUBO virtualUbo, uint32_t uboSize);
        SR_NODISCARD VirtualUBO AllocateUBO(VirtualUBO virtualUbo);

        bool FreeUBO(VirtualUBO* ubo);
        BindResult BindUBO(VirtualUBO ubo) noexcept;
        BindResult BindUBO(VirtualUBO ubo, uint32_t uboSize) noexcept;

        SR_NODISCARD UBO GetUBO(VirtualUBO virtualUbo) const noexcept;

    private:
        SR_NODISCARD bool AllocMemory(UBO* ubo, uint32_t uboSize);

    private:
        PipelinePtr m_pipeline;
        SR_HTYPES_NS::ObjectPool<VirtualUBOInfo, VirtualUBO> m_uboPool;

    };
}

#endif //SR_ENGINE_UBOMANAGER_H
