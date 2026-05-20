//
// Created by Nariman on 17.05.2026.
//


#include <Graphics/Particles/ParticleUpdater.h>

namespace SR_GRAPH_NS{
    void ParticleUpdater::Update(float_t dt){
        for (uint32_t i = 0; i < m_Emitters.size(); i++){
            m_Emitters[i]->UpdateEmitter(dt);
        }
    }

    void ParticleUpdater::RegisterEmitter(ParticleEmitter* pEmitter){
        m_Emitters.push_back(pEmitter);
    }

    void ParticleUpdater::RemoveEmitter(ParticleEmitter* pEmitter){
        for (int i = 0; i < m_Emitters.size(); i++){
            if (m_Emitters[i] == pEmitter){
                m_Emitters.erase(m_Emitters.begin() + i);
                break;
            }
        }
    }
}
