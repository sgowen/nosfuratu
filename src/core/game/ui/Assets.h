//
//  Assets.h
//  nosfuratu
//
//  Created by Stephen Gowen on 2/22/14.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#ifndef __nosfuratu__Assets__
#define __nosfuratu__Assets__

#include "TextureRegion.h"
#include "Animation.h"
#include "Vector2D.h"
#include "Rectangle.h"
#include "GameConstants.h"
#include "Background.h"
#include "Midground.h"
#include "Ground.h"
#include "ExitGround.h"
#include "Hole.h"
#include "DustCloud.h"
#include "Jon.h"
#include "BackButton.h"
#include "LevelEditorButton.h"
#include "TrashCan.h"
#include "LevelSelectorPanel.h"

#include <vector>

#define MAX_SOUNDS_TO_PLAY_PER_FRAME 4

class CollectibleItem;
class LevelEditorActionsPanel;
class LevelEditorEntitiesPanel;
class Enemy;
class EnemySpirit;
class ForegroundObject;
class TitlePanel;
class WorldMapPanel;
class LevelThumbnail;
class CountHissWithMina;

class Assets
{
public:
	static Assets * getInstance();
    
    TextureRegion& get(TitlePanel* panel);
    
    TextureRegion& get(WorldMapPanel* panel);
    
    TextureRegion& get(LevelThumbnail* thumbnail);
    
    TextureRegion& get(Background* background);
    
    TextureRegion& get(Midground* midground);
    
    TextureRegion& get(Ground* ground);
    
    TextureRegion& get(ExitGround* exitGround);
    
    TextureRegion& get(ExitGroundCover* exitGroundCover);
    
    TextureRegion& get(Hole* hole);
    
    TextureRegion& get(HoleCover* holeCover);
    
    TextureRegion& get(ForegroundObject* foregroundObject);
    
    TextureRegion& get(CountHissWithMina* countHissWithMina);
    
    TextureRegion& get(Enemy* enemy);
    
    TextureRegion& get(EnemySpirit* spirit);
    
    TextureRegion& get(CollectibleItem* collectibleItem);
    
    TextureRegion& get(Jon* jon);
    
    TextureRegion& get(DustCloud* dustCloud);
    
    TextureRegion& get(BackButton* backButton);
    
    TextureRegion& get(LevelEditorButton* levelEditorButton);
    
    TextureRegion& get(LevelEditorEntitiesPanel* levelEditorEntitiesPanel);
    
    TextureRegion& get(LevelEditorActionsPanel* levelEditorActionsPanel);
    
    TextureRegion& get(TrashCan* trashCan);
    
    TextureRegion& get(LevelSelectorPanel* levelSelectorPanel);
    
    short getFirstSoundId();
    
    void addSoundIdToPlayQueue(short soundId);
    
    void forceAddSoundIdToPlayQueue(short soundId);
    
    void eraseFirstSoundId();
    
    void setMusicId(short musicId);
    
    short getMusicId();
    
    bool isUsingCompressedTextureSet();
    
    void setUsingCompressedTextureSet(bool isUsingCompressedTextureSet);

private:
    std::vector<short> m_sSoundIds;
    short m_sMusicId;
    bool m_isUsingCompressedTextureSet;
    
    Animation createAnimation(int x, int y, int regionWidth, int regionHeight, int animationWidth, int animationHeight, int textureWidth, int textureHeight, bool looping, int numFrames);
    
    Animation createAnimation(int x, int y, int regionWidth, int regionHeight, int animationWidth, int animationHeight, int textureWidth, int textureHeight, bool looping, float frameTime, int numFrames, int firstLoopingFrame = 0);
    
    TextureRegion createTextureRegion(int x, int y, int regionWidth, int regionHeight, int textureWidth, int textureHeight);

	void initTextureRegion(TextureRegion& tr, int x, int regionWidth, int textureWidth);
    
    // ctor, copy ctor, and assignment should be private in a Singleton
    Assets();
    Assets(const Assets&);
    Assets& operator=(const Assets&);
};

#endif /* defined(__nosfuratu__Assets__) */