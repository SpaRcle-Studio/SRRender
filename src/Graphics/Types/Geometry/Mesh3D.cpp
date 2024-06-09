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

namespace SR_GTYPES_NS {
    Mesh3D::Mesh3D()
        : Super(MeshType::Static)
    { }

    bool Mesh3D::Calculate()  {
        if (IsCalculated()) {
            return true;
        }

        SR_TRACY_ZONE;

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Full) {
            SR_LOG("Mesh3D::Calculate() : calculating \"" + GetGeometryName() + "\"...");
        }

        if (!CalculateVBO<Vertices::VertexType::StaticMeshVertex, Vertices::StaticMeshVertex>([this]() {
            return Vertices::CastVertices<Vertices::StaticMeshVertex>(GetVertices());
        })) {
            return false;
        }

        return IndexedMesh::Calculate();
    }

    void Mesh3D::Draw() {
        SR_TRACY_ZONE;

        if (!Calculate() || m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        m_pipeline->BindVBO(m_VBO);
        m_pipeline->BindIBO(m_IBO);
        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
                UseSamplers();
                m_descriptorManager.Flush();
            }
        }

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            m_pipeline->DrawIndices(m_countIndices);
        }

        m_dirtyMaterial = false;
    }

    std::vector<uint32_t> Mesh3D::GetIndices() const {
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
        auto&& pShader = GetRenderContext()->GetCurrentShader();

        pShader->SetMat4(SHADER_MODEL_MATRIX, m_modelMatrix);
    }

    void Mesh3D::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();

        if (GetRawMesh() && IsValidMeshId()) {
            SetGeometryName(GetRawMesh()->GetGeometryName(GetMeshId()));
        }

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

    bool Mesh3D::OnResourceReloaded(SR_UTILS_NS::IResource* pResource) {
        bool changed = Mesh::OnResourceReloaded(pResource);
        if (GetRawMesh() == pResource) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    bool Mesh3D::InitializeEntity() noexcept {
        m_properties.AddCustomProperty<SR_UTILS_NS::PathProperty>("Mesh")
            .AddFileFilter("Mesh", SR_GRAPH_NS::SR_SUPPORTED_MESH_FORMATS)
            .SetGetter([this]()-> SR_UTILS_NS::Path {
                return GetRawMesh() ? GetRawMesh()->GetResourcePath() : SR_UTILS_NS::Path();
            })
            .SetSetter([this](const SR_UTILS_NS::Path& path) {
                SetRawMesh(path);
            });

        m_properties.AddCustomProperty<SR_UTILS_NS::StandardProperty>("Index")
            .SetGetter([this](void* pData) {
                *reinterpret_cast<int16_t*>(pData) = static_cast<int16_t>(GetMeshId());
            })
            .SetSetter([this](void* pData) {
                SetMeshId(static_cast<MeshIndex>(*reinterpret_cast<int16_t*>(pData)));
            })
            .SetType(SR_UTILS_NS::StandardType::Int16);

        m_properties.AddEnumProperty("FrustumCullingType", &m_frustumCullingType);

        return Super::InitializeEntity();
    }
}