//
//  SuperpoweredSoundManager.h
//  noctisgames-framework
//
//  Created by Stephen Gowen on 3/8/17.
//  Copyright (c) 2017 Noctis Games. All rights reserved.
//

#ifndef __noctisgames__NGWindowsSoundCollection__
#define __noctisgames__NGWindowsSoundCollection__

#include "Audio.h"

#include <vector>
#include <memory>

class NGWindowsSoundCollection
{
public:
    NGWindowsSoundCollection(int soundId, const char *path, int numInstances = 1);
    
    ~NGWindowsSoundCollection();
    
    DirectX::SoundEffectInstance* getSound();
    
    std::vector<std::unique_ptr<DirectX::SoundEffectInstance>>& getSounds();
    
    int getSoundIndex();
    
    int getSoundId();
    
    int getNumInstances();
    
private:
    std::unique_ptr<DirectX::SoundEffect> m_sound;
    std::vector<std::unique_ptr<DirectX::SoundEffectInstance>> m_sounds;
    int m_iSoundIndex;
    int m_iSoundId;
    int m_iNumInstances;
};

#endif /* defined(__noctisgames__NGWindowsSoundCollection__) */
