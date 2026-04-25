//
// Created by Nikita on 17.11.2020.
//

#ifndef SR_ENGINE_GRAPHICS_SHADER_H
#define SR_ENGINE_GRAPHICS_SHADER_H

#include <Graphics/Memory/ShaderUBOBlock.h>
#include <Graphics/Loaders/SRSL.h>
#include <Graphics/Memory/ShaderProgramManager.h>
#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Memory/UBOManager.h>

#include <Utils/Common/NonCopyable.h>
#include <Utils/Common/Hashes.h>
#include <Utils/Resources/IResource.h>
#include <Utils/Math/Rect.h>

namespace SR_GTYPES_NS {
    class Texture;
}

namespace SR_GRAPH_NS {
    class Render;
    class RenderContext;
    class ShaderCache;
}

namespace SR_GTYPES_NS {
    /// @extension(srsl)
    class Shader : public SR_UTILS_NS::IResource, public Memory::IGraphicsResource {
        SR_CLASS()
        using ShaderProgram = int32_t;
        friend class SR_GRAPH_NS::ShaderCache;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Shader>;

    public:
        Shader();
        ~Shader() override;

    public:
        ShaderBindResult Use() noexcept;

        bool Init();
        void UnUse() noexcept;
        bool Flush() const;
        void FlushSamplers();
        void FlushConstants();
        void FreeVMemory() override;
        void Dispatch(uint32_t x, uint32_t y, uint32_t z);
        void Dispatch();
        void StartWatch() override;
        void SetVariant(const SR_UTILS_NS::IResourceVariant& variant) override;

        bool AttachDescriptorSets();

        bool BeginSharedUBO();
        void EndSharedUBO();

        void ResetUBOToDefaults();

        RemoveUPResult RemoveUsePoint() override;

    public:
        SR_NODISCARD SR_UTILS_NS::Path GetAssociatedPath() const override;
        SR_NODISCARD int32_t GetId() noexcept;
        SR_NODISCARD ShaderProgram GetVirtualProgram() const noexcept { return m_shaderProgram; }
        SR_NODISCARD bool Ready() const;
        SR_NODISCARD uint64_t GetUBOBlockSize() const;
        SR_NODISCARD uint32_t GetSamplersCount() const;
        SR_NODISCARD const ShaderProperties& GetProperties() const;
        SR_NODISCARD const ShaderSamplers& GetSamplers() const noexcept { return m_samplers; };
        SR_NODISCARD bool IsBlendEnabled() const;
        SR_NODISCARD bool IsAvailable() const;
        SR_NODISCARD bool IsSamplersValid() const;
        SR_NODISCARD bool HasSharedUBO() const noexcept { return m_uniformSharedBlock.Valid(); }
        SR_NODISCARD bool HasSSBOBindings() const noexcept { return !m_ssboBindings.empty(); }
        SR_NODISCARD SR_SRSL_NS::ShaderType GetType() const noexcept;
        SR_NODISCARD const SR_MATH_NS::UVector3& GetComputeWorkGroupSize() const noexcept { return m_computeWorkGroupSize; }
        SR_NODISCARD const SR_SRSL_NS::ShaderParams& GetMacros() const noexcept { return m_params; }

    public:
        template<bool constant, typename T> void SetValue(uint64_t hashId, const T* v) noexcept {
            if constexpr (constant) {
                m_constBlock.SetField(hashId, v);
            }
            else {
                if (m_sharedUBOMode) SR_UNLIKELY_ATTRIBUTE {
                    m_uniformSharedBlock.SetField(hashId, v);
                }
                else {
                    m_uniformBlock.SetField(hashId, v);
                }
            }
        }

        void SR_FASTCALL SetBool(uint64_t hashId, bool v) noexcept;
        void SR_FASTCALL SetFloat(uint64_t hashId, float_t v) noexcept;
        void SR_FASTCALL SetInt(uint64_t hashId, int32_t v) noexcept;
        void SR_FASTCALL SetMat4(uint64_t hashId, const SR_MATH_NS::Matrix4x4& v) noexcept;
        void SR_FASTCALL SetVec3(uint64_t hashId, const SR_MATH_NS::FVector3& v) noexcept;
        void SR_FASTCALL SetVec4(uint64_t hashId, const SR_MATH_NS::FVector4& v) noexcept;
        void SR_FASTCALL SetColor(uint64_t hashId, const SR_MATH_NS::FColor& v) noexcept;
        void SR_FASTCALL SetRect(uint64_t hashId, const SR_MATH_NS::FRect& v) noexcept;
        void SR_FASTCALL SetVec2(uint64_t hashId, const SR_MATH_NS::FVector2& v) noexcept;
        void SR_FASTCALL SetIVec2(uint64_t hashId, const SR_MATH_NS::IVector2& v) noexcept;
        void SR_FASTCALL SetIVec3(uint64_t hashId, const SR_MATH_NS::IVector3& v) noexcept;

        void SR_FASTCALL SetConstBool(uint64_t hashId, bool v) noexcept;
        void SR_FASTCALL SetConstFloat(uint64_t hashId, float_t v) noexcept;
        void SR_FASTCALL SetConstInt(uint64_t hashId, int32_t v) noexcept;
        void SR_FASTCALL SetConstMat4(uint64_t hashId, const SR_MATH_NS::Matrix4x4& v) noexcept;
        void SR_FASTCALL SetConstVec4(uint64_t hashId, const SR_MATH_NS::FVector4& v) noexcept;
        void SR_FASTCALL SetConstColor(uint64_t hashId, const SR_MATH_NS::FColor& v) noexcept;
        void SR_FASTCALL SetConstVec3(uint64_t hashId, const SR_MATH_NS::FVector3& v) noexcept;
        void SR_FASTCALL SetConstVec2(uint64_t hashId, const SR_MATH_NS::FVector2& v) noexcept;
        void SR_FASTCALL SetConstIVec2(uint64_t hashId, const SR_MATH_NS::IVector2& v) noexcept;
        void SR_FASTCALL SetConstIVec3(uint64_t hashId, const SR_MATH_NS::IVector3& v) noexcept;

        void SR_FASTCALL SetSampler2D(SR_UTILS_NS::StringAtom name, SR_HTYPES_NS::SharedPtr<Texture> pSampler) noexcept;
        void SR_FASTCALL SetSampler2D(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept;
        void SR_FASTCALL SetSamplerCube(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept;

        void BindSSBO(SR_UTILS_NS::StringAtom name, uint32_t ssbo) noexcept;

        SR_NODISCARD bool HasErrors() const noexcept { return m_hasErrors; }
        SR_NODISCARD const SR_UTILS_NS::IResourceVariant* GetVariant() const override;

    protected:
        bool IsAllowedToRevive() const override;
        void ReviveResource() override;

        bool Load() override;
        bool Unload() override;

        void OnReloadDone() override;

        void LoadDefaultSampler(SR_UTILS_NS::StringAtom name);
        void UnloadDefaultSamplers();

    private:
        void SetSampler(SR_UTILS_NS::StringAtom name, int32_t sampler) noexcept;

    private:
        Memory::UBOManager& m_uboManager;
        Memory::ShaderProgramManager& m_manager;

        ShaderProgram m_shaderProgram = SR_ID_INVALID;

        bool m_isDirty = false;
        bool m_hasErrors = false;
        bool m_sharedUBOMode = false;

        SRShaderCreateInfo m_shaderCreateInfo = { };

        std::pair<int32_t, bool> m_virtualUBO = { SR_ID_INVALID, true };

        SR_MATH_NS::UVector3 m_computeWorkGroupSize = { 1, 1, 1 };

        SR_SRSL_NS::ShaderParams m_params;

        std::vector<SR_SRSL_NS::SRSLInclude> m_includes;
        Memory::ShaderUBOBlock m_uniformBlock;
        Memory::ShaderUBOBlock m_uniformSharedBlock;
        Memory::ShaderUBOBlock m_constBlock;
        ShaderSamplers m_samplers;
        ShaderProperties m_properties;
        SSBOBindings m_ssboBindings;
        std::map<SR_UTILS_NS::StringAtom, SR_HTYPES_NS::SharedPtr<Texture>> m_defaultSamplers;
        bool m_isGLayerUsed = false;

        SR_SRSL_NS::ShaderType m_type = SR_SRSL_NS::ShaderType::Unknown;

    };
}

#endif //SR_ENGINE_GRAPHICS_SHADER_H
