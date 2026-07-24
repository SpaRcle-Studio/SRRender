//
// Created by Monika on 22.01.2023.
//

#ifndef SR_ENGINE_SRSL_SHADER_H
#define SR_ENGINE_SRSL_SHADER_H

#include <Graphics/Loaders/SRSL.h>
#include <Graphics/SRSL/RefAnalyzer.h>
#include <Graphics/SRSL/ICodeGenerator.h>
#include <Graphics/SRSL/ShaderType.h>
#include <Graphics/Pipeline/ShaderUtils.h>

#include <Utils/Types/RawPointerHolder.h>

namespace SR_SRSL_NS {
    SR_ENUM_NS_CLASS_T(ShaderLanguage, uint8_t,
        PseudoCode, GLSL, WGSL, HLSL, Metal
    );

    struct SRSLSampler {
        SR_UTILS_NS::StringAtom type;
        bool isPublic = false;
        uint64_t binding = 0;
        int32_t attachment = -1;
        SR_UTILS_NS::Set<ShaderStage> stages;
        SR_UTILS_NS::StringAtom defaultValue;
    };
    typedef SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SRSLSampler> SRSLSamplers;

    struct SRSLUniformBlock {
        struct Field {
            SR_UTILS_NS::StringAtom type;
            SR_UTILS_NS::StringAtom name;
            uint64_t size = 0;
            uint64_t alignedSize = 0;
            bool isPublic = false;
            std::optional<ShaderPropertyVariant> defaultValue;
        };

        void Align(const SRSLAnalyzedTree* pAnalyzedTree);

        std::optional<bool> isReadOnly; /// true - read only, false - write only, nullopt - read/write
        bool isVolatile = false;
        bool isCoherent = false;
        bool isRestrict = false;

        uint64_t size = 0;
        uint64_t binding = 0;

        bool hasUsage = false;

        SR_UTILS_NS::Vector<Field> fields;
        SR_UTILS_NS::Set<ShaderStage> stages;
    };

    class ShaderCache;

    /** Это не шейдер в привычном понимании, это набор всех данных для генерирования любого
     * шейдерного кода и для последующей его экспортации. */
    class SRSLShader : public SR_HTYPES_NS::SharedPtr<SRSLShader> {
        using Super = SR_HTYPES_NS::SharedPtr<SRSLShader>;
        using UniformBlocks = SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SRSLUniformBlock>;
        friend ShaderCache;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<SRSLShader>;

        ~SRSLShader() override;

    private:
        explicit SRSLShader(SR_UTILS_NS::Path path);

    public:
        SR_NODISCARD static SRSLShader::Ptr Load(SR_UTILS_NS::IAllocator* pAllocator, const SR_UTILS_NS::Path& path, const ShaderParams& params);
        static void ClearShadersCache();

    public:
        SR_NODISCARD std::string ToString(ShaderLanguage shaderLanguage) const;
        SR_NODISCARD bool Export(ShaderLanguage shaderLanguage) const;

        SR_NODISCARD bool IsCacheActual() const;
        SR_NODISCARD bool IsCacheActual(ShaderLanguage shaderLanguage) const;

        SR_NODISCARD const SRSLStructureStatement* FindStructure(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD const SRSLUniformBlock* FindUniformBlock(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD const SRSLUniformBlock::Field* FindField(const SR_UTILS_NS::StringAtom& name) const;
        //SR_NODISCARD Vertices::VertexType GetVertexType() const;
        SR_NODISCARD SR_SRSL_NS::ShaderType GetType() const;
        SR_NODISCARD SR_UTILS_NS::Path GetPath() const { return m_path; }
        SR_NODISCARD SRSLAnalyzedTree* GetAnalyzedTree() const;
        SR_NODISCARD SRSLUseStack::Ptr GetUseStack() const;
        SR_NODISCARD const UniformBlocks& GetUniformBlocks() const { return m_uniformBlocks; }
        SR_NODISCARD const UniformBlocks& GetSSBOBlocks() const { return m_ssboBlocks; }
        SR_NODISCARD const SRSLUniformBlock& GetPushConstants() const { return m_pushConstants; }
        SR_NODISCARD const SRSLSamplers& GetSamplers() const { return m_samplers; }
        SR_NODISCARD const SRShaderCreateInfo& GetCreateInfo() const { return m_createInfo; }
        SR_NODISCARD SRShaderCreateInfo& GetCreateInfo() { return m_createInfo; }
        SR_NODISCARD const SR_UTILS_NS::Vector<std::pair<SR_UTILS_NS::StringAtom, SRSLVariable*>>& GetShared() const { return m_shared; }
        SR_NODISCARD const SR_UTILS_NS::Vector<std::pair<SR_UTILS_NS::StringAtom, SRSLVariable*>>& GetSharedWorkgroup() const { return m_sharedWorkgroup; }
        SR_NODISCARD const SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SRSLVariable*>& GetConstants() const { return m_constants; }
        SR_NODISCARD const SR_UTILS_NS::Vector<SRSLInclude>& GetIncludes() const { return m_includes; }
        SR_NODISCARD const SR_MATH_NS::UVector3& GetComputeWorkGroupSize() const { return m_computeWorkGroupSize; }
        SR_NODISCARD bool IsMacroDefined(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD bool IsGLayerUsed() const { return m_gLayerUsed; }
        SR_NODISCARD SR_UTILS_NS::IAllocator* GetAllocator() const { return m_pAllocator; }

    private:
        SR_NODISCARD SR_UTILS_NS::Path GetCachePath() const;
        SR_NODISCARD float_t EvalExpressionFloat(SRSLExpr* pExpression) const;
        SR_NODISCARD int32_t EvalExpressionInt(SRSLExpr* pExpression) const;
        SR_NODISCARD SR_MATH_NS::FVector2 EvalExpressionVec2(SRSLExpr* pExpression) const;
        SR_NODISCARD SR_MATH_NS::FVector3 EvalExpressionVec3(SRSLExpr* pExpression) const;
        SR_NODISCARD SR_MATH_NS::IVector3 EvalExpressionIVec3(SRSLExpr* pExpression) const;
        SR_NODISCARD SR_MATH_NS::FVector4 EvalExpressionVec4(SRSLExpr* pExpression) const;
        SR_NODISCARD std::optional<ShaderPropertyVariant> EvalExpressionValue(SRSLExpr* pExpression, SRSLExpr* pType) const;

        SR_NODISCARD ISRSLCodeGenerator::SRSLCodeGenRes GenerateStages(ShaderLanguage shaderLanguage) const;

        SR_NODISCARD bool SaveCache() const;
        SR_NODISCARD uint64_t GetHash() const;

        bool Prepare();
        bool PrepareSettings();
        bool PrepareUniformBlocks();
        bool PrepareSamplers();
        bool PrepareStages();

    private:
        SR_UTILS_NS::Path m_path;

        SR_UTILS_NS::IAllocator* m_pAllocator = nullptr;
        bool m_gLayerUsed = false;
        ShaderParams m_params;
        SR_UTILS_NS::Vector<SRSLInclude> m_includes;
        SR_UTILS_NS::Vector<std::pair<SR_UTILS_NS::StringAtom, SRSLVariable*>> m_shared;
        SR_UTILS_NS::Vector<std::pair<SR_UTILS_NS::StringAtom, SRSLVariable*>> m_sharedWorkgroup;
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SRSLVariable*> m_constants;
        SRShaderCreateInfo m_createInfo;
        SRSLAnalyzedTree* m_analyzedTree = nullptr;
        SRSLUseStack::Ptr m_useStack;
        UniformBlocks m_ssboBlocks;
        UniformBlocks m_uniformBlocks;
        SRSLUniformBlock m_pushConstants;
        SRSLSamplers m_samplers;
        SR_MATH_NS::UVector3 m_computeWorkGroupSize = { 1, 1, 1 };

    };
}

#endif //SR_ENGINE_SRSL_SHADER_H
