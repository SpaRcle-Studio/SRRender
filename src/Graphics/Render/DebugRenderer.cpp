//
// Created by Monika on 20.09.2022.
//

#include <Utils/DebugDraw.h>
#include <Utils/Types/Time.h>
#include <Utils/Types/RawMesh.h>

#include <Graphics/Render/DebugRenderer.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Graphics/Types/Geometry/DebugLine.h>
#include <Graphics/Material/FileMaterial.h>

#include <Codegen/DebugRenderer.generated.hpp>

namespace SR_GRAPH_NS {
    DebugRenderer::~DebugRenderer() {
        SRAssert2(m_timedObjects.IsEmpty(), "DebugRenderer::~DebugRenderer() : timed objects are not empty!");
    }

    void DebugRenderer::Init() {
        SR_INFO("DebugRenderer::Init() : initializing debug renderer...");

        m_meshes.emplace_back(Memory::BakedMesh::Bake(GetRenderScene()->GetPipeline().Get(), "Engine/Models/cubeWireframe.obj", 0, Vertices::VertexType::SimpleVertex));
        m_meshes.emplace_back(Memory::BakedMesh::Bake(GetRenderScene()->GetPipeline().Get(), "Engine/Models/planeWireframe.obj", 0, Vertices::VertexType::SimpleVertex));
        m_meshes.emplace_back(Memory::BakedMesh::Bake(GetRenderScene()->GetPipeline().Get(), "Engine/Models/sphere_circle.obj", 0, Vertices::VertexType::SimpleVertex));
        m_meshes.emplace_back(Memory::BakedMesh::Bake(GetRenderScene()->GetPipeline().Get(), "Engine/Models/capsule_circle.obj", 0, Vertices::VertexType::SimpleVertex));

        for (uint64_t i = 0; i < m_meshes.size(); ++i) {
            if (!m_meshes[i]) {
                SR_ERROR("DebugRenderer::Init() : failed to load debug mesh on index {}!", i);
            }
            else {
                m_meshes[i]->AddUsePoint();
            }
        }

        using namespace std::placeholders;

        SR_UTILS_NS::DebugDraw::Callbacks callbacks;
        callbacks.removeCallback = std::bind(&DebugRenderer::Remove, this, _1, true);
        callbacks.drawLineCallback = std::bind(&DebugRenderer::AddLine, this, _1, _2, _3, _4, _5);
        callbacks.drawCubeCallback = std::bind(&DebugRenderer::AddMesh, this, _1, 0, _2, _3, _4, _5, _6);
        callbacks.drawPlaneCallback = std::bind(&DebugRenderer::AddMesh, this, _1, 1, _2, _3, _4, _5, _6);
        callbacks.drawSphereCallback = std::bind(&DebugRenderer::AddMesh, this, _1, 2, _2, _3, _4, _5, _6);
        callbacks.drawCapsuleCallback = std::bind(&DebugRenderer::AddMesh, this, _1, 3, _2, _3, _4, _5, _6);
        callbacks.drawMeshCallback = std::bind(&DebugRenderer::AddCustomMesh, this, _1, _2, _3, _4, _5, _6, _7, _8);

        SR_UTILS_NS::DebugDraw::Instance().SetCallbacks(this, std::move(callbacks));
    }

    void DebugRenderer::DeInit() {
        SR_LOCK_GUARD;
        SR_UTILS_NS::DebugDraw::Instance().RemoveCallbacks(this);

        for (auto&& pBaseMesh : m_meshes) {
            if (!pBaseMesh) {
                continue;
            }
            pBaseMesh->RemoveUsePoint();
        }

        m_meshes.clear();
    }

    void DebugRenderer::Prepare() {
        /// INFO: Меняем тут, иначе будет дедлок
        SR_UTILS_NS::DebugDraw::Instance().SwitchCallbacks(this);

        SR_LOCK_GUARD;

        auto&& timePoint = SR_HTYPES_NS::Time::Instance().Count();

        m_timedObjects.RemoveIf([timePoint](uint64_t /* index */, const DebugTimedObject& timed) {
            return timed.endTimePoint <= timePoint;
        },
        [this](const DebugTimedObject& timed) {
            m_renderSceneChanged = true;
            GetRenderScene()->GetPipeline()->SetDirty(true);

            if (timed.drawInfo.type >= DrawType::Mesh) {
                m_meshes[static_cast<uint64_t>(timed.drawInfo.type)]->RemoveUsePoint();
            }
        });
    }

    uint64_t DebugRenderer::AddLine(const uint64_t id, const SR_MATH_NS::FVector3& start, const SR_MATH_NS::FVector3& end, const SR_MATH_NS::FColor& color, float_t seconds) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        auto&& duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float_t>(seconds));
        auto&& timePoint = SR_HTYPES_NS::Time::Instance().Count();

        OnSceneChanged();

        DebugTimedObject timed;
        timed.drawInfo.type = DrawType::Line;
        timed.drawInfo.start = start;
        timed.drawInfo.end = end;
        timed.drawInfo.color = color;
        timed.duration = duration.count();
        timed.startTimePoint = timePoint;
        timed.endTimePoint = timed.startTimePoint + timed.duration;

        if (m_timedObjects.IsAlive(id)) {
            if (m_timedObjects.AtUnchecked(id).drawInfo.type != DrawType::Line) {
                Remove(id, false);
            }
            m_timedObjects.At(id) = timed;
            return id;
        }

        return m_timedObjects.Add(std::move(timed));
    }

    uint64_t DebugRenderer::AddMesh(const uint64_t id, uint32_t meshId, const SR_MATH_NS::FVector3& pos, const SR_MATH_NS::Quaternion& rot, const SR_MATH_NS::FVector3& scale, const SR_MATH_NS::FColor& color, float_t seconds) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        if (meshId >= m_meshes.size()) {
            SRHalt("DebugRenderer::DrawMesh() : invalid mesh id \"{}\"!", meshId);
            return SR_ID_INVALID;
        }

        OnSceneChanged();

        auto&& duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float_t>(seconds));
        auto&& timePoint = SR_HTYPES_NS::Time::Instance().Count();

        DebugTimedObject timed;
        timed.drawInfo.type = static_cast<DrawType>(meshId);
        timed.drawInfo.modelMatrix = SR_MATH_NS::Matrix4x4::CreateTRS(pos, rot, scale);
        timed.drawInfo.color = color;
        timed.duration = duration.count();
        timed.startTimePoint = timePoint;
        timed.endTimePoint = timed.startTimePoint + timed.duration;

        if (m_timedObjects.IsAlive(id)) {
            if (m_timedObjects.AtUnchecked(id).drawInfo.type != timed.drawInfo.type) {
                if (m_meshes[meshId]) {
                    m_meshes[meshId]->AddUsePoint();
                }
                Remove(id, false);
            }
            m_timedObjects.At(id) = timed;
            return id;
        }

        if (m_meshes[meshId]) {
            m_meshes[meshId]->AddUsePoint();
        }

        return m_timedObjects.Add(std::move(timed));
    }

    uint64_t DebugRenderer::AddCustomMesh(SR_HTYPES_NS::RawMesh* pRawMesh, int32_t meshIndex, uint64_t id,
        const SR_MATH_NS::FVector3& pos, const SR_MATH_NS::Quaternion& rot, const SR_MATH_NS::FVector3& scale,
        const SR_MATH_NS::FColor& color, float_t seconds
    ) {
        if (pRawMesh->GetMeshesCount() <= static_cast<uint32_t>(meshIndex)) {
            SR_ERROR("DebugRenderer::DrawMesh() : invalid mesh index \"{}\"!", meshIndex);
            return SR_ID_INVALID;
        }

        int64_t freeIndex = SR_ID_INVALID;
        for (int64_t i = static_cast<int64_t>(DrawType::CustomMesh); i < m_meshes.size(); ++i) {
            if (!m_meshes[i]) {
                freeIndex = i;
                continue;
            }
            if (m_meshes[i]->GetRawMesh() == pRawMesh && m_meshes[i]->GetMeshIndex() == meshIndex) {
                return AddMesh(id, i, pos, rot, scale, color, seconds);
            }
        }

        Memory::BakedMesh::Ptr pMesh = Memory::BakedMesh::Bake(GetRenderScene()->GetPipeline().Get(), pRawMesh, meshIndex, Vertices::VertexType::SimpleVertex);

        if (freeIndex == SR_ID_INVALID) {
            pMesh->AddUsePoint();
            m_meshes.emplace_back(pMesh);
            return AddMesh(id, m_meshes.size() - 1, pos, rot, scale, color, seconds);
        }

        pMesh->AddUsePoint();
        m_meshes[freeIndex] = pMesh;
        return AddMesh(id, freeIndex, pos, rot, scale, color, seconds);
    }

    void DebugRenderer::Remove(uint64_t id, bool fromPool) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        OnSceneChanged();

        if (id >= m_timedObjects.GetCapacity()) {
            SRHalt("DebugRenderer::Remove() : out of range with id \"{}\"!", id);
            return;
        }

        DebugTimedObject& timed = m_timedObjects.At(id);
        if (timed.drawInfo.type >= DrawType::Mesh) {
            m_meshes[static_cast<uint64_t>(timed.drawInfo.type)]->RemoveUsePoint();
        }

        if (fromPool) {
            m_timedObjects.RemoveByIndex(id);
        }
    }

    void DebugRenderer::OnSceneChanged() {
        m_renderSceneChanged = true;
    }

    void DebugRenderer::Clear() {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        OnSceneChanged();

        m_timedObjects.ForEach([this](uint64_t /* index */, const DebugTimedObject& timed) {
            if (timed.drawInfo.type >= DrawType::Mesh) {
                m_meshes[static_cast<uint64_t>(timed.drawInfo.type)]->RemoveUsePoint();
            }
        });
        m_timedObjects.Clear();
    }

    void DebugRenderer::ResetChangedFlags() noexcept {
        m_renderSceneChanged = false;
    }
}
