//
// Created by Monika on 12.11.2026.
//

#ifndef SR_ENGINE_SRSL_CODEGEN_COMMON_H
#define SR_ENGINE_SRSL_CODEGEN_COMMON_H

#include <Graphics/SRSL/ICodeGenerator.h>
#include <Graphics/SRSL/TypeInfo.h>
#include <Utils/Memory/Allocator.h>
#include <Utils/Profile/TracyContext.h>

namespace SR_SRSL_NS::CodeGenDetail {
    template <class... Parts> void Append(SR_UTILS_NS::String& code, const Parts&... parts) {
        SR_TRACY_ZONE;
        ((code += parts), ...);
    }

    inline void AppendRange(SR_UTILS_NS::String& code, size_t begin, size_t length) {
        SR_TRACY_ZONE;
        if (length == 0) {
            return;
        }
        const auto required = code.size() + length;
        if (required > code.capacity()) {
            code.reserve(SR_MAX(required, code.capacity() * 2));
        }
        code.append(code.data() + begin, length);
    }

    enum class VertexScalarKind : uint8_t { Float, Signed, Unsigned, Half };

    inline VertexScalarKind GetVertexScalarKind(SR_UTILS_NS::VertexAttributeFormat format) {
        SR_TRACY_ZONE;
        using Format = SR_UTILS_NS::VertexAttributeFormat;
        switch (format) {
        case Format::Float16:
            return VertexScalarKind::Half;
        case Format::Int32:
        case Format::Int16:
        case Format::Int8:
            return VertexScalarKind::Signed;
        case Format::UInt32:
        case Format::UInt16:
        case Format::UInt8:
            return VertexScalarKind::Unsigned;
        case Format::Float32:
        case Format::R11G11B10_Float:
        case Format::UNorm8:
        case Format::SNorm8:
        case Format::UNorm16:
        case Format::SNorm16:
        case Format::R10G10B10A2_UNorm:
            return VertexScalarKind::Float;
        default:
            SRHalt("Code generation: unsupported vertex format");
            return VertexScalarKind::Float;
        }
    }

    struct TypeDescription {
        SR_UTILS_NS::String base;
        SR_UTILS_NS::Vector<uint64_t> dimensions;
    };

    class Context : public ISRSLCodeGenerator {
    protected:
        Context() {
            SR_TRACY_ZONE;
            m_tmpBuffer.reserve(512);
        }

        SR_UTILS_NS::StringView KeepText(SR_UTILS_NS::StringView text) const {
            SR_TRACY_ZONE;
            auto* storage = static_cast<char*>(m_scratch.Allocate(text.size() + 1, alignof(char)));
            if (!text.empty()) {
                memcpy(storage, text.data(), text.size());
            }
            storage[text.size()] = '\0';
            return SR_UTILS_NS::StringView(storage, text.size());
        }

        const TypeDescription& DescribeType(SR_UTILS_NS::StringView type, const SRSLAnalyzedTree* tree) const {
            SR_TRACY_ZONE;
            auto it = m_types.find(type);
            if (it != m_types.end()) {
                return it->second;
            }
            TypeDescription description;
            description.base = SR_UTILS_NS::String(SRSLTypeInfo::Instance().GetTypeName(&m_scratch, type), &m_scratch);
            description.dimensions = SRSLTypeInfo::Instance().GetDimension(&m_scratch, type, tree);
            return m_types.emplace(KeepText(type), std::move(description)).first->second;
        }

        // The arena belongs to one generation call. Output strings use their own storage.
        mutable SR_UTILS_NS::MonotonicAllocator m_scratch{64 * 1024};
        mutable SR_UTILS_NS::String m_tmpBuffer{&m_scratch};

    private:
        mutable SR_UTILS_NS::Map<SR_UTILS_NS::StringView, TypeDescription> m_types;
    };

    template <class Emit>
    bool AppendCommonExpression(SR_UTILS_NS::String& code, const SRSLExpr* expression, Emit&& emit) {
        SR_TRACY_ZONE;
        const auto& args = expression->args;
        const auto& token = expression->token;
        if (expression->isCall || expression->isList || token == "++" || token == "--") {
            return false;
        }
        if (expression->isArray) {
            if (args.size() != 2) {
                return false;
            }
            emit(args[0]);
            code += '[';
            emit(args[1]);
            code += ']';
            return true;
        }
        if (args.size() == 1) {
            code += '(';
            code += token;
            emit(args[0]);
            code += ')';
            return true;
        }
        if (args.size() != 2 || token.empty() || token == "=" || token == "." || token == "?" || token == ":" ||
            token == "+=" || token == "-=" || token == "*=" || token == "/=" || token == "%=" || token == "&=" ||
            token == "|=" || token == "^=" || token == "<<=" || token == ">>=" || token == "&&=" || token == "||=") {
            return false;
        }
        if (token == "==" || token == "!=") {
            code += "((";
            emit(args[0]);
            code += ") ";
            code += token;
            code += " (";
            emit(args[1]);
            code += "))";
        } else {
            code += '(';
            emit(args[0]);
            code += ' ';
            code += token;
            code += ' ';
            emit(args[1]);
            code += ')';
        }
        return true;
    }

    template <class Visitor> decltype(auto) VisitStatement(const SRSLLexicalUnit* unit, Visitor&& visitor) {
        SR_TRACY_ZONE;
        if (!unit) {
            return visitor(unit);
        }
        switch (unit->GetLexicalUnitType()) {
        case LexicalUnitType::Variable:
            return visitor(static_cast<const SRSLVariable*>(unit));
        case LexicalUnitType::Function:
            return visitor(static_cast<const SRSLFunction*>(unit));
        case LexicalUnitType::LexcialTree:
            return visitor(static_cast<const SRSLLexicalTree*>(unit));
        case LexicalUnitType::Expr:
            return visitor(static_cast<const SRSLExpr*>(unit));
        case LexicalUnitType::IfStatement:
            return visitor(static_cast<const SRSLIfStatement*>(unit));
        case LexicalUnitType::Return:
            return visitor(static_cast<const SRSLReturn*>(unit));
        case LexicalUnitType::ForStatement:
            return visitor(static_cast<const SRSLForStatement*>(unit));
        case LexicalUnitType::WhileStatement:
            return visitor(static_cast<const SRSLWhileStatement*>(unit));
        default:
            return visitor(unit);
        }
    }
} // namespace SR_SRSL_NS::CodeGenDetail
#endif
