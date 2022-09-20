//
// Created by Nikita on 19.11.2020.
//

#ifndef GAMEENGINE_POSTPROCESSING_H
#define GAMEENGINE_POSTPROCESSING_H

/*
#include <Utils/Math/Vector3.h>

namespace SR_GRAPH_NS::Types {
    class Camera;
}

namespace SR_GRAPH_NS {
    class Render;
    class Shader;

    // TODO: add freeing video buffers!
    class PostProcessing {
    protected:
        virtual ~PostProcessing() = default;
        //PostProcessing(Camera* camera);
    public:
        PostProcessing(const PostProcessing&) = delete;
    protected:
        float			       m_gamma                 = 0.8f;
        float                  m_exposure              = 1.f;
        float                  m_saturation            = 1.f;
        Helper::Math::FVector3 m_colorCorrection       = { 1, 1, 1 };
        Helper::Math::FVector3 m_bloomColor            = { 1, 1, 1 };
        float                  m_bloomIntensity        = 1.f;
        volatile uint8_t       m_bloomAmount           = 6;
    protected:
        volatile bool          m_bloom                 = true;
        bool                   m_bloomClear            = false;
    protected:
        bool                   m_horizontal            = false;
        bool                   m_firstIteration        = false;

        int32_t                m_finalDepth            = -1;
        int32_t                m_finalFBO              = -1;
        int32_t                m_finalColorBuffer      = -1;


        std::vector<int32_t>  m_colors                = { -1, -1, -1, -1, -1 };
        int32_t               m_depth                 = -1;
        int32_t               m_frameBuffer           = -1;

        int32_t               m_descriptorSet         = -1;
        int32_t               m_ubo                   = -1;
    protected:
        Environment*          m_env                   = nullptr;

        Shader*               m_postProcessingShader  = nullptr;
        Shader*               m_blurShader            = nullptr;

        //Camera*               m_camera                = nullptr;
        Render*               m_render                = nullptr;
        bool                  m_isInit                = false;
    public:
        //static PostProcessing* Allocate(Camera* camera);
    public:
        [[nodiscard]] SR_FORCE_INLINE Helper::Math::FVector3 GetColorCorrection() const noexcept { return m_colorCorrection; }
        [[nodiscard]] SR_FORCE_INLINE Helper::Math::FVector3 GetBloomColor()      const noexcept { return m_bloomColor;       }

        [[nodiscard]] SR_FORCE_INLINE int32_t GetFinalFBO()          const noexcept { return m_finalFBO;            }

        [[nodiscard]] SR_FORCE_INLINE float GetGamma()               const noexcept { return m_gamma;               }
        [[nodiscard]] SR_FORCE_INLINE float GetExposure()            const noexcept { return m_exposure;            }
        [[nodiscard]] SR_FORCE_INLINE float GetSaturation()          const noexcept { return m_saturation;          }
        [[nodiscard]] SR_FORCE_INLINE unsigned char GetBloomAmount() const noexcept { return m_bloomAmount;         }
        [[nodiscard]] SR_FORCE_INLINE float GetBloomIntensity()      const noexcept { return m_bloomIntensity;      }
        [[nodiscard]] SR_FORCE_INLINE bool GetBloomEnabled()         const noexcept { return this->m_bloom;         }
    public:
        SR_FORCE_INLINE void SetGamma(float gamma)                           noexcept { m_gamma                = gamma;     }
        SR_FORCE_INLINE void SetSaturation(float gamma)                      noexcept { m_saturation           = gamma;     }
        SR_FORCE_INLINE void SetExposure(float exposure)                     noexcept { m_exposure             = exposure;  }
        SR_FORCE_INLINE void SetBloomAmount(unsigned int amount)             noexcept { this->m_bloomAmount    = amount;    }
        SR_FORCE_INLINE void SetBloomIntensity(float intensity)              noexcept { this->m_bloomIntensity = intensity; }
        SR_FORCE_INLINE void SetColorCorrection(Helper::Math::FVector3 value) noexcept { m_colorCorrection     = value;     }
        SR_FORCE_INLINE void SetBloomColor(Helper::Math::FVector3 value)      noexcept { m_bloomColor           = value;     }
        SR_FORCE_INLINE void SetBloom(bool v)                                noexcept { this->m_bloom = v;                  }
    public:
        virtual bool Init(Render* render);

        virtual bool Destroy();

        virtual bool Free() = 0;
        virtual bool OnResize(uint32_t w, uint32_t h);

        virtual void BeginSkybox()   = 0;
        virtual void EndSkybox()     = 0;

        virtual bool BeginGeometry() = 0;
        virtual void EndGeometry()   = 0;

        //! \warning Привязывается к конкретному кадровому буферу
        virtual void Complete() = 0;
        //! \warning Должна вызываться в том же кадровом буфере, что и Complete
        virtual void Draw() { };
    public:
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetFinally()       const noexcept { return this->m_finalColorBuffer;  }
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetColoredImage()  const noexcept { return this->m_colors[0];   }
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetBloomMask()     const noexcept { return this->m_colors[1];   }
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetDepthBuffer()   const noexcept { return this->m_colors[2];   }
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetSkyboxColor()   const noexcept { return m_colors[4];         }
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetStencilBuffer() const noexcept { return m_colors[3];         }
        [[nodiscard]] SR_FORCE_INLINE uint32_t GetCustomColorBuffer(uint8_t id) const noexcept { return m_colors[id]; }
        //[[nodiscard]] SR_FORCE_INLINE uint32_t GetBlurBloomMask() const noexcept { return m_PingPongColorBuffers[0]; }
    };
}

*/

#endif //GAMEENGINE_POSTPROCESSING_H
