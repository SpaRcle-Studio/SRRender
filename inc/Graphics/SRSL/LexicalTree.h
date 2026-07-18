//
// Created by Monika on 22.01.2023.
//

#ifndef SR_ENGINE_SRSL_LEXICALTREE_H
#define SR_ENGINE_SRSL_LEXICALTREE_H

#include <Graphics/Loaders/SRSL.h>
#include <Graphics/SRSL/LexerUtils.h>

namespace SR_SRSL_NS {
    template<typename T, typename ...Args> T* AllocateLexicalUnit(SR_UTILS_NS::IAllocator& allocator, Args&&... args) {
        T* pLexicalUnit = (T*)allocator.Allocate(sizeof(T), alignof(T));
        new(pLexicalUnit) T(allocator, std::forward<Args>(args)...);
        return pLexicalUnit;
    }

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
        virtual SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const { return { }; }
        SR_NODISCARD LexicalUnitType GetLexicalUnitType() const { return m_type; }

    private:
        LexicalUnitType m_type = LexicalUnitType::Unknown;

    };

    class SRSLLexicalTree;

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLExpr : public SRSLLexicalUnit {
    public:
        static SRSLExpr* CreateStringExpression(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView token);

        explicit SRSLExpr(SR_UTILS_NS::IAllocator& allocator);
        SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView token);
        SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView token, SRSLExpr* pAExpr);
        SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView token, SRSLExpr* pAExpr, SRSLExpr* pBExpr);
        SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SRSLExpr* pAExpr, SRSLExpr* pBExpr);
        SRSLExpr(SRSLExpr&& other) noexcept;

        SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const override;
        SR_UTILS_NS::StringView GetAsName();

        SR_UTILS_NS::String token;
        SR_UTILS_NS::Vector<SRSLExpr*> args;

        bool isCall   = false; /// function(arg1, arg2, arg3)
        bool isArray  = false; /// variable[expression]
        bool isList   = false; /// { expr1, expr2, expr3 }
        bool isString = false; /// "some string" but without quotes

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLDecorator : public SRSLLexicalUnit {
    public:
        SRSLDecorator(SR_UTILS_NS::IAllocator& allocator);
        ~SRSLDecorator() override = default;

        SRSLDecorator(SRSLDecorator&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::Decorator)
            , name(SR_UTILS_NS::Exchange(other.name, { }))
            , args(SR_UTILS_NS::Exchange(other.args, { }))
        { }

        SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const override;

        SR_UTILS_NS::String name;
        SR_UTILS_NS::Vector<SRSLExpr*> args;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLDecorators : public SRSLLexicalUnit {
    public:
        SRSLDecorators(SR_UTILS_NS::IAllocator& allocator);

        SRSLDecorators(SRSLDecorators&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::Decorators)
            , decorators(SR_UTILS_NS::Exchange(other.decorators, { }))
        { }

        SRSLDecorators& operator=(SRSLDecorators&& other) noexcept {
            decorators = SR_UTILS_NS::Exchange(other.decorators, { });
            return *this;
        }

        SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const override;
        SR_NODISCARD SRSLDecorator* Find(SR_UTILS_NS::StringView name);

        SR_UTILS_NS::Vector<SRSLDecorator> decorators;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLVariable : public SRSLLexicalUnit {
    public:
        SRSLVariable(SR_UTILS_NS::IAllocator&) : SRSLLexicalUnit(LexicalUnitType::Variable) { }

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

        SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const override;

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
        explicit SRSLReturn(SR_UTILS_NS::IAllocator&, SRSLExpr* pExpr)
            : SRSLLexicalUnit(LexicalUnitType::Return)
            , pExpr(pExpr)
        { }

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
        SRSLFunction(SR_UTILS_NS::IAllocator&) : SRSLLexicalUnit(LexicalUnitType::Function) { }

        SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const override;
        SR_NODISCARD SR_UTILS_NS::StringView GetName() const { return pName->token; }

        SRSLDecorators* pDecorators = nullptr;
        SRSLExpr* pType = nullptr;
        SRSLExpr* pName = nullptr;

        SR_UTILS_NS::Vector<SRSLVariable*> args;

        SRSLLexicalTree* pLexicalTree = nullptr;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLIfStatement : public SRSLLexicalUnit {
    public:
        SRSLIfStatement(SR_UTILS_NS::IAllocator&) : SRSLLexicalUnit(LexicalUnitType::IfStatement) { }
        SRSLIfStatement(SR_UTILS_NS::IAllocator&, bool isElse);

        SRSLExpr* pExpr = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;
        bool isElse = false;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLForStatement : public SRSLLexicalUnit {
    public:
        SRSLForStatement(SR_UTILS_NS::IAllocator&) : SRSLLexicalUnit(LexicalUnitType::ForStatement) { }

        SRSLVariable* pVar = nullptr;
        SRSLExpr* pCondition = nullptr;
        SRSLExpr* pExpr = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;
    };

    class SRSLWhileStatement : public SRSLLexicalUnit {
    public:
        SRSLWhileStatement(SR_UTILS_NS::IAllocator&) : SRSLLexicalUnit(LexicalUnitType::WhileStatement) { }

        SRSLExpr* pCondition = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;
    };

    class SRSLStructureStatement : public SRSLLexicalUnit {
    public:
        SRSLStructureStatement(SR_UTILS_NS::IAllocator&) : SRSLLexicalUnit(LexicalUnitType::Struct) { }

        SRSLExpr* pName = nullptr;
        SRSLLexicalTree* pLexicalTree = nullptr;

        SR_NODISCARD bool HasDynamicArray() const;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLLexicalTree : public SRSLLexicalUnit {
    public:
        SRSLLexicalTree(SR_UTILS_NS::IAllocator& allocator);

        SRSLLexicalTree(SRSLLexicalTree&& other) noexcept
            : SRSLLexicalUnit(LexicalUnitType::LexcialTree)
            , lexicalTree(SR_UTILS_NS::Exchange(other.lexicalTree, { }))
        { }

        SRSLLexicalTree& operator=(SRSLLexicalTree&& other) noexcept {
            lexicalTree = SR_UTILS_NS::Exchange(other.lexicalTree, { });
            return *this;
        }

        SR_UTILS_NS::StringView ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const override;
        SR_NODISCARD SRSLFunction* FindFunction(SR_UTILS_NS::StringView name) const;
        SR_NODISCARD SRSLExpr* AsExpression() const;

        void Clear();

        SR_UTILS_NS::Vector<SRSLLexicalUnit*> lexicalTree;
    };

    /// ----------------------------------------------------------------------------------------------------------------

    class SRSLAnalyzedTree : public SR_UTILS_NS::NonCopyable {
    public:
        explicit SRSLAnalyzedTree(SR_UTILS_NS::IAllocator& allocator);

        void PostProcess(const ShaderParams& params);

        SRSLLexicalTree* pLexicalTree = nullptr;
        SR_UTILS_NS::IAllocator& allocator;
    };
}

#endif //SR_ENGINE_SRSL_LEXICALTREE_H
