//
// Created by Monika on 06.02.2023.
//

#ifndef SR_ENGINE_SRSL_TYPEINFO_H
#define SR_ENGINE_SRSL_TYPEINFO_H

#include <Graphics/SRSL/LexicalTree.h>
#include <Graphics/SRSL/ShaderType.h>
#include <Graphics/Loaders/ShaderProperties.h>

namespace SR_SRSL_NS {
    class SRSLTypeInfo : public SR_UTILS_NS::Singleton<SRSLTypeInfo> {
        SR_REGISTER_SINGLETON(SRSLTypeInfo)
    public:
        SR_NODISCARD std::vector<uint64_t> GetDimension(SR_UTILS_NS::IAllocator* pAllocator, SR_UTILS_NS::StringView code, const SRSLAnalyzedTree* pAnalyzedTree);
        SR_NODISCARD std::vector<uint64_t> GetDimension(const SRSLExpr* pExpr, const SRSLAnalyzedTree* pAnalyzedTree);

        SR_NODISCARD ShaderVarType StringToType(SR_UTILS_NS::StringView str);
        SR_NODISCARD std::string GetTypeName(SR_UTILS_NS::IAllocator* pAllocator, SR_UTILS_NS::StringView code);
        SR_NODISCARD std::string GetTypeName(const SRSLExpr* pExpr);

        SR_NODISCARD uint64_t GetTypeSize(SR_UTILS_NS::IAllocator* pAllocator, SR_UTILS_NS::StringView code, const SRSLAnalyzedTree* pAnalyzedTree);
        SR_NODISCARD uint64_t GetTypeSize(const SRSLExpr* pExpr, const SRSLAnalyzedTree* pAnalyzedTree);

        SR_NODISCARD uint64_t GetAlignedTypeSize(SR_UTILS_NS::IAllocator* pAllocator, SR_UTILS_NS::StringView code, const SRSLAnalyzedTree* pAnalyzedTree);
        SR_NODISCARD uint64_t GetAlignedTypeSize(const SRSLExpr* pExpr, const SRSLAnalyzedTree* pAnalyzedTree);

        SR_NODISCARD uint64_t GetStructSize(SR_UTILS_NS::StringView name, const SRSLAnalyzedTree* pAnalyzedTree);

    private:
        SR_NODISCARD SRSLAnalyzedTree* Analyze(SR_UTILS_NS::IAllocator* pAllocator, SR_UTILS_NS::StringView code);

    };
}

#endif //SR_ENGINE_SRSL_TYPEINFO_H
