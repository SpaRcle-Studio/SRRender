//
// Created by Nariman on 17.05.2026.
//

#ifndef SRENGINE_PARTICLEUPDATER_H
#define SRENGINE_PARTICLEUPDATER_H

#include <Graphics/Particles/ParticleEmitter.h>

namespace SR_GRAPH_NS{
    class ParticleUpdater{
    public:
        void RegisterEmitter(ParticleEmitter* pEmitter);
        void RemoveEmitter(ParticleEmitter* pEmitter);

        void Update(float_t dt);
    private:
        SR_HTYPES_NS::FastMemoryArray<ParticleEmitter*> m_Emitters;

    };
}
#endif //SRENGINE_PARTICLEUPDATER_H
