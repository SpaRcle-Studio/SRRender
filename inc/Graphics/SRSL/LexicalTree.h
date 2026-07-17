//
// Created by Monika on 22.01.2023.
//

#ifndef SR_ENGINE_SRSL_LEXICALTREE_H
#define SR_ENGINE_SRSL_LEXICALTREE_H

#include <Graphics/Loaders/SRSL.h>
#include <Graphics/SRSL/LexerUtils.h>

namespace SR_SRSL_NS {
    SR_ENUM_NS_CLASS_T(LexicalUnitType, uint8_t,
        Unknown,
        Expr,
        Decorator,
        Decorators,
        Variable,
        Function,
        Struct,
        Return,
        IfStatement,
        ForStatement,
        WhileStatement,
        LexcialTree
    );

    /// минимальная лексическая единица
    class SRSLLexicalUnit : public SR_UTILS_NS::NonCopyable {
    public:
        explicit SRSLLexicalUnit(LexicalUnitType type)
            : m_type(type)
        { }

    public:
        SR_NODISCARD virtual std::string ToString(uint32_t deep) const { return std::string(); }

        SR_NODISCARD LexicalUnitType GetLexicalUnitType() const { return m_type; }

    private:
        LexicalUnitType m_type = LexicalUnitType::Unknown;

    };

    class SRSLLexicalTree;

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLExpr : public SRSLLexicalUnit {
    public:
        SRSLExpr() : SRSLLexicalUnit(LexicalUnitType::Expr) { }

        static SRSLExpr* CreateStringExpression(SR_UTILS_NS::String token);
        static SRSLExpr* CreateStringExpression(std::string_view token);

        explicit SRSLExpr(SR_UTILS_NS::String token);
        explicit SRSLExpr(std::string_view token);
        explicit SRSLExpr(std::string_view token, SRSLExpr* pAExpr);
        explicit SRSLExpr(std::string_view token, SRSLExpr* pAExpr, SRSLExpr* pBExpr);
        explicit SRSLExpr(SRSLExpr* pAExpr, SRSLExpr* pBExpr);
        SRSLExpr(SRSLExpr&& other) noexcept;
        ~SRSLExpr() override;

        SR_NODISCARD std::string ToString(uint32_t deep) const override;
        SR_UTILS_NS::StringView GetAsName();

        SR_UTILS_NS::String token;
        SR_UTILS_NS::SmallVector<SRSLExpr*, 8> args;

        bool isCall   = false; /// function(arg1, arg2, arg3)
        bool isArray  = false; /// variable[expression]
        bool isList   = false; /// { expr1, expr2, expr3 }
        bool isString = false; /// "some string" but without quotes

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLDecorator : public SRSLLexicalUnit {
    public:
        SRSLDecorator() : SRSLLexicalUnit(LexicalUnitType::Decorator) { }

        ~SRSLDecorator() override {
            for (auto&& pExpr : args) {
                delete pExpr;
            }
        }

        SRSLDecorator(SRSLDecorator&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::Decorator)
            , name(SR_UTILS_NS::Exchange(other.name, { }))
            , args(SR_UTILS_NS::Exchange(other.args, { }))
        { }

        SR_NODISCARD std::string ToString(uint32_t deep) const override;

        SR_UTILS_NS::String name;
        SR_UTILS_NS::SmallVector<SRSLExpr*, 4> args;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLDecorators : public SRSLLexicalUnit {
    public:
        SRSLDecorators() : SRSLLexicalUnit(LexicalUnitType::Decorators) { }

        SRSLDecorators(SRSLDecorators&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::Decorators)
            , decorators(SR_UTILS_NS::Exchange(other.decorators, { }))
        { }

        SRSLDecorators& operator=(SRSLDecorators&& other) noexcept {
            decorators = SR_UTILS_NS::Exchange(other.decorators, { });
            return *this;
        }

        SR_NODISCARD std::string ToString(uint32_t deep) const override;
        SR_NODISCARD SRSLDecorator* Find(const std::string& name);

        std::vector<SRSLDecorator> decorators;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLVariable : public SRSLLexicalUnit {
    public:
        SRSLVariable() : SRSLLexicalUnit(LexicalUnitType::Variable) { }

        SRSLVariable(SRSLVariable&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::Variable)
            , pDecorators(SR_UTILS_NS::Exchange(other.pDecorators, { }))
            , pType(SR_UTILS_NS::Exchange(other.pType, { }))
            , pName(SR_UTILS_NS::Exchange(other.pName, { }))
            , pExpr(SR_UTILS_NS::Exchange(other.pExpr, { }))
        { }

        SRSLVariable& operator=(SRSLVariable&& other) noexcept {
            pDecorators = SR_UTILS_NS::Exchange(other.pDecorators, { });
            pType = SR_UTILS_NS::Exchange(other.pType, { });
            pName = SR_UTILS_NS::Exchange(other.pName, { });
            pExpr = SR_UTILS_NS::Exchange(other.pExpr, { });
            return *this;
        }

        ~SRSLVariable() override {
            SR_SAFE_DELETE_PTR(pDecorators);
            SR_SAFE_DELETE_PTR(pExpr);
            SR_SAFE_DELETE_PTR(pType);
            SR_SAFE_DELETE_PTR(pName);
        }

        SR_NODISCARD std::string ToString(uint32_t deep) const override;

        SR_NODISCARD SR_UTILS_NS::StringView GetType() const;
        SR_NODISCARD SR_UTILS_NS::StringView GetName() const;

        SRSLDecorators* pDecorators = nullptr;
        SRSLExpr* pType = nullptr;
        SRSLExpr* pName = nullptr;
        SRSLExpr* pExpr = nullptr;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLReturn : public SRSLLexicalUnit {
    public:
        explicit SRSLReturn(SRSLExpr* pExpr)
            : SRSLLexicalUnit(LexicalUnitType::Return)
            , pExpr(pExpr)
        { }

        ~SRSLReturn() override {
            delete pExpr;
        }

        SRSLReturn(SRSLReturn&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::Return)
            , pExpr(SR_UTILS_NS::Exchange(other.pExpr, { }))
        { }

        SRSLReturn& operator=(SRSLReturn&& other) noexcept {
            pExpr = SR_UTILS_NS::Exchange(other.pExpr, { });
            return *this;
        }

        SRSLExpr* pExpr = nullptr;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLFunction : public SRSLLexicalUnit {
    public:
        SRSLFunction() : SRSLLexicalUnit(LexicalUnitType::Function) { }
        ~SRSLFunction() override;

        SR_NODISCARD std::string ToString(uint32_t deep) const override;
        SR_NODISCARD std::string GetName() const { return pName->token; }

        SRSLDecorators* pDecorators = nullptr;
        SRSLExpr* pType = nullptr;
        SRSLExpr* pName = nullptr;

        std::vector<SRSLVariable*> args;

        SRSLLexicalTree* pLexicalTree = nullptr;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLIfStatement : public SRSLLexicalUnit {
    public:
        SRSLIfStatement() : SRSLLexicalUnit(LexicalUnitType::IfStatement) { }
        explicit SRSLIfStatement(bool isElse);

        ~SRSLIfStatement() override;

        SRSLExpr* pExpr = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;
        bool isElse = false;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLForStatement : public SRSLLexicalUnit {
    public:
        SRSLForStatement() : SRSLLexicalUnit(LexicalUnitType::ForStatement) { }
        ~SRSLForStatement() override;

        SRSLVariable* pVar = nullptr;
        SRSLExpr* pCondition = nullptr;
        SRSLExpr* pExpr = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;
    };

    class SRSLWhileStatement : public SRSLLexicalUnit {
    public:
        SRSLWhileStatement() : SRSLLexicalUnit(LexicalUnitType::WhileStatement) { }
        ~SRSLWhileStatement() override;

        SRSLExpr* pCondition = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;
    };

    class SRSLStructureStatement : public SRSLLexicalUnit {
    public:
        SRSLStructureStatement() : SRSLLexicalUnit(LexicalUnitType::Struct) { }
        ~SRSLStructureStatement() override;

        SRSLExpr* pName = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;

        bool HasDynamicArray() const;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLLexicalTree : public SRSLLexicalUnit {
    public:
        SRSLLexicalTree() : SRSLLexicalUnit(LexicalUnitType::LexcialTree) { }

        ~SRSLLexicalTree() override {
            Clear();
        }

        SRSLLexicalTree(SRSLLexicalTree&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::LexcialTree)
            , lexicalTree(SR_UTILS_NS::Exchange(other.lexicalTree, { }))
        { }

        SRSLLexicalTree& operator=(SRSLLexicalTree&& other) noexcept {
            lexicalTree = SR_UTILS_NS::Exchange(other.lexicalTree, { });
            return *this;
        }

        SR_NODISCARD std::string ToString(uint32_t deep) const override;

        SR_NODISCARD SRSLFunction* FindFunction(SR_UTILS_NS::StringView name) const;
        SR_NODISCARD SRSLExpr* AsExpression() const;

        void Clear();

        std::vector<SRSLLexicalUnit*> lexicalTree;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLAnalyzedTree : public SR_UTILS_NS::NonCopyable {
    public:
        using Ptr = std::shared_ptr<SRSLAnalyzedTree>;

        SRSLAnalyzedTree() = default;

        ~SRSLAnalyzedTree() override {
            SR_SAFE_DELETE_PTR(pLexicalTree);
        }

        void PostProcess(const ShaderParams& params);

        SRSLLexicalTree* pLexicalTree = nullptr;
    };
}

#endif //SR_ENGINE_SRSL_LEXICALTREE_H
