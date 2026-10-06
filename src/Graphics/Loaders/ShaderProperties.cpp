//
// Created by Monika on 07.08.2022.
//

#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Material/BaseMaterial.h>

namespace SR_GRAPH_NS {
    const ShaderPropertyData& ShaderProperty::GetData() const {
        if (defaultData) {
            return *defaultData;
        }
        static const auto def = GetVariantFromShaderVarType(type);
        return def;
    }

    const ShaderPropertyData& ShaderProperty::GetDefaultData() const {
        if (defaultData) {
            return *defaultData;
        }
        SRHalt("Default data is not set!");
        static const auto def = ShaderPropertyData();
        return def;
    }

    ShaderPropertyData GetVariantFromShaderVarType(ShaderVarType type) {
        SR_TRACY_ZONE;

        switch (type) {
            case ShaderVarType::Bool:
                return static_cast<int32_t>(0);
            case ShaderVarType::Int:
                return static_cast<int32_t>(0);
            case ShaderVarType::Float:
                return static_cast<float_t>(0.f);
            case ShaderVarType::Vec2:
                return SR_MATH_NS::FVector2(SR_MATH_NS::Unit(0));
            case ShaderVarType::Vec3:
                return SR_MATH_NS::FVector3(SR_MATH_NS::Unit(0));
            case ShaderVarType::IVec3:
                return SR_MATH_NS::IVector3(0);
            case ShaderVarType::Vec4:
                return SR_MATH_NS::FVector4(SR_MATH_NS::Unit(0));
            case ShaderVarType::Sampler1D:
            case ShaderVarType::Sampler2D:
            case ShaderVarType::Sampler3D:
            case ShaderVarType::SamplerCube:
            case ShaderVarType::Sampler1DShadow:
            case ShaderVarType::Sampler2DShadow:
                return SR_UTILS_NS::ResourceRef<SR_GTYPES_NS::Texture>();
            default:
                SRAssert(false);
            return ShaderPropertyData();
        }
    }

    SR_GTYPES_NS::Texture *ShaderPropertyData::GetSampler() const {
        if (type != Type::Sampler) {
            SRHalt("ShaderPropertyData::GetSampler() : property type is not a sampler!");
            return nullptr;
        }
        return sampler.GetResource().Get();
    }

    float_t& ShaderPropertyData::GetFloat() {
        if (type != Type::Float) {
            SRHalt("ShaderPropertyData::GetFloat() : property type is not a float!");
            static float_t def = 0.f;
            return def;
        }
        return data.floatValue;
    }

    int32_t& ShaderPropertyData::GetInt() {
        if (type != Type::Int) {
            SRHalt("ShaderPropertyData::GetInt() : property type is not an int or bool!");
            static int32_t def = 0;
            return def;
        }
        return data.intValue;
    }

    SR_MATH_NS::FVector2& ShaderPropertyData::GetVec2() {
        if (type != Type::Vec2) {
            SRHalt("ShaderPropertyData::GetVec2() : property type is not a vec2!");
            static auto def = SR_MATH_NS::FVector2(SR_MATH_NS::Unit(0));
            return def;
        }
        return data.vec2Value;
    }

    SR_MATH_NS::FVector3& ShaderPropertyData::GetVec3() {
        if (type != Type::Vec3) {
            SRHalt("ShaderPropertyData::GetVec3() : property type is not a vec3!");
            static auto def = SR_MATH_NS::FVector3(SR_MATH_NS::Unit(0));
            return def;
        }
        return data.vec3Value;
    }

    SR_MATH_NS::IVector3& ShaderPropertyData::GetIVec3() {
        if (type != Type::IVec3) {
            SRHalt("ShaderPropertyData::GetIVec3() : property type is not an ivec3!");
            static auto def = SR_MATH_NS::IVector3(0);
            return def;
        }
        return data.ivec3Value;
    }

    SR_MATH_NS::FVector4& ShaderPropertyData::GetVec4() {
        if (type != Type::Vec4) {
            SRHalt("ShaderPropertyData::GetVec4() : property type is not a vec4!");
            static auto def = SR_MATH_NS::FVector4(SR_MATH_NS::Unit(0));
            return def;
        }
        return data.vec4Value;
    }

    const float_t& ShaderPropertyData::GetFloat() const {
        if (type != Type::Float) {
            SRHalt("ShaderPropertyData::GetFloat() : property type is not a float!");
            static float_t def = 0.f;
            return def;
        }
        return data.floatValue;
    }

    const int32_t& ShaderPropertyData::GetInt() const {
        if (type != Type::Int) {
            SRHalt("ShaderPropertyData::GetInt() : property type is not an int or bool!");
            static int32_t def = 0;
            return def;
        }
        return data.intValue;
    }

    const SR_MATH_NS::FVector2& ShaderPropertyData::GetVec2() const {
        if (type != Type::Vec2) {
            SRHalt("ShaderPropertyData::GetVec2() : property type is not a vec2!");
            static auto def = SR_MATH_NS::FVector2(SR_MATH_NS::Unit(0));
            return def;
        }
        return data.vec2Value;
    }

    const SR_MATH_NS::FVector3& ShaderPropertyData::GetVec3() const {
        if (type != Type::Vec3) {
            SRHalt("ShaderPropertyData::GetVec3() : property type is not a vec3!");
            static auto def = SR_MATH_NS::FVector3(SR_MATH_NS::Unit(0));
            return def;
        }
        return data.vec3Value;
    }

    const SR_MATH_NS::IVector3& ShaderPropertyData::GetIVec3() const {
        if (type != Type::IVec3) {
            SRHalt("ShaderPropertyData::GetIVec3() : property type is not an ivec3!");
            static auto def = SR_MATH_NS::IVector3(0);
            return def;
        }
        return data.ivec3Value;
    }

    const SR_MATH_NS::FVector4& ShaderPropertyData::GetVec4() const {
        if (type != Type::Vec4) {
            SRHalt("ShaderPropertyData::GetVec4() : property type is not a vec4!");
            static auto def = SR_MATH_NS::FVector4(SR_MATH_NS::Unit(0));
            return def;
        }
        return data.vec4Value;
    }
}
