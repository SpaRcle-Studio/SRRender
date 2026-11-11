//
// Created by Monika on 11.04.2026.
//

#ifndef SR_ENGINE_WGSL_GENERATOR_INTERNAL_H
#define SR_ENGINE_WGSL_GENERATOR_INTERNAL_H

#include <Graphics/SRSL/CodeGen/Common.h>
#include <Graphics/SRSL/ShaderType.h>
#include <Graphics/SRSL/RefAnalyzer.h>

namespace SR_SRSL_NS {
    class SRSLUniformBlock;

    class WGSLGenerator : public CodeGenDetail::Context {
    public:
        WGSLGenerator() = default;
        ~WGSLGenerator() override = default;

    public:
        SR_NODISCARD SRSLCodeGenRes GenerateStages(SR_UTILS_NS::IAllocator* pAllocator, const SRSLShader* pShader) override;

    private:
        /// Entry-point function wrapper (adds @vertex / @fragment / @compute decorator in the caller).

        SR_UTILS_NS::String GenerateDependencies(const SRSLShader* pShader, ShaderStage stage);
        SR_UTILS_NS::String GenerateVertexStage(const SRSLShader* pShader);
        SR_UTILS_NS::String GenerateFragmentStage(const SRSLShader* pShader);
        SR_UTILS_NS::String GenerateComputeStage(const SRSLShader* pShader);

    private:
        struct TypeBinding {
            SR_UTILS_NS::String name;
            SR_UTILS_NS::String previous;
            bool existed = false;
        };
        class Scope {
        public:
            explicit Scope(const WGSLGenerator& generator)
                : m_generator(generator), m_types(generator.m_typeBindings.size()),
                  m_samplers(generator.m_samplerParameterNames.size()) {
                SR_TRACY_ZONE;
                ++m_generator.m_scopeDepth;
            }
            ~Scope() {
                SR_TRACY_ZONE;
                while (m_generator.m_typeBindings.size() > m_types) {
                    auto& binding = m_generator.m_typeBindings.back();
                    if (binding.existed) {
                        m_generator.m_variableTypes[binding.name] = std::move(binding.previous);
                    } else {
                        m_generator.m_variableTypes.erase(binding.name);
                    }
                    m_generator.m_typeBindings.pop_back();
                }
                m_generator.m_samplerParameterNames.resize(m_samplers);
                --m_generator.m_scopeDepth;
            }
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;

        private:
            const WGSLGenerator& m_generator;
            size_t m_types;
            size_t m_samplers;
        };
        bool IsSamplerParameter(const SR_UTILS_NS::String& name) const {
            SR_TRACY_ZONE;
            for (const auto& parameter : m_samplerParameterNames) {
                if (parameter == name) {
                    return true;
                }
            }
            return false;
        }
        bool IsLocalVariable(const SR_UTILS_NS::String& name) const {
            SR_TRACY_ZONE;
            for (const auto& binding : m_typeBindings) {
                if (binding.name == name) {
                    return true;
                }
            }
            return false;
        }
        mutable SR_UTILS_NS::Vector<TypeBinding> m_typeBindings{&m_scratch};
        mutable uint32_t m_scopeDepth = 0;

        void AppendStatement(SR_UTILS_NS::String& code, const SRSLLexicalUnit* unit, int32_t deep,
                             SR_UTILS_NS::StringView returnCode, bool linewise) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateConstants(const SRSLShader* pShader) const;

        /// Собирает таблицу типов (uniform/ssbo/push-constant поля, shared-переменные, атрибуты вершин,
        /// константы и возвращаемые типы функций), чтобы InferExprType() мог определять типы выражений.
        void CollectDeclaredTypes(const SRSLShader* pShader) const;

        /// Регистрирует тип переменной в таблице типов (имя -> WGSL тип).
        SR_UTILS_NS::StringView GetResourceType(SR_UTILS_NS::StringView type) const;
        mutable SR_UTILS_NS::Map<const CodeGenDetail::TypeDescription*, SR_UTILS_NS::String> m_arrayTypes;

        void RegisterVariableType(SR_UTILS_NS::StringView name, SR_UTILS_NS::StringView wgslType) const;

        /// WGSL-тип shared (межстадийной) переменной. Пустая строка, если тип не поддерживается.
        SR_NODISCARD SR_UTILS_NS::StringView GetSharedVariableType(const SRSLShader* pShader,
                                                               const SRSLVariable* pVariable) const;

        /// Пытается определить WGSL-тип выражения. Возвращает пустую строку, если тип неизвестен.
        SR_NODISCARD SR_UTILS_NS::StringView InferExprType(const SRSLExpr* pExpr) const;

        /// Генерирует тела вспомогательных функций (inverse, преобразования матриц и т.д.),
        /// которые были использованы при генерации кода. В WGSL нет этих встроенных функций.
        SR_NODISCARD SR_UTILS_NS::String GenerateHelperFunctions() const;

        /// Non-entry-point function (regular function body without stage decorator).

        void AppendVariable(SR_UTILS_NS::String& code, const SRSLVariable* pVariable, int32_t deep) const;

        void AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep,
                               SR_UTILS_NS::StringView returnCode = SR_UTILS_NS::StringView()) const;

        void AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep,
                               const SR_UTILS_NS::String& preCode, const SR_UTILS_NS::String& postCode,
                               SR_UTILS_NS::StringView returnCode = SR_UTILS_NS::StringView()) const;

        void AppendIfStatement(SR_UTILS_NS::String& code, const SRSLIfStatement* pIfStatement, int32_t deep,
                               SR_UTILS_NS::StringView returnCode) const;

        void AppendForStatement(SR_UTILS_NS::String& code, const SRSLForStatement* pForStatement, int32_t deep,
                                SR_UTILS_NS::StringView returnCode) const;

        void AppendWhileStatement(SR_UTILS_NS::String& code, const SRSLWhileStatement* pWhileStatement, int32_t deep,
                                  SR_UTILS_NS::StringView returnCode) const;

        void AppendStructure(SR_UTILS_NS::String& code, const SRSLStructureStatement* pStructure, int32_t deep) const;
        void AppendFunctionBody(SR_UTILS_NS::String& code, const SRSLFunction* pFunction, int32_t deep) const;

        void AppendFunction(SR_UTILS_NS::String& result, SRSLFunction* pFunction, const int32_t deep,
                            const SR_UTILS_NS::StringView& preArgs = {}, const SR_UTILS_NS::StringView& preCode = {},
                            const SR_UTILS_NS::StringView& postCode = {},
                            const SR_UTILS_NS::StringView& returnType = {});

        void AppendExpression(SR_UTILS_NS::String& code, const SRSLExpr* expression, int32_t deep = 0) const;
        void AppendSpecialExpression(SR_UTILS_NS::String& code, const SRSLExpr* expression) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateExpression(const SRSLExpr* pExpr, int32_t deep) const;
        SR_NODISCARD SR_UTILS_NS::String GenerateVariable(const SRSLVariable* pVariable, int32_t deep) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateStructure(const SRSLStructureStatement* pStructure, int32_t deep) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateUniforms(const SRSLShader* pShader) const;

        /// Generates function-local `let` aliases for push constant fields used in the given stage.
        SR_NODISCARD SR_UTILS_NS::String GeneratePushConstantAliases(const SRSLShader* pShader, ShaderStage stage) const;

        /// Generates function-local `let` aliases for all UBO block fields used in the given stage.
        /// In GLSL UBO fields are globally visible; in WGSL they require `block.field` access.
        SR_NODISCARD SR_UTILS_NS::String GenerateUniformBlockAliases(const SRSLShader* pShader, ShaderStage stage) const;

        /// Generates function-local `let` aliases for SSBO fields used in the given stage.
        /// For non-array fields: `let field = blockVar.field;`
        /// For dynamic-array fields: emits nothing (access is rewritten at expression level via m_ssboFieldToBlock).
        SR_NODISCARD SR_UTILS_NS::String GenerateSSBOBlockAliases(const SRSLShader* pShader, ShaderStage stage) const;

    private:
        /// Set for the duration of a GenerateStages() call so that GenerateExpression()
        /// can look up sampler names when rewriting texture() calls.
        const SRSLShader* m_pCurrentShader = nullptr;
        SR_UTILS_NS::Set<const SRSLFunction*> m_emittedFunctions;
        mutable bool m_forceExplicitTextureLod = false;

        /// Populated during GenerateUniforms(): maps SSBO field name → "blockVarName.fieldName"
        /// so that GenerateExpression() can rewrite bare field references.
        mutable SR_UTILS_NS::Map<SR_UTILS_NS::StringView, SR_UTILS_NS::String> m_ssboFieldToQualified;

        /// Таблица типов (имя переменной -> WGSL тип), нужна для вывода типов в GenerateExpression().
        mutable SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringView, SR_UTILS_NS::String> m_variableTypes;

        /// Возвращаемые типы пользовательских функций (имя -> WGSL тип).
        mutable SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringView, SR_UTILS_NS::String> m_functionReturnTypes;
        mutable SR_UTILS_NS::Vector<SR_UTILS_NS::String> m_samplerParameterNames{&m_scratch};
        mutable SR_UTILS_NS::Set<SR_UTILS_NS::String> m_attachmentSamplerNames;

        /// Имена вспомогательных функций, которые понадобились при генерации кода.
        mutable bool m_inverseHelpers[3]{};
        mutable bool m_matrixCastHelpers[3][3]{};
        mutable bool m_diagonalHelpers[3]{};
    };
} // namespace SR_SRSL_NS

#endif // SR_ENGINE_WGSL_GENERATOR_INTERNAL_H
