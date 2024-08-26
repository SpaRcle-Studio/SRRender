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
}
