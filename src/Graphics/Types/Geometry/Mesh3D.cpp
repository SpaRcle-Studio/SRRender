//
// Created by Nikita on 02.06.2021.
//

#include <Utils/Types/RawMesh.h>
#include <Utils/Types/DataStorage.h>
#include <Utils/ECS/ComponentManager.h>

#include <Graphics/Types/Geometry/Mesh3D.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Uniforms.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Utils/MeshUtils.h>

#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/Mesh3D.generated.hpp>

namespace SR_GTYPES_NS {
    bool Mesh3D::Calculate()  {
        if (IsCalculated()) {
            return true;
        }

        SR_TRACY_ZONE;

        FreeVMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Full) {
            SR_LOG("Mesh3D::Calculate() : calculating \"" + GetMeshIdentifier() + "\"...");
        }

        if (!CalculateVBO<Vertices::VertexType::StaticMeshVertex, Vertices::StaticMeshVertex>([this]() {
            return Vertices::CastVertices<Vertices::StaticMeshVertex>(GetVertices());
        })) {
            return false;
        }

        return IndexedMesh::Calculate();
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& Mesh3D::GetIndices() const {
        SR_TRACY_ZONE;
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool Mesh3D::IsCalculatable() const {
        return IsValidMeshId() && Super::IsCalculatable();
    }

    void Mesh3D::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void Mesh3D::UseModelMatrix() {
        auto&& pShader = GetPipeline()->GetCurrentShader();
        pShader->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
    }

    void Mesh3D::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();

        ReRegisterMesh();

        MarkMaterialDirty();
        m_isCalculated = false;
    }

    std::string Mesh3D::GetMeshIdentifier() const {
        SR_TRACY_ZONE;

        if (auto&& pRawMesh = GetRawMesh()) {
            return SR_FORMAT("{}|{}|{}", pRawMesh->GetResourceId().c_str(), GetMeshId(), pRawMesh->GetReloadCount());
        }

        return Super::GetMeshIdentifier();
    }

    bool Mesh3D::OnResourceReloaded(const SR_UTILS_NS::IResource* pResource) {
        bool changed = Mesh::OnResourceReloaded(pResource);
        if (GetRawMesh().Get() == pResource) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }
}