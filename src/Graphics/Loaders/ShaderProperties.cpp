//
// Created by Monika on 07.08.2022.
//

#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Material/BaseMaterial.h>

namespace SR_GRAPH_NS {
    ShaderPropertyVariant ShaderProperty::GetData() const {
        if (defaultData) {
            return *defaultData;
        }
        return GetVariantFromShaderVarType(type);
    }

    ShaderPropertyVariant ShaderProperty::GetDefaultData() const {
        if (defaultData) {
            return *defaultData;
        }
        SRHalt("Default data is not set!");
        return {};
    }

    ShaderPropertyVariant GetVariantFromShaderVarType(ShaderVarType type) {
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
                return SR_HTYPES_NS::SharedPtr<Types::Texture>();
            //return static_cast<Types::Texture*>(nullptr);
            default:
                SRAssert(false);
            return ShaderPropertyVariant();
        }
    }
}
