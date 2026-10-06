//
// Created by Monika on 07.08.2022.
//

#ifndef SR_ENGINE_SHADERPROPERTIES_H
#define SR_ENGINE_SHADERPROPERTIES_H

#include <Graphics/Types/Texture.h>

#include <Utils/Types/SharedPtr.h>
#include <Utils/Types/Optional.h>
#include <Utils/Common/Hashes.h>
#include <Utils/Common/Enumerations.h>
#include <Utils/Profile/TracyContext.h>
#include <Utils/Resources/ResourceRef.h>

namespace SR_GTYPES_NS {
    class Texture;
    class Shader;
}

namespace SR_SRSL_NS {
    SR_ENUM_NS_CLASS(ShaderType,
        Unknown,
        Spatial,            /// пространственный шейдер, все статические меши
        SpatialCustom,      /// пространственный шейдер (только вершины), все статические меши
        Skinned,            /// пространтсвенный шейдер, геометрия со скелетом
        PostProcessing,     /// шейдер пост-обработки
        Skybox,             /// шейдер скайбокса
        Simple,             ///
        Canvas,             /// шейдер 2д пользовательского интерфейса
        Particles,          /// шейдер для частиц
        Compute,            ///
        Line,               /// просто линия, имеет начало и конец
        Custom,             /// полностью чистый шейдер, все настраивается вручную
        //Raygen,             /// трасировка лучей. генерация лучей и вызов трассировки
        //AnyHit,             /// трасировка лучей. проверка на пересечение с примитивом (необязательный)
        //ClosestHit,         /// трасировка лучей. проверка на пересечение с примитивом (обязательный)
        //Miss,               /// трасировка лучей. пересечение не было найдено (в пределах [tmin; tmax])
        //Intersection        /// трасировка лучей. проверка пересечения луча и геометрии
        RayTrace           /// шейдер трассировки лучей
    );
}

namespace SR_GRAPH_NS {
    class BaseMaterial;

    /// Реализация аттачментов (выходов кадровых буферов) сделана на уровне проходов рендера.
    /// См. ISamplersPass. На уровне шейдера не должны поддерживаться аттачменты, т.к. это не безопасно.

    struct ShaderPropertyData {
        enum class Type : uint8_t {
            Unknown,
            Sampler,
            Float,
            Int,
            Vec2,
            Vec3,
            IVec3,
            Vec4
        };

        ShaderPropertyData() = default;
        ShaderPropertyData(const ShaderPropertyData& other) = default;
        ShaderPropertyData(ShaderPropertyData&& other) noexcept = default;
        ShaderPropertyData& operator=(const ShaderPropertyData& other) = default;
        ShaderPropertyData& operator=(ShaderPropertyData&& other) noexcept = default;
        ShaderPropertyData(const float_t value) { data.floatValue = value; type = Type::Float; }
        ShaderPropertyData(const int32_t value) { data.intValue = value; type = Type::Int; }
        ShaderPropertyData(const SR_MATH_NS::FVector2& value) { data.vec2Value = value; type = Type::Vec2; }
        ShaderPropertyData(const SR_MATH_NS::FVector3& value) { data.vec3Value = value; type = Type::Vec3; }
        ShaderPropertyData(const SR_MATH_NS::IVector3& value) { data.ivec3Value = value; type = Type::IVec3; }
        ShaderPropertyData(const SR_MATH_NS::FVector4& value) { data.vec4Value = value; type = Type::Vec4; }
        ShaderPropertyData(const SR_UTILS_NS::ResourceRef<SR_GTYPES_NS::Texture>& value) { sampler = value; type = Type::Sampler; }
        ShaderPropertyData(const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Texture>& value) { sampler = value; type = Type::Sampler; }

        SR_NODISCARD bool operator==(const ShaderPropertyData& other) const {
            if (type != other.type) {
                return false;
            }

            switch (type) {
                case Type::Sampler:
                    return sampler == other.sampler;
                case Type::Float:
                    return SR_EQUALS(data.floatValue, other.data.floatValue);
                case Type::Int:
                    return data.intValue == other.data.intValue;
                case Type::Vec2:
                    return data.vec2Value == other.data.vec2Value;
                case Type::Vec3:
                    return data.vec3Value == other.data.vec3Value;
                case Type::IVec3:
                    return data.ivec3Value == other.data.ivec3Value;
                case Type::Vec4:
                    return data.vec4Value == other.data.vec4Value;
                default:
                    return false;
            }
        }

        SR_NODISCARD SR_GTYPES_NS::Texture* GetSampler() const;

        SR_NODISCARD float_t& GetFloat();
        SR_NODISCARD int32_t& GetInt();
        SR_NODISCARD SR_MATH_NS::FVector2& GetVec2();
        SR_NODISCARD SR_MATH_NS::FVector3& GetVec3();
        SR_NODISCARD SR_MATH_NS::IVector3& GetIVec3();
        SR_NODISCARD SR_MATH_NS::FVector4& GetVec4();

        SR_NODISCARD const float_t& GetFloat() const;
        SR_NODISCARD const int32_t& GetInt() const;
        SR_NODISCARD const SR_MATH_NS::FVector2& GetVec2() const;
        SR_NODISCARD const SR_MATH_NS::FVector3& GetVec3() const;
        SR_NODISCARD const SR_MATH_NS::IVector3& GetIVec3() const;
        SR_NODISCARD const SR_MATH_NS::FVector4& GetVec4() const;

        SR_UTILS_NS::ResourceRef<SR_GTYPES_NS::Texture> sampler;
        union Data {
            float_t floatValue = 0.f;
            int32_t intValue;
            SR_MATH_NS::FVector2 vec2Value;
            SR_MATH_NS::FVector3 vec3Value;
            SR_MATH_NS::IVector3 ivec3Value;
            SR_MATH_NS::FVector4 vec4Value;
        } data;
        Type type = Type::Unknown;
    };

    SR_ENUM_NS_CLASS_T(ShaderRenderPassType, uint32_t,
        Undefined,
        Main,
        Depth,
        ColorBuffer
    );

    SR_ENUM_NS_CLASS_T(ShaderVarType, uint8_t,
          Unknown,
          Bool,
          Int,
          Float,
          Vec2,
          Vec3,
          Vec4,
          IVec2,
          IVec3,
          IVec4,
          BVec2,
          BVec3,
          BVec4,
          Mat2,
          Mat3,
          Mat4,
          Sampler1D,
          Sampler2D,
          Sampler3D,
          SamplerCube,
          Sampler1DShadow,
          Sampler2DShadow, /// see MaterialProperty::IsSampler()
          Skeleton128
    )

    struct ShaderProperty {
        ShaderProperty() = default;
        ShaderProperty(const SR_UTILS_NS::StringAtom id, const ShaderVarType type, const bool pushConstant)
            : id(id)
            , type(type)
            , pushConstant(pushConstant)
        { }
        ShaderProperty(const SR_UTILS_NS::StringAtom id, const ShaderVarType type, const bool pushConstant, const std::optional<ShaderPropertyData>& defaultData)
            : id(id)
            , type(type)
            , pushConstant(pushConstant)
            , defaultData(defaultData)
        { }

        SR_UTILS_NS::StringAtom id;
        ShaderVarType type = ShaderVarType::Unknown;
        bool pushConstant = false;
        std::optional<ShaderPropertyData> defaultData;

        SR_NODISCARD bool IsPushConstant() const { return pushConstant; }
        SR_NODISCARD bool HasDefaultData() const { return defaultData.has_value(); }
        SR_NODISCARD const ShaderPropertyData& GetData() const;
        SR_NODISCARD const ShaderPropertyData& GetDefaultData() const;
    };

    typedef SR_UTILS_NS::Vector<ShaderProperty> ShaderProperties;

    struct ShaderSampler {
        uint32_t binding = SR_UINT32_MAX;
        uint32_t samplerId = SR_UINT32_MAX;
        bool isArray = false;
        bool isAttachment = false;
        SR_UTILS_NS::StringAtom defaultValue;
    };
    typedef SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, ShaderSampler> ShaderSamplers;

    struct SSBOBinding {
        SR_UTILS_NS::StringAtom name;
        uint32_t binding = SR_UINT32_MAX;
        uint32_t ssbo = SR_UINT32_MAX;
    };
    typedef SR_UTILS_NS::Vector<SSBOBinding> SSBOBindings;

    SR_MAYBE_UNUSED static bool SR_FASTCALL IsSamplerType(const ShaderVarType type) {
        switch (type) {
            case ShaderVarType::Sampler1D:
            case ShaderVarType::Sampler2D:
            case ShaderVarType::Sampler3D:
            case ShaderVarType::SamplerCube:
            case ShaderVarType::Sampler1DShadow:
            case ShaderVarType::Sampler2DShadow:
                return true;
            default:
                return false;
        }
    }

    SR_MAYBE_UNUSED static bool IsMatrixType(ShaderVarType type) {
        switch (type) {
            case ShaderVarType::Mat2:
            case ShaderVarType::Mat3:
            case ShaderVarType::Mat4:
            case ShaderVarType::Skeleton128:
                return true;
            default:
                return false;
        }
    }

    SR_MAYBE_UNUSED static std::string ShaderVarTypeToString(ShaderVarType type) {
        if (type == ShaderVarType::Skeleton128) {
            type = ShaderVarType::Mat4;
        }

        std::string str = SR_UTILS_NS::EnumReflector::ToStringAtom(type);

        if (!str.empty()) {
            str[0] = tolower(str[0]);
        }

        return str;
    }

    SR_MAYBE_UNUSED static std::string MakeShaderVariable(ShaderVarType type, const std::string& name) {
        if (type == ShaderVarType::Skeleton128) {
            return ShaderVarTypeToString(type) + " " + name + "[128]";
        }

        return ShaderVarTypeToString(type) + " " + name;
    }

    SR_MAYBE_UNUSED static uint32_t GetShaderVarSize(ShaderVarType type) {
        switch (type) {
            case ShaderVarType::Int:
            case ShaderVarType::Float:
            case ShaderVarType::Bool:
                return 4;
            case ShaderVarType::Vec2:
                return 4 * 2;
            case ShaderVarType::Vec3:
                return 4 * 3;
            case ShaderVarType::Vec4:
                return 4 * 4;
            case ShaderVarType::Mat2:
                return 4 * 2 * 2;
            case ShaderVarType::Mat3:
                return 4 * 3 * 3;
            case ShaderVarType::Mat4:
                return 4 * 4 * 4;
            case ShaderVarType::Skeleton128:
                return 4 * 4 * 4 * 128;
            case ShaderVarType::Unknown:
            default:
                SRAssert2(false, "unknown type!");
                return 0;
        }
    }

    SR_MAYBE_UNUSED ShaderPropertyData GetVariantFromShaderVarType(ShaderVarType type);

    SR_MAYBE_UNUSED static ShaderVarType GetShaderVarTypeFromString(std::string str) {
        if (!str.empty()) {
            str[0] = toupper(str[0]);
        }

        return SR_UTILS_NS::EnumReflector::FromString<ShaderVarType>(str);
    }
}

template<> struct SR_UTILS_NS::SRHash<SR_GRAPH_NS::ShaderSamplers> {
    size_t operator()(SR_GRAPH_NS::ShaderSamplers const& value) const {
        std::size_t res = 0;

        for (auto&& [key, val] : value) {
            res = SR_UTILS_NS::HashCombine(key.GetHash(), res);
            res = SR_UTILS_NS::HashCombine(val.binding, res);
        }

        return res;
    }
};

template<> struct SR_UTILS_NS::SRHash<SR_GRAPH_NS::ShaderProperties> {
    size_t operator()(SR_GRAPH_NS::ShaderProperties const& value) const {
        std::size_t res = 0;

        for (auto&& info : value) {
            res = SR_UTILS_NS::HashCombine(info.id.GetHash(), res);
            res = SR_UTILS_NS::HashCombine(info.type, res);
        }

        return res;
    }
};

#endif //SR_ENGINE_SHADERPROPERTIES_H
