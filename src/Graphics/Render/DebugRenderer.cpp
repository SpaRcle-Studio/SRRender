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

namespace SR_GRAPH_NS {
    DebugRenderer::DebugRenderer(RenderScene* pRenderScene)
        : Super()
        , m_renderScene(pRenderScene)
    {
        using namespace std::placeholders;

        SR_UTILS_NS::DebugDraw::Callbacks callbacks;
        callbacks.removeCallback = std::bind(&DebugRenderer::Remove, this, _1);
        callbacks.drawLineCallback = std::bind(&DebugRenderer::DrawLine, this, _1, _2, _3, _4, _5);
        callbacks.drawCubeCallback = std::bind(&DebugRenderer::DrawGeometry, this, "Engine/Models/cubeWireframe.obj", _1, _2, _3, _4, _5, _6);
        callbacks.drawPlaneCallback = std::bind(&DebugRenderer::DrawGeometry, this, "Engine/Models/planeWireframe.obj", _1, _2, _3, _4, _5, _6);
        callbacks.drawSphereCallback = std::bind(&DebugRenderer::DrawGeometry, this, "Engine/Models/sphere_circle.obj", _1, _2, _3, _4, _5, _6);
        callbacks.drawCapsuleCallback = std::bind(&DebugRenderer::DrawGeometry, this, "Engine/Models/capsule_circle.obj", _1, _2, _3, _4, _5, _6);
        callbacks.drawMeshCallback = std::bind(&DebugRenderer::DrawMesh, this, _1, _2, _3, _4, _5, _6, _7, _8);

        SR_UTILS_NS::DebugDraw::Instance().SetCallbacks(this, std::move(callbacks));
    }

    DebugRenderer::~DebugRenderer() {
        SRAssert(m_timedObjects.size() == m_emptyIds.size());
    }

    void DebugRenderer::Init() {
        m_wireFrameMaterial = FileMaterial::Load("Engine/Materials/Debug/wireframe.mat");

        if (m_wireFrameMaterial) {
            m_wireFrameMaterial->AddUsePoint();
        }

        m_lineMaterial = FileMaterial::Load("Engine/Materials/Debug/line.mat");

        if (m_lineMaterial) {
            m_lineMaterial->AddUsePoint();
        }
    }

    void DebugRenderer::DeInit() {
        SR_LOCK_GUARD;
        SR_UTILS_NS::DebugDraw::Instance().RemoveCallbacks(this);

        if (m_wireFrameMaterial) {
            m_wireFrameMaterial->RemoveUsePoint();
        }

        if (m_lineMaterial) {
            m_lineMaterial->RemoveUsePoint();
        }
    }

    void DebugRenderer::Prepare() {
        /// меняем тут, иначе дедлок
        SR_UTILS_NS::DebugDraw::Instance().SwitchCallbacks(this);

        SR_LOCK_GUARD;

        auto&& timePoint = SR_HTYPES_NS::Time::Instance().Count();

        for (uint64_t i = 0; i < m_timedObjects.size(); ++i) {
            auto&& timed = m_timedObjects[i];

            if (!timed.registered) {
                m_renderScene->Register(timed.pMesh);
                timed.registered = true;
            }

            if (!timed.pMesh) {
                continue;
            }

            if (timed.endPoint <= timePoint && !timed.pMesh->IsUniformsDirty()) {
                if (!timed.pMesh->DestroyMesh()) {
                    SRHalt("Failed to unregister mesh!");
                }
                timed.pMesh = nullptr;
                m_emptyIds.emplace_back(i);
            }
        }
    }

    void DebugRenderer::Remove(uint64_t id) {
        SR_LOCK_GUARD;

        if (id == SR_ID_INVALID || id >= m_timedObjects.size()) {
            SRHalt0();
        }
        else if (m_timedObjects[id].pMesh) {
            UpdateTimedObject(id, 0.f);
        }
    }

    uint64_t DebugRenderer::DrawLine(uint64_t id, const SR_MATH_NS::FVector3 &start, const SR_MATH_NS::FVector3 &end, const SR_MATH_NS::FColor &color, float_t time) {
        SR_LOCK_GUARD;

    retry:
        if (id == SR_ID_INVALID) {
            if (m_timedObjects.size() == SR_UINT16_MAX) {
                SRHaltOnce("Too many debug objects!");
                return SR_ID_INVALID;
            }

            auto&& pDebugLine = new SR_GTYPES_NS::DebugLine(start, end, color);
            pDebugLine->SetMaterial(m_lineMaterial);
            return AddTimedObject(time, pDebugLine);
        }
        else if (id >= m_timedObjects.size()) {
            id = SR_ID_INVALID;
            goto retry;
        }
        else {
            if (!m_timedObjects[id].pMesh) {
                return DrawLine(SR_ID_INVALID, start, end, color, time);
            }

            auto&& pMesh = dynamic_cast<SR_GTYPES_NS::DebugLine*>(m_timedObjects[id].pMesh);

            if (!pMesh && time > 0) {
                SRHalt0();
                return SR_ID_INVALID;
            }

            if (!pMesh) {
                return SR_ID_INVALID;
            }

            pMesh->SetColor(color);
            pMesh->SetStartPoint(start);
            pMesh->SetEndPoint(end);

            UpdateTimedObject(id, time);

            return id;
        }
    }

    uint64_t DebugRenderer::DrawGeometry(const std::string_view& path, uint64_t id, const SR_MATH_NS::FVector3& pos,
        const SR_MATH_NS::Quaternion& rot, const SR_MATH_NS::FVector3& scale,
        const SR_MATH_NS::FColor& color, float_t time
    ) {
        SR_LOCK_GUARD;

        if (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(SR_UTILS_NS::Path(path))) {
            return DrawMesh(pRawMesh, 0, id, pos, rot, scale, color, time);
        }

        return SR_ID_INVALID;
    }

    uint64_t DebugRenderer::AddTimedObject(float_t seconds, SR_GTYPES_NS::Mesh *pMesh) {
        SR_LOCK_GUARD;

        auto&& duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float_t>(seconds));
        auto&& timePoint = SR_HTYPES_NS::Time::Instance().Count();

        DebugTimedObject timedObject{};

        timedObject.pMesh = pMesh;
        timedObject.startPoint = timePoint;
        timedObject.duration = duration.count();
        timedObject.endPoint = timedObject.startPoint + timedObject.duration;
        timedObject.registered = false;

        if (m_emptyIds.empty()) {
            m_timedObjects.emplace_back(timedObject);
            return m_timedObjects.size() - 1;
        }
        else {
            uint64_t id = m_emptyIds.front();
            m_emptyIds.pop_front();
            m_timedObjects[id] = timedObject;
            return id;
        }
    }

    void DebugRenderer::UpdateTimedObject(uint64_t id, float_t seconds) {
        SR_LOCK_GUARD;

        auto&& duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float_t>(seconds));
        auto&& timePoint = SR_HTYPES_NS::Time::Instance().Count();

        auto&& timedObject = m_timedObjects[id];

        timedObject.startPoint = timePoint;
        timedObject.duration = duration.count();
        timedObject.endPoint = timedObject.startPoint + timedObject.duration;
    }

    void DebugRenderer::Clear() {
        SR_LOCK_GUARD;

        for (uint64_t i = 0; i < m_timedObjects.size(); ++i) {
            auto&& timed = m_timedObjects[i];

            if (!timed.pMesh) {
                continue;
            }

            if (!timed.pMesh->DestroyMesh() && timed.registered) {
                SRHalt("Failed to unregister mesh!");
            }

            timed.pMesh = nullptr;

            m_emptyIds.emplace_back(i);
        }
    }

    bool DebugRenderer::IsEmpty() const {
        return m_timedObjects.size() == m_emptyIds.size();
    }

    uint64_t DebugRenderer::DrawMesh(SR_HTYPES_NS::RawMesh* pRawMesh, int32_t meshId, uint64_t id, const SR_MATH_NS::FVector3& pos,
        const SR_MATH_NS::Quaternion& rot, const SR_MATH_NS::FVector3& scale,
        const SR_MATH_NS::FColor& color, float_t time
    ) {
        SR_LOCK_GUARD;

        if (id == SR_ID_INVALID) {
            if (m_timedObjects.size() == SR_UINT16_MAX) {
                SRHaltOnce("Too many debug objects!");
                return SR_ID_INVALID;
            }

            SR_GTYPES_NS::DebugWireframeMesh* pMesh = dynamic_cast<SR_GTYPES_NS::DebugWireframeMesh *>(
                SR_GTYPES_NS::Mesh::TryLoad(pRawMesh, MeshType::Wireframe, meshId)
            );

            if (pMesh) {
                pMesh->SetColor(color);
                pMesh->SetMaterial(m_wireFrameMaterial);
                pMesh->SetMatrix(SR_MATH_NS::Matrix4x4(pos, rot, scale));

                return AddTimedObject(time, pMesh);
            }

            return SR_ID_INVALID;
        }
        else if (id >= m_timedObjects.size()) {
            SRHalt0();
            return SR_ID_INVALID;
        }
        else {
            if (!m_timedObjects[id].pMesh) {
                return DrawMesh(pRawMesh, meshId, SR_ID_INVALID, pos, rot, scale, color, time);
            }

            auto&& pMesh = dynamic_cast<SR_GTYPES_NS::DebugWireframeMesh*>(m_timedObjects[id].pMesh);

            if (!pMesh && time > 0) {
                SRHalt0();
                return SR_ID_INVALID;
            }

            if (!pMesh) {
                return SR_ID_INVALID;
            }

            pMesh->SetColor(color);
            pMesh->SetMatrix(SR_MATH_NS::Matrix4x4(pos, rot, scale));

            UpdateTimedObject(id, time);

            return id;
        }
    }
}