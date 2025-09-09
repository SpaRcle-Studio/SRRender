//
// Created by Monika on 06.06.2023.
//

#include <Graphics/Pass/CascadedShadowMapPass.h>

#include <Codegen/CascadedShadowMapPass.generated.hpp>

namespace SR_GRAPH_NS {
    void CascadedShadowMapPass::UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh) {
        SR_TRACY_ZONE;
        Super::UseUniforms(pShader, pMesh);
    }

    void CascadedShadowMapPass::ResetViewFrustumCorners() {
        m_frustumCorners[0] = SR_MATH_NS::FVector3(-1.0f, 1.0f, 0.0f);
        m_frustumCorners[1] = SR_MATH_NS::FVector3(1.0f, 1.0f, 0.0f);
        m_frustumCorners[2] = SR_MATH_NS::FVector3(1.0f, -1.0f, 0.0f);
        m_frustumCorners[3] = SR_MATH_NS::FVector3(-1.0f, -1.0f, 0.0f);
        m_frustumCorners[4] = SR_MATH_NS::FVector3(-1.0f, 1.0f, 1.0f);
        m_frustumCorners[5] = SR_MATH_NS::FVector3(1.0f, 1.0f, 1.0f);
        m_frustumCorners[6] = SR_MATH_NS::FVector3(1.0f, -1.0f, 1.0f);
        m_frustumCorners[7] = SR_MATH_NS::FVector3(-1.0f, -1.0f, 1.0f);
    }

    void CreateOrthographicCamera(
        float minX, float minY, float maxX, float maxY, float nearZ, float farZ,
        const SR_MATH_NS::FVector3& cameraPosition, const SR_MATH_NS::FVector3& cameraTarget, const SR_MATH_NS::FVector3& up,
        SR_MATH_NS::Matrix4x4& view, SR_MATH_NS::Matrix4x4& projection
    ) {
        //projection = SR_MATH_NS::Matrix4x4::CreateOrthographicOffCenter(minX, maxX, minY, maxY, nearZ, farZ);
        projection = SR_MATH_NS::Matrix4x4::Ortho(minX, maxX, minY, maxY, nearZ, farZ);
        view = SR_MATH_NS::Matrix4x4::LookAt(cameraPosition, cameraTarget, up);
    }

    void CascadedShadowMapPass::UpdateCascades2() {
        auto&& pCamera = GetCamera();
        if (!pCamera) {
            return;
        }

        const auto lightDirection = GetRenderScene()->GetLightSystem()->GetDirectionalLightDirection();

        const auto numCascades = GetLayersCount();

        m_cascadeMatrices.resize(numCascades);
        m_cascadeSplitDepths.resize(numCascades);

        for (uint8_t cascadeIdx = 0; cascadeIdx < numCascades; ++cascadeIdx) {
            // Get the 8 points of the view frustum in world space
            ResetViewFrustumCorners();

            const float prevSplitDist = cascadeIdx == 0 ? 0.0f : m_cascadeSplitDepths[cascadeIdx - 1];
            const float splitDist = m_cascadeSplitDepths[cascadeIdx];

            //var invViewProj = Matrix4x4Utility.Invert(camera.ViewProjection);
            const auto invViewProj = (pCamera->GetProjection() * pCamera->GetView()).Inverse();

            for (uint8_t i = 0; i < 8; ++i) {
                //m_frustumCorners[i] = Vector4.Transform(_frustumCorners[i], invViewProj).ToVector3();
                m_frustumCorners[i] = invViewProj.TransformVector(m_frustumCorners[i]).XYZ();
            }

            // Get the corners of the current cascade slice of the view frustum
            for (uint8_t i = 0; i < 4; ++i) {
                const SR_MATH_NS::FVector3 cornerRay = m_frustumCorners[i + 4] - m_frustumCorners[i];
                const SR_MATH_NS::FVector3 nearCornerRay = cornerRay * prevSplitDist;
                const SR_MATH_NS::FVector3 farCornerRay = cornerRay * splitDist;
                m_frustumCorners[i + 4] = m_frustumCorners[i] + farCornerRay;
                m_frustumCorners[i] = m_frustumCorners[i] + nearCornerRay;
            }

            // Calculate the centroid of the view frustum slice
            SR_MATH_NS::FVector3 frustumCenter = SR_MATH_NS::FVector3::Zero();
            for (uint8_t i = 0; i < 8; ++i) {
                frustumCenter = frustumCenter + m_frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            // Pick the up vector to use for the light camera
            //var upDir = camera.View.Right();
            //SR_MATH_NS::FVector3 upDir = pCamera->GetView().Up();
            SR_MATH_NS::FVector3 upDir = pCamera->GetRotation() * SR_MATH_NS::FVector3::UnitY();

            SR_MATH_NS::FVector3 minExtents;
            SR_MATH_NS::FVector3 maxExtents;

            const float shadowMapSize = pCamera->GetSize().x;

            if (m_stabilizeShadowCascades) {
                // This needs to be constant for it to be stable
                //upDir = Vector3.UnitZ;
                upDir = SR_MATH_NS::FVector3::UnitY();

                // Calculate the radius of a bounding sphere surrounding the frustum corners
                float sphereRadius = 0.0f;
                for (uint8_t i = 0; i < 8; ++i) {
                    const float dist = (m_frustumCorners[i] - frustumCenter).Length();
                    sphereRadius = SR_MAX(sphereRadius, dist);
                }

                sphereRadius = SR_MATH_NS::Ceiling(sphereRadius * 16.0f) / 16.0f;

                maxExtents = SR_MATH_NS::FVector3(sphereRadius);
                minExtents = -maxExtents;
            }
            else {
                // Create a temporary view matrix for the light
                const auto lightCameraPos = frustumCenter;
                const auto lookAt = frustumCenter - lightDirection;
                const auto lightView = SR_MATH_NS::Matrix4x4::LookAt(lightCameraPos, lookAt, upDir);

                // Calculate an AABB around the frustum corners
                auto mins = SR_MATH_NS::Vector3(std::numeric_limits<float>::max());
                auto maxes = SR_MATH_NS::Vector3(std::numeric_limits<float>::min());
                for (uint8_t i = 0; i < 8; ++i)
                {
                    //auto corner = Vector4.Transform(_frustumCorners[i], lightView).ToVector3();
                    auto corner = lightView.TransformVector(m_frustumCorners[i]).XYZ();
                    mins = mins.Min(corner);
                    maxes = maxes.Max(corner);
                }

                minExtents = mins;
                maxExtents = maxes;

                const float kernelSize = 2.f;
                /*
                 * GetFixedFilterKernelSize
                 *    case ShadowsType.Hard:
                        return 2;

                    case ShadowsType.Soft:
                        return 5;
                 */

                // Adjust the min/max to accommodate the filtering size
                const float scale = (shadowMapSize + kernelSize) / shadowMapSize;
                minExtents.x *= scale;
                minExtents.y *= scale;
                maxExtents.x *= scale;
                maxExtents.y *= scale;
            }

            const auto cascadeExtents = maxExtents - minExtents;

            // Get position of the shadow camera
            const auto shadowCameraPos = frustumCenter + -lightDirection * -minExtents.z;

            SR_MATH_NS::Matrix4x4 shadowCameraView, shadowCameraProjection;

            // Come up with a new orthographic camera for the shadow caster
            CreateOrthographicCamera(
                minExtents.x, minExtents.y, maxExtents.x, maxExtents.y,
                0.0f, cascadeExtents.z,
                shadowCameraPos, frustumCenter, upDir,
                shadowCameraView, shadowCameraProjection
            );

            if (m_stabilizeShadowCascades) {
                // Create the rounding matrix, by projecting the world-space origin and determining
                // the fractional offset in texel space
                const auto shadowMatrixTemp = shadowCameraView * shadowCameraProjection;

                auto shadowOrigin = SR_MATH_NS::FVector4(0.0f, 0.0f, 0.0f, 1.0f);
                //shadowOrigin = Vector4.Transform(shadowOrigin, shadowMatrixTemp);
                shadowOrigin = shadowMatrixTemp.TransformVector(shadowOrigin);
                shadowOrigin = shadowOrigin * (shadowMapSize / 2.0f);

                const auto roundedOrigin = shadowOrigin.Round();
                auto roundOffset = roundedOrigin - shadowOrigin;
                roundOffset = roundOffset * (2.0f / shadowMapSize);
                roundOffset.z = 0.0f;
                roundOffset.w = 0.0f;

                SR_MATH_NS::Matrix4x4 shadowProj = shadowCameraProjection;
                //shadowProj.M41 += roundOffset.x;
                //shadowProj.M42 += roundOffset.y;
                //shadowProj.M43 += roundOffset.z;
                //shadowProj.M44 += roundOffset.w;

                shadowProj.mm[12] += roundOffset.x;
                shadowProj.mm[13] += roundOffset.y;
                shadowProj.mm[14] += roundOffset.z;
                shadowProj.mm[15] += roundOffset.w;

                shadowCameraProjection = shadowProj;
            }

            const auto shadowCameraViewProjection = shadowCameraView * shadowCameraProjection;
            m_cascadeMatrices[cascadeIdx] = shadowCameraViewProjection;

            // Apply the scale/offset matrix, which transforms from [-1,1]
            // post-projection space to [0,1] UV space
            const auto texScaleBias =
                SR_MATH_NS::Matrix4x4::FromScale(SR_MATH_NS::FVector3(0.5f, -0.5f, 1.0f)) *
                SR_MATH_NS::Matrix4x4::FromTranslate(SR_MATH_NS::FVector3(0.5f, 0.5f, 0.0f));

            auto shadowMatrix = shadowCameraViewProjection;
            shadowMatrix = shadowMatrix * texScaleBias;

            // Store the split distance in terms of view space depth
            const float clipDist = m_far - m_near;

            m_cascadeSplitDepths[cascadeIdx] = m_near + splitDist * clipDist;

            // Calculate the position of the lower corner of the cascade partition, in the UV space
            // of the first cascade partition
            /*var invCascadeMat = Matrix4x4Utility.Invert(shadowMatrix);
            var cascadeCorner = Vector4.Transform(Vector3.Zero, invCascadeMat).ToVector3();
            cascadeCorner = Vector4.Transform(cascadeCorner, globalShadowMatrix).ToVector3();

            // Do the same for the upper corner
            var otherCorner = Vector4.Transform(Vector3.One, invCascadeMat).ToVector3();
            otherCorner = Vector4.Transform(otherCorner, globalShadowMatrix).ToVector3();

            // Calculate the scale and offset
            var cascadeScale = Vector3.One / (otherCorner - cascadeCorner);
            shadowData.CascadeOffsets[cascadeIdx] = new Vector4(-cascadeCorner, 0.0f);
            shadowData.CascadeScales[cascadeIdx] = new Vector4(cascadeScale, 1.0f);*/
        }
    }

    void CascadedShadowMapPass::UpdateCascades() {
        SR_TRACY_ZONE;

        auto&& pCamera = GetCamera();
        const auto lightPos = GetRenderScene()->GetLightSystem()->GetDirectionalLightPosition();

        std::vector<float_t> cascadeSplits;
        cascadeSplits.resize(GetLayersCount());

        m_cascadeMatrices.resize(4);
        m_cascadeSplitDepths.resize(4);

        const float_t clipRange = m_far - m_near;

        const float_t minZ = m_near;
        const float_t maxZ = m_near + clipRange;

        const float_t range = maxZ - minZ;
        const float_t ratio = maxZ / minZ;

        for (uint32_t i = 0; i < GetLayersCount(); i++) {
            const float_t p = static_cast<float_t>(i + 1) / static_cast<float_t>(GetLayersCount());
            const float_t log = minZ * std::pow(ratio, p);
            const float_t uniform = minZ + range * p;
            const float_t d = m_cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - m_near) / clipRange;
        }

        float_t lastSplitDist = 0.0;

        for (uint32_t i = 0; i < GetLayersCount(); i++) {
            const float_t splitDist = cascadeSplits[i];

            SR_MATH_NS::FVector3 frustumCorners[8] = {
                SR_MATH_NS::FVector3(-1.0f,  1.0f, -1.0f),
                SR_MATH_NS::FVector3( 1.0f,  1.0f, -1.0f),
                SR_MATH_NS::FVector3( 1.0f, -1.0f, -1.0f),
                SR_MATH_NS::FVector3(-1.0f, -1.0f, -1.0f),
                SR_MATH_NS::FVector3(-1.0f,  1.0f,  1.0f),
                SR_MATH_NS::FVector3( 1.0f,  1.0f,  1.0f),
                SR_MATH_NS::FVector3( 1.0f, -1.0f,  1.0f),
                SR_MATH_NS::FVector3(-1.0f, -1.0f,  1.0f),
            };

            auto&& invCamera = (pCamera->GetProjection() * pCamera->GetViewTranslate()).Inverse();

            for (auto&& frustumCorner : frustumCorners) {
                SR_MATH_NS::FVector4 invCorner = invCamera * SR_MATH_NS::FVector4(frustumCorner, 1.0f);
                frustumCorner = (invCorner / invCorner.w).XYZ();
            }

            for (uint32_t j = 0; j < 4; j++) {
                SR_MATH_NS::FVector3 dist = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
            }

            auto&& frustumCenter = SR_MATH_NS::FVector3(0.0f);
            for (auto&& frustumCorner : frustumCorners) {
                frustumCenter += frustumCorner;
            }
            frustumCenter /= 8.0f;

            float_t radius = 0.0f;
            for (auto&& frustumCorner : frustumCorners) {
                float_t distance = (frustumCorner - frustumCenter).Length();
                radius = SR_MAX(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            auto&& maxExtents = SR_MATH_NS::FVector3(radius);
            SR_MATH_NS::FVector3 minExtents = -maxExtents;

            SR_MATH_NS::FVector3 lightDir = (-lightPos).Normalize();

            SR_MATH_NS::Matrix4x4 lightViewMatrix = SR_MATH_NS::Matrix4x4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f));

            m_cascadeSplitDepths[i] = (m_near + splitDist * clipRange) * -1.0f;

            if (m_usePerspective) {
                /// TODO: not works
                m_cascadeMatrices[i] = pCamera->GetProjection() * lightViewMatrix;
            }
            else {
                auto&& lightOrthoMatrix = SR_MATH_NS::Matrix4x4::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
                m_cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;
            }

            lastSplitDist = cascadeSplits[i];
        }
    }

    void CascadedShadowMapPass::UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader* pShader) {
        Super::UseUniformsFromAnotherPass(pShader);

        pShader->SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());
        pShader->SetValue<false>(SHADER_CASCADE_SPLITS, m_cascadeSplitDepths.data());

        //else if (m_shadowMapPass) {
        //    pShader->SetMat4(SHADER_LIGHT_SPACE_MATRIX, m_shadowMapPass->GetLightSpaceMatrix());
        //}
    }

    void CascadedShadowMapPass::UseConstants(SR_GTYPES_NS::Shader* pShader) {
        Super::UseConstants(pShader);

        pShader->SetConstInt(SHADER_PC_SHADOW_CASCADE_INDEX, GetPipeline()->GetCurrentFrameBufferLayer());
    }

    bool CascadedShadowMapPass::CheckCamera() {
        auto&& pCamera = GetCamera();
        if (!pCamera) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        if (m_directionalLightPosition != GetRenderScene()->GetLightSystem()->GetDirectionalLightPosition()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_cameraPosition.Distance(pCamera->GetPosition()) > 1.0) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_cameraRotation != pCamera->GetRotation()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_screenSize != pCamera->GetSize()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        return false;

    dirty:
        m_directionalLightPosition = GetRenderScene()->GetLightSystem()->GetDirectionalLightPosition();
        m_cameraPosition = pCamera->GetPosition();
        m_cameraRotation = pCamera->GetRotation();
        m_screenSize = pCamera->GetSize();

        return true;
    }

    void CascadedShadowMapPass::UseSharedUniforms(SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;

        Super::UseSharedUniforms(pShader);

        static auto lastCheck = std::chrono::high_resolution_clock::now();
        const auto now = std::chrono::high_resolution_clock::now();
        bool passed = false;
        //if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck).count() > 1000) {
        //    lastCheck = now;
        //    passed = true;
        //}
        passed = true;

        //if (CheckCamera()) SR_UNLIKELY_ATTRIBUTE {
        if (m_useOtherAlgorithm) {
            UpdateCascades2();
        }
        else {
            if (passed && CheckCamera()) SR_UNLIKELY_ATTRIBUTE {
                UpdateCascades();
            }
        }
        //
        //}

        pShader->SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());

        const auto lightPos = GetRenderScene()->GetLightSystem()->GetDirectionalLightPosition();
        pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_POSITION, lightPos);
    }
}