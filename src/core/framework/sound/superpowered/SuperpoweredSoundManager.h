//
//  SuperpoweredSoundManager.h
//  nosfuratu
//
//  Created by Stephen Gowen on 10/20/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#ifndef __nosfuratu__SuperpoweredSoundManager__
#define __nosfuratu__SuperpoweredSoundManager__

#include "SuperpoweredSound.h"

#include "EntityUtils.h"

#include <vector>

#define MAX_NUM_SOUND_PLAYERS 12

class SuperpoweredSoundCollection
{
public:
    std::vector<SuperpoweredSound*> m_sounds;
    int m_iSoundIndex;
    
    SuperpoweredSoundCollection(const char *path, unsigned int sampleRate, int rawResourceId, int numCopies = 1, int fileOffset = -1, int fileLength = -1) :
    m_iSoundIndex(0),
    m_iRawResourceId(rawResourceId),
    m_iNumCopies(numCopies)
    {
        for (int i = 0; i < m_iNumCopies; i++)
        {
            m_sounds.push_back(new SuperpoweredSound(path, sampleRate, rawResourceId, fileOffset, fileLength));
        }
    }
    
    ~SuperpoweredSoundCollection()
    {
        EntityUtils::cleanUpVectorOfPointers(m_sounds);
    }
    
    SuperpoweredSound* getSound()
    {
        SuperpoweredSound* ret = m_sounds[m_iSoundIndex++];
        if (m_iSoundIndex >= m_iNumCopies)
        {
            m_iSoundIndex = 0;
        }
        
        return ret;
    }
    
    int getRawResourceId() { return m_iRawResourceId; }
    int getNumCopies() { return m_iNumCopies; }
    
private:
    int m_iRawResourceId;
    int m_iNumCopies;
};

class SuperpoweredSoundManager
{
public:
    SuperpoweredSoundManager(unsigned int sampleRate, unsigned int bufferSize = 0);
	~SuperpoweredSoundManager();
    
    void loadSound(int rawResourceId, const char *path, int numCopies = 1, int fileOffset = -1, int fileLength = -1);
    
    void playSound(int rawResourceId, float volume, bool isLooping);
    
    void stopSound(int rawResourceId);
    
    void loadMusic(int rawResourceId, const char *path, int fileOffset = -1, int fileLength = -1);
    
    void playMusic(int rawResourceId, float volume, bool isLooping);
    
    void setMusicVolume(float volume);
    
    void resumeMusic();
    
    void pauseMusic();
    
    bool processMusic(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    
    bool processSound1(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound2(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound3(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound4(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound5(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound6(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound7(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound8(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound9(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound10(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    bool processSound11(void *output, unsigned int numberOfSamples, unsigned int sampleRate);
    
    bool processSound(void *output, unsigned int numberOfSamples, SuperpoweredSound *sound, float *stereoBuffer, unsigned int sampleRate);

private:
    SuperpoweredSound* m_music;
    std::vector<SuperpoweredSoundCollection*> m_sounds;
    std::vector<float*> m_stereoBuffers;
    SuperpoweredSound *m_activeSounds[MAX_NUM_SOUND_PLAYERS - 1];
    int m_iSampleRate;
    int m_iSoundIndex;
};

#endif /* defined(__nosfuratu__SuperpoweredSound__) */
