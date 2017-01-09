//
//  GameScreenLevels.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 2/10/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#include "GameScreenLevels.h"

#include "State.h"
#include "GameScreen.h"
#include "EntityUtils.h"
#include "Vector2D.h"
#include "Game.h"
#include "GameScreenLevelEditor.h"
#include "MathUtil.h"
#include "GameTracker.h"
#include "BatPanel.h"
#include "GameScreenWorldMap.h"
#include "GameScreenTransitions.h"

/// Level ///

Level * Level::getInstance()
{
    static Level *instance = new Level();
    
    return instance;
}

void Level::enter(GameScreen* gs)
{
    m_fStateTime = 0;
    m_iScoreFromTime = 0;
    m_iScoreFromObjects = 0;
    m_iScore = 0;
    m_iOnlineScore = 0;
    m_iLevelStatsFlag = m_iBestLevelStatsFlag;
    m_iNumGoldenCarrots = m_iLastKnownNumGoldenCarrots;
    m_hasCompletedLevel = false;
    m_isDisplayingResults = false;
    m_hasStoppedAllLoopingSoundsAfterJonDeath = false;
    gs->m_isReleasingShockwave = false;
    gs->m_isScreenHeldDown = false;
    gs->m_fScreenHeldTime = 0;
    
    if (!m_game->isLoaded())
    {
        if (m_sourceGame)
        {
            m_game->copy(m_sourceGame);
        }
        else
        {
            m_game->load(m_json);
        }
        
        m_game->setBestLevelStatsFlag(m_iBestLevelStatsFlag);
        m_game->setCameraBounds(&gs->m_renderer->getCameraBounds());
        
        Jon& jon = m_game->getJon();
        jon.setAbilityFlag(m_iLastKnownJonAbilityFlag);
    }
    
    m_continueButton->getColor().alpha = 0;
    
    m_batPanel->reset();
    
    configBatPanel();

    initRenderer(gs);
    
    gs->m_renderer->beginOpeningPanningSequence(*m_game);
    
    onEnter(gs);
    
    m_game->updateBackgrounds(gs->m_renderer->getCameraPosition(), gs->m_fDeltaTime);
    
    static float fgWidth = CAM_WIDTH / 32;
    static float fgHeight = fgWidth * 1.171875f;
    
    static float textY = CAM_HEIGHT - fgHeight;
    
    GameTracker::getInstance()->config(CAM_WIDTH * 0.5f,
                                       textY,
                                       3.34f,
                                       textY - 0.14f,
                                       fgWidth,
                                       fgHeight);
    
    m_playLevelSelectMusicOnExit = gs->m_stateMachine->getPreviousState() == WorldMap::getInstance();
	m_stopMusicOnExit = gs->m_stateMachine->getPreviousState() == GameScreenLevelEditor::getInstance();
}

void Level::initRenderer(GameScreen* gs)
{
    gs->m_renderer->init(calcRendererTypeFromLevel(m_game->getWorld(), m_game->getLevel()));
}

void Level::execute(GameScreen* gs)
{
    if (gs->m_isRequestingRender)
    {
        render(gs);
    }
    else
    {
        update(gs);
    }
}

void Level::exit(GameScreen* gs)
{
    m_iNumTimesBatPanelDisplayed = 0;
    m_iNumAttemptsSinceLastAdBreak = 0;
    
    m_game->reset();
    
    m_sourceGame = nullptr;
    
    stopAllSounds();
    
    if (m_playLevelSelectMusicOnExit)
    {
        ASSETS->addMusicIdToPlayQueue(MUSIC_LOAD_LEVEL_SELECT_LOOP);
        ASSETS->addMusicIdToPlayQueue(MUSIC_PLAY_LOOP);
    }
	else if (m_stopMusicOnExit)
	{
		ASSETS->addMusicIdToPlayQueue(MUSIC_STOP);
	}
    
    m_fStateTime = 0;
    gs->m_isReleasingShockwave = false;
    gs->m_fShockwaveElapsedTime = 0;
    gs->m_fShockwaveCenterX = 0;
    gs->m_fShockwaveCenterY = 0;
    m_hasShownOpeningSequence = false;
    m_hasOpeningSequenceCompleted = false;
    gs->m_hasSwiped = false;
    m_showDeathTransOut = false;
    m_hasCompletedLevel = false;
    m_isDisplayingResults = false;
    m_exitLoop = false;
    
    m_iBestScore = 0;
    m_iBestOnlineScore = 0;
    m_iBestLevelStatsFlag = 0;
    m_iLastKnownNumGoldenCarrots = 0;
    m_iLastKnownJonAbilityFlag = 0;
}

void Level::setSourceGame(Game* game)
{
    m_sourceGame = game;
}

void Level::setBestStats(int bestScore, int bestOnlineScore, int bestLevelStatsFlag, int numGoldenCarrots, int jonAbilityFlag)
{
    m_iBestScore = bestScore;
    m_iBestOnlineScore = bestOnlineScore;
    m_iBestLevelStatsFlag = bestLevelStatsFlag;
    m_iLastKnownNumGoldenCarrots = numGoldenCarrots;
    m_iLastKnownJonAbilityFlag = jonAbilityFlag;
}

void Level::stopAllSounds()
{
    ASSETS->forceAddSoundIdToPlayQueue(STOP_ALL_SOUNDS);
}

void Level::stopAllLoopingSounds()
{
    ASSETS->forceAddSoundIdToPlayQueue(STOP_ALL_LOOPING_SOUNDS);
}

int Level::getJonAbilityFlag()
{
    return m_game->getJon().getAbilityFlag();
}

void Level::onEnter(GameScreen* gs)
{
    if (m_hasOpeningSequenceCompleted)
    {
        CountHissWithMina& countHissWithMina = m_game->getCountHissWithMina();
        countHissWithMina.getPosition().setX(m_game->getFarRight() + CAM_WIDTH * 2);
        
        updateCamera(gs, 0, false, true);
    }
}

void Level::beginOpeningSequence(GameScreen* gs)
{
    CountHissWithMina& countHissWithMina = m_game->getCountHissWithMina();
    countHissWithMina.beginMovement();
    
    if (gs->m_stateMachine->getPreviousState() == GameScreenLevelEditor::getInstance())
	{
		m_hasShownOpeningSequence = true;
		m_hasOpeningSequenceCompleted = true;

		updateCamera(gs, 0, false, true);

        if (ASSETS->isMusicEnabled())
        {
            ASSETS->addMusicIdToPlayQueue(MUSIC_LOAD_WORLD_1_LOOP);
            ASSETS->addMusicIdToPlayQueue(MUSIC_PLAY_LOOP);
        }

		return;
	}

	gs->m_renderer->beginOpeningPanningSequence(*m_game);

	m_game->updateBackgrounds(gs->m_renderer->getCameraPosition(), gs->m_fDeltaTime);

	m_hasShownOpeningSequence = true;

	if (ASSETS->isMusicEnabled())
	{
		ASSETS->addSoundIdToPlayQueue(SOUND_WORLD_1_LOOP_INTRO);
        ASSETS->addMusicIdToPlayQueue(MUSIC_LOAD_WORLD_1_LOOP);
	}
}

void Level::handleOpeningSequence(GameScreen* gs)
{
	CountHissWithMina& countHissWithMina = m_game->getCountHissWithMina();
	countHissWithMina.update(gs->m_fDeltaTime);

	Jon& jon = m_game->getJon();
	jon.update(gs->m_fDeltaTime);

	int result = gs->m_renderer->updateCameraToFollowPathToJon(*m_game);
	m_hasOpeningSequenceCompleted = result == 3;
	m_activateRadialBlur = result == 1;
	jon.setAllowedToMove(m_hasOpeningSequenceCompleted);

	if (m_hasOpeningSequenceCompleted)
	{
		countHissWithMina.getPosition().setX(m_game->getFarRight() + CAM_WIDTH * 2);

		ASSETS->addMusicIdToPlayQueue(MUSIC_PLAY_LOOP);
	}

	if (result == 2)
	{
		jon.beginWarmingUp();
	}

	m_game->updateBackgrounds(gs->m_renderer->getCameraPosition(), gs->m_fDeltaTime);
}

void Level::update(GameScreen* gs)
{
    Jon& jon = m_game->getJon();
    jon.setAllowedToMove(m_hasOpeningSequenceCompleted);
    
    if (!m_hasShownOpeningSequence)
    {
		beginOpeningSequence(gs);
    }
    else if (!m_hasOpeningSequenceCompleted)
    {
		handleOpeningSequence(gs);
    }
    else
    {
        GameTracker::getInstance()->update(gs->m_fDeltaTime);
        
        m_batPanel->update(gs);
        
        if (m_batPanel->isRequestingInput())
        {
            return;
        }
        
        if (m_isDisplayingResults)
        {
            m_continueButton->getColor().alpha += gs->m_fDeltaTime;
            if (m_continueButton->getColor().alpha > 1)
            {
                m_continueButton->getColor().alpha = 1;
            }
            
            for (std::vector<TouchEvent *>::iterator i = gs->m_touchEvents.begin(); i != gs->m_touchEvents.end(); i++)
            {
                gs->touchToWorld(*(*i));
                
                switch ((*i)->getTouchType())
                {
                    case DOWN:
                    case DRAGGED:
                        continue;
                    case UP:
                        if (m_continueButton->handleClick(*gs->m_touchPoint))
                        {
                            if (m_game->getWorld() == 1
                                && m_game->getLevel() == 21)
                            {
                                m_playLevelSelectMusicOnExit = false;
                                LevelToComingSoon::getInstance()->setLevelComingFrom(this);
                                gs->m_stateMachine->changeState(LevelToComingSoon::getInstance());
                            }
                            else
                            {
                                gs->m_stateMachine->revertToPreviousState();
                            }
                            
                            return;
                        }
                        
                        break;
                }
            }
            
            return;
        }
        
        if (handleTouchInput(gs))
        {
            return;
        }
        
        if (m_showDeathTransOut)
        {
            // Starting new game after death
            
            m_fStateTime += gs->m_fDeltaTime * 2;
            
            if (m_fStateTime > 2.4f)
            {
                m_fStateTime = 0;
                m_showDeathTransOut = false;
            }
        }
        
        if (jon.isDead())
        {
            // Starting death transition, when screen goes black, new game begins
            
            if (!m_hasStoppedAllLoopingSoundsAfterJonDeath)
            {
                stopAllLoopingSounds();
                
                m_hasStoppedAllLoopingSoundsAfterJonDeath = true;
            }
            
            m_fStateTime += gs->m_fDeltaTime * 2;
            
            if (m_fStateTime > 1.6f)
            {
                m_game->reset();
                enter(gs);
                
                updateCamera(gs, 0, false, true);
                
                m_iNumAttemptsSinceLastAdBreak++;
                if (m_iNumAttemptsSinceLastAdBreak >= 9)
                {
                    gs->m_iRequestedAction = REQUESTED_ACTION_DISPLAY_INTERSTITIAL_AD;
                    m_iNumAttemptsSinceLastAdBreak = 0;
                }
                
                m_showDeathTransOut = true;
            }
            
            return;
        }
        else
        {
            updateScore();
            
            // Is Still Actively playing the Level
            
			if (!m_hasCompletedLevel)
			{
				m_game->update(gs->m_fDeltaTime);
			}
            
            if (isInSlowMotionMode())
            {
                gs->m_fDeltaTime /= 8;
            }
            
            if (jon.isTransformingIntoVampire() || jon.isRevertingToRabbit())
            {
                if (jon.isReleasingShockwave())
                {
                    if (!gs->m_isReleasingShockwave)
                    {
                        gs->m_fShockwaveCenterX = jon.getPosition().getX();
                        gs->m_fShockwaveCenterY = jon.getPosition().getY();
                        gs->m_fShockwaveElapsedTime = 0.0f;
                        gs->m_isReleasingShockwave = true;
                    }
                }
                else
                {
                    if (!isInSlowMotionMode())
                    {
                        gs->m_fDeltaTime /= 8;
                    }
                }
            }
            
            if (gs->m_isReleasingShockwave)
            {
                gs->m_fShockwaveElapsedTime += gs->m_fDeltaTime * 1.2f;
                
                if (gs->m_fShockwaveElapsedTime > 2)
                {
                    gs->m_fShockwaveElapsedTime = 0;
                    gs->m_isReleasingShockwave = false;
                }
            }
            
            m_game->updateAndClean(gs->m_fDeltaTime);
            
            handleCollections(jon, m_game->getCollectibleItems(), gs->m_fDeltaTime);
            
            updateScore();
            
            if (gs->m_isScreenHeldDown)
            {
                gs->m_fScreenHeldTime += gs->m_fDeltaTime;
                
                if (gs->m_fScreenHeldTime > 0.4f)
                {
                    jon.triggerTransform();
                    gs->m_isScreenHeldDown = false;
                    gs->m_fShockwaveElapsedTime = 0;
                    gs->m_isReleasingShockwave = false;
                }
            }
        }

        updateCamera(gs, (jon.isAlive() && jon.getVelocity().getX() < 0) ? jon.getVelocity().getX() : 0);
        
        if (m_game->getMarkers().size() > 1)
        {
            GameMarker* nextEndLoopMarker = m_game->getMarkers().at(1);
            if (gs->m_renderer->getCameraBounds().getRight() > nextEndLoopMarker->getGridX() * GRID_CELL_SIZE)
            {
                if (m_exitLoop)
                {
                    m_game->getMarkers().erase(m_game->getMarkers().begin());
                    m_game->getMarkers().erase(m_game->getMarkers().begin());
                    
                    m_exitLoop = false;
                }
                else
                {
                    GameMarker* beginLoopMarker = m_game->getMarkers().at(0);
                    
                    int beginGridX = beginLoopMarker->getGridX();
                    int endGridX = nextEndLoopMarker->getGridX();
                    
                    EntityUtils::copyAndOffset(m_game->getMidgrounds(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getGrounds(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getPits(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getExitGrounds(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getHoles(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getForegroundObjects(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getMidBossForegroundObjects(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getEndBossForegroundObjects(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getCountHissWithMinas(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getEndBossSnakes(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getEnemies(), beginGridX, endGridX);
                    EntityUtils::offsetOnly(m_game->getCollectibleItems(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getExtraForegroundObjects(), beginGridX, endGridX);
                    EntityUtils::copyAndOffset(m_game->getForegroundCoverObjects(), beginGridX, endGridX);
                    
                    EntityUtils::offsetAll(m_game->getMarkers(), beginGridX, endGridX);
                    
                    EntityUtils::setGameToEntities(m_game->getForegroundObjects(), m_game.get());
                    EntityUtils::setGameToEntities(m_game->getMidBossForegroundObjects(), m_game.get());
                    EntityUtils::setGameToEntities(m_game->getEndBossForegroundObjects(), m_game.get());
                    EntityUtils::setGameToEntities(m_game->getCollectibleItems(), m_game.get());
                    EntityUtils::setGameToEntities(m_game->getEnemies(), m_game.get());
                    EntityUtils::setGameToEntities(m_game->getExtraForegroundObjects(), m_game.get());
                    EntityUtils::setGameToEntities(m_game->getEndBossSnakes(), m_game.get());
                    
                    m_game->calcFarRight();
                }
            }
        }
        
        if (m_hasCompletedLevel)
        {
            m_fStateTime += gs->m_fDeltaTime / 2;
            if (m_fStateTime > 1)
            {
				stopAllLoopingSounds();

                m_fStateTime = 1;
                m_isDisplayingResults = true;
            }
            
            if (m_game->getLevel() != 10
                && m_game->getLevel() != 21)
            {
                float musicVolume = 1 - m_fStateTime;
                musicVolume = clamp(musicVolume, 1, 0);
                short musicId = MUSIC_SET_VOLUME * 1000 + (short) (musicVolume * 100);
                ASSETS->addMusicIdToPlayQueue(musicId);
            }
        }
        else if (jon.getMainBounds().getLeft() > m_game->getFarRight())
        {
            // Has Cleared the Level
            
            m_iLevelStatsFlag = FlagUtil::setFlag(m_iLevelStatsFlag, FLAG_LEVEL_COMPLETE);
            
            if (m_game->getNumCarrotsCollected() >= 100)
            {
                m_game->setNumGoldenCarrotsCollected(m_game->getNumGoldenCarrotsCollected() + 1);
                
                GameTracker::getInstance()->onScored(SCORE_GOLDEN_CARROT);
                GameTracker::getInstance()->onBonusGoldenCarrotEarned();
                
                m_iLevelStatsFlag = FlagUtil::setFlag(m_iLevelStatsFlag, FLAG_BONUS_GOLDEN_CARROT_COLLECTED);
            }
            
            static float startingTime = 150.0f;
            
            float secondsLeft = clamp(startingTime - m_game->getStateTime(), startingTime, 0);
            
            m_iScoreFromTime = secondsLeft * 1000;
            
            if (m_iScoreFromTime > 0)
            {
                GameTracker::getInstance()->onTimeBonusScoreEarned(m_iScoreFromTime);
            }
            
            updateScore();
            
            if (m_iScore < m_iBestScore)
            {
                m_iScore = m_iBestScore;
            }
            
            m_iOnlineScore = m_iScore;
            
            if (m_iOnlineScore > m_iBestOnlineScore)
            {
                // TODO, show Submit Highscore button
            }
            
            if (FlagUtil::isFlagSet(m_iLevelStatsFlag, FLAG_FIRST_GOLDEN_CARROT_COLLECTED)
                && !FlagUtil::isFlagSet(m_iBestLevelStatsFlag, FLAG_FIRST_GOLDEN_CARROT_COLLECTED))
            {
                m_iNumGoldenCarrots++;
            }
            
            if (FlagUtil::isFlagSet(m_iLevelStatsFlag, FLAG_SECOND_GOLDEN_CARROT_COLLECTED)
                && !FlagUtil::isFlagSet(m_iBestLevelStatsFlag, FLAG_SECOND_GOLDEN_CARROT_COLLECTED))
            {
                m_iNumGoldenCarrots++;
            }
            
            if (FlagUtil::isFlagSet(m_iLevelStatsFlag, FLAG_THIRD_GOLDEN_CARROT_COLLECTED)
                && !FlagUtil::isFlagSet(m_iBestLevelStatsFlag, FLAG_THIRD_GOLDEN_CARROT_COLLECTED))
            {
                m_iNumGoldenCarrots++;
            }
            
            if (FlagUtil::isFlagSet(m_iLevelStatsFlag, FLAG_BONUS_GOLDEN_CARROT_COLLECTED)
                && !FlagUtil::isFlagSet(m_iBestLevelStatsFlag, FLAG_BONUS_GOLDEN_CARROT_COLLECTED))
            {
                m_iNumGoldenCarrots++;
            }
            
            gs->m_iRequestedAction = REQUESTED_ACTION_LEVEL_COMPLETED * 1000;
            gs->m_iRequestedAction += m_game->getWorld() * 100;
            gs->m_iRequestedAction += m_game->getLevel();
  
            m_fStateTime = 0;
			gs->m_renderer->stopCamera();
            m_hasCompletedLevel = true;
            
            ASSETS->addSoundIdToPlayQueue(SOUND_LEVEL_COMPLETE);
        }
        
        m_game->updateBackgrounds(gs->m_renderer->getCameraPosition(), gs->m_fDeltaTime);
    }
}

void Level::render(GameScreen* gs)
{
    Jon& jon = m_game->getJon();
    
    gs->m_renderer->beginFrame(gs->m_fDeltaTime);
    
    gs->m_renderer->renderWorld(*m_game);
    
    if (gs->m_isReleasingShockwave)
    {
        gs->m_renderer->renderToSecondFramebufferWithShockwave(gs->m_fShockwaveCenterX, gs->m_fShockwaveCenterY, gs->m_fShockwaveElapsedTime, jon.isTransformingIntoVampire() || jon.isVampire());
    }
    else
    {
        gs->m_renderer->renderToSecondFramebuffer(*m_game);
    }
    
    gs->m_renderer->renderJonAndExtraForegroundObjects(*m_game);
    
    additionalRenderingBeforeHud(gs);
    
    if (m_hasCompletedLevel)
    {
        gs->m_renderer->renderBlackOverlay(m_fStateTime);
    }
    
    gs->m_renderer->renderBatPanel(*m_batPanel);
    
    if (gs->m_isPaused)
    {
        gs->m_renderer->renderToThirdFramebufferWithObfuscation();
    }
    
    if (m_hasOpeningSequenceCompleted)
    {
        gs->m_renderer->renderHud(*m_game, m_hasCompletedLevel ? nullptr : m_backButton.get(), m_isDisplayingResults ? m_continueButton.get() : nullptr, m_iScore);
    }

#if DEBUG || _DEBUG
	gs->m_renderer->renderDebugInfo(*m_game, gs->m_iFPS);
#endif
    
    if (gs->m_isPaused)
    {
		m_fStateTime = 0;
		m_showDeathTransOut = false;

        gs->m_renderer->renderResumeButtonOverlay();
    }
    
    if (jon.isDead())
    {
        gs->m_renderer->renderToThirdFramebufferWithTransDeathIn(m_fStateTime);
    }
    else if (m_showDeathTransOut)
    {
        gs->m_renderer->renderToThirdFramebufferWithTransDeathOut(m_fStateTime);
    }
    else if (m_activateRadialBlur)
    {
        gs->m_renderer->renderToThirdFramebufferWithRadialBlur();
    }
    
    gs->m_renderer->renderToScreen();
    
    gs->m_renderer->endFrame();
}

void Level::updateCamera(GameScreen* gs, float paddingX, bool ignoreY, bool instant)
{
    gs->m_renderer->updateCameraToFollowJon(*m_game, m_batPanel.get(), gs->m_fDeltaTime, paddingX, false, ignoreY, instant);
}

void Level::additionalRenderingBeforeHud(GameScreen* gs)
{
    // Empty, override in subclass
}

bool Level::isInSlowMotionMode()
{
    return false;
}

void Level::configBatPanel()
{
    if (!FlagUtil::isFlagSet(m_iBestLevelStatsFlag, FLAG_LEVEL_COMPLETE))
    {
        if (m_iNumTimesBatPanelDisplayed < 2)
        {
            m_batPanel->config(m_game.get(), m_game->getWorld(), m_game->getLevel());
        }
        else
        {
            m_batPanel->configWithoutUi(m_game.get(), m_game->getWorld(), m_game->getLevel());
        }
        
        m_iNumTimesBatPanelDisplayed++;
    }
}

bool Level::handleOpeningSequenceTouchInput(GameScreen* gs)
{
    for (std::vector<TouchEvent *>::iterator i = gs->m_touchEvents.begin(); i != gs->m_touchEvents.end(); i++)
    {
        switch ((*i)->getTouchType())
        {
            case DOWN:
                continue;
            case DRAGGED:
                continue;
            case UP:
                return true;
        }
    }
    
    return false;
}

bool Level::handleTouchInput(GameScreen* gs)
{
    Jon& jon = m_game->getJon();
    bool isJonAlive = jon.isAlive();
    
    for (std::vector<TouchEvent *>::iterator i = gs->m_touchEvents.begin(); i != gs->m_touchEvents.end(); i++)
    {
        gs->touchToWorld(*(*i));
        
        switch ((*i)->getTouchType())
        {
            case DOWN:
                if (isJonAlive)
                {
                    gs->m_touchPointDown->set(gs->m_touchPoint->getX(), gs->m_touchPoint->getY());
                    gs->m_isScreenHeldDown = true;
                    gs->m_fScreenHeldTime = 0.0f;
                }
                continue;
            case DRAGGED:
                if (isJonAlive && !gs->m_hasSwiped)
                {
                    if (gs->m_touchPoint->getX() >= (gs->m_touchPointDown->getX() + SWIPE_WIDTH))
                    {
                        // Swipe Right
                        jon.triggerRightAction();
                        gs->m_hasSwiped = true;
                    }
                    else if (gs->m_touchPoint->getX() <= (gs->m_touchPointDown->getX() - SWIPE_WIDTH))
                    {
                        // Swipe Left
                        jon.triggerLeftAction();
                        gs->m_hasSwiped = true;
                    }
                    else if (gs->m_touchPoint->getY() >= (gs->m_touchPointDown->getY() + SWIPE_HEIGHT))
                    {
                        // Swipe Up
                        jon.triggerUpAction();
                        gs->m_hasSwiped = true;
                    }
                    else if (gs->m_touchPoint->getY() <= (gs->m_touchPointDown->getY() - SWIPE_HEIGHT))
                    {
                        // Swipe Down
                        jon.triggerDownAction();
                        gs->m_hasSwiped = true;
                    }
                    
                    if (gs->m_hasSwiped)
                    {
                        gs->m_isScreenHeldDown = false;
                        gs->m_fScreenHeldTime = 0.0f;
                    }
                }
                continue;
            case UP:
                if (!m_hasCompletedLevel
                    && m_backButton->handleClick(*gs->m_touchPoint))
                {
                    m_exitLoop = true;
                    
                    gs->m_renderer->stopCamera();
                    
                    gs->m_stateMachine->revertToPreviousState();
                    
                    return true;
                }
                
                if (isJonAlive)
                {
                    if (!gs->m_hasSwiped && gs->m_fScreenHeldTime < 0.4f)
                    {
                        jon.triggerJump();
                    }
                    
                    if (gs->m_fScreenHeldTime > 0.4f)
                    {
                        jon.triggerCancelTransform();
                    }
                    
                    gs->m_isScreenHeldDown = false;
                    gs->m_fScreenHeldTime = 0;
                    
                    gs->m_hasSwiped = false;
                    
                    gs->m_touchPointDown->set(gs->m_touchPoint->getX(), gs->m_touchPoint->getY());
                }
                break;
        }
    }
    
    return false;
}

void Level::updateScore()
{
    m_iScoreFromObjects = m_game->getNumCarrotsCollected() * SCORE_CARROT;
    m_iScoreFromObjects += m_game->getNumGoldenCarrotsCollected() * SCORE_GOLDEN_CARROT;
    
    if (m_game->getJons().size() > 0)
    {
        m_iScoreFromObjects += m_game->getJon().getNumEnemiesDestroyed() * SCORE_ENEMY;
    }
    
    m_iScore = m_iScoreFromTime + m_iScoreFromObjects;
}

void Level::handleCollections(PhysicalEntity& entity, std::vector<CollectibleItem *>& items, float deltaTime)
{
    for (std::vector<CollectibleItem *>::iterator i = items.begin(); i != items.end(); i++)
    {
        if ((*i)->isCollected())
        {
            continue;
        }
        
        if (OverlapTester::doRectanglesOverlap(entity.getMainBounds(), (*i)->getMainBounds()))
        {
            (*i)->collect();
            
            if ((*i)->getType() == CollectibleItemType_GoldenCarrot)
            {
                GoldenCarrot* gc = reinterpret_cast<GoldenCarrot *>((*i));
                
                m_game->setNumGoldenCarrotsCollected(m_game->getNumGoldenCarrotsCollected() + 1);
                
                GameTracker::getInstance()->onScored(SCORE_GOLDEN_CARROT);
                
                switch (gc->getIndex())
                {
                    case 0:
                        m_iLevelStatsFlag = FlagUtil::setFlag(m_iLevelStatsFlag, FLAG_FIRST_GOLDEN_CARROT_COLLECTED);
                        break;
                    case 1:
                        m_iLevelStatsFlag = FlagUtil::setFlag(m_iLevelStatsFlag, FLAG_SECOND_GOLDEN_CARROT_COLLECTED);
                        break;
                    case 2:
                        m_iLevelStatsFlag = FlagUtil::setFlag(m_iLevelStatsFlag, FLAG_THIRD_GOLDEN_CARROT_COLLECTED);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                m_game->setNumCarrotsCollected(m_game->getNumCarrotsCollected() + 1);
                
                GameTracker::getInstance()->onScored(SCORE_CARROT);
            }
        }
    }
}

Level::Level(const char* json) :
m_sourceGame(nullptr),
m_fStateTime(0.0f),
m_iScoreFromTime(0),
m_iScoreFromObjects(0),
m_iScore(0),
m_iOnlineScore(0),
m_iLevelStatsFlag(0),
m_iNumGoldenCarrots(0),
m_iNumTimesBatPanelDisplayed(0),
m_iNumAttemptsSinceLastAdBreak(0),
m_hasShownOpeningSequence(false),
m_hasOpeningSequenceCompleted(false),
m_activateRadialBlur(false),
m_showDeathTransOut(false),
m_exitLoop(false),
m_hasCompletedLevel(false),
m_isDisplayingResults(false),
m_iBestScore(0),
m_iBestOnlineScore(0),
m_iBestLevelStatsFlag(0),
m_iLastKnownNumGoldenCarrots(0),
m_iLastKnownJonAbilityFlag(0),
m_playLevelSelectMusicOnExit(false),
m_stopMusicOnExit(false),
m_hasStoppedAllLoopingSoundsAfterJonDeath(false)
{
    m_json = json;
    m_game = std::unique_ptr<Game>(new Game());
    m_batPanel = std::unique_ptr<BatPanel>(new BatPanel());
    m_backButton = std::unique_ptr<GameButton>(GameButton::create(GameButtonType_BackToLevelSelect));
    m_continueButton = std::unique_ptr<GameButton>(GameButton::create(GameButtonType_ContinueToLevelSelect));
}

RTTI_IMPL(Level, GameScreenState);
RTTI_IMPL(Chapter1Level1, Level);
RTTI_IMPL(Chapter1Level2, Level);
RTTI_IMPL(Chapter1Level3, Level);
RTTI_IMPL(Chapter1Level4, Level);
RTTI_IMPL(Chapter1Level5, Level);
RTTI_IMPL(Chapter1Level6, Level);
RTTI_IMPL(Chapter1Level7, Level);
RTTI_IMPL(Chapter1Level8, Level);
RTTI_IMPL(Chapter1Level9, Level);
RTTI_IMPL(Chapter1Level11, Level);
RTTI_IMPL(Chapter1Level12, Level);
RTTI_IMPL(Chapter1Level13, Level);
RTTI_IMPL(Chapter1Level14, Level);
RTTI_IMPL(Chapter1Level15, Level);
RTTI_IMPL(Chapter1Level16, Level);
RTTI_IMPL(Chapter1Level17, Level);
RTTI_IMPL(Chapter1Level18, Level);
RTTI_IMPL(Chapter1Level19, Level);
RTTI_IMPL(Chapter1Level20, Level);
RTTI_IMPL_NOPARENT(LevelUtil);

#include <sstream>

/// Chapter 1 Level 1 ///

Chapter1Level1 * Chapter1Level1::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":1,\"midgrounds\":[{\"gridX\":0,\"gridY\":96,\"type\":0},{\"gridX\":32,\"gridY\":96,\"type\":1},{\"gridX\":72,\"gridY\":96,\"type\":2},{\"gridX\":196,\"gridY\":96,\"type\":0},{\"gridX\":228,\"gridY\":96,\"type\":1},{\"gridX\":272,\"gridY\":96,\"type\":2},{\"gridX\":368,\"gridY\":96,\"type\":2},{\"gridX\":568,\"gridY\":96,\"type\":0},{\"gridX\":640,\"gridY\":96,\"type\":2},{\"gridX\":688,\"gridY\":96,\"type\":0},{\"gridX\":760,\"gridY\":96,\"type\":1},{\"gridX\":800,\"gridY\":96,\"type\":2},{\"gridX\":912,\"gridY\":96,\"type\":2},{\"gridX\":964,\"gridY\":96,\"type\":1},{\"gridX\":1076,\"gridY\":96,\"type\":0},{\"gridX\":1124,\"gridY\":96,\"type\":2},{\"gridX\":1196,\"gridY\":96,\"type\":2},{\"gridX\":1232,\"gridY\":96,\"type\":0},{\"gridX\":1328,\"gridY\":96,\"type\":0},{\"gridX\":1384,\"gridY\":96,\"type\":2},{\"gridX\":1464,\"gridY\":96,\"type\":1},{\"gridX\":1516,\"gridY\":96,\"type\":2},{\"gridX\":1624,\"gridY\":96,\"type\":0},{\"gridX\":1672,\"gridY\":96,\"type\":1},{\"gridX\":1712,\"gridY\":96,\"type\":2},{\"gridX\":1772,\"gridY\":96,\"type\":0},{\"gridX\":1804,\"gridY\":96,\"type\":1},{\"gridX\":1844,\"gridY\":96,\"type\":1},{\"gridX\":1884,\"gridY\":96,\"type\":2},{\"gridX\":1020,\"gridY\":96,\"type\":1},{\"gridX\":390,\"gridY\":96,\"type\":11},{\"gridX\":89,\"gridY\":96,\"type\":6},{\"gridX\":565,\"gridY\":96,\"type\":11},{\"gridX\":1563,\"gridY\":96,\"type\":11},{\"gridX\":1540,\"gridY\":96,\"type\":6},{\"gridX\":1219,\"gridY\":96,\"type\":6}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":136,\"gridY\":0,\"type\":28},{\"gridX\":264,\"gridY\":0,\"type\":28},{\"gridX\":392,\"gridY\":0,\"type\":28},{\"gridX\":520,\"gridY\":0,\"type\":28},{\"gridX\":648,\"gridY\":0,\"type\":28},{\"gridX\":776,\"gridY\":0,\"type\":28},{\"gridX\":1544,\"gridY\":0,\"type\":28},{\"gridX\":1672,\"gridY\":0,\"type\":28},{\"gridX\":1800,\"gridY\":0,\"type\":28},{\"gridX\":904,\"gridY\":0,\"type\":28},{\"gridX\":1032,\"gridY\":0,\"type\":28},{\"gridX\":1160,\"gridY\":0,\"type\":28},{\"gridX\":1288,\"gridY\":0,\"type\":28},{\"gridX\":1416,\"gridY\":0,\"type\":28},{\"gridX\":1928,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":132,\"gridY\":0,\"type\":31},{\"gridX\":512,\"gridY\":0,\"type\":30},{\"gridX\":780,\"gridY\":0,\"type\":30},{\"gridX\":836,\"gridY\":0,\"type\":31},{\"gridX\":360,\"gridY\":0,\"type\":30},{\"gridX\":1024,\"gridY\":0,\"type\":31},{\"gridX\":1112,\"gridY\":0,\"type\":31},{\"gridX\":1372,\"gridY\":0,\"type\":32},{\"gridX\":1324,\"gridY\":0,\"type\":30},{\"gridX\":926,\"gridY\":0,\"type\":30}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1664,\"gridY\":96,\"type\":12},{\"gridX\":554,\"gridY\":96,\"type\":14},{\"gridX\":549,\"gridY\":96,\"type\":13},{\"gridX\":565,\"gridY\":96,\"type\":16},{\"gridX\":329,\"gridY\":108,\"type\":50},{\"gridX\":314,\"gridY\":96,\"type\":51},{\"gridX\":344,\"gridY\":96,\"type\":51},{\"gridX\":392,\"gridY\":96,\"type\":51},{\"gridX\":408,\"gridY\":108,\"type\":50},{\"gridX\":424,\"gridY\":96,\"type\":51},{\"gridX\":656,\"gridY\":96,\"type\":51},{\"gridX\":672,\"gridY\":96,\"type\":51},{\"gridX\":688,\"gridY\":96,\"type\":52},{\"gridX\":688,\"gridY\":112,\"type\":51},{\"gridX\":704,\"gridY\":96,\"type\":52},{\"gridX\":704,\"gridY\":112,\"type\":51},{\"gridX\":720,\"gridY\":112,\"type\":51},{\"gridX\":736,\"gridY\":112,\"type\":51},{\"gridX\":752,\"gridY\":112,\"type\":51},{\"gridX\":720,\"gridY\":96,\"type\":52},{\"gridX\":736,\"gridY\":96,\"type\":52},{\"gridX\":816,\"gridY\":128,\"type\":51},{\"gridX\":816,\"gridY\":112,\"type\":52},{\"gridX\":816,\"gridY\":96,\"type\":52},{\"gridX\":640,\"gridY\":96,\"type\":51},{\"gridX\":752,\"gridY\":96,\"type\":52},{\"gridX\":832,\"gridY\":140,\"type\":50},{\"gridX\":904,\"gridY\":96,\"type\":52},{\"gridX\":904,\"gridY\":112,\"type\":52},{\"gridX\":904,\"gridY\":128,\"type\":51},{\"gridX\":920,\"gridY\":140,\"type\":50},{\"gridX\":800,\"gridY\":140,\"type\":50},{\"gridX\":960,\"gridY\":96,\"type\":51},{\"gridX\":976,\"gridY\":96,\"type\":51},{\"gridX\":992,\"gridY\":96,\"type\":51},{\"gridX\":1008,\"gridY\":108,\"type\":50},{\"gridX\":1300,\"gridY\":96,\"type\":52},{\"gridX\":1300,\"gridY\":112,\"type\":51},{\"gridX\":1316,\"gridY\":124,\"type\":50},{\"gridX\":1484,\"gridY\":108,\"type\":50},{\"gridX\":1468,\"gridY\":108,\"type\":50},{\"gridX\":1452,\"gridY\":108,\"type\":50},{\"gridX\":1500,\"gridY\":96,\"type\":51},{\"gridX\":1601,\"gridY\":96,\"type\":8}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1632,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":1092,\"gridY\":96,\"type\":6},{\"gridX\":1356,\"gridY\":96,\"type\":6}],\"collectibles\":[{\"gridX\":148,\"gridY\":132,\"type\":0},{\"gridX\":140,\"gridY\":116,\"type\":0},{\"gridX\":144,\"gridY\":124,\"type\":0},{\"gridX\":164,\"gridY\":136,\"type\":1},{\"gridX\":188,\"gridY\":116,\"type\":0},{\"gridX\":180,\"gridY\":132,\"type\":0},{\"gridX\":224,\"gridY\":104,\"type\":0},{\"gridX\":184,\"gridY\":124,\"type\":0},{\"gridX\":232,\"gridY\":104,\"type\":0},{\"gridX\":240,\"gridY\":104,\"type\":0},{\"gridX\":248,\"gridY\":104,\"type\":0},{\"gridX\":256,\"gridY\":104,\"type\":0},{\"gridX\":264,\"gridY\":104,\"type\":0},{\"gridX\":348,\"gridY\":117,\"type\":0},{\"gridX\":367,\"gridY\":133,\"type\":0},{\"gridX\":272,\"gridY\":104,\"type\":0},{\"gridX\":440,\"gridY\":116,\"type\":0},{\"gridX\":392,\"gridY\":136,\"type\":0},{\"gridX\":384,\"gridY\":136,\"type\":0},{\"gridX\":432,\"gridY\":116,\"type\":0},{\"gridX\":424,\"gridY\":116,\"type\":0},{\"gridX\":416,\"gridY\":116,\"type\":0},{\"gridX\":512,\"gridY\":104,\"type\":0},{\"gridX\":516,\"gridY\":112,\"type\":0},{\"gridX\":520,\"gridY\":120,\"type\":0},{\"gridX\":524,\"gridY\":128,\"type\":0},{\"gridX\":560,\"gridY\":128,\"type\":0},{\"gridX\":564,\"gridY\":120,\"type\":0},{\"gridX\":568,\"gridY\":112,\"type\":0},{\"gridX\":572,\"gridY\":104,\"type\":0},{\"gridX\":580,\"gridY\":104,\"type\":0},{\"gridX\":828,\"gridY\":148,\"type\":0},{\"gridX\":596,\"gridY\":112,\"type\":0},{\"gridX\":588,\"gridY\":104,\"type\":0},{\"gridX\":768,\"gridY\":140,\"type\":0},{\"gridX\":820,\"gridY\":148,\"type\":0},{\"gridX\":836,\"gridY\":148,\"type\":0},{\"gridX\":980,\"gridY\":116,\"type\":0},{\"gridX\":728,\"gridY\":132,\"type\":0},{\"gridX\":712,\"gridY\":132,\"type\":0},{\"gridX\":736,\"gridY\":132,\"type\":0},{\"gridX\":704,\"gridY\":132,\"type\":0},{\"gridX\":744,\"gridY\":132,\"type\":0},{\"gridX\":604,\"gridY\":116,\"type\":0},{\"gridX\":720,\"gridY\":132,\"type\":0},{\"gridX\":1020,\"gridY\":120,\"type\":0},{\"gridX\":852,\"gridY\":164,\"type\":0},{\"gridX\":844,\"gridY\":148,\"type\":0},{\"gridX\":848,\"gridY\":156,\"type\":0},{\"gridX\":752,\"gridY\":132,\"type\":0},{\"gridX\":760,\"gridY\":132,\"type\":0},{\"gridX\":776,\"gridY\":144,\"type\":0},{\"gridX\":868,\"gridY\":176,\"type\":0},{\"gridX\":1440,\"gridY\":156,\"type\":0},{\"gridX\":1392,\"gridY\":172,\"type\":0},{\"gridX\":876,\"gridY\":176,\"type\":0},{\"gridX\":1108,\"gridY\":132,\"type\":0},{\"gridX\":996,\"gridY\":116,\"type\":0},{\"gridX\":1012,\"gridY\":116,\"type\":0},{\"gridX\":988,\"gridY\":116,\"type\":0},{\"gridX\":1004,\"gridY\":116,\"type\":0},{\"gridX\":1128,\"gridY\":172,\"type\":0},{\"gridX\":860,\"gridY\":172,\"type\":0},{\"gridX\":1124,\"gridY\":164,\"type\":0},{\"gridX\":1120,\"gridY\":156,\"type\":0},{\"gridX\":1132,\"gridY\":180,\"type\":0},{\"gridX\":1112,\"gridY\":140,\"type\":0},{\"gridX\":1116,\"gridY\":148,\"type\":0},{\"gridX\":1140,\"gridY\":180,\"type\":1},{\"gridX\":528,\"gridY\":136,\"type\":0},{\"gridX\":556,\"gridY\":136,\"type\":0},{\"gridX\":1272,\"gridY\":120,\"type\":0},{\"gridX\":1268,\"gridY\":112,\"type\":0},{\"gridX\":1240,\"gridY\":104,\"type\":0},{\"gridX\":1248,\"gridY\":104,\"type\":0},{\"gridX\":1256,\"gridY\":104,\"type\":0},{\"gridX\":1264,\"gridY\":104,\"type\":0},{\"gridX\":1308,\"gridY\":132,\"type\":0},{\"gridX\":1316,\"gridY\":1" << "32,\"type\":0},{\"gridX\":1324,\"gridY\":132,\"type\":0},{\"gridX\":1332,\"gridY\":132,\"type\":0},{\"gridX\":1384,\"gridY\":156,\"type\":0},{\"gridX\":1388,\"gridY\":164,\"type\":0},{\"gridX\":1372,\"gridY\":132,\"type\":0},{\"gridX\":1376,\"gridY\":140,\"type\":0},{\"gridX\":1380,\"gridY\":148,\"type\":0},{\"gridX\":1444,\"gridY\":148,\"type\":0},{\"gridX\":1448,\"gridY\":140,\"type\":0},{\"gridX\":1452,\"gridY\":132,\"type\":0},{\"gridX\":1456,\"gridY\":124,\"type\":0},{\"gridX\":1460,\"gridY\":116,\"type\":0},{\"gridX\":1572,\"gridY\":116,\"type\":0},{\"gridX\":1576,\"gridY\":124,\"type\":0},{\"gridX\":1580,\"gridY\":132,\"type\":0},{\"gridX\":1612,\"gridY\":132,\"type\":0},{\"gridX\":1616,\"gridY\":124,\"type\":0},{\"gridX\":1620,\"gridY\":116,\"type\":0},{\"gridX\":1596,\"gridY\":136,\"type\":1},{\"gridX\":356,\"gridY\":126,\"type\":0},{\"gridX\":376,\"gridY\":136,\"type\":0},{\"gridX\":1400,\"gridY\":176,\"type\":0},{\"gridX\":1360,\"gridY\":112,\"type\":0},{\"gridX\":1104,\"gridY\":124,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":96}],\"extraForegroundObjects\":[],\"foregroundCoverObjects\":[{\"gridX\":309,\"gridY\":92,\"type\":3},{\"gridX\":1213,\"gridY\":92,\"type\":3},{\"gridX\":414,\"gridY\":92,\"type\":1},{\"gridX\":209,\"gridY\":85,\"type\":0},{\"gridX\":41,\"gridY\":85,\"type\":0},{\"gridX\":68,\"gridY\":92,\"type\":1}],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level1 *instance = new Chapter1Level1(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 2 ///

Chapter1Level2 * Chapter1Level2::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":2,\"midgrounds\":[{\"gridX\":12,\"gridY\":96,\"type\":2},{\"gridX\":80,\"gridY\":96,\"type\":1},{\"gridX\":208,\"gridY\":96,\"type\":2},{\"gridX\":160,\"gridY\":96,\"type\":0},{\"gridX\":456,\"gridY\":96,\"type\":1},{\"gridX\":508,\"gridY\":96,\"type\":0},{\"gridX\":708,\"gridY\":96,\"type\":0},{\"gridX\":616,\"gridY\":96,\"type\":2},{\"gridX\":736,\"gridY\":96,\"type\":1},{\"gridX\":800,\"gridY\":96,\"type\":0},{\"gridX\":984,\"gridY\":96,\"type\":2},{\"gridX\":952,\"gridY\":96,\"type\":0},{\"gridX\":1092,\"gridY\":96,\"type\":2},{\"gridX\":1044,\"gridY\":96,\"type\":0},{\"gridX\":1160,\"gridY\":96,\"type\":1},{\"gridX\":1232,\"gridY\":96,\"type\":0},{\"gridX\":1328,\"gridY\":96,\"type\":0},{\"gridX\":1404,\"gridY\":96,\"type\":0},{\"gridX\":1468,\"gridY\":96,\"type\":1},{\"gridX\":1532,\"gridY\":96,\"type\":2},{\"gridX\":1592,\"gridY\":96,\"type\":1},{\"gridX\":1672,\"gridY\":96,\"type\":2},{\"gridX\":1728,\"gridY\":96,\"type\":0},{\"gridX\":1760,\"gridY\":96,\"type\":2},{\"gridX\":1816,\"gridY\":96,\"type\":1},{\"gridX\":1856,\"gridY\":96,\"type\":2},{\"gridX\":1636,\"gridY\":96,\"type\":0},{\"gridX\":357,\"gridY\":96,\"type\":6},{\"gridX\":313,\"gridY\":96,\"type\":2},{\"gridX\":320,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":1640,\"gridY\":0,\"type\":26},{\"gridX\":1288,\"gridY\":0,\"type\":26},{\"gridX\":1576,\"gridY\":0,\"type\":27},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":392,\"gridY\":0,\"type\":28},{\"gridX\":648,\"gridY\":0,\"type\":28},{\"gridX\":776,\"gridY\":0,\"type\":28},{\"gridX\":1448,\"gridY\":0,\"type\":28},{\"gridX\":1672,\"gridY\":0,\"type\":28},{\"gridX\":1800,\"gridY\":0,\"type\":28},{\"gridX\":904,\"gridY\":0,\"type\":28},{\"gridX\":1032,\"gridY\":0,\"type\":28},{\"gridX\":1160,\"gridY\":0,\"type\":28},{\"gridX\":1320,\"gridY\":0,\"type\":28},{\"gridX\":520,\"gridY\":0,\"type\":28},{\"gridX\":264,\"gridY\":0,\"type\":28},{\"gridX\":136,\"gridY\":0,\"type\":28},{\"gridX\":1928,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":388,\"gridY\":0,\"type\":30},{\"gridX\":552,\"gridY\":0,\"type\":30},{\"gridX\":264,\"gridY\":0,\"type\":30},{\"gridX\":1364,\"gridY\":0,\"type\":30},{\"gridX\":1078,\"gridY\":0,\"type\":31},{\"gridX\":1232,\"gridY\":0,\"type\":30},{\"gridX\":643,\"gridY\":0,\"type\":30}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1664,\"gridY\":96,\"type\":12},{\"gridX\":896,\"gridY\":96,\"type\":13},{\"gridX\":900,\"gridY\":96,\"type\":15},{\"gridX\":922,\"gridY\":96,\"type\":16},{\"gridX\":1146,\"gridY\":97,\"type\":22},{\"gridX\":156,\"gridY\":96,\"type\":51},{\"gridX\":172,\"gridY\":96,\"type\":51},{\"gridX\":298,\"gridY\":112,\"type\":51},{\"gridX\":298,\"gridY\":96,\"type\":52},{\"gridX\":421,\"gridY\":96,\"type\":52},{\"gridX\":421,\"gridY\":111,\"type\":51},{\"gridX\":594,\"gridY\":96,\"type\":52},{\"gridX\":594,\"gridY\":112,\"type\":51},{\"gridX\":677,\"gridY\":96,\"type\":52},{\"gridX\":677,\"gridY\":112,\"type\":51},{\"gridX\":693,\"gridY\":96,\"type\":52},{\"gridX\":693,\"gridY\":112,\"type\":51},{\"gridX\":877,\"gridY\":96,\"type\":52},{\"gridX\":877,\"gridY\":112,\"type\":51},{\"gridX\":861,\"gridY\":124,\"type\":50},{\"gridX\":936,\"gridY\":96,\"type\":52},{\"gridX\":936,\"gridY\":112,\"type\":52},{\"gridX\":936,\"gridY\":128,\"type\":51},{\"gridX\":952,\"gridY\":140,\"type\":50},{\"gridX\":968,\"gridY\":140,\"type\":50},{\"gridX\":610,\"gridY\":124,\"type\":50}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1632,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":212,\"gridY\":96,\"type\":8},{\"gridX\":468,\"gridY\":96,\"type\":8},{\"gridX\":1060,\"gridY\":96,\"type\":6},{\"gridX\":1296,\"gridY\":96,\"type\":8},{\"gridX\":1516,\"gridY\":96,\"type\":6},{\"gridX\":752,\"gridY\":96,\"type\":8},{\"gridX\":1032,\"gridY\":96,\"type\":8}],\"collectibles\":[{\"gridX\":384,\"gridY\":104,\"type\":0},{\"gridX\":388,\"gridY\":112,\"type\":0},{\"gridX\":392,\"gridY\":120,\"type\":0},{\"gridX\":396,\"gridY\":128,\"type\":0},{\"gridX\":424,\"gridY\":128,\"type\":0},{\"gridX\":416,\"gridY\":128,\"type\":0},{\"gridX\":432,\"gridY\":128,\"type\":0},{\"gridX\":540,\"gridY\":104,\"type\":0},{\"gridX\":532,\"gridY\":104,\"type\":0},{\"gridX\":548,\"gridY\":104,\"type\":0},{\"gridX\":552,\"gridY\":112,\"type\":0},{\"gridX\":556,\"gridY\":120,\"type\":0},{\"gridX\":560,\"gridY\":128,\"type\":0},{\"gridX\":564,\"gridY\":136,\"type\":0},{\"gridX\":592,\"gridY\":128,\"type\":0},{\"gridX\":600,\"gridY\":128,\"type\":0},{\"gridX\":608,\"gridY\":128,\"type\":0},{\"gridX\":688,\"gridY\":128,\"type\":0},{\"gridX\":696,\"gridY\":128,\"type\":0},{\"gridX\":704,\"gridY\":128,\"type\":0},{\"gridX\":400,\"gridY\":136,\"type\":1},{\"gridX\":260,\"gridY\":104,\"type\":0},{\"gridX\":264,\"gridY\":112,\"type\":0},{\"gridX\":268,\"gridY\":120,\"type\":0},{\"gridX\":272,\"gridY\":128,\"type\":0},{\"gridX\":276,\"gridY\":136,\"type\":0},{\"gridX\":336,\"gridY\":104,\"type\":0},{\"gridX\":344,\"gridY\":104,\"type\":0},{\"gridX\":352,\"gridY\":104,\"type\":0},{\"gridX\":764,\"gridY\":104,\"type\":0},{\"gridX\":780,\"gridY\":104,\"type\":0},{\"gridX\":772,\"gridY\":104,\"type\":0},{\"gridX\":128,\"gridY\":111,\"type\":0},{\"gridX\":123,\"gridY\":103,\"type\":0},{\"gridX\":132,\"gridY\":119,\"type\":0},{\"gridX\":139,\"gridY\":126,\"type\":0},{\"gridX\":149,\"gridY\":133,\"type\":0},{\"gridX\":868,\"gridY\":128,\"type\":0},{\"gridX\":860,\"gridY\":128,\"type\":0},{\"gridX\":876,\"gridY\":128,\"type\":0},{\"gridX\":884,\"gridY\":128,\"type\":0},{\"gridX\":976,\"gridY\":152,\"type\":0},{\"gridX\":952,\"gridY\":152,\"type\":0},{\"gridX\":960,\"gridY\":152,\"type\":0},{\"gridX\":968,\"gridY\":152,\"type\":0},{\"gridX\":1112,\"gridY\":176,\"type\":1},{\"gridX\":216,\"gridY\":112,\"type\":0},{\"gridX\":204,\"gridY\":104,\"type\":0},{\"gridX\":228,\"gridY\":104,\"type\":0},{\"gridX\":464,\"gridY\":104,\"type\":0},{\"gridX\":476,\"gridY\":112,\"type\":0},{\"gridX\":488,\"gridY\":104,\"type\":0},{\"gridX\":1098,\"gridY\":173,\"type\":0},{\"gridX\":1072,\"gridY\":132,\"type\":0},{\"gridX\":1077,\"gridY\":140,\"type\":0},{\"gridX\":1081,\"gridY\":148,\"type\":0},{\"gridX\":1086,\"gridY\":156,\"type\":0},{\"gridX\":1092,\"gridY\":165,\"type\":0},{\"gridX\":1368,\"gridY\":128,\"type\":0},{\"gridX\":1252,\"gridY\":128,\"type\":0},{\"gridX\":1244,\"gridY\":128,\"type\":0},{\"gridX\":1236,\"gridY\":128,\"type\":0},{\"gridX\":1372,\"gridY\":136,\"type\":0},{\"gridX\":1268,\"gridY\":124,\"type\":0},{\"gridX\":1276,\"gridY\":116,\"type\":0},{\"gridX\":1284,\"gridY\":108,\"type\":0},{\"gridX\":1126,\"gridY\":172,\"type\":0},{\"gridX\":1131,\"gridY\":163,\"type\":0},{\"gridX\":1136,\"gridY\":156,\"type\":0},{\"gridX\":1140,\"gridY\":148,\"type\":0},{\"gridX\":1144,\"gridY\":139,\"type\":0},{\"gridX\":1148,\"gridY\":129,\"type\":0},{\"gridX\":1580,\"gridY\":168,\"type\":1},{\"gridX\":1308,\"gridY\":108,\"type\":0},{\"gridX\":1300,\"gridY\":112,\"type\":0},{\"gridX\":1316,\"gridY\":104,\"type\":0},{\"gridX\":1356,\"gridY\":104,\"type\":0},{\"gridX\":1364,\"gridY\":120,\"type\":0},{\"gridX\":1452,\"gridY\":104,\"type\":0},{\"gridX\":824,\"gridY\":104,\"type\":0},{\"gridX\":828,\"gridY\":112,\"type\":0},{\"gridX\":832,\"gridY\":120,\"type\":0},{\"gridX\":888,\"gridY\":136,\"type\":0},{\"gridX\":892,\"gridY\":144,\"type\":0},{\"gridX\":1444,\"gridY\":104,\"type\":0},{\"gridX\":1460,\"gridY\":104,\"type\":0},{\"gridX\":1436,\"gridY\":104,\"type\":0},{\"gridX\":1464,\"gridY\":112,\"type\":0},{\"gridX\":1468,\"gridY\":120,\"type\":0},{\"gridX\":1472,\"gridY\":128,\"type\":0},{\"gridX\":1476,\"gridY\":136,\"type\":0},{\"gridX\":1544,\"gridY\":160,\"type\":0},{\"gridX\":1528,\"gridY\":128,\"type\":0},{\"gridX\":1532,\"gridY\":136,\"type\":0},{\"gridX\":1536,\"gridY\":144,\"type\":0},{\"gridX\":1540,\"gridY\":152,\"type\":0},{\"gridX\":680,\"gridY\":128,\"type\":0},{\"gridX\":944,\"gridY\":152,\"type\":0},{\"gridX\":836,\"grid" << "Y\":128,\"type\":0},{\"gridX\":984,\"gridY\":152,\"type\":0},{\"gridX\":840,\"gridY\":136,\"type\":0},{\"gridX\":1360,\"gridY\":112,\"type\":0},{\"gridX\":1428,\"gridY\":104,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":96}],\"extraForegroundObjects\":[],\"foregroundCoverObjects\":[{\"gridX\":297,\"gridY\":92,\"type\":2},{\"gridX\":344,\"gridY\":92,\"type\":1},{\"gridX\":329,\"gridY\":92,\"type\":3},{\"gridX\":357,\"gridY\":85,\"type\":0},{\"gridX\":830,\"gridY\":92,\"type\":2},{\"gridX\":713,\"gridY\":92,\"type\":3},{\"gridX\":768,\"gridY\":92,\"type\":1},{\"gridX\":791,\"gridY\":85,\"type\":0}],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level2 *instance = new Chapter1Level2(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 3 ///

Chapter1Level3 * Chapter1Level3::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":3,\"midgrounds\":[{\"gridX\":0,\"gridY\":96,\"type\":2},{\"gridX\":56,\"gridY\":96,\"type\":0},{\"gridX\":92,\"gridY\":96,\"type\":1},{\"gridX\":256,\"gridY\":96,\"type\":2},{\"gridX\":372,\"gridY\":96,\"type\":1},{\"gridX\":316,\"gridY\":96,\"type\":0},{\"gridX\":412,\"gridY\":96,\"type\":2},{\"gridX\":468,\"gridY\":96,\"type\":1},{\"gridX\":520,\"gridY\":96,\"type\":0},{\"gridX\":580,\"gridY\":96,\"type\":2},{\"gridX\":648,\"gridY\":96,\"type\":1},{\"gridX\":700,\"gridY\":96,\"type\":0},{\"gridX\":760,\"gridY\":96,\"type\":1},{\"gridX\":796,\"gridY\":96,\"type\":0},{\"gridX\":864,\"gridY\":96,\"type\":0},{\"gridX\":940,\"gridY\":96,\"type\":2},{\"gridX\":896,\"gridY\":96,\"type\":0},{\"gridX\":1028,\"gridY\":96,\"type\":1},{\"gridX\":1064,\"gridY\":96,\"type\":0},{\"gridX\":1104,\"gridY\":96,\"type\":0},{\"gridX\":1140,\"gridY\":96,\"type\":0},{\"gridX\":1208,\"gridY\":96,\"type\":1},{\"gridX\":1260,\"gridY\":96,\"type\":2},{\"gridX\":1324,\"gridY\":96,\"type\":2},{\"gridX\":1384,\"gridY\":96,\"type\":0},{\"gridX\":1416,\"gridY\":96,\"type\":1},{\"gridX\":1496,\"gridY\":96,\"type\":1},{\"gridX\":1668,\"gridY\":96,\"type\":2},{\"gridX\":1752,\"gridY\":96,\"type\":1},{\"gridX\":1792,\"gridY\":96,\"type\":0},{\"gridX\":1824,\"gridY\":96,\"type\":2},{\"gridX\":1884,\"gridY\":96,\"type\":2},{\"gridX\":1944,\"gridY\":96,\"type\":2},{\"gridX\":2012,\"gridY\":96,\"type\":0},{\"gridX\":2056,\"gridY\":96,\"type\":2},{\"gridX\":2116,\"gridY\":96,\"type\":2},{\"gridX\":2176,\"gridY\":96,\"type\":1},{\"gridX\":2220,\"gridY\":96,\"type\":0},{\"gridX\":1460,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":136,\"gridY\":0,\"type\":28},{\"gridX\":264,\"gridY\":0,\"type\":28},{\"gridX\":392,\"gridY\":0,\"type\":28},{\"gridX\":520,\"gridY\":0,\"type\":28},{\"gridX\":648,\"gridY\":0,\"type\":28},{\"gridX\":1288,\"gridY\":0,\"type\":28},{\"gridX\":1416,\"gridY\":0,\"type\":28},{\"gridX\":1544,\"gridY\":0,\"type\":28},{\"gridX\":1672,\"gridY\":0,\"type\":28},{\"gridX\":1800,\"gridY\":0,\"type\":28},{\"gridX\":1928,\"gridY\":0,\"type\":28},{\"gridX\":2056,\"gridY\":0,\"type\":28},{\"gridX\":2184,\"gridY\":0,\"type\":28},{\"gridX\":776,\"gridY\":0,\"type\":28},{\"gridX\":904,\"gridY\":0,\"type\":28},{\"gridX\":1032,\"gridY\":0,\"type\":28},{\"gridX\":1160,\"gridY\":0,\"type\":28},{\"gridX\":2312,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":136,\"gridY\":0,\"type\":32},{\"gridX\":1544,\"gridY\":0,\"type\":32}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":2056,\"gridY\":96,\"type\":12},{\"gridX\":340,\"gridY\":132,\"type\":0},{\"gridX\":344,\"gridY\":132,\"type\":1},{\"gridX\":372,\"gridY\":132,\"type\":2},{\"gridX\":358,\"gridY\":132,\"type\":1},{\"gridX\":504,\"gridY\":132,\"type\":0},{\"gridX\":508,\"gridY\":132,\"type\":1},{\"gridX\":522,\"gridY\":132,\"type\":1},{\"gridX\":550,\"gridY\":132,\"type\":2},{\"gridX\":432,\"gridY\":172,\"type\":0},{\"gridX\":436,\"gridY\":172,\"type\":1},{\"gridX\":450,\"gridY\":172,\"type\":1},{\"gridX\":464,\"gridY\":172,\"type\":2},{\"gridX\":728,\"gridY\":116,\"type\":0},{\"gridX\":732,\"gridY\":116,\"type\":1},{\"gridX\":746,\"gridY\":116,\"type\":1},{\"gridX\":760,\"gridY\":116,\"type\":2},{\"gridX\":828,\"gridY\":116,\"type\":0},{\"gridX\":832,\"gridY\":116,\"type\":1},{\"gridX\":860,\"gridY\":116,\"type\":2},{\"gridX\":846,\"gridY\":116,\"type\":1},{\"gridX\":916,\"gridY\":96,\"type\":10},{\"gridX\":891,\"gridY\":96,\"type\":13},{\"gridX\":896,\"gridY\":96,\"type\":14},{\"gridX\":907,\"gridY\":96,\"type\":16},{\"gridX\":1208,\"gridY\":96,\"type\":11},{\"gridX\":1328,\"gridY\":96,\"type\":8},{\"gridX\":1376,\"gridY\":96,\"type\":8},{\"gridX\":1620,\"gridY\":48,\"type\":3},{\"gridX\":1638,\"gridY\":48,\"type\":4},{\"gridX\":1652,\"gridY\":48,\"type\":4},{\"gridX\":1666,\"gridY\":48,\"type\":5},{\"gridX\":1624,\"gridY\":48,\"type\":4},{\"gridX\":1716,\"gridY\":116,\"type\":0},{\"gridX\":1720,\"gridY\":116,\"type\":1},{\"gridX\":1734,\"gridY\":116,\"type\":1},{\"gridX\":1748,\"gridY\":116,\"type\":2},{\"gridX\":1772,\"gridY\":140,\"type\":0},{\"gridX\":1776,\"gridY\":140,\"type\":1},{\"gridX\":1790,\"gridY\":140,\"type\":2},{\"gridX\":1816,\"gridY\":116,\"type\":0},{\"gridX\":1820,\"gridY\":116,\"type\":1},{\"gridX\":1834,\"gridY\":116,\"type\":2},{\"gridX\":1656,\"gridY\":53,\"type\":41},{\"gridX\":536,\"gridY\":132,\"type\":1},{\"gridX\":792,\"gridY\":148,\"type\":34},{\"gridX\":792,\"gridY\":164,\"type\":34}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":2024,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":400,\"gridY\":96,\"type\":8},{\"gridX\":488,\"gridY\":96,\"type\":8},{\"gridX\":592,\"gridY\":96,\"type\":8},{\"gridX\":800,\"gridY\":96,\"type\":8},{\"gridX\":1129,\"gridY\":96,\"type\":9},{\"gridX\":1168,\"gridY\":96,\"type\":9},{\"gridX\":1744,\"gridY\":121,\"type\":8},{\"gridX\":1812,\"gridY\":121,\"type\":9}],\"collectibles\":[{\"gridX\":144,\"gridY\":112,\"type\":0},{\"gridX\":148,\"gridY\":120,\"type\":0},{\"gridX\":152,\"gridY\":128,\"type\":0},{\"gridX\":252,\"gridY\":120,\"type\":0},{\"gridX\":212,\"gridY\":120,\"type\":0},{\"gridX\":232,\"gridY\":132,\"type\":0},{\"gridX\":472,\"gridY\":228,\"type\":1},{\"gridX\":344,\"gridY\":144,\"type\":0},{\"gridX\":352,\"gridY\":144,\"type\":0},{\"gridX\":360,\"gridY\":144,\"type\":0},{\"gridX\":640,\"gridY\":124,\"type\":0},{\"gridX\":536,\"gridY\":144,\"type\":0},{\"gridX\":544,\"gridY\":144,\"type\":0},{\"gridX\":560,\"gridY\":136,\"type\":0},{\"gridX\":568,\"gridY\":128,\"type\":0},{\"gridX\":576,\"gridY\":120,\"type\":0},{\"gridX\":632,\"gridY\":116,\"type\":0},{\"gridX\":648,\"gridY\":128,\"type\":0},{\"gridX\":664,\"gridY\":116,\"type\":0},{\"gridX\":584,\"gridY\":112,\"type\":0},{\"gridX\":656,\"gridY\":124,\"type\":0},{\"gridX\":452,\"gridY\":184,\"type\":0},{\"gridX\":436,\"gridY\":184,\"type\":0},{\"gridX\":444,\"gridY\":184,\"type\":0},{\"gridX\":368,\"gridY\":144,\"type\":0},{\"gridX\":456,\"gridY\":192,\"type\":0},{\"gridX\":552,\"gridY\":144,\"type\":0},{\"gridX\":220,\"gridY\":128,\"type\":0},{\"gridX\":244,\"gridY\":128,\"type\":0},{\"gridX\":688,\"gridY\":104,\"type\":0},{\"gridX\":696,\"gridY\":104,\"type\":0},{\"gridX\":704,\"gridY\":104,\"type\":0},{\"gridX\":732,\"gridY\":128,\"type\":0},{\"gridX\":748,\"gridY\":128,\"type\":0},{\"gridX\":740,\"gridY\":128,\"type\":0},{\"gridX\":756,\"gridY\":128,\"type\":0},{\"gridX\":832,\"gridY\":128,\"type\":0},{\"gridX\":840,\"gridY\":128,\"type\":0},{\"gridX\":848,\"gridY\":128,\"type\":0},{\"gridX\":856,\"gridY\":128,\"type\":0},{\"gridX\":932,\"gridY\":132,\"type\":0},{\"gridX\":936,\"gridY\":140,\"type\":0},{\"gridX\":940,\"gridY\":148,\"type\":0},{\"gridX\":944,\"gridY\":156,\"type\":0},{\"gridX\":980,\"gridY\":172,\"type\":1},{\"gridX\":460,\"gridY\":200,\"type\":0},{\"gridX\":1024,\"gridY\":104,\"type\":0},{\"gridX\":1032,\"gridY\":104,\"type\":0},{\"gridX\":1040,\"gridY\":104,\"type\":0},{\"gridX\":1100,\"gridY\":120,\"type\":0},{\"gridX\":1104,\"gridY\":112,\"type\":0},{\"gridX\":1108,\"gridY\":104,\"type\":0},{\"gridX\":1120,\"gridY\":104,\"type\":0},{\"gridX\":1128,\"gridY\":112,\"type\":0},{\"gridX\":1136,\"gridY\":116,\"type\":0},{\"gridX\":1144,\"gridY\":112,\"type\":0},{\"gridX\":1152,\"gridY\":104,\"type\":0},{\"gridX\":1096,\"gridY\":128,\"type\":0},{\"gridX\":1172,\"gridY\":120,\"type\":0},{\"gridX\":1168,\"gridY\":112,\"type\":0},{\"gridX\":1164,\"gridY\":104,\"type\":0},{\"gridX\":1180,\"gridY\":124,\"type\":0},{\"gridX\":1188,\"gridY\":128,\"type\":0},{\"gridX\":1256,\"gridY\":104,\"type\":0},{\"gridX\":1264,\"gridY\":104,\"type\":0},{\"gridX\":1272,\"gridY\":104,\"type\":0},{\"gridX\":1336,\"gridY\":112,\"type\":0},{\"gridX\":1328,\"gridY\":112,\"type\":0},{\"gridX\":1344,\"gridY\":104,\"type\":0},{\"gridX\":1352,\"gridY\":104,\"type\":0},{\"gridX\":1360,\"gridY\":104,\"type\":0},{\"gridX\":1368,\"gridY\":112,\"typ" << "e\":0},{\"gridX\":1376,\"gridY\":112,\"type\":0},{\"gridX\":1444,\"gridY\":104,\"type\":0},{\"gridX\":1460,\"gridY\":104,\"type\":0},{\"gridX\":1452,\"gridY\":104,\"type\":0},{\"gridX\":1540,\"gridY\":104,\"type\":0},{\"gridX\":1524,\"gridY\":104,\"type\":0},{\"gridX\":1532,\"gridY\":104,\"type\":0},{\"gridX\":1712,\"gridY\":104,\"type\":0},{\"gridX\":1624,\"gridY\":92,\"type\":0},{\"gridX\":1618,\"gridY\":92,\"type\":0},{\"gridX\":1616,\"gridY\":100,\"type\":0},{\"gridX\":1612,\"gridY\":104,\"type\":0},{\"gridX\":1624,\"gridY\":104,\"type\":0},{\"gridX\":1660,\"gridY\":84,\"type\":1},{\"gridX\":1624,\"gridY\":98,\"type\":0},{\"gridX\":1612,\"gridY\":92,\"type\":0},{\"gridX\":1720,\"gridY\":104,\"type\":0},{\"gridX\":1728,\"gridY\":104,\"type\":0},{\"gridX\":1608,\"gridY\":108,\"type\":0},{\"gridX\":1816,\"gridY\":124,\"type\":0},{\"gridX\":1824,\"gridY\":124,\"type\":0},{\"gridX\":1832,\"gridY\":124,\"type\":0},{\"gridX\":1988,\"gridY\":104,\"type\":0},{\"gridX\":1996,\"gridY\":104,\"type\":0},{\"gridX\":1980,\"gridY\":104,\"type\":0},{\"gridX\":1752,\"gridY\":108,\"type\":0},{\"gridX\":1760,\"gridY\":108,\"type\":0},{\"gridX\":1768,\"gridY\":104,\"type\":0},{\"gridX\":1916,\"gridY\":104,\"type\":0},{\"gridX\":1924,\"gridY\":104,\"type\":0},{\"gridX\":1932,\"gridY\":104,\"type\":0}],\"jons\":[{\"gridX\":28,\"gridY\":100}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level3 *instance = new Chapter1Level3(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 4 ///

Chapter1Level4 * Chapter1Level4::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":4,\"midgrounds\":[{\"gridX\":276,\"gridY\":96,\"type\":2},{\"gridX\":360,\"gridY\":96,\"type\":1},{\"gridX\":428,\"gridY\":96,\"type\":0},{\"gridX\":736,\"gridY\":96,\"type\":1},{\"gridX\":704,\"gridY\":96,\"type\":0},{\"gridX\":537,\"gridY\":96,\"type\":2},{\"gridX\":1220,\"gridY\":96,\"type\":1},{\"gridX\":1264,\"gridY\":96,\"type\":0},{\"gridX\":1644,\"gridY\":96,\"type\":2},{\"gridX\":1444,\"gridY\":96,\"type\":1},{\"gridX\":0,\"gridY\":96,\"type\":2},{\"gridX\":64,\"gridY\":96,\"type\":0},{\"gridX\":100,\"gridY\":96,\"type\":0},{\"gridX\":2048,\"gridY\":96,\"type\":2},{\"gridX\":2108,\"gridY\":96,\"type\":1},{\"gridX\":2152,\"gridY\":96,\"type\":0},{\"gridX\":2188,\"gridY\":96,\"type\":2},{\"gridX\":1956,\"gridY\":96,\"type\":0},{\"gridX\":464,\"gridY\":96,\"type\":1},{\"gridX\":656,\"gridY\":96,\"type\":0},{\"gridX\":776,\"gridY\":96,\"type\":0},{\"gridX\":836,\"gridY\":96,\"type\":1},{\"gridX\":920,\"gridY\":96,\"type\":2},{\"gridX\":1084,\"gridY\":96,\"type\":2},{\"gridX\":1512,\"gridY\":96,\"type\":0},{\"gridX\":1572,\"gridY\":96,\"type\":0},{\"gridX\":2012,\"gridY\":96,\"type\":0},{\"gridX\":1000,\"gridY\":96,\"type\":1},{\"gridX\":1140,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":8,\"gridY\":0,\"type\":27},{\"gridX\":72,\"gridY\":0,\"type\":28},{\"gridX\":200,\"gridY\":0,\"type\":28},{\"gridX\":328,\"gridY\":0,\"type\":28},{\"gridX\":456,\"gridY\":0,\"type\":28},{\"gridX\":584,\"gridY\":0,\"type\":28},{\"gridX\":712,\"gridY\":0,\"type\":28},{\"gridX\":840,\"gridY\":0,\"type\":28},{\"gridX\":968,\"gridY\":0,\"type\":28},{\"gridX\":1096,\"gridY\":0,\"type\":28},{\"gridX\":1224,\"gridY\":0,\"type\":28},{\"gridX\":1352,\"gridY\":0,\"type\":28},{\"gridX\":1480,\"gridY\":0,\"type\":28},{\"gridX\":1608,\"gridY\":0,\"type\":28},{\"gridX\":1736,\"gridY\":0,\"type\":28},{\"gridX\":1864,\"gridY\":0,\"type\":28},{\"gridX\":1992,\"gridY\":0,\"type\":28},{\"gridX\":2120,\"gridY\":0,\"type\":28},{\"gridX\":2248,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":1064,\"gridY\":0,\"type\":30},{\"gridX\":1168,\"gridY\":0,\"type\":31},{\"gridX\":1316,\"gridY\":0,\"type\":32},{\"gridX\":1704,\"gridY\":0,\"type\":33},{\"gridX\":132,\"gridY\":0,\"type\":32},{\"gridX\":720,\"gridY\":0,\"type\":32}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":2016,\"gridY\":96,\"type\":12},{\"gridX\":336,\"gridY\":96,\"type\":10},{\"gridX\":416,\"gridY\":96,\"type\":8},{\"gridX\":504,\"gridY\":96,\"type\":7},{\"gridX\":603,\"gridY\":112,\"type\":1},{\"gridX\":617,\"gridY\":112,\"type\":1},{\"gridX\":631,\"gridY\":112,\"type\":1},{\"gridX\":645,\"gridY\":112,\"type\":2},{\"gridX\":599,\"gridY\":112,\"type\":0},{\"gridX\":592,\"gridY\":96,\"type\":24},{\"gridX\":624,\"gridY\":96,\"type\":24},{\"gridX\":656,\"gridY\":96,\"type\":24},{\"gridX\":689,\"gridY\":96,\"type\":24},{\"gridX\":851,\"gridY\":96,\"type\":24},{\"gridX\":883,\"gridY\":96,\"type\":24},{\"gridX\":689,\"gridY\":135,\"type\":1},{\"gridX\":703,\"gridY\":135,\"type\":2},{\"gridX\":685,\"gridY\":135,\"type\":0},{\"gridX\":812,\"gridY\":128,\"type\":1},{\"gridX\":826,\"gridY\":128,\"type\":2},{\"gridX\":808,\"gridY\":128,\"type\":0},{\"gridX\":882,\"gridY\":112,\"type\":1},{\"gridX\":878,\"gridY\":112,\"type\":0},{\"gridX\":910,\"gridY\":112,\"type\":2},{\"gridX\":896,\"gridY\":112,\"type\":1},{\"gridX\":1492,\"gridY\":96,\"type\":24},{\"gridX\":1560,\"gridY\":96,\"type\":23},{\"gridX\":1624,\"gridY\":96,\"type\":22},{\"gridX\":1740,\"gridY\":132,\"type\":1},{\"gridX\":1736,\"gridY\":132,\"type\":0},{\"gridX\":1754,\"gridY\":132,\"type\":2},{\"gridX\":1804,\"gridY\":160,\"type\":1},{\"gridX\":1800,\"gridY\":160,\"type\":0},{\"gridX\":1818,\"gridY\":160,\"type\":1},{\"gridX\":1832,\"gridY\":160,\"type\":1},{\"gridX\":1846,\"gridY\":160,\"type\":1},{\"gridX\":1860,\"gridY\":160,\"type\":1},{\"gridX\":1874,\"gridY\":160,\"type\":1},{\"gridX\":1888,\"gridY\":160,\"type\":1},{\"gridX\":1902,\"gridY\":160,\"type\":2},{\"gridX\":560,\"gridY\":96,\"type\":24},{\"gridX\":528,\"gridY\":96,\"type\":24},{\"gridX\":915,\"gridY\":96,\"type\":24},{\"gridX\":947,\"gridY\":96,\"type\":24},{\"gridX\":978,\"gridY\":96,\"type\":23},{\"gridX\":483,\"gridY\":96,\"type\":13},{\"gridX\":488,\"gridY\":96,\"type\":14},{\"gridX\":499,\"gridY\":96,\"type\":16},{\"gridX\":160,\"gridY\":116,\"type\":1},{\"gridX\":156,\"gridY\":116,\"type\":0},{\"gridX\":174,\"gridY\":116,\"type\":2},{\"gridX\":188,\"gridY\":96,\"type\":0},{\"gridX\":192,\"gridY\":96,\"type\":1},{\"gridX\":206,\"gridY\":96,\"type\":2},{\"gridX\":1852,\"gridY\":140,\"type\":34},{\"gridX\":1852,\"gridY\":124,\"type\":34},{\"gridX\":1852,\"gridY\":108,\"type\":34},{\"gridX\":1192,\"gridY\":116,\"type\":0},{\"gridX\":1196,\"gridY\":116,\"type\":1},{\"gridX\":1210,\"gridY\":116,\"type\":2},{\"gridX\":1356,\"gridY\":108,\"type\":0},{\"gridX\":1360,\"gridY\":108,\"type\":1},{\"gridX\":1374,\"gridY\":108,\"type\":1},{\"gridX\":1388,\"gridY\":108,\"type\":2},{\"gridX\":1544,\"gridY\":116,\"type\":0},{\"gridX\":1548,\"gridY\":116,\"type\":1},{\"gridX\":1562,\"gridY\":116,\"type\":2}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1984,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":1784,\"gridY\":148,\"type\":3}],\"collectibles\":[{\"gridX\":136,\"gridY\":108,\"type\":0},{\"gridX\":144,\"gridY\":116,\"type\":0},{\"gridX\":152,\"gridY\":124,\"type\":0},{\"gridX\":240,\"gridY\":124,\"type\":0},{\"gridX\":248,\"gridY\":116,\"type\":0},{\"gridX\":256,\"gridY\":108,\"type\":0},{\"gridX\":312,\"gridY\":104,\"type\":0},{\"gridX\":320,\"gridY\":112,\"type\":0},{\"gridX\":328,\"gridY\":120,\"type\":0},{\"gridX\":336,\"gridY\":128,\"type\":0},{\"gridX\":344,\"gridY\":128,\"type\":0},{\"gridX\":352,\"gridY\":128,\"type\":0},{\"gridX\":416,\"gridY\":136,\"type\":0},{\"gridX\":408,\"gridY\":128,\"type\":0},{\"gridX\":424,\"gridY\":136,\"type\":0},{\"gridX\":400,\"gridY\":120,\"type\":0},{\"gridX\":416,\"gridY\":128,\"type\":0},{\"gridX\":588,\"gridY\":140,\"type\":0},{\"gridX\":572,\"gridY\":156,\"type\":0},{\"gridX\":532,\"gridY\":140,\"type\":0},{\"gridX\":560,\"gridY\":160,\"type\":1},{\"gridX\":408,\"gridY\":120,\"type\":0},{\"gridX\":540,\"gridY\":148,\"type\":0},{\"gridX\":580,\"gridY\":148,\"type\":0},{\"gridX\":624,\"gridY\":124,\"type\":0},{\"gridX\":632,\"gridY\":124,\"type\":0},{\"gridX\":640,\"gridY\":124,\"type\":0},{\"gridX\":648,\"gridY\":124,\"type\":0},{\"gridX\":616,\"gridY\":124,\"type\":0},{\"gridX\":688,\"gridY\":148,\"type\":0},{\"gridX\":696,\"gridY\":148,\"type\":0},{\"gridX\":704,\"gridY\":148,\"type\":0},{\"gridX\":824,\"gridY\":140,\"type\":0},{\"gridX\":828,\"gridY\":148,\"type\":0},{\"gridX\":816,\"gridY\":140,\"type\":0},{\"gridX\":912,\"gridY\":132,\"type\":0},{\"gridX\":900,\"gridY\":124,\"type\":0},{\"gridX\":908,\"gridY\":124,\"type\":0},{\"gridX\":748,\"gridY\":164,\"type\":0},{\"gridX\":740,\"gridY\":168,\"type\":0},{\"gridX\":756,\"gridY\":160,\"type\":0},{\"gridX\":1076,\"gridY\":88,\"type\":1},{\"gridX\":1064,\"gridY\":104,\"type\":0},{\"gridX\":1032,\"gridY\":104,\"type\":0},{\"gridX\":1040,\"gridY\":104,\"type\":0},{\"gridX\":1048,\"gridY\":104,\"type\":0},{\"gridX\":1056,\"gridY\":104,\"type\":0},{\"gridX\":1120,\"gridY\":104,\"type\":0},{\"gridX\":1128,\"gridY\":104,\"type\":0},{\"gridX\":1136,\"gridY\":104,\"type\":0},{\"gridX\":1164,\"gridY\":104,\"type\":0},{\"gridX\":1168,\"gridY\":112,\"type\":0},{\"gridX\":1172,\"gridY\":120,\"type\":0},{\"gridX\":1176,\"gridY\":128,\"type\":0},{\"gridX\":1180,\"gridY\":136,\"type\":0},{\"gridX\":1384,\"gridY\":128,\"type\":0},{\"gridX\":1392,\"gridY\":132,\"type\":0},{\"gridX\":1400,\"gridY\":136,\"type\":0},{\"gridX\":1248,\"gridY\":104,\"type\":0},{\"gridX\":1256,\"gridY\":104,\"type\":0},{\"gridX\":1264,\"gridY\":104,\"type\":0},{\"gridX\":1304,\"gridY\":104,\"type\":0},{\"gridX\":1288,\"gridY\":104,\"type\":0},{\"gridX\":1296,\"gridY\":104,\"type\":0},{\"gridX\":1536,\"grid" << "Y\":104,\"type\":0},{\"gridX\":1532,\"gridY\":108,\"type\":0},{\"gridX\":1592,\"gridY\":108,\"type\":0},{\"gridX\":1596,\"gridY\":104,\"type\":0},{\"gridX\":1500,\"gridY\":124,\"type\":0},{\"gridX\":1508,\"gridY\":124,\"type\":0},{\"gridX\":1504,\"gridY\":128,\"type\":0},{\"gridX\":1560,\"gridY\":124,\"type\":0},{\"gridX\":1568,\"gridY\":124,\"type\":0},{\"gridX\":1564,\"gridY\":128,\"type\":0},{\"gridX\":1620,\"gridY\":124,\"type\":0},{\"gridX\":1636,\"gridY\":132,\"type\":0},{\"gridX\":1628,\"gridY\":128,\"type\":0},{\"gridX\":1748,\"gridY\":144,\"type\":0},{\"gridX\":1756,\"gridY\":144,\"type\":0},{\"gridX\":1812,\"gridY\":172,\"type\":0},{\"gridX\":1820,\"gridY\":172,\"type\":0},{\"gridX\":1828,\"gridY\":172,\"type\":0},{\"gridX\":1992,\"gridY\":128,\"type\":0},{\"gridX\":1844,\"gridY\":172,\"type\":0},{\"gridX\":1852,\"gridY\":172,\"type\":0},{\"gridX\":1860,\"gridY\":172,\"type\":0},{\"gridX\":2000,\"gridY\":124,\"type\":0},{\"gridX\":1760,\"gridY\":152,\"type\":0},{\"gridX\":1876,\"gridY\":172,\"type\":0},{\"gridX\":1884,\"gridY\":172,\"type\":0},{\"gridX\":1892,\"gridY\":172,\"type\":0},{\"gridX\":1952,\"gridY\":124,\"type\":0},{\"gridX\":1984,\"gridY\":132,\"type\":0},{\"gridX\":1912,\"gridY\":164,\"type\":0},{\"gridX\":1920,\"gridY\":156,\"type\":0},{\"gridX\":1928,\"gridY\":148,\"type\":0},{\"gridX\":1936,\"gridY\":140,\"type\":0},{\"gridX\":1944,\"gridY\":132,\"type\":0},{\"gridX\":1976,\"gridY\":136,\"type\":1},{\"gridX\":548,\"gridY\":156,\"type\":0},{\"gridX\":524,\"gridY\":132,\"type\":0},{\"gridX\":1540,\"gridY\":108,\"type\":0},{\"gridX\":1600,\"gridY\":108,\"type\":0}],\"jons\":[{\"gridX\":28,\"gridY\":100}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level4 *instance = new Chapter1Level4(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 5 ///

Chapter1Level5 * Chapter1Level5::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":5,\"midgrounds\":[{\"gridX\":0,\"gridY\":96,\"type\":2},{\"gridX\":60,\"gridY\":96,\"type\":0},{\"gridX\":92,\"gridY\":96,\"type\":2},{\"gridX\":212,\"gridY\":96,\"type\":0},{\"gridX\":152,\"gridY\":96,\"type\":2},{\"gridX\":244,\"gridY\":96,\"type\":1},{\"gridX\":540,\"gridY\":96,\"type\":2},{\"gridX\":600,\"gridY\":96,\"type\":1},{\"gridX\":672,\"gridY\":96,\"type\":0},{\"gridX\":736,\"gridY\":96,\"type\":2},{\"gridX\":832,\"gridY\":96,\"type\":0},{\"gridX\":872,\"gridY\":96,\"type\":1},{\"gridX\":916,\"gridY\":96,\"type\":2},{\"gridX\":1196,\"gridY\":96,\"type\":2},{\"gridX\":1252,\"gridY\":96,\"type\":1},{\"gridX\":1300,\"gridY\":96,\"type\":2},{\"gridX\":1376,\"gridY\":96,\"type\":0},{\"gridX\":1428,\"gridY\":96,\"type\":0},{\"gridX\":1488,\"gridY\":96,\"type\":2},{\"gridX\":1584,\"gridY\":96,\"type\":1},{\"gridX\":1548,\"gridY\":96,\"type\":0},{\"gridX\":1960,\"gridY\":96,\"type\":2},{\"gridX\":1872,\"gridY\":96,\"type\":0},{\"gridX\":1904,\"gridY\":96,\"type\":1},{\"gridX\":2020,\"gridY\":96,\"type\":1},{\"gridX\":2064,\"gridY\":96,\"type\":2},{\"gridX\":2124,\"gridY\":96,\"type\":0},{\"gridX\":2160,\"gridY\":96,\"type\":0},{\"gridX\":640,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":648,\"gridY\":0,\"type\":26},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":136,\"gridY\":0,\"type\":28},{\"gridX\":264,\"gridY\":0,\"type\":28},{\"gridX\":392,\"gridY\":0,\"type\":28},{\"gridX\":680,\"gridY\":0,\"type\":28},{\"gridX\":808,\"gridY\":0,\"type\":28},{\"gridX\":1832,\"gridY\":0,\"type\":28},{\"gridX\":1960,\"gridY\":0,\"type\":28},{\"gridX\":2088,\"gridY\":0,\"type\":28},{\"gridX\":936,\"gridY\":0,\"type\":28},{\"gridX\":1064,\"gridY\":0,\"type\":28},{\"gridX\":1192,\"gridY\":0,\"type\":28},{\"gridX\":1320,\"gridY\":0,\"type\":28},{\"gridX\":1448,\"gridY\":0,\"type\":28},{\"gridX\":520,\"gridY\":0,\"type\":28},{\"gridX\":1576,\"gridY\":0,\"type\":28},{\"gridX\":1704,\"gridY\":0,\"type\":28},{\"gridX\":2216,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":292,\"gridY\":0,\"type\":33},{\"gridX\":954,\"gridY\":0,\"type\":33},{\"gridX\":1616,\"gridY\":0,\"type\":33},{\"gridX\":1405,\"gridY\":0,\"type\":30}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1952,\"gridY\":96,\"type\":12},{\"gridX\":128,\"gridY\":96,\"type\":41},{\"gridX\":198,\"gridY\":150,\"type\":0},{\"gridX\":258,\"gridY\":150,\"type\":2},{\"gridX\":202,\"gridY\":150,\"type\":1},{\"gridX\":216,\"gridY\":150,\"type\":1},{\"gridX\":230,\"gridY\":150,\"type\":1},{\"gridX\":700,\"gridY\":116,\"type\":0},{\"gridX\":704,\"gridY\":116,\"type\":1},{\"gridX\":718,\"gridY\":116,\"type\":1},{\"gridX\":732,\"gridY\":116,\"type\":2},{\"gridX\":244,\"gridY\":150,\"type\":1},{\"gridX\":735,\"gridY\":96,\"type\":16},{\"gridX\":713,\"gridY\":96,\"type\":15},{\"gridX\":143,\"gridY\":96,\"type\":13},{\"gridX\":148,\"gridY\":96,\"type\":15},{\"gridX\":209,\"gridY\":96,\"type\":15},{\"gridX\":170,\"gridY\":96,\"type\":16},{\"gridX\":184,\"gridY\":96,\"type\":20},{\"gridX\":204,\"gridY\":96,\"type\":13},{\"gridX\":231,\"gridY\":96,\"type\":15},{\"gridX\":253,\"gridY\":96,\"type\":15},{\"gridX\":275,\"gridY\":96,\"type\":16},{\"gridX\":540,\"gridY\":224,\"type\":34},{\"gridX\":540,\"gridY\":208,\"type\":34},{\"gridX\":540,\"gridY\":240,\"type\":34},{\"gridX\":540,\"gridY\":192,\"type\":34},{\"gridX\":540,\"gridY\":176,\"type\":34},{\"gridX\":808,\"gridY\":116,\"type\":0},{\"gridX\":812,\"gridY\":116,\"type\":1},{\"gridX\":826,\"gridY\":116,\"type\":2},{\"gridX\":840,\"gridY\":140,\"type\":0},{\"gridX\":844,\"gridY\":140,\"type\":1},{\"gridX\":872,\"gridY\":140,\"type\":2},{\"gridX\":896,\"gridY\":164,\"type\":0},{\"gridX\":900,\"gridY\":164,\"type\":1},{\"gridX\":914,\"gridY\":164,\"type\":1},{\"gridX\":956,\"gridY\":164,\"type\":2},{\"gridX\":928,\"gridY\":164,\"type\":1},{\"gridX\":772,\"gridY\":96,\"type\":19},{\"gridX\":858,\"gridY\":140,\"type\":1},{\"gridX\":866,\"gridY\":96,\"type\":13},{\"gridX\":871,\"gridY\":96,\"type\":15},{\"gridX\":893,\"gridY\":96,\"type\":15},{\"gridX\":915,\"gridY\":96,\"type\":15},{\"gridX\":937,\"gridY\":96,\"type\":14},{\"gridX\":948,\"gridY\":96,\"type\":16},{\"gridX\":1360,\"gridY\":116,\"type\":0},{\"gridX\":1364,\"gridY\":116,\"type\":1},{\"gridX\":1378,\"gridY\":116,\"type\":2},{\"gridX\":1456,\"gridY\":116,\"type\":0},{\"gridX\":1460,\"gridY\":116,\"type\":1},{\"gridX\":1474,\"gridY\":116,\"type\":2},{\"gridX\":1360,\"gridY\":96,\"type\":13},{\"gridX\":1365,\"gridY\":96,\"type\":14},{\"gridX\":1376,\"gridY\":96,\"type\":16},{\"gridX\":276,\"gridY\":112,\"type\":0},{\"gridX\":280,\"gridY\":112,\"type\":1},{\"gridX\":294,\"gridY\":112,\"type\":2},{\"gridX\":756,\"gridY\":144,\"type\":0},{\"gridX\":760,\"gridY\":144,\"type\":1},{\"gridX\":774,\"gridY\":144,\"type\":2},{\"gridX\":144,\"gridY\":112,\"type\":0},{\"gridX\":148,\"gridY\":112,\"type\":1},{\"gridX\":162,\"gridY\":112,\"type\":2},{\"gridX\":808,\"gridY\":172,\"type\":0},{\"gridX\":812,\"gridY\":172,\"type\":1},{\"gridX\":826,\"gridY\":172,\"type\":2},{\"gridX\":1528,\"gridY\":96,\"type\":13},{\"gridX\":1533,\"gridY\":96,\"type\":15},{\"gridX\":1555,\"gridY\":96,\"type\":16},{\"gridX\":1568,\"gridY\":96,\"type\":19},{\"gridX\":1620,\"gridY\":152,\"type\":0},{\"gridX\":1624,\"gridY\":152,\"type\":1},{\"gridX\":1638,\"gridY\":152,\"type\":1},{\"gridX\":1652,\"gridY\":152,\"type\":2},{\"gridX\":1868,\"gridY\":138,\"type\":34},{\"gridX\":1868,\"gridY\":154,\"type\":34},{\"gridX\":1868,\"gridY\":170,\"type\":34},{\"gridX\":1868,\"gridY\":186,\"type\":34},{\"gridX\":1868,\"gridY\":202,\"type\":34},{\"gridX\":1868,\"gridY\":218,\"type\":34},{\"gridX\":1702,\"gridY\":88,\"type\":3},{\"gridX\":1706,\"gridY\":88,\"type\":4},{\"gridX\":1720,\"gridY\":88,\"type\":4},{\"gridX\":1734,\"gridY\":88,\"type\":4},{\"gridX\":1762,\"gridY\":88,\"type\":5},{\"gridX\":1748,\"gridY\":88,\"type\":4},{\"gridX\":942,\"gridY\":164,\"type\":1},{\"gridX\":708,\"gridY\":96,\"type\":13}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1920,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":252,\"gridY\":172,\"type\":3},{\"gridX\":848,\"gridY\":96,\"type\":8},{\"gridX\":940,\"gridY\":188,\"type\":3},{\"gridX\":1272,\"gridY\":96,\"type\":9},{\"gridX\":328,\"gridY\":96,\"type\":3},{\"gridX\":376,\"gridY\":96,\"type\":3},{\"gridX\":424,\"gridY\":96,\"type\":3},{\"gridX\":1608,\"gridY\":96,\"type\":8},{\"gridX\":1940,\"gridY\":96,\"type\":4},{\"gridX\":472,\"gridY\":96,\"type\":3}],\"collectibles\":[{\"gridX\":704,\"gridY\":128,\"type\":0},{\"gridX\":1484,\"gridY\":124,\"type\":0},{\"gridX\":1492,\"gridY\":116,\"type\":0},{\"gridX\":1500,\"gridY\":108,\"type\":0},{\"gridX\":712,\"gridY\":128,\"type\":0},{\"gridX\":720,\"gridY\":128,\"type\":0},{\"gridX\":284,\"gridY\":216,\"type\":1},{\"gridX\":728,\"gridY\":128,\"type\":0},{\"gridX\":756,\"gridY\":116,\"type\":0},{\"gridX\":764,\"gridY\":112,\"type\":0},{\"gridX\":772,\"gridY\":108,\"type\":0},{\"gridX\":808,\"gridY\":128,\"type\":0},{\"gridX\":816,\"gridY\":128,\"type\":0},{\"gridX\":824,\"gridY\":128,\"type\":0},{\"gridX\":864,\"gridY\":152,\"type\":0},{\"gridX\":848,\"gridY\":152,\"type\":0},{\"gridX\":856,\"gridY\":152,\"type\":0},{\"gridX\":896,\"gridY\":176,\"type\":0},{\"gridX\":904,\"gridY\":176,\"type\":0},{\"gridX\":912,\"gridY\":176,\"type\":0},{\"gridX\":928,\"gridY\":176,\"type\":0},{\"gridX\":936,\"gridY\":176,\"type\":0},{\"gridX\":944,\"gridY\":176,\"type\":0},{\"gridX\":1212,\"gridY\":104,\"type\":1},{\"gridX\":960,\"gridY\":184,\"type\":0},{\"gridX\":968,\"gridY\":184,\"type\":0},{\"gridX\":1000,\"gridY\":180,\"type\":0},{\"gridX\":1040,\"gridY\":172,\"type\":0},{\"gridX\":1048,\"gridY\":172,\"type\":0},{\"gridX\":1088,\"gridY\":164,\"type\":0},{\"gridX\":1080,\"gridY\":164,\"type\":0},{\"gridX\":1120,\"gridY\":156,\"type\":0},{\"gridX\":1128,\"gridY\":156,\"type\":0},{\"gridX\":1168,\"gridY\":148,\"type\":0},{\"gridX\":1160,\"gridY\":148," << "\"type\":0},{\"gridX\":612,\"gridY\":104,\"type\":0},{\"gridX\":596,\"gridY\":104,\"type\":0},{\"gridX\":604,\"gridY\":104,\"type\":0},{\"gridX\":668,\"gridY\":112,\"type\":0},{\"gridX\":672,\"gridY\":120,\"type\":0},{\"gridX\":676,\"gridY\":128,\"type\":0},{\"gridX\":136,\"gridY\":112,\"type\":0},{\"gridX\":140,\"gridY\":120,\"type\":0},{\"gridX\":144,\"gridY\":128,\"type\":0},{\"gridX\":200,\"gridY\":164,\"type\":0},{\"gridX\":208,\"gridY\":164,\"type\":0},{\"gridX\":1360,\"gridY\":128,\"type\":0},{\"gridX\":1368,\"gridY\":128,\"type\":0},{\"gridX\":1376,\"gridY\":128,\"type\":0},{\"gridX\":1424,\"gridY\":116,\"type\":0},{\"gridX\":1408,\"gridY\":116,\"type\":0},{\"gridX\":1456,\"gridY\":128,\"type\":0},{\"gridX\":1464,\"gridY\":128,\"type\":0},{\"gridX\":1472,\"gridY\":128,\"type\":0},{\"gridX\":188,\"gridY\":124,\"type\":0},{\"gridX\":192,\"gridY\":132,\"type\":0},{\"gridX\":196,\"gridY\":140,\"type\":0},{\"gridX\":1524,\"gridY\":112,\"type\":0},{\"gridX\":1532,\"gridY\":116,\"type\":0},{\"gridX\":1540,\"gridY\":120,\"type\":0},{\"gridX\":1548,\"gridY\":116,\"type\":0},{\"gridX\":1556,\"gridY\":112,\"type\":0},{\"gridX\":1400,\"gridY\":120,\"type\":0},{\"gridX\":1432,\"gridY\":120,\"type\":0},{\"gridX\":1572,\"gridY\":120,\"type\":0},{\"gridX\":1576,\"gridY\":128,\"type\":0},{\"gridX\":1580,\"gridY\":136,\"type\":0},{\"gridX\":1584,\"gridY\":144,\"type\":0},{\"gridX\":1588,\"gridY\":152,\"type\":0},{\"gridX\":1292,\"gridY\":104,\"type\":0},{\"gridX\":1276,\"gridY\":112,\"type\":0},{\"gridX\":1284,\"gridY\":108,\"type\":0},{\"gridX\":1008,\"gridY\":180,\"type\":0},{\"gridX\":976,\"gridY\":184,\"type\":0},{\"gridX\":1016,\"gridY\":180,\"type\":0},{\"gridX\":1056,\"gridY\":172,\"type\":0},{\"gridX\":1096,\"gridY\":164,\"type\":0},{\"gridX\":1136,\"gridY\":156,\"type\":0},{\"gridX\":1176,\"gridY\":148,\"type\":0},{\"gridX\":1592,\"gridY\":160,\"type\":0},{\"gridX\":1596,\"gridY\":168,\"type\":0},{\"gridX\":1676,\"gridY\":144,\"type\":0},{\"gridX\":1668,\"gridY\":152,\"type\":0},{\"gridX\":1660,\"gridY\":160,\"type\":0},{\"gridX\":1624,\"gridY\":164,\"type\":0},{\"gridX\":1632,\"gridY\":164,\"type\":0},{\"gridX\":1640,\"gridY\":164,\"type\":0},{\"gridX\":1648,\"gridY\":164,\"type\":0},{\"gridX\":1716,\"gridY\":100,\"type\":0},{\"gridX\":1724,\"gridY\":108,\"type\":0},{\"gridX\":1732,\"gridY\":116,\"type\":0},{\"gridX\":1756,\"gridY\":116,\"type\":0},{\"gridX\":1772,\"gridY\":116,\"type\":0},{\"gridX\":1816,\"gridY\":112,\"type\":0},{\"gridX\":1800,\"gridY\":112,\"type\":0},{\"gridX\":1852,\"gridY\":108,\"type\":0},{\"gridX\":1808,\"gridY\":112,\"type\":0},{\"gridX\":1844,\"gridY\":108,\"type\":0},{\"gridX\":1764,\"gridY\":116,\"type\":0},{\"gridX\":1860,\"gridY\":108,\"type\":0},{\"gridX\":1708,\"gridY\":108,\"type\":0},{\"gridX\":1416,\"gridY\":112,\"type\":1},{\"gridX\":1700,\"gridY\":116,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":96}],\"extraForegroundObjects\":[],\"markers\":[{\"gridX\":2228,\"gridY\":0,\"type\":1}]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level5 *instance = new Chapter1Level5(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 6 ///

Chapter1Level6 * Chapter1Level6::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":6,\"midgrounds\":[{\"gridX\":0,\"gridY\":96,\"type\":2},{\"gridX\":60,\"gridY\":96,\"type\":0},{\"gridX\":92,\"gridY\":96,\"type\":2},{\"gridX\":212,\"gridY\":96,\"type\":0},{\"gridX\":152,\"gridY\":96,\"type\":2},{\"gridX\":244,\"gridY\":96,\"type\":1},{\"gridX\":540,\"gridY\":96,\"type\":2},{\"gridX\":600,\"gridY\":96,\"type\":1},{\"gridX\":672,\"gridY\":96,\"type\":0},{\"gridX\":736,\"gridY\":96,\"type\":2},{\"gridX\":832,\"gridY\":96,\"type\":0},{\"gridX\":872,\"gridY\":96,\"type\":1},{\"gridX\":916,\"gridY\":96,\"type\":2},{\"gridX\":1196,\"gridY\":96,\"type\":2},{\"gridX\":1252,\"gridY\":96,\"type\":1},{\"gridX\":1300,\"gridY\":96,\"type\":2},{\"gridX\":1376,\"gridY\":96,\"type\":0},{\"gridX\":1428,\"gridY\":96,\"type\":0},{\"gridX\":1488,\"gridY\":96,\"type\":2},{\"gridX\":1584,\"gridY\":96,\"type\":1},{\"gridX\":1548,\"gridY\":96,\"type\":0},{\"gridX\":1960,\"gridY\":96,\"type\":2},{\"gridX\":1872,\"gridY\":96,\"type\":0},{\"gridX\":1904,\"gridY\":96,\"type\":1},{\"gridX\":2020,\"gridY\":96,\"type\":1},{\"gridX\":2064,\"gridY\":96,\"type\":2},{\"gridX\":2124,\"gridY\":96,\"type\":0},{\"gridX\":2160,\"gridY\":96,\"type\":0},{\"gridX\":640,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":648,\"gridY\":0,\"type\":26},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":136,\"gridY\":0,\"type\":28},{\"gridX\":264,\"gridY\":0,\"type\":28},{\"gridX\":392,\"gridY\":0,\"type\":28},{\"gridX\":680,\"gridY\":0,\"type\":28},{\"gridX\":808,\"gridY\":0,\"type\":28},{\"gridX\":1832,\"gridY\":0,\"type\":28},{\"gridX\":1960,\"gridY\":0,\"type\":28},{\"gridX\":2088,\"gridY\":0,\"type\":28},{\"gridX\":936,\"gridY\":0,\"type\":28},{\"gridX\":1064,\"gridY\":0,\"type\":28},{\"gridX\":1192,\"gridY\":0,\"type\":28},{\"gridX\":1320,\"gridY\":0,\"type\":28},{\"gridX\":1448,\"gridY\":0,\"type\":28},{\"gridX\":520,\"gridY\":0,\"type\":28},{\"gridX\":1576,\"gridY\":0,\"type\":28},{\"gridX\":1704,\"gridY\":0,\"type\":28},{\"gridX\":2216,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":292,\"gridY\":0,\"type\":33},{\"gridX\":954,\"gridY\":0,\"type\":33},{\"gridX\":1616,\"gridY\":0,\"type\":33},{\"gridX\":1403,\"gridY\":0,\"type\":30}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1952,\"gridY\":96,\"type\":12},{\"gridX\":128,\"gridY\":96,\"type\":41},{\"gridX\":198,\"gridY\":150,\"type\":0},{\"gridX\":258,\"gridY\":150,\"type\":2},{\"gridX\":202,\"gridY\":150,\"type\":1},{\"gridX\":216,\"gridY\":150,\"type\":1},{\"gridX\":230,\"gridY\":150,\"type\":1},{\"gridX\":700,\"gridY\":116,\"type\":0},{\"gridX\":704,\"gridY\":116,\"type\":1},{\"gridX\":718,\"gridY\":116,\"type\":1},{\"gridX\":732,\"gridY\":116,\"type\":2},{\"gridX\":244,\"gridY\":150,\"type\":1},{\"gridX\":735,\"gridY\":96,\"type\":16},{\"gridX\":713,\"gridY\":96,\"type\":15},{\"gridX\":143,\"gridY\":96,\"type\":13},{\"gridX\":148,\"gridY\":96,\"type\":15},{\"gridX\":209,\"gridY\":96,\"type\":15},{\"gridX\":170,\"gridY\":96,\"type\":16},{\"gridX\":184,\"gridY\":96,\"type\":20},{\"gridX\":204,\"gridY\":96,\"type\":13},{\"gridX\":231,\"gridY\":96,\"type\":15},{\"gridX\":253,\"gridY\":96,\"type\":15},{\"gridX\":275,\"gridY\":96,\"type\":16},{\"gridX\":540,\"gridY\":224,\"type\":34},{\"gridX\":540,\"gridY\":208,\"type\":34},{\"gridX\":540,\"gridY\":240,\"type\":34},{\"gridX\":540,\"gridY\":192,\"type\":34},{\"gridX\":540,\"gridY\":176,\"type\":34},{\"gridX\":808,\"gridY\":116,\"type\":0},{\"gridX\":812,\"gridY\":116,\"type\":1},{\"gridX\":826,\"gridY\":116,\"type\":2},{\"gridX\":840,\"gridY\":140,\"type\":0},{\"gridX\":844,\"gridY\":140,\"type\":1},{\"gridX\":872,\"gridY\":140,\"type\":2},{\"gridX\":896,\"gridY\":164,\"type\":0},{\"gridX\":900,\"gridY\":164,\"type\":1},{\"gridX\":914,\"gridY\":164,\"type\":1},{\"gridX\":956,\"gridY\":164,\"type\":2},{\"gridX\":928,\"gridY\":164,\"type\":1},{\"gridX\":772,\"gridY\":96,\"type\":19},{\"gridX\":858,\"gridY\":140,\"type\":1},{\"gridX\":866,\"gridY\":96,\"type\":13},{\"gridX\":871,\"gridY\":96,\"type\":15},{\"gridX\":893,\"gridY\":96,\"type\":15},{\"gridX\":915,\"gridY\":96,\"type\":15},{\"gridX\":937,\"gridY\":96,\"type\":14},{\"gridX\":948,\"gridY\":96,\"type\":16},{\"gridX\":1360,\"gridY\":116,\"type\":0},{\"gridX\":1364,\"gridY\":116,\"type\":1},{\"gridX\":1378,\"gridY\":116,\"type\":2},{\"gridX\":1456,\"gridY\":116,\"type\":0},{\"gridX\":1460,\"gridY\":116,\"type\":1},{\"gridX\":1474,\"gridY\":116,\"type\":2},{\"gridX\":1360,\"gridY\":96,\"type\":13},{\"gridX\":1365,\"gridY\":96,\"type\":14},{\"gridX\":1376,\"gridY\":96,\"type\":16},{\"gridX\":276,\"gridY\":112,\"type\":0},{\"gridX\":280,\"gridY\":112,\"type\":1},{\"gridX\":294,\"gridY\":112,\"type\":2},{\"gridX\":756,\"gridY\":144,\"type\":0},{\"gridX\":760,\"gridY\":144,\"type\":1},{\"gridX\":774,\"gridY\":144,\"type\":2},{\"gridX\":144,\"gridY\":112,\"type\":0},{\"gridX\":148,\"gridY\":112,\"type\":1},{\"gridX\":162,\"gridY\":112,\"type\":2},{\"gridX\":808,\"gridY\":172,\"type\":0},{\"gridX\":812,\"gridY\":172,\"type\":1},{\"gridX\":826,\"gridY\":172,\"type\":2},{\"gridX\":1528,\"gridY\":96,\"type\":13},{\"gridX\":1533,\"gridY\":96,\"type\":15},{\"gridX\":1555,\"gridY\":96,\"type\":16},{\"gridX\":1568,\"gridY\":96,\"type\":19},{\"gridX\":1620,\"gridY\":152,\"type\":0},{\"gridX\":1624,\"gridY\":152,\"type\":1},{\"gridX\":1638,\"gridY\":152,\"type\":1},{\"gridX\":1652,\"gridY\":152,\"type\":2},{\"gridX\":1868,\"gridY\":138,\"type\":34},{\"gridX\":1868,\"gridY\":154,\"type\":34},{\"gridX\":1868,\"gridY\":170,\"type\":34},{\"gridX\":1868,\"gridY\":186,\"type\":34},{\"gridX\":1868,\"gridY\":202,\"type\":34},{\"gridX\":1868,\"gridY\":218,\"type\":34},{\"gridX\":1702,\"gridY\":88,\"type\":3},{\"gridX\":1706,\"gridY\":88,\"type\":4},{\"gridX\":1720,\"gridY\":88,\"type\":4},{\"gridX\":1734,\"gridY\":88,\"type\":4},{\"gridX\":1762,\"gridY\":88,\"type\":5},{\"gridX\":1748,\"gridY\":88,\"type\":4},{\"gridX\":942,\"gridY\":164,\"type\":1},{\"gridX\":708,\"gridY\":96,\"type\":13}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1920,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":252,\"gridY\":172,\"type\":3},{\"gridX\":848,\"gridY\":96,\"type\":8},{\"gridX\":940,\"gridY\":188,\"type\":3},{\"gridX\":1272,\"gridY\":96,\"type\":9},{\"gridX\":328,\"gridY\":96,\"type\":3},{\"gridX\":376,\"gridY\":96,\"type\":3},{\"gridX\":424,\"gridY\":96,\"type\":3},{\"gridX\":1608,\"gridY\":96,\"type\":8},{\"gridX\":1940,\"gridY\":96,\"type\":4},{\"gridX\":472,\"gridY\":96,\"type\":3}],\"collectibles\":[{\"gridX\":704,\"gridY\":128,\"type\":0},{\"gridX\":1484,\"gridY\":124,\"type\":0},{\"gridX\":1492,\"gridY\":116,\"type\":0},{\"gridX\":1500,\"gridY\":108,\"type\":0},{\"gridX\":712,\"gridY\":128,\"type\":0},{\"gridX\":720,\"gridY\":128,\"type\":0},{\"gridX\":284,\"gridY\":216,\"type\":1},{\"gridX\":728,\"gridY\":128,\"type\":0},{\"gridX\":756,\"gridY\":116,\"type\":0},{\"gridX\":764,\"gridY\":112,\"type\":0},{\"gridX\":772,\"gridY\":108,\"type\":0},{\"gridX\":808,\"gridY\":128,\"type\":0},{\"gridX\":816,\"gridY\":128,\"type\":0},{\"gridX\":824,\"gridY\":128,\"type\":0},{\"gridX\":864,\"gridY\":152,\"type\":0},{\"gridX\":848,\"gridY\":152,\"type\":0},{\"gridX\":856,\"gridY\":152,\"type\":0},{\"gridX\":896,\"gridY\":176,\"type\":0},{\"gridX\":904,\"gridY\":176,\"type\":0},{\"gridX\":912,\"gridY\":176,\"type\":0},{\"gridX\":928,\"gridY\":176,\"type\":0},{\"gridX\":936,\"gridY\":176,\"type\":0},{\"gridX\":944,\"gridY\":176,\"type\":0},{\"gridX\":1212,\"gridY\":104,\"type\":1},{\"gridX\":960,\"gridY\":184,\"type\":0},{\"gridX\":968,\"gridY\":184,\"type\":0},{\"gridX\":1000,\"gridY\":180,\"type\":0},{\"gridX\":1040,\"gridY\":172,\"type\":0},{\"gridX\":1048,\"gridY\":172,\"type\":0},{\"gridX\":1088,\"gridY\":164,\"type\":0},{\"gridX\":1080,\"gridY\":164,\"type\":0},{\"gridX\":1120,\"gridY\":156,\"type\":0},{\"gridX\":1128,\"gridY\":156,\"type\":0},{\"gridX\":1168,\"gridY\":148,\"type\":0},{\"gridX\":1160,\"gridY\":148," << "\"type\":0},{\"gridX\":612,\"gridY\":104,\"type\":0},{\"gridX\":596,\"gridY\":104,\"type\":0},{\"gridX\":604,\"gridY\":104,\"type\":0},{\"gridX\":668,\"gridY\":112,\"type\":0},{\"gridX\":672,\"gridY\":120,\"type\":0},{\"gridX\":676,\"gridY\":128,\"type\":0},{\"gridX\":136,\"gridY\":112,\"type\":0},{\"gridX\":140,\"gridY\":120,\"type\":0},{\"gridX\":144,\"gridY\":128,\"type\":0},{\"gridX\":200,\"gridY\":164,\"type\":0},{\"gridX\":208,\"gridY\":164,\"type\":0},{\"gridX\":1360,\"gridY\":128,\"type\":0},{\"gridX\":1368,\"gridY\":128,\"type\":0},{\"gridX\":1376,\"gridY\":128,\"type\":0},{\"gridX\":1424,\"gridY\":116,\"type\":0},{\"gridX\":1408,\"gridY\":116,\"type\":0},{\"gridX\":1456,\"gridY\":128,\"type\":0},{\"gridX\":1464,\"gridY\":128,\"type\":0},{\"gridX\":1472,\"gridY\":128,\"type\":0},{\"gridX\":188,\"gridY\":124,\"type\":0},{\"gridX\":192,\"gridY\":132,\"type\":0},{\"gridX\":196,\"gridY\":140,\"type\":0},{\"gridX\":1524,\"gridY\":112,\"type\":0},{\"gridX\":1532,\"gridY\":116,\"type\":0},{\"gridX\":1540,\"gridY\":120,\"type\":0},{\"gridX\":1548,\"gridY\":116,\"type\":0},{\"gridX\":1556,\"gridY\":112,\"type\":0},{\"gridX\":1400,\"gridY\":120,\"type\":0},{\"gridX\":1432,\"gridY\":120,\"type\":0},{\"gridX\":1572,\"gridY\":120,\"type\":0},{\"gridX\":1576,\"gridY\":128,\"type\":0},{\"gridX\":1580,\"gridY\":136,\"type\":0},{\"gridX\":1584,\"gridY\":144,\"type\":0},{\"gridX\":1588,\"gridY\":152,\"type\":0},{\"gridX\":1292,\"gridY\":104,\"type\":0},{\"gridX\":1276,\"gridY\":112,\"type\":0},{\"gridX\":1284,\"gridY\":108,\"type\":0},{\"gridX\":1008,\"gridY\":180,\"type\":0},{\"gridX\":976,\"gridY\":184,\"type\":0},{\"gridX\":1016,\"gridY\":180,\"type\":0},{\"gridX\":1056,\"gridY\":172,\"type\":0},{\"gridX\":1096,\"gridY\":164,\"type\":0},{\"gridX\":1136,\"gridY\":156,\"type\":0},{\"gridX\":1176,\"gridY\":148,\"type\":0},{\"gridX\":1592,\"gridY\":160,\"type\":0},{\"gridX\":1596,\"gridY\":168,\"type\":0},{\"gridX\":1676,\"gridY\":144,\"type\":0},{\"gridX\":1668,\"gridY\":152,\"type\":0},{\"gridX\":1660,\"gridY\":160,\"type\":0},{\"gridX\":1624,\"gridY\":164,\"type\":0},{\"gridX\":1632,\"gridY\":164,\"type\":0},{\"gridX\":1640,\"gridY\":164,\"type\":0},{\"gridX\":1648,\"gridY\":164,\"type\":0},{\"gridX\":1716,\"gridY\":100,\"type\":0},{\"gridX\":1724,\"gridY\":108,\"type\":0},{\"gridX\":1732,\"gridY\":116,\"type\":0},{\"gridX\":1756,\"gridY\":116,\"type\":0},{\"gridX\":1772,\"gridY\":116,\"type\":0},{\"gridX\":1816,\"gridY\":112,\"type\":0},{\"gridX\":1800,\"gridY\":112,\"type\":0},{\"gridX\":1852,\"gridY\":108,\"type\":0},{\"gridX\":1808,\"gridY\":112,\"type\":0},{\"gridX\":1844,\"gridY\":108,\"type\":0},{\"gridX\":1764,\"gridY\":116,\"type\":0},{\"gridX\":1860,\"gridY\":108,\"type\":0},{\"gridX\":1708,\"gridY\":108,\"type\":0},{\"gridX\":1416,\"gridY\":112,\"type\":1},{\"gridX\":1700,\"gridY\":116,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":96}],\"extraForegroundObjects\":[],\"markers\":[{\"gridX\":2228,\"gridY\":0,\"type\":1}]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level6 *instance = new Chapter1Level6(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 7 ///

Chapter1Level7 * Chapter1Level7::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":7,\"midgrounds\":[{\"gridX\":2216,\"gridY\":96,\"type\":2},{\"gridX\":2276,\"gridY\":96,\"type\":1},{\"gridX\":2316,\"gridY\":96,\"type\":0},{\"gridX\":2348,\"gridY\":96,\"type\":2},{\"gridX\":2408,\"gridY\":96,\"type\":1},{\"gridX\":2448,\"gridY\":96,\"type\":0},{\"gridX\":0,\"gridY\":96,\"type\":1},{\"gridX\":36,\"gridY\":96,\"type\":1},{\"gridX\":72,\"gridY\":96,\"type\":0},{\"gridX\":100,\"gridY\":96,\"type\":1},{\"gridX\":200,\"gridY\":96,\"type\":2},{\"gridX\":260,\"gridY\":96,\"type\":1},{\"gridX\":540,\"gridY\":96,\"type\":0},{\"gridX\":304,\"gridY\":96,\"type\":1},{\"gridX\":432,\"gridY\":96,\"type\":2},{\"gridX\":780,\"gridY\":96,\"type\":0},{\"gridX\":856,\"gridY\":96,\"type\":2},{\"gridX\":812,\"gridY\":96,\"type\":1},{\"gridX\":728,\"gridY\":96,\"type\":1},{\"gridX\":968,\"gridY\":96,\"type\":0},{\"gridX\":1036,\"gridY\":96,\"type\":2},{\"gridX\":1008,\"gridY\":96,\"type\":0},{\"gridX\":1108,\"gridY\":96,\"type\":2},{\"gridX\":1168,\"gridY\":96,\"type\":0},{\"gridX\":1188,\"gridY\":96,\"type\":1},{\"gridX\":1224,\"gridY\":96,\"type\":2},{\"gridX\":1520,\"gridY\":96,\"type\":2},{\"gridX\":1628,\"gridY\":96,\"type\":0},{\"gridX\":1668,\"gridY\":96,\"type\":1},{\"gridX\":1720,\"gridY\":96,\"type\":2},{\"gridX\":1840,\"gridY\":96,\"type\":2},{\"gridX\":1772,\"gridY\":96,\"type\":0},{\"gridX\":2136,\"gridY\":96,\"type\":1},{\"gridX\":2180,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":776,\"gridY\":0,\"type\":26},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":136,\"gridY\":0,\"type\":28},{\"gridX\":264,\"gridY\":0,\"type\":28},{\"gridX\":392,\"gridY\":0,\"type\":28},{\"gridX\":520,\"gridY\":0,\"type\":28},{\"gridX\":648,\"gridY\":0,\"type\":28},{\"gridX\":808,\"gridY\":0,\"type\":28},{\"gridX\":936,\"gridY\":0,\"type\":28},{\"gridX\":1064,\"gridY\":0,\"type\":28},{\"gridX\":1192,\"gridY\":0,\"type\":28},{\"gridX\":1320,\"gridY\":0,\"type\":28},{\"gridX\":2216,\"gridY\":0,\"type\":28},{\"gridX\":2344,\"gridY\":0,\"type\":28},{\"gridX\":1448,\"gridY\":0,\"type\":28},{\"gridX\":1576,\"gridY\":0,\"type\":28},{\"gridX\":1704,\"gridY\":0,\"type\":28},{\"gridX\":1832,\"gridY\":0,\"type\":28},{\"gridX\":1960,\"gridY\":0,\"type\":28},{\"gridX\":2088,\"gridY\":0,\"type\":28},{\"gridX\":2472,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":144,\"gridY\":0,\"type\":31},{\"gridX\":480,\"gridY\":0,\"type\":31},{\"gridX\":568,\"gridY\":0,\"type\":32},{\"gridX\":1268,\"gridY\":0,\"type\":33},{\"gridX\":1824,\"gridY\":0,\"type\":30},{\"gridX\":1892,\"gridY\":0,\"type\":33}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":2216,\"gridY\":96,\"type\":12},{\"gridX\":192,\"gridY\":124,\"type\":34},{\"gridX\":168,\"gridY\":96,\"type\":34},{\"gridX\":592,\"gridY\":116,\"type\":0},{\"gridX\":596,\"gridY\":116,\"type\":1},{\"gridX\":610,\"gridY\":116,\"type\":2},{\"gridX\":680,\"gridY\":116,\"type\":0},{\"gridX\":684,\"gridY\":116,\"type\":1},{\"gridX\":698,\"gridY\":116,\"type\":1},{\"gridX\":712,\"gridY\":116,\"type\":2},{\"gridX\":900,\"gridY\":108,\"type\":0},{\"gridX\":904,\"gridY\":108,\"type\":1},{\"gridX\":918,\"gridY\":108,\"type\":1},{\"gridX\":932,\"gridY\":108,\"type\":2},{\"gridX\":996,\"gridY\":96,\"type\":13},{\"gridX\":1023,\"gridY\":96,\"type\":15},{\"gridX\":1001,\"gridY\":96,\"type\":15},{\"gridX\":980,\"gridY\":96,\"type\":19},{\"gridX\":1045,\"gridY\":96,\"type\":15},{\"gridX\":1067,\"gridY\":96,\"type\":15},{\"gridX\":1089,\"gridY\":96,\"type\":14},{\"gridX\":1100,\"gridY\":96,\"type\":16},{\"gridX\":1128,\"gridY\":96,\"type\":13},{\"gridX\":1133,\"gridY\":96,\"type\":15},{\"gridX\":1155,\"gridY\":96,\"type\":15},{\"gridX\":1177,\"gridY\":96,\"type\":15},{\"gridX\":1199,\"gridY\":96,\"type\":15},{\"gridX\":1243,\"gridY\":96,\"type\":16},{\"gridX\":1221,\"gridY\":96,\"type\":15},{\"gridX\":1256,\"gridY\":96,\"type\":41},{\"gridX\":1236,\"gridY\":189,\"type\":34},{\"gridX\":1272,\"gridY\":164,\"type\":34},{\"gridX\":1260,\"gridY\":172,\"type\":34},{\"gridX\":1248,\"gridY\":180,\"type\":34},{\"gridX\":788,\"gridY\":120,\"type\":0},{\"gridX\":806,\"gridY\":120,\"type\":2},{\"gridX\":792,\"gridY\":120,\"type\":1},{\"gridX\":1592,\"gridY\":96,\"type\":7},{\"gridX\":1696,\"gridY\":120,\"type\":34},{\"gridX\":1712,\"gridY\":120,\"type\":34},{\"gridX\":1816,\"gridY\":96,\"type\":19}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":2184,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":132,\"gridY\":96,\"type\":4},{\"gridX\":376,\"gridY\":96,\"type\":9},{\"gridX\":540,\"gridY\":96,\"type\":9},{\"gridX\":764,\"gridY\":96,\"type\":6},{\"gridX\":884,\"gridY\":96,\"type\":4},{\"gridX\":900,\"gridY\":113,\"type\":4},{\"gridX\":912,\"gridY\":113,\"type\":4},{\"gridX\":1108,\"gridY\":96,\"type\":6},{\"gridX\":1640,\"gridY\":96,\"type\":4},{\"gridX\":1780,\"gridY\":96,\"type\":4},{\"gridX\":848,\"gridY\":96,\"type\":10},{\"gridX\":1880,\"gridY\":96,\"type\":4}],\"collectibles\":[{\"gridX\":172,\"gridY\":112,\"type\":0},{\"gridX\":156,\"gridY\":112,\"type\":0},{\"gridX\":164,\"gridY\":112,\"type\":0},{\"gridX\":208,\"gridY\":108,\"type\":0},{\"gridX\":192,\"gridY\":108,\"type\":0},{\"gridX\":200,\"gridY\":108,\"type\":0},{\"gridX\":228,\"gridY\":104,\"type\":0},{\"gridX\":244,\"gridY\":104,\"type\":0},{\"gridX\":236,\"gridY\":104,\"type\":0},{\"gridX\":260,\"gridY\":136,\"type\":1},{\"gridX\":300,\"gridY\":104,\"type\":0},{\"gridX\":308,\"gridY\":104,\"type\":0},{\"gridX\":316,\"gridY\":104,\"type\":0},{\"gridX\":360,\"gridY\":116,\"type\":0},{\"gridX\":368,\"gridY\":116,\"type\":0},{\"gridX\":376,\"gridY\":116,\"type\":0},{\"gridX\":384,\"gridY\":112,\"type\":0},{\"gridX\":392,\"gridY\":112,\"type\":0},{\"gridX\":400,\"gridY\":112,\"type\":0},{\"gridX\":408,\"gridY\":108,\"type\":0},{\"gridX\":416,\"gridY\":108,\"type\":0},{\"gridX\":424,\"gridY\":108,\"type\":0},{\"gridX\":440,\"gridY\":104,\"type\":0},{\"gridX\":432,\"gridY\":104,\"type\":0},{\"gridX\":448,\"gridY\":104,\"type\":0},{\"gridX\":488,\"gridY\":104,\"type\":0},{\"gridX\":496,\"gridY\":104,\"type\":0},{\"gridX\":504,\"gridY\":104,\"type\":0},{\"gridX\":572,\"gridY\":104,\"type\":0},{\"gridX\":580,\"gridY\":104,\"type\":0},{\"gridX\":588,\"gridY\":104,\"type\":0},{\"gridX\":668,\"gridY\":96,\"type\":0},{\"gridX\":676,\"gridY\":96,\"type\":0},{\"gridX\":684,\"gridY\":96,\"type\":0},{\"gridX\":776,\"gridY\":128,\"type\":0},{\"gridX\":784,\"gridY\":136,\"type\":0},{\"gridX\":792,\"gridY\":144,\"type\":0},{\"gridX\":800,\"gridY\":136,\"type\":0},{\"gridX\":808,\"gridY\":128,\"type\":0},{\"gridX\":932,\"gridY\":128,\"type\":0},{\"gridX\":924,\"gridY\":128,\"type\":0},{\"gridX\":940,\"gridY\":128,\"type\":0},{\"gridX\":964,\"gridY\":124,\"type\":0},{\"gridX\":948,\"gridY\":124,\"type\":0},{\"gridX\":956,\"gridY\":124,\"type\":0},{\"gridX\":996,\"gridY\":124,\"type\":0},{\"gridX\":1004,\"gridY\":132,\"type\":0},{\"gridX\":1012,\"gridY\":140,\"type\":0},{\"gridX\":1020,\"gridY\":148,\"type\":0},{\"gridX\":1028,\"gridY\":156,\"type\":0},{\"gridX\":848,\"gridY\":104,\"type\":0},{\"gridX\":856,\"gridY\":104,\"type\":0},{\"gridX\":840,\"gridY\":104,\"type\":0},{\"gridX\":1072,\"gridY\":156,\"type\":0},{\"gridX\":1080,\"gridY\":148,\"type\":0},{\"gridX\":1088,\"gridY\":140,\"type\":0},{\"gridX\":1096,\"gridY\":132,\"type\":0},{\"gridX\":1104,\"gridY\":124,\"type\":0},{\"gridX\":1196,\"gridY\":108,\"type\":1},{\"gridX\":1288,\"gridY\":140,\"type\":0},{\"gridX\":1296,\"gridY\":140,\"type\":0},{\"gridX\":1304,\"gridY\":140,\"type\":0},{\"gridX\":1336,\"gridY\":132,\"type\":0},{\"gridX\":1344,\"gridY\":132,\"type\":0},{\"gridX\":1352,\"gridY\":132,\"type\":0},{\"gridX\":1384,\"gridY\":124,\"type\":0},{\"gridX\":1392,\"gridY\":124,\"type\":0},{\"gridX\":1400,\"gridY\":124,\"type\":0},{\"gridX\":1432,\"gridY\":116,\"type\":0},{\"gridX\":1440,\"gridY\":116,\"type\":0},{\"gridX\":1448,\"gridY\":116,\"type\":0},{\"gridX\":1480,\"gridY\":108,\"type\":0},{\"gridX\":1488,\"gridY\":108" << ",\"type\":0},{\"gridX\":1496,\"gridY\":108,\"type\":0},{\"gridX\":620,\"gridY\":100,\"type\":0},{\"gridX\":628,\"gridY\":100,\"type\":0},{\"gridX\":636,\"gridY\":100,\"type\":0},{\"gridX\":732,\"gridY\":104,\"type\":0},{\"gridX\":740,\"gridY\":104,\"type\":0},{\"gridX\":724,\"gridY\":104,\"type\":0},{\"gridX\":1568,\"gridY\":104,\"type\":0},{\"gridX\":1552,\"gridY\":104,\"type\":0},{\"gridX\":1560,\"gridY\":104,\"type\":0},{\"gridX\":1592,\"gridY\":120,\"type\":0},{\"gridX\":1600,\"gridY\":128,\"type\":0},{\"gridX\":1608,\"gridY\":136,\"type\":0},{\"gridX\":1616,\"gridY\":128,\"type\":0},{\"gridX\":1624,\"gridY\":120,\"type\":0},{\"gridX\":1656,\"gridY\":120,\"type\":0},{\"gridX\":1660,\"gridY\":128,\"type\":0},{\"gridX\":1664,\"gridY\":136,\"type\":0},{\"gridX\":708,\"gridY\":128,\"type\":0},{\"gridX\":700,\"gridY\":128,\"type\":0},{\"gridX\":716,\"gridY\":128,\"type\":0},{\"gridX\":1716,\"gridY\":140,\"type\":1},{\"gridX\":1812,\"gridY\":104,\"type\":0},{\"gridX\":1796,\"gridY\":112,\"type\":0},{\"gridX\":1804,\"gridY\":108,\"type\":0},{\"gridX\":2041,\"gridY\":147,\"type\":0},{\"gridX\":2052,\"gridY\":144,\"type\":0},{\"gridX\":2040,\"gridY\":141,\"type\":0},{\"gridX\":2048,\"gridY\":148,\"type\":0},{\"gridX\":2046,\"gridY\":140,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":108}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level7 *instance = new Chapter1Level7(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 8 ///

Chapter1Level8 * Chapter1Level8::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":8,\"midgrounds\":[{\"gridX\":0,\"gridY\":96,\"type\":2},{\"gridX\":56,\"gridY\":96,\"type\":0},{\"gridX\":84,\"gridY\":96,\"type\":2},{\"gridX\":276,\"gridY\":96,\"type\":0},{\"gridX\":212,\"gridY\":96,\"type\":2},{\"gridX\":312,\"gridY\":96,\"type\":1},{\"gridX\":600,\"gridY\":96,\"type\":2},{\"gridX\":660,\"gridY\":96,\"type\":1},{\"gridX\":704,\"gridY\":96,\"type\":0},{\"gridX\":760,\"gridY\":96,\"type\":2},{\"gridX\":820,\"gridY\":96,\"type\":0},{\"gridX\":860,\"gridY\":96,\"type\":1},{\"gridX\":908,\"gridY\":96,\"type\":2},{\"gridX\":1224,\"gridY\":96,\"type\":2},{\"gridX\":1288,\"gridY\":96,\"type\":1},{\"gridX\":168,\"gridY\":96,\"type\":1},{\"gridX\":1356,\"gridY\":96,\"type\":1},{\"gridX\":1400,\"gridY\":96,\"type\":0},{\"gridX\":1436,\"gridY\":96,\"type\":1},{\"gridX\":1504,\"gridY\":96,\"type\":1},{\"gridX\":1556,\"gridY\":96,\"type\":0},{\"gridX\":1604,\"gridY\":96,\"type\":0},{\"gridX\":1652,\"gridY\":96,\"type\":0},{\"gridX\":1764,\"gridY\":96,\"type\":2},{\"gridX\":1816,\"gridY\":96,\"type\":1},{\"gridX\":1856,\"gridY\":96,\"type\":2},{\"gridX\":1912,\"gridY\":96,\"type\":0},{\"gridX\":1940,\"gridY\":96,\"type\":1},{\"gridX\":1980,\"gridY\":96,\"type\":2},{\"gridX\":268,\"gridY\":96,\"type\":7},{\"gridX\":268,\"gridY\":108,\"type\":7},{\"gridX\":268,\"gridY\":120,\"type\":7},{\"gridX\":268,\"gridY\":132,\"type\":7},{\"gridX\":268,\"gridY\":144,\"type\":7},{\"gridX\":316,\"gridY\":96,\"type\":7},{\"gridX\":316,\"gridY\":108,\"type\":7},{\"gridX\":316,\"gridY\":120,\"type\":7},{\"gridX\":316,\"gridY\":132,\"type\":7},{\"gridX\":316,\"gridY\":144,\"type\":7},{\"gridX\":316,\"gridY\":156,\"type\":7},{\"gridX\":316,\"gridY\":168,\"type\":7},{\"gridX\":316,\"gridY\":180,\"type\":7},{\"gridX\":368,\"gridY\":188,\"type\":7},{\"gridX\":368,\"gridY\":200,\"type\":7},{\"gridX\":368,\"gridY\":212,\"type\":7},{\"gridX\":440,\"gridY\":108,\"type\":7},{\"gridX\":440,\"gridY\":96,\"type\":7},{\"gridX\":440,\"gridY\":120,\"type\":7},{\"gridX\":440,\"gridY\":132,\"type\":7},{\"gridX\":440,\"gridY\":144,\"type\":7},{\"gridX\":440,\"gridY\":156,\"type\":7},{\"gridX\":440,\"gridY\":168,\"type\":7},{\"gridX\":440,\"gridY\":180,\"type\":7},{\"gridX\":440,\"gridY\":192,\"type\":7},{\"gridX\":440,\"gridY\":204,\"type\":7}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":136,\"gridY\":0,\"type\":27},{\"gridX\":1992,\"gridY\":0,\"type\":27},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":200,\"gridY\":0,\"type\":28},{\"gridX\":328,\"gridY\":0,\"type\":28},{\"gridX\":456,\"gridY\":0,\"type\":28},{\"gridX\":584,\"gridY\":0,\"type\":28},{\"gridX\":712,\"gridY\":0,\"type\":28},{\"gridX\":840,\"gridY\":0,\"type\":28},{\"gridX\":1608,\"gridY\":0,\"type\":28},{\"gridX\":1736,\"gridY\":0,\"type\":28},{\"gridX\":1864,\"gridY\":0,\"type\":28},{\"gridX\":968,\"gridY\":0,\"type\":28},{\"gridX\":1096,\"gridY\":0,\"type\":28},{\"gridX\":1224,\"gridY\":0,\"type\":28},{\"gridX\":1352,\"gridY\":0,\"type\":28},{\"gridX\":1480,\"gridY\":0,\"type\":28},{\"gridX\":2056,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":356,\"gridY\":0,\"type\":33},{\"gridX\":964,\"gridY\":0,\"type\":33}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1772,\"gridY\":96,\"type\":12},{\"gridX\":192,\"gridY\":96,\"type\":41},{\"gridX\":244,\"gridY\":96,\"type\":24},{\"gridX\":276,\"gridY\":96,\"type\":24},{\"gridX\":308,\"gridY\":96,\"type\":24},{\"gridX\":732,\"gridY\":116,\"type\":0},{\"gridX\":736,\"gridY\":116,\"type\":1},{\"gridX\":750,\"gridY\":116,\"type\":1},{\"gridX\":764,\"gridY\":116,\"type\":2},{\"gridX\":820,\"gridY\":140,\"type\":0},{\"gridX\":838,\"gridY\":140,\"type\":1},{\"gridX\":936,\"gridY\":140,\"type\":2},{\"gridX\":824,\"gridY\":140,\"type\":1},{\"gridX\":852,\"gridY\":140,\"type\":1},{\"gridX\":866,\"gridY\":140,\"type\":1},{\"gridX\":880,\"gridY\":140,\"type\":1},{\"gridX\":894,\"gridY\":140,\"type\":1},{\"gridX\":908,\"gridY\":140,\"type\":1},{\"gridX\":922,\"gridY\":140,\"type\":1},{\"gridX\":212,\"gridY\":96,\"type\":24},{\"gridX\":800,\"gridY\":96,\"type\":19},{\"gridX\":732,\"gridY\":96,\"type\":13},{\"gridX\":737,\"gridY\":96,\"type\":15},{\"gridX\":781,\"gridY\":96,\"type\":16},{\"gridX\":759,\"gridY\":96,\"type\":15},{\"gridX\":340,\"gridY\":96,\"type\":23},{\"gridX\":825,\"gridY\":96,\"type\":13},{\"gridX\":830,\"gridY\":96,\"type\":15},{\"gridX\":852,\"gridY\":96,\"type\":16},{\"gridX\":836,\"gridY\":176,\"type\":34},{\"gridX\":1232,\"gridY\":116,\"type\":0},{\"gridX\":1236,\"gridY\":116,\"type\":1},{\"gridX\":1250,\"gridY\":116,\"type\":1},{\"gridX\":1264,\"gridY\":116,\"type\":2},{\"gridX\":1332,\"gridY\":116,\"type\":0},{\"gridX\":1336,\"gridY\":116,\"type\":1},{\"gridX\":1350,\"gridY\":116,\"type\":2},{\"gridX\":1380,\"gridY\":140,\"type\":0},{\"gridX\":1384,\"gridY\":140,\"type\":1},{\"gridX\":1398,\"gridY\":140,\"type\":2},{\"gridX\":1428,\"gridY\":140,\"type\":0},{\"gridX\":1432,\"gridY\":140,\"type\":1},{\"gridX\":1396,\"gridY\":124,\"type\":0},{\"gridX\":1400,\"gridY\":124,\"type\":1},{\"gridX\":1414,\"gridY\":124,\"type\":2},{\"gridX\":1508,\"gridY\":152,\"type\":34},{\"gridX\":1520,\"gridY\":160,\"type\":34},{\"gridX\":1500,\"gridY\":96,\"type\":24},{\"gridX\":916,\"gridY\":176,\"type\":34},{\"gridX\":1348,\"gridY\":96,\"type\":11},{\"gridX\":1688,\"gridY\":96,\"type\":24},{\"gridX\":1720,\"gridY\":96,\"type\":24},{\"gridX\":864,\"gridY\":145,\"type\":23},{\"gridX\":1446,\"gridY\":140,\"type\":1},{\"gridX\":1474,\"gridY\":140,\"type\":2},{\"gridX\":1432,\"gridY\":168,\"type\":34},{\"gridX\":1460,\"gridY\":140,\"type\":1},{\"gridX\":1364,\"gridY\":96,\"type\":13},{\"gridX\":1369,\"gridY\":96,\"type\":15},{\"gridX\":1391,\"gridY\":96,\"type\":16},{\"gridX\":264,\"gridY\":152,\"type\":48},{\"gridX\":260,\"gridY\":152,\"type\":47},{\"gridX\":280,\"gridY\":152,\"type\":48},{\"gridX\":296,\"gridY\":152,\"type\":48},{\"gridX\":312,\"gridY\":152,\"type\":48},{\"gridX\":328,\"gridY\":152,\"type\":49},{\"gridX\":316,\"gridY\":188,\"type\":48},{\"gridX\":312,\"gridY\":188,\"type\":47},{\"gridX\":332,\"gridY\":188,\"type\":48},{\"gridX\":348,\"gridY\":188,\"type\":48},{\"gridX\":364,\"gridY\":188,\"type\":48},{\"gridX\":380,\"gridY\":188,\"type\":49},{\"gridX\":368,\"gridY\":220,\"type\":48},{\"gridX\":364,\"gridY\":220,\"type\":47},{\"gridX\":384,\"gridY\":220,\"type\":48},{\"gridX\":400,\"gridY\":220,\"type\":48},{\"gridX\":416,\"gridY\":220,\"type\":48},{\"gridX\":440,\"gridY\":212,\"type\":48},{\"gridX\":456,\"gridY\":212,\"type\":48},{\"gridX\":436,\"gridY\":212,\"type\":47},{\"gridX\":432,\"gridY\":220,\"type\":49}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1740,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":344,\"gridY\":140,\"type\":3},{\"gridX\":960,\"gridY\":128,\"type\":3},{\"gridX\":1024,\"gridY\":112,\"type\":3},{\"gridX\":1088,\"gridY\":112,\"type\":3},{\"gridX\":1176,\"gridY\":96,\"type\":3},{\"gridX\":384,\"gridY\":132,\"type\":3},{\"gridX\":428,\"gridY\":128,\"type\":3},{\"gridX\":484,\"gridY\":120,\"type\":3},{\"gridX\":524,\"gridY\":100,\"type\":3},{\"gridX\":568,\"gridY\":100,\"type\":3},{\"gridX\":152,\"gridY\":96,\"type\":4},{\"gridX\":1328,\"gridY\":96,\"type\":4},{\"gridX\":1468,\"gridY\":96,\"type\":10},{\"gridX\":1580,\"gridY\":96,\"type\":8},{\"gridX\":1628,\"gridY\":96,\"type\":8},{\"gridX\":1672,\"gridY\":96,\"type\":8}],\"collectibles\":[{\"gridX\":732,\"gridY\":128,\"type\":0},{\"gridX\":804,\"gridY\":116,\"type\":0},{\"gridX\":808,\"gridY\":124,\"type\":0},{\"gridX\":1060,\"gridY\":148,\"type\":0},{\"gridX\":996,\"gridY\":160,\"type\":0},{\"gridX\":1136,\"gridY\":148,\"type\":0},{\"gridX\":1220,\"gridY\":136,\"type\":0},{\"gridX\":1208,\"gridY\":128,\"type\":0},{\"gridX\":976,\"gridY\":148,\"type\":0},{\"gridX\":1016,\"gridY\":148,\"type\":0},{\"gridX\":1080,\"gridY\":136,\"type\":0},{\"gridX\":1116,\"gridY\":136,\"type\":0},{\"gridX\":1156,\"gridY\":136,\"type\":0},{\"gridX\":1196,\"gridY\":120,\"type\":0},{\"gridX" << "\":1292,\"gridY\":104,\"type\":0},{\"gridX\":1284,\"gridY\":104,\"type\":0},{\"gridX\":1300,\"gridY\":104,\"type\":0},{\"gridX\":932,\"gridY\":124,\"type\":1},{\"gridX\":740,\"gridY\":128,\"type\":0},{\"gridX\":748,\"gridY\":128,\"type\":0},{\"gridX\":812,\"gridY\":132,\"type\":0},{\"gridX\":896,\"gridY\":104,\"type\":0},{\"gridX\":904,\"gridY\":104,\"type\":0},{\"gridX\":268,\"gridY\":164,\"type\":0},{\"gridX\":276,\"gridY\":164,\"type\":0},{\"gridX\":284,\"gridY\":164,\"type\":0},{\"gridX\":276,\"gridY\":176,\"type\":0},{\"gridX\":268,\"gridY\":192,\"type\":0},{\"gridX\":276,\"gridY\":196,\"type\":0},{\"gridX\":340,\"gridY\":156,\"type\":0},{\"gridX\":348,\"gridY\":152,\"type\":0},{\"gridX\":356,\"gridY\":156,\"type\":0},{\"gridX\":384,\"gridY\":152,\"type\":0},{\"gridX\":388,\"gridY\":144,\"type\":0},{\"gridX\":392,\"gridY\":152,\"type\":0},{\"gridX\":428,\"gridY\":148,\"type\":0},{\"gridX\":432,\"gridY\":140,\"type\":0},{\"gridX\":436,\"gridY\":148,\"type\":0},{\"gridX\":524,\"gridY\":120,\"type\":0},{\"gridX\":528,\"gridY\":112,\"type\":0},{\"gridX\":532,\"gridY\":120,\"type\":0},{\"gridX\":484,\"gridY\":140,\"type\":0},{\"gridX\":488,\"gridY\":132,\"type\":0},{\"gridX\":148,\"gridY\":108,\"type\":0},{\"gridX\":160,\"gridY\":108,\"type\":0},{\"gridX\":168,\"gridY\":112,\"type\":0},{\"gridX\":144,\"gridY\":116,\"type\":0},{\"gridX\":140,\"gridY\":124,\"type\":0},{\"gridX\":176,\"gridY\":108,\"type\":0},{\"gridX\":492,\"gridY\":140,\"type\":0},{\"gridX\":568,\"gridY\":120,\"type\":0},{\"gridX\":572,\"gridY\":112,\"type\":0},{\"gridX\":576,\"gridY\":120,\"type\":0},{\"gridX\":648,\"gridY\":104,\"type\":0},{\"gridX\":656,\"gridY\":104,\"type\":0},{\"gridX\":664,\"gridY\":104,\"type\":0},{\"gridX\":912,\"gridY\":104,\"type\":0},{\"gridX\":1028,\"gridY\":124,\"type\":0},{\"gridX\":1092,\"gridY\":124,\"type\":0},{\"gridX\":1180,\"gridY\":108,\"type\":0},{\"gridX\":1040,\"gridY\":136,\"type\":0},{\"gridX\":1332,\"gridY\":128,\"type\":0},{\"gridX\":1340,\"gridY\":128,\"type\":0},{\"gridX\":1348,\"gridY\":128,\"type\":0},{\"gridX\":1380,\"gridY\":152,\"type\":0},{\"gridX\":1388,\"gridY\":152,\"type\":0},{\"gridX\":1396,\"gridY\":152,\"type\":0},{\"gridX\":1452,\"gridY\":152,\"type\":0},{\"gridX\":1460,\"gridY\":152,\"type\":0},{\"gridX\":1444,\"gridY\":152,\"type\":0},{\"gridX\":1528,\"gridY\":144,\"type\":1},{\"gridX\":848,\"gridY\":116,\"type\":0},{\"gridX\":856,\"gridY\":112,\"type\":0},{\"gridX\":864,\"gridY\":108,\"type\":0},{\"gridX\":872,\"gridY\":104,\"type\":0},{\"gridX\":1500,\"gridY\":132,\"type\":0},{\"gridX\":1484,\"gridY\":148,\"type\":0},{\"gridX\":1492,\"gridY\":140,\"type\":0},{\"gridX\":1404,\"gridY\":148,\"type\":0},{\"gridX\":1420,\"gridY\":140,\"type\":0},{\"gridX\":1412,\"gridY\":144,\"type\":0},{\"gridX\":1592,\"gridY\":128,\"type\":0},{\"gridX\":1544,\"gridY\":136,\"type\":0},{\"gridX\":1552,\"gridY\":128,\"type\":0},{\"gridX\":1560,\"gridY\":120,\"type\":0},{\"gridX\":1748,\"gridY\":120,\"type\":0},{\"gridX\":1580,\"gridY\":120,\"type\":0},{\"gridX\":1604,\"gridY\":120,\"type\":0},{\"gridX\":1628,\"gridY\":120,\"type\":0},{\"gridX\":1568,\"gridY\":112,\"type\":0},{\"gridX\":1616,\"gridY\":112,\"type\":0},{\"gridX\":1696,\"gridY\":128,\"type\":0},{\"gridX\":1640,\"gridY\":128,\"type\":0},{\"gridX\":1676,\"gridY\":120,\"type\":0},{\"gridX\":1756,\"gridY\":120,\"type\":0},{\"gridX\":1664,\"gridY\":112,\"type\":0},{\"gridX\":1652,\"gridY\":120,\"type\":0},{\"gridX\":1508,\"gridY\":124,\"type\":0},{\"gridX\":1764,\"gridY\":120,\"type\":0},{\"gridX\":1688,\"gridY\":128,\"type\":0},{\"gridX\":1704,\"gridY\":128,\"type\":0},{\"gridX\":1340,\"gridY\":108,\"type\":1},{\"gridX\":756,\"gridY\":128,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":96}],\"extraForegroundObjects\":[],\"foregroundCoverObjects\":[],\"markers\":[{\"gridX\":2088,\"gridY\":0,\"type\":1}]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level8 *instance = new Chapter1Level8(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 9 ///

Chapter1Level9 * Chapter1Level9::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":9,\"midgrounds\":[{\"gridX\":56,\"gridY\":96,\"type\":0},{\"gridX\":88,\"gridY\":96,\"type\":1},{\"gridX\":572,\"gridY\":96,\"type\":2},{\"gridX\":492,\"gridY\":96,\"type\":0},{\"gridX\":1140,\"gridY\":96,\"type\":2},{\"gridX\":1204,\"gridY\":96,\"type\":2},{\"gridX\":1605,\"gridY\":96,\"type\":1},{\"gridX\":1528,\"gridY\":96,\"type\":2},{\"gridX\":1704,\"gridY\":96,\"type\":1},{\"gridX\":1748,\"gridY\":96,\"type\":0},{\"gridX\":1640,\"gridY\":96,\"type\":2},{\"gridX\":1784,\"gridY\":96,\"type\":2},{\"gridX\":1844,\"gridY\":96,\"type\":0},{\"gridX\":1880,\"gridY\":96,\"type\":1},{\"gridX\":140,\"gridY\":96,\"type\":0},{\"gridX\":688,\"gridY\":96,\"type\":1},{\"gridX\":0,\"gridY\":96,\"type\":2}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":25},{\"gridX\":136,\"gridY\":0,\"type\":26},{\"gridX\":8,\"gridY\":0,\"type\":28},{\"gridX\":168,\"gridY\":0,\"type\":28},{\"gridX\":296,\"gridY\":0,\"type\":28},{\"gridX\":424,\"gridY\":0,\"type\":28},{\"gridX\":552,\"gridY\":0,\"type\":28},{\"gridX\":680,\"gridY\":0,\"type\":28},{\"gridX\":808,\"gridY\":0,\"type\":28},{\"gridX\":1576,\"gridY\":0,\"type\":28},{\"gridX\":1704,\"gridY\":0,\"type\":28},{\"gridX\":1832,\"gridY\":0,\"type\":28},{\"gridX\":936,\"gridY\":0,\"type\":28},{\"gridX\":1064,\"gridY\":0,\"type\":28},{\"gridX\":1192,\"gridY\":0,\"type\":28},{\"gridX\":1320,\"gridY\":0,\"type\":28},{\"gridX\":1448,\"gridY\":0,\"type\":28},{\"gridX\":1960,\"gridY\":0,\"type\":29}],\"pits\":[{\"gridX\":224,\"gridY\":0,\"type\":33},{\"gridX\":668,\"gridY\":0,\"type\":30},{\"gridX\":1258,\"gridY\":0,\"type\":33},{\"gridX\":728,\"gridY\":0,\"type\":33},{\"gridX\":1004,\"gridY\":0,\"type\":32},{\"gridX\":1180,\"gridY\":0,\"type\":30},{\"gridX\":544,\"gridY\":0,\"type\":30}],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1696,\"gridY\":96,\"type\":12},{\"gridX\":128,\"gridY\":104,\"type\":34},{\"gridX\":140,\"gridY\":116,\"type\":34},{\"gridX\":152,\"gridY\":128,\"type\":34},{\"gridX\":272,\"gridY\":120,\"type\":1},{\"gridX\":268,\"gridY\":120,\"type\":0},{\"gridX\":286,\"gridY\":120,\"type\":2},{\"gridX\":364,\"gridY\":156,\"type\":1},{\"gridX\":378,\"gridY\":156,\"type\":2},{\"gridX\":360,\"gridY\":156,\"type\":0},{\"gridX\":652,\"gridY\":96,\"type\":23},{\"gridX\":788,\"gridY\":136,\"type\":1},{\"gridX\":860,\"gridY\":192,\"type\":1},{\"gridX\":1064,\"gridY\":140,\"type\":1},{\"gridX\":1112,\"gridY\":216,\"type\":1},{\"gridX\":1028,\"gridY\":128,\"type\":1},{\"gridX\":1296,\"gridY\":204,\"type\":1},{\"gridX\":1315,\"gridY\":161,\"type\":1},{\"gridX\":1344,\"gridY\":188,\"type\":1},{\"gridX\":1358,\"gridY\":188,\"type\":1},{\"gridX\":1372,\"gridY\":188,\"type\":1},{\"gridX\":1432,\"gridY\":196,\"type\":1},{\"gridX\":1446,\"gridY\":196,\"type\":1},{\"gridX\":1460,\"gridY\":196,\"type\":1},{\"gridX\":1512,\"gridY\":196,\"type\":34},{\"gridX\":1512,\"gridY\":180,\"type\":34},{\"gridX\":1512,\"gridY\":164,\"type\":34},{\"gridX\":1512,\"gridY\":148,\"type\":34},{\"gridX\":1512,\"gridY\":132,\"type\":34},{\"gridX\":1512,\"gridY\":116,\"type\":34},{\"gridX\":916,\"gridY\":180,\"type\":1},{\"gridX\":874,\"gridY\":192,\"type\":1},{\"gridX\":876,\"gridY\":148,\"type\":1},{\"gridX\":872,\"gridY\":148,\"type\":0},{\"gridX\":856,\"gridY\":192,\"type\":0},{\"gridX\":888,\"gridY\":192,\"type\":2},{\"gridX\":912,\"gridY\":180,\"type\":0},{\"gridX\":930,\"gridY\":180,\"type\":2},{\"gridX\":802,\"gridY\":136,\"type\":2},{\"gridX\":784,\"gridY\":136,\"type\":0},{\"gridX\":1124,\"gridY\":124,\"type\":1},{\"gridX\":1148,\"gridY\":172,\"type\":1},{\"gridX\":1162,\"gridY\":172,\"type\":1},{\"gridX\":1144,\"gridY\":172,\"type\":0},{\"gridX\":1176,\"gridY\":172,\"type\":2},{\"gridX\":1024,\"gridY\":128,\"type\":0},{\"gridX\":1042,\"gridY\":128,\"type\":2},{\"gridX\":1292,\"gridY\":204,\"type\":0},{\"gridX\":1310,\"gridY\":204,\"type\":2},{\"gridX\":1340,\"gridY\":188,\"type\":0},{\"gridX\":1386,\"gridY\":188,\"type\":2},{\"gridX\":1428,\"gridY\":196,\"type\":0},{\"gridX\":1474,\"gridY\":196,\"type\":2},{\"gridX\":1060,\"gridY\":140,\"type\":0},{\"gridX\":1078,\"gridY\":140,\"type\":2},{\"gridX\":1120,\"gridY\":124,\"type\":0},{\"gridX\":1138,\"gridY\":124,\"type\":2},{\"gridX\":1108,\"gridY\":216,\"type\":0},{\"gridX\":1126,\"gridY\":216,\"type\":2},{\"gridX\":1396,\"gridY\":204,\"type\":34},{\"gridX\":1407,\"gridY\":216,\"type\":34},{\"gridX\":1512,\"gridY\":212,\"type\":34},{\"gridX\":890,\"gridY\":148,\"type\":2},{\"gridX\":204,\"gridY\":96,\"type\":23},{\"gridX\":1297,\"gridY\":161,\"type\":0},{\"gridX\":1301,\"gridY\":161,\"type\":1},{\"gridX\":1329,\"gridY\":161,\"type\":2},{\"gridX\":1512,\"gridY\":228,\"type\":34}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1664,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":184,\"gridY\":96,\"type\":6},{\"gridX\":268,\"gridY\":125,\"type\":6},{\"gridX\":484,\"gridY\":96,\"type\":6},{\"gridX\":712,\"gridY\":96,\"type\":6},{\"gridX\":788,\"gridY\":141,\"type\":6},{\"gridX\":912,\"gridY\":185,\"type\":6},{\"gridX\":876,\"gridY\":153,\"type\":6},{\"gridX\":984,\"gridY\":96,\"type\":6},{\"gridX\":1064,\"gridY\":145,\"type\":6},{\"gridX\":1108,\"gridY\":221,\"type\":6},{\"gridX\":1028,\"gridY\":133,\"type\":6},{\"gridX\":1124,\"gridY\":129,\"type\":6},{\"gridX\":1160,\"gridY\":177,\"type\":6},{\"gridX\":1240,\"gridY\":96,\"type\":6},{\"gridX\":1144,\"gridY\":177,\"type\":6},{\"gridX\":632,\"gridY\":96,\"type\":6},{\"gridX\":1528,\"gridY\":96,\"type\":6}],\"collectibles\":[{\"gridX\":456,\"gridY\":112,\"type\":0},{\"gridX\":464,\"gridY\":116,\"type\":0},{\"gridX\":480,\"gridY\":116,\"type\":0},{\"gridX\":680,\"gridY\":88,\"type\":1},{\"gridX\":884,\"gridY\":176,\"type\":1},{\"gridX\":876,\"gridY\":204,\"type\":0},{\"gridX\":952,\"gridY\":224,\"type\":0},{\"gridX\":956,\"gridY\":212,\"type\":0},{\"gridX\":960,\"gridY\":200,\"type\":0},{\"gridX\":964,\"gridY\":188,\"type\":0},{\"gridX\":968,\"gridY\":176,\"type\":0},{\"gridX\":868,\"gridY\":204,\"type\":0},{\"gridX\":1004,\"gridY\":152,\"type\":0},{\"gridX\":972,\"gridY\":164,\"type\":0},{\"gridX\":976,\"gridY\":152,\"type\":0},{\"gridX\":980,\"gridY\":140,\"type\":0},{\"gridX\":860,\"gridY\":204,\"type\":0},{\"gridX\":1036,\"gridY\":152,\"type\":0},{\"gridX\":1012,\"gridY\":160,\"type\":0},{\"gridX\":1020,\"gridY\":168,\"type\":0},{\"gridX\":1028,\"gridY\":176,\"type\":0},{\"gridX\":1060,\"gridY\":200,\"type\":0},{\"gridX\":1092,\"gridY\":216,\"type\":0},{\"gridX\":1084,\"gridY\":204,\"type\":0},{\"gridX\":132,\"gridY\":124,\"type\":0},{\"gridX\":140,\"gridY\":132,\"type\":0},{\"gridX\":148,\"gridY\":140,\"type\":0},{\"gridX\":124,\"gridY\":116,\"type\":0},{\"gridX\":180,\"gridY\":124,\"type\":0},{\"gridX\":172,\"gridY\":132,\"type\":0},{\"gridX\":164,\"gridY\":140,\"type\":0},{\"gridX\":156,\"gridY\":148,\"type\":0},{\"gridX\":204,\"gridY\":132,\"type\":0},{\"gridX\":212,\"gridY\":148,\"type\":0},{\"gridX\":188,\"gridY\":116,\"type\":0},{\"gridX\":208,\"gridY\":140,\"type\":0},{\"gridX\":368,\"gridY\":168,\"type\":0},{\"gridX\":360,\"gridY\":168,\"type\":0},{\"gridX\":488,\"gridY\":112,\"type\":0},{\"gridX\":472,\"gridY\":120,\"type\":0},{\"gridX\":580,\"gridY\":104,\"type\":0},{\"gridX\":568,\"gridY\":128,\"type\":0},{\"gridX\":576,\"gridY\":112,\"type\":0},{\"gridX\":572,\"gridY\":120,\"type\":0},{\"gridX\":748,\"gridY\":168,\"type\":0},{\"gridX\":752,\"gridY\":176,\"type\":0},{\"gridX\":732,\"gridY\":136,\"type\":0},{\"gridX\":736,\"gridY\":144,\"type\":0},{\"gridX\":740,\"gridY\":152,\"type\":0},{\"gridX\":1072,\"gridY\":168,\"type\":0},{\"gridX\":1076,\"gridY\":180,\"type\":0},{\"gridX\":1080,\"gridY\":192,\"type\":0},{\"gridX\":1096,\"gridY\":204,\"type\":0},{\"gridX\":1104,\"gridY\":192,\"type\":0},{\"gridX\":1112,\"gridY\":180,\"type\":0},{\"gridX\":1120,\"gridY\":168,\"type\":0},{\"gridX\":1128,\"gridY\":156,\"type\":0},{\"gridX\":1052,\"gridY\":184,\"type\":0},{\"gridX\":1056,\"gridY\":192,\"type\":0},{\"gridX\":1264,\"gridY\":156,\"t" << "ype\":1},{\"gridX\":1424,\"gridY\":224,\"type\":0},{\"gridX\":1353,\"gridY\":198,\"type\":0},{\"gridX\":1376,\"gridY\":198,\"type\":0},{\"gridX\":1432,\"gridY\":216,\"type\":0},{\"gridX\":1400,\"gridY\":224,\"type\":0},{\"gridX\":1392,\"gridY\":216,\"type\":0},{\"gridX\":1468,\"gridY\":208,\"type\":0},{\"gridX\":1460,\"gridY\":208,\"type\":0},{\"gridX\":1476,\"gridY\":208,\"type\":0},{\"gridX\":1556,\"gridY\":212,\"type\":0},{\"gridX\":1526,\"gridY\":244,\"type\":0},{\"gridX\":1532,\"gridY\":236,\"type\":0},{\"gridX\":1540,\"gridY\":228,\"type\":0},{\"gridX\":1548,\"gridY\":220,\"type\":0},{\"gridX\":1040,\"gridY\":160,\"type\":0},{\"gridX\":1384,\"gridY\":208,\"type\":0},{\"gridX\":1365,\"gridY\":198,\"type\":0},{\"gridX\":812,\"gridY\":196,\"type\":0},{\"gridX\":816,\"gridY\":204,\"type\":0},{\"gridX\":744,\"gridY\":160,\"type\":0},{\"gridX\":492,\"gridY\":124,\"type\":0},{\"gridX\":820,\"gridY\":212,\"type\":0},{\"gridX\":824,\"gridY\":220,\"type\":0},{\"gridX\":804,\"gridY\":180,\"type\":0},{\"gridX\":808,\"gridY\":188,\"type\":0},{\"gridX\":892,\"gridY\":212,\"type\":0},{\"gridX\":896,\"gridY\":220,\"type\":0},{\"gridX\":900,\"gridY\":228,\"type\":0},{\"gridX\":984,\"gridY\":128,\"type\":0},{\"gridX\":1564,\"gridY\":204,\"type\":0},{\"gridX\":1636,\"gridY\":100,\"type\":0},{\"gridX\":1644,\"gridY\":100,\"type\":0},{\"gridX\":1652,\"gridY\":100,\"type\":0},{\"gridX\":1640,\"gridY\":108,\"type\":0},{\"gridX\":1648,\"gridY\":108,\"type\":0},{\"gridX\":564,\"gridY\":136,\"type\":0},{\"gridX\":496,\"gridY\":132,\"type\":0},{\"gridX\":500,\"gridY\":140,\"type\":0},{\"gridX\":504,\"gridY\":148,\"type\":0},{\"gridX\":508,\"gridY\":156,\"type\":0},{\"gridX\":1032,\"gridY\":144,\"type\":0},{\"gridX\":376,\"gridY\":168,\"type\":0},{\"gridX\":1440,\"gridY\":208,\"type\":0}],\"jons\":[{\"gridX\":20,\"gridY\":100}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level9 *instance = new Chapter1Level9(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 11 ///

Chapter1Level11 * Chapter1Level11::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":11,\"midgrounds\":[{\"gridX\":2108,\"gridY\":96,\"type\":2},{\"gridX\":2164,\"gridY\":96,\"type\":0},{\"gridX\":2192,\"gridY\":96,\"type\":1},{\"gridX\":2228,\"gridY\":96,\"type\":1},{\"gridX\":2268,\"gridY\":96,\"type\":2},{\"gridX\":2324,\"gridY\":96,\"type\":0},{\"gridX\":2356,\"gridY\":96,\"type\":0},{\"gridX\":0,\"gridY\":96,\"type\":1},{\"gridX\":44,\"gridY\":96,\"type\":1},{\"gridX\":88,\"gridY\":96,\"type\":2},{\"gridX\":444,\"gridY\":96,\"type\":2},{\"gridX\":528,\"gridY\":96,\"type\":1},{\"gridX\":564,\"gridY\":96,\"type\":0},{\"gridX\":644,\"gridY\":96,\"type\":1},{\"gridX\":604,\"gridY\":96,\"type\":0},{\"gridX\":764,\"gridY\":96,\"type\":2},{\"gridX\":844,\"gridY\":96,\"type\":1},{\"gridX\":896,\"gridY\":96,\"type\":1},{\"gridX\":1180,\"gridY\":96,\"type\":2},{\"gridX\":1232,\"gridY\":96,\"type\":2},{\"gridX\":1292,\"gridY\":96,\"type\":0},{\"gridX\":1324,\"gridY\":96,\"type\":0},{\"gridX\":1356,\"gridY\":96,\"type\":0},{\"gridX\":1388,\"gridY\":96,\"type\":1},{\"gridX\":1428,\"gridY\":96,\"type\":2},{\"gridX\":1476,\"gridY\":96,\"type\":2},{\"gridX\":1532,\"gridY\":96,\"type\":1},{\"gridX\":1692,\"gridY\":96,\"type\":1},{\"gridX\":1728,\"gridY\":96,\"type\":0},{\"gridX\":1756,\"gridY\":96,\"type\":1},{\"gridX\":1804,\"gridY\":96,\"type\":0},{\"gridX\":1840,\"gridY\":96,\"type\":1},{\"gridX\":1888,\"gridY\":96,\"type\":0},{\"gridX\":1924,\"gridY\":96,\"type\":1},{\"gridX\":2020,\"gridY\":96,\"type\":2},{\"gridX\":2076,\"gridY\":96,\"type\":0},{\"gridX\":216,\"gridY\":96,\"type\":1},{\"gridX\":256,\"gridY\":96,\"type\":2},{\"gridX\":316,\"gridY\":96,\"type\":0},{\"gridX\":344,\"gridY\":96,\"type\":2}],\"grounds\":[{\"gridX\":4,\"gridY\":50,\"type\":10},{\"gridX\":488,\"gridY\":50,\"type\":10},{\"gridX\":1816,\"gridY\":48,\"type\":10},{\"gridX\":268,\"gridY\":50,\"type\":12},{\"gridX\":624,\"gridY\":50,\"type\":12},{\"gridX\":12,\"gridY\":50,\"type\":13},{\"gridX\":140,\"gridY\":50,\"type\":13},{\"gridX\":332,\"gridY\":50,\"type\":13},{\"gridX\":496,\"gridY\":50,\"type\":13},{\"gridX\":1824,\"gridY\":48,\"type\":13},{\"gridX\":1952,\"gridY\":48,\"type\":13},{\"gridX\":688,\"gridY\":50,\"type\":14},{\"gridX\":320,\"gridY\":48,\"type\":15},{\"gridX\":328,\"gridY\":48,\"type\":18},{\"gridX\":456,\"gridY\":48,\"type\":19},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":1180,\"gridY\":90,\"type\":20},{\"gridX\":1700,\"gridY\":90,\"type\":20},{\"gridX\":494,\"gridY\":90,\"type\":20},{\"gridX\":264,\"gridY\":90,\"type\":21},{\"gridX\":392,\"gridY\":90,\"type\":21},{\"gridX\":616,\"gridY\":90,\"type\":22},{\"gridX\":424,\"gridY\":90,\"type\":22},{\"gridX\":1964,\"gridY\":88,\"type\":22},{\"gridX\":488,\"gridY\":90,\"type\":22},{\"gridX\":328,\"gridY\":90,\"type\":22},{\"gridX\":552,\"gridY\":90,\"type\":22},{\"gridX\":840,\"gridY\":90,\"type\":23},{\"gridX\":1188,\"gridY\":90,\"type\":23},{\"gridX\":1316,\"gridY\":90,\"type\":23},{\"gridX\":1444,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":712,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1708,\"gridY\":90,\"type\":23},{\"gridX\":1836,\"gridY\":88,\"type\":23},{\"gridX\":968,\"gridY\":90,\"type\":24},{\"gridX\":1572,\"gridY\":90,\"type\":24},{\"gridX\":2052,\"gridY\":0,\"type\":25},{\"gridX\":2060,\"gridY\":0,\"type\":27},{\"gridX\":2124,\"gridY\":0,\"type\":28},{\"gridX\":2252,\"gridY\":0,\"type\":28},{\"gridX\":2380,\"gridY\":0,\"type\":29}],\"pits\":[],\"exitGrounds\":[{\"gridX\":296,\"gridY\":88,\"type\":0},{\"gridX\":680,\"gridY\":88,\"type\":0},{\"gridX\":2028,\"gridY\":88,\"type\":1}],\"holes\":[{\"gridX\":160,\"gridY\":80,\"type\":3},{\"gridX\":620,\"gridY\":80,\"type\":3},{\"gridX\":1844,\"gridY\":72,\"type\":0},{\"gridX\":1860,\"gridY\":72,\"type\":1},{\"gridX\":1876,\"gridY\":72,\"type\":1},{\"gridX\":1892,\"gridY\":72,\"type\":1},{\"gridX\":1908,\"gridY\":72,\"type\":1},{\"gridX\":1924,\"gridY\":72,\"type\":1},{\"gridX\":1940,\"gridY\":72,\"type\":1},{\"gridX\":1956,\"gridY\":72,\"type\":1},{\"gridX\":1972,\"gridY\":72,\"type\":2}],\"foregroundObjects\":[{\"gridX\":2116,\"gridY\":96,\"type\":12},{\"gridX\":428,\"gridY\":95,\"type\":35},{\"gridX\":600,\"gridY\":95,\"type\":35},{\"gridX\":486,\"gridY\":116,\"type\":0},{\"gridX\":504,\"gridY\":116,\"type\":1},{\"gridX\":518,\"gridY\":116,\"type\":2},{\"gridX\":690,\"gridY\":116,\"type\":0},{\"gridX\":708,\"gridY\":116,\"type\":1},{\"gridX\":722,\"gridY\":116,\"type\":1},{\"gridX\":736,\"gridY\":116,\"type\":2},{\"gridX\":908,\"gridY\":160,\"type\":0},{\"gridX\":912,\"gridY\":160,\"type\":1},{\"gridX\":926,\"gridY\":160,\"type\":2},{\"gridX\":910,\"gridY\":165,\"type\":17},{\"gridX\":956,\"gridY\":124,\"type\":0},{\"gridX\":960,\"gridY\":124,\"type\":1},{\"gridX\":974,\"gridY\":124,\"type\":2},{\"gridX\":1300,\"gridY\":95,\"type\":35},{\"gridX\":1348,\"gridY\":95,\"type\":35},{\"gridX\":1404,\"gridY\":95,\"type\":35},{\"gridX\":1596,\"gridY\":120,\"type\":0},{\"gridX\":1600,\"gridY\":120,\"type\":1},{\"gridX\":1614,\"gridY\":120,\"type\":2},{\"gridX\":1668,\"gridY\":120,\"type\":0},{\"gridX\":1672,\"gridY\":120,\"type\":1},{\"gridX\":1686,\"gridY\":120,\"type\":2},{\"gridX\":1781,\"gridY\":96,\"type\":24},{\"gridX\":1813,\"gridY\":96,\"type\":24},{\"gridX\":588,\"gridY\":140,\"type\":34},{\"gridX\":560,\"gridY\":140,\"type\":0},{\"gridX\":564,\"gridY\":140,\"type\":1},{\"gridX\":578,\"gridY\":140,\"type\":2},{\"gridX\":490,\"gridY\":116,\"type\":1},{\"gridX\":184,\"gridY\":96,\"type\":36},{\"gridX\":204,\"gridY\":55,\"type\":26},{\"gridX\":272,\"gridY\":55,\"type\":26},{\"gridX\":324,\"gridY\":75,\"type\":27},{\"gridX\":310,\"gridY\":55,\"type\":41},{\"gridX\":232,\"gridY\":84,\"type\":30},{\"gridX\":320,\"gridY\":96,\"type\":22},{\"gridX\":324,\"gridY\":96,\"type\":22},{\"gridX\":400,\"gridY\":96,\"type\":13},{\"gridX\":405,\"gridY\":96,\"type\":14},{\"gridX\":416,\"gridY\":96,\"type\":16},{\"gridX\":688,\"gridY\":55,\"type\":19},{\"gridX\":708,\"gridY\":96,\"type\":23},{\"gridX\":694,\"gridY\":116,\"type\":1},{\"gridX\":2040,\"gridY\":55,\"type\":41},{\"gridX\":1992,\"gridY\":96,\"type\":36}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":2084,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":496,\"gridY\":96,\"type\":10},{\"gridX\":568,\"gridY\":96,\"type\":10},{\"gridX\":804,\"gridY\":96,\"type\":10},{\"gridX\":812,\"gridY\":116,\"type\":3},{\"gridX\":844,\"gridY\":128,\"type\":3},{\"gridX\":876,\"gridY\":140,\"type\":3},{\"gridX\":1244,\"gridY\":96,\"type\":10},{\"gridX\":1556,\"gridY\":96,\"type\":10},{\"gridX\":1520,\"gridY\":116,\"type\":3},{\"gridX\":384,\"gridY\":96,\"type\":10},{\"gridX\":624,\"gridY\":55,\"type\":10},{\"gridX\":668,\"gridY\":55,\"type\":8},{\"gridX\":2076,\"gridY\":96,\"type\":5}],\"collectibles\":[{\"gridX\":392,\"gridY\":104,\"type\":0},{\"gridX\":396,\"gridY\":112,\"type\":0},{\"gridX\":400,\"gridY\":120,\"type\":0},{\"gridX\":404,\"gridY\":128,\"type\":0},{\"gridX\":408,\"gridY\":136,\"type\":0},{\"gridX\":412,\"gridY\":144,\"type\":0},{\"gridX\":452,\"gridY\":124,\"type\":0},{\"gridX\":460,\"gridY\":116,\"type\":0},{\"gridX\":488,\"gridY\":124,\"type\":0},{\"gridX\":496,\"gridY\":132,\"type\":0},{\"gridX\":504,\"gridY\":140,\"type\":0},{\"gridX\":512,\"gridY\":132,\"type\":0},{\"gridX\":520,\"gridY\":124,\"type\":0},{\"gridX\":600,\"gridY\":120,\"type\":1},{\"gridX\":716,\"gridY\":128,\"type\":0},{\"gridX\":724,\"gridY\":128,\"type\":0},{\"gridX\":732,\"gridY\":128,\"type\":0},{\"gridX\":748,\"gridY\":124,\"type\":0},{\"gridX\":756,\"gridY\":120,\"type\":0},{\"gridX\":764,\"gridY\":116,\"type\":0},{\"gridX\":900,\"gridY\":168,\"type\":1},{\"gridX\":872,\"gridY\":104,\"type\":0},{\"gridX\":888,\"gridY\":104,\"type\":0},{\"gridX\":880,\"gridY\":104,\"type\":0},{\"gridX\":772,\"gridY\":112,\"type\":0},{\"gridX\":936,\"gridY\":112,\"type\":0},{\"gridX\":932,\"gridY\":104,\"type\":0},{\"gridX\":940,\"gridY\":120,\"type\":0},{\"gridX\":944,\"gridY\":128,\"type\":" << "0},{\"gridX\":956,\"gridY\":136,\"type\":0},{\"gridX\":964,\"gridY\":136,\"type\":0},{\"gridX\":972,\"gridY\":136,\"type\":0},{\"gridX\":1004,\"gridY\":132,\"type\":0},{\"gridX\":1012,\"gridY\":132,\"type\":0},{\"gridX\":1020,\"gridY\":132,\"type\":0},{\"gridX\":1052,\"gridY\":128,\"type\":0},{\"gridX\":1060,\"gridY\":128,\"type\":0},{\"gridX\":1068,\"gridY\":128,\"type\":0},{\"gridX\":1096,\"gridY\":124,\"type\":0},{\"gridX\":1104,\"gridY\":124,\"type\":0},{\"gridX\":1112,\"gridY\":124,\"type\":0},{\"gridX\":1160,\"gridY\":120,\"type\":0},{\"gridX\":1152,\"gridY\":120,\"type\":0},{\"gridX\":1144,\"gridY\":120,\"type\":0},{\"gridX\":1232,\"gridY\":120,\"type\":0},{\"gridX\":1240,\"gridY\":116,\"type\":0},{\"gridX\":1248,\"gridY\":112,\"type\":0},{\"gridX\":1276,\"gridY\":104,\"type\":0},{\"gridX\":1280,\"gridY\":112,\"type\":0},{\"gridX\":1284,\"gridY\":120,\"type\":0},{\"gridX\":1288,\"gridY\":128,\"type\":0},{\"gridX\":1292,\"gridY\":136,\"type\":0},{\"gridX\":1316,\"gridY\":128,\"type\":0},{\"gridX\":1320,\"gridY\":120,\"type\":0},{\"gridX\":1324,\"gridY\":112,\"type\":0},{\"gridX\":1328,\"gridY\":104,\"type\":0},{\"gridX\":1348,\"gridY\":128,\"type\":0},{\"gridX\":1356,\"gridY\":128,\"type\":0},{\"gridX\":1404,\"gridY\":124,\"type\":0},{\"gridX\":1396,\"gridY\":124,\"type\":0},{\"gridX\":1388,\"gridY\":124,\"type\":0},{\"gridX\":1364,\"gridY\":128,\"type\":0},{\"gridX\":1420,\"gridY\":120,\"type\":0},{\"gridX\":1428,\"gridY\":120,\"type\":0},{\"gridX\":1436,\"gridY\":120,\"type\":0},{\"gridX\":1452,\"gridY\":116,\"type\":0},{\"gridX\":1460,\"gridY\":116,\"type\":0},{\"gridX\":1468,\"gridY\":116,\"type\":0},{\"gridX\":1576,\"gridY\":104,\"type\":0},{\"gridX\":1580,\"gridY\":112,\"type\":0},{\"gridX\":1584,\"gridY\":120,\"type\":0},{\"gridX\":1696,\"gridY\":120,\"type\":0},{\"gridX\":1704,\"gridY\":112,\"type\":0},{\"gridX\":1712,\"gridY\":104,\"type\":0},{\"gridX\":1784,\"gridY\":112,\"type\":0},{\"gridX\":1792,\"gridY\":112,\"type\":0},{\"gridX\":1800,\"gridY\":112,\"type\":0},{\"gridX\":1820,\"gridY\":108,\"type\":0},{\"gridX\":1828,\"gridY\":108,\"type\":0},{\"gridX\":1836,\"gridY\":108,\"type\":0},{\"gridX\":468,\"gridY\":108,\"type\":0},{\"gridX\":176,\"gridY\":64,\"type\":0},{\"gridX\":184,\"gridY\":64,\"type\":0},{\"gridX\":192,\"gridY\":64,\"type\":0},{\"gridX\":236,\"gridY\":64,\"type\":0},{\"gridX\":244,\"gridY\":64,\"type\":0},{\"gridX\":252,\"gridY\":64,\"type\":0},{\"gridX\":296,\"gridY\":64,\"type\":0},{\"gridX\":304,\"gridY\":64,\"type\":0},{\"gridX\":312,\"gridY\":64,\"type\":0},{\"gridX\":692,\"gridY\":80,\"type\":1},{\"gridX\":1848,\"gridY\":64,\"type\":0},{\"gridX\":1856,\"gridY\":64,\"type\":0},{\"gridX\":1864,\"gridY\":64,\"type\":0},{\"gridX\":1892,\"gridY\":64,\"type\":0},{\"gridX\":1900,\"gridY\":64,\"type\":0},{\"gridX\":1908,\"gridY\":64,\"type\":0},{\"gridX\":1936,\"gridY\":64,\"type\":0},{\"gridX\":1944,\"gridY\":64,\"type\":0},{\"gridX\":1952,\"gridY\":64,\"type\":0},{\"gridX\":1976,\"gridY\":64,\"type\":0},{\"gridX\":1984,\"gridY\":64,\"type\":0},{\"gridX\":1992,\"gridY\":64,\"type\":0}],\"jons\":[{\"gridX\":20,\"gridY\":100}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level11 *instance = new Chapter1Level11(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 12 ///

Chapter1Level12 * Chapter1Level12::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":12,\"midgrounds\":[{\"gridX\":1536,\"gridY\":96,\"type\":2},{\"gridX\":1596,\"gridY\":96,\"type\":2},{\"gridX\":1656,\"gridY\":96,\"type\":1},{\"gridX\":1700,\"gridY\":96,\"type\":0},{\"gridX\":1736,\"gridY\":96,\"type\":0},{\"gridX\":0,\"gridY\":96,\"type\":1},{\"gridX\":40,\"gridY\":96,\"type\":0},{\"gridX\":72,\"gridY\":96,\"type\":2},{\"gridX\":128,\"gridY\":96,\"type\":0},{\"gridX\":160,\"gridY\":96,\"type\":1},{\"gridX\":240,\"gridY\":96,\"type\":1},{\"gridX\":212,\"gridY\":96,\"type\":0},{\"gridX\":276,\"gridY\":96,\"type\":2},{\"gridX\":395,\"gridY\":96,\"type\":2},{\"gridX\":460,\"gridY\":96,\"type\":1},{\"gridX\":504,\"gridY\":96,\"type\":0},{\"gridX\":540,\"gridY\":96,\"type\":2},{\"gridX\":604,\"gridY\":96,\"type\":1},{\"gridX\":648,\"gridY\":96,\"type\":0},{\"gridX\":684,\"gridY\":96,\"type\":2},{\"gridX\":744,\"gridY\":96,\"type\":1},{\"gridX\":812,\"gridY\":96,\"type\":2},{\"gridX\":884,\"gridY\":96,\"type\":2},{\"gridX\":960,\"gridY\":96,\"type\":1},{\"gridX\":932,\"gridY\":96,\"type\":0},{\"gridX\":1028,\"gridY\":96,\"type\":2},{\"gridX\":1000,\"gridY\":96,\"type\":0},{\"gridX\":1116,\"gridY\":96,\"type\":2},{\"gridX\":1180,\"gridY\":96,\"type\":2},{\"gridX\":1236,\"gridY\":96,\"type\":0},{\"gridX\":1268,\"gridY\":96,\"type\":1},{\"gridX\":1308,\"gridY\":96,\"type\":2},{\"gridX\":1424,\"gridY\":96,\"type\":2},{\"gridX\":1488,\"gridY\":96,\"type\":1},{\"gridX\":1396,\"gridY\":96,\"type\":0},{\"gridX\":344,\"gridY\":96,\"type\":1}],\"grounds\":[{\"gridX\":552,\"gridY\":20,\"type\":5},{\"gridX\":560,\"gridY\":20,\"type\":6},{\"gridX\":624,\"gridY\":20,\"type\":7},{\"gridX\":720,\"gridY\":20,\"type\":7},{\"gridX\":784,\"gridY\":20,\"type\":9},{\"gridX\":0,\"gridY\":50,\"type\":10},{\"gridX\":8,\"gridY\":50,\"type\":13},{\"gridX\":264,\"gridY\":50,\"type\":13},{\"gridX\":136,\"gridY\":50,\"type\":13},{\"gridX\":392,\"gridY\":50,\"type\":13},{\"gridX\":520,\"gridY\":50,\"type\":14},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":264,\"gridY\":90,\"type\":21},{\"gridX\":872,\"gridY\":90,\"type\":21},{\"gridX\":488,\"gridY\":90,\"type\":21},{\"gridX\":328,\"gridY\":90,\"type\":22},{\"gridX\":808,\"gridY\":90,\"type\":22},{\"gridX\":424,\"gridY\":90,\"type\":22},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":648,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":1288,\"gridY\":90,\"type\":23},{\"gridX\":1416,\"gridY\":90,\"type\":23},{\"gridX\":1544,\"gridY\":90,\"type\":23},{\"gridX\":1672,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1800,\"gridY\":90,\"type\":24}],\"pits\":[],\"exitGrounds\":[{\"gridX\":296,\"gridY\":88,\"type\":0},{\"gridX\":592,\"gridY\":0,\"type\":3},{\"gridX\":688,\"gridY\":0,\"type\":3},{\"gridX\":776,\"gridY\":88,\"type\":0},{\"gridX\":392,\"gridY\":88,\"type\":0}],\"holes\":[{\"gridX\":144,\"gridY\":80,\"type\":3},{\"gridX\":348,\"gridY\":80,\"type\":3},{\"gridX\":512,\"gridY\":73,\"type\":0},{\"gridX\":528,\"gridY\":73,\"type\":1},{\"gridX\":544,\"gridY\":73,\"type\":1},{\"gridX\":560,\"gridY\":73,\"type\":2}],\"foregroundObjects\":[{\"gridX\":1536,\"gridY\":96,\"type\":12},{\"gridX\":116,\"gridY\":96,\"type\":41},{\"gridX\":200,\"gridY\":84,\"type\":30},{\"gridX\":232,\"gridY\":84,\"type\":30},{\"gridX\":232,\"gridY\":55,\"type\":27},{\"gridX\":264,\"gridY\":84,\"type\":30},{\"gridX\":264,\"gridY\":55,\"type\":27},{\"gridX\":324,\"gridY\":116,\"type\":0},{\"gridX\":300,\"gridY\":55,\"type\":20},{\"gridX\":328,\"gridY\":116,\"type\":1},{\"gridX\":342,\"gridY\":116,\"type\":2},{\"gridX\":320,\"gridY\":55,\"type\":26},{\"gridX\":772,\"gridY\":27,\"type\":20},{\"gridX\":396,\"gridY\":55,\"type\":19},{\"gridX\":1044,\"gridY\":244,\"type\":34},{\"gridX\":1044,\"gridY\":228,\"type\":34},{\"gridX\":1044,\"gridY\":212,\"type\":34},{\"gridX\":1044,\"gridY\":196,\"type\":34},{\"gridX\":1044,\"gridY\":180,\"type\":34},{\"gridX\":1044,\"gridY\":164,\"type\":34},{\"gridX\":956,\"gridY\":96,\"type\":10},{\"gridX\":860,\"gridY\":96,\"type\":6},{\"gridX\":1069,\"gridY\":96,\"type\":6},{\"gridX\":1373,\"gridY\":96,\"type\":6},{\"gridX\":1420,\"gridY\":116,\"type\":0},{\"gridX\":1424,\"gridY\":116,\"type\":1},{\"gridX\":1476,\"gridY\":116,\"type\":0},{\"gridX\":1480,\"gridY\":116,\"type\":1},{\"gridX\":1494,\"gridY\":116,\"type\":2},{\"gridX\":1438,\"gridY\":116,\"type\":2},{\"gridX\":576,\"gridY\":96,\"type\":13},{\"gridX\":581,\"gridY\":96,\"type\":15},{\"gridX\":603,\"gridY\":96,\"type\":15},{\"gridX\":625,\"gridY\":96,\"type\":15},{\"gridX\":647,\"gridY\":96,\"type\":15},{\"gridX\":669,\"gridY\":96,\"type\":15},{\"gridX\":691,\"gridY\":96,\"type\":15},{\"gridX\":713,\"gridY\":96,\"type\":15},{\"gridX\":735,\"gridY\":96,\"type\":15},{\"gridX\":757,\"gridY\":96,\"type\":16},{\"gridX\":372,\"gridY\":96,\"type\":13},{\"gridX\":377,\"gridY\":96,\"type\":14},{\"gridX\":388,\"gridY\":96,\"type\":16}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1504,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":196,\"gridY\":96,\"type\":5},{\"gridX\":329,\"gridY\":121,\"type\":6},{\"gridX\":592,\"gridY\":77,\"type\":7},{\"gridX\":688,\"gridY\":77,\"type\":7},{\"gridX\":448,\"gridY\":96,\"type\":5},{\"gridX\":504,\"gridY\":96,\"type\":5},{\"gridX\":840,\"gridY\":96,\"type\":5},{\"gridX\":912,\"gridY\":96,\"type\":5},{\"gridX\":1004,\"gridY\":96,\"type\":5},{\"gridX\":1148,\"gridY\":96,\"type\":9},{\"gridX\":1180,\"gridY\":96,\"type\":10},{\"gridX\":1232,\"gridY\":96,\"type\":10},{\"gridX\":1280,\"gridY\":96,\"type\":8},{\"gridX\":1312,\"gridY\":96,\"type\":8},{\"gridX\":1348,\"gridY\":96,\"type\":5},{\"gridX\":1456,\"gridY\":96,\"type\":5}],\"collectibles\":[{\"gridX\":208,\"gridY\":116,\"type\":0},{\"gridX\":216,\"gridY\":132,\"type\":0},{\"gridX\":212,\"gridY\":124,\"type\":0},{\"gridX\":240,\"gridY\":132,\"type\":0},{\"gridX\":244,\"gridY\":124,\"type\":0},{\"gridX\":248,\"gridY\":116,\"type\":0},{\"gridX\":228,\"gridY\":136,\"type\":0},{\"gridX\":276,\"gridY\":64,\"type\":1},{\"gridX\":348,\"gridY\":72,\"type\":0},{\"gridX\":356,\"gridY\":72,\"type\":0},{\"gridX\":348,\"gridY\":64,\"type\":0},{\"gridX\":356,\"gridY\":64,\"type\":0},{\"gridX\":292,\"gridY\":104,\"type\":0},{\"gridX\":284,\"gridY\":104,\"type\":0},{\"gridX\":276,\"gridY\":104,\"type\":0},{\"gridX\":336,\"gridY\":104,\"type\":0},{\"gridX\":344,\"gridY\":104,\"type\":0},{\"gridX\":352,\"gridY\":104,\"type\":0},{\"gridX\":120,\"gridY\":112,\"type\":0},{\"gridX\":124,\"gridY\":120,\"type\":0},{\"gridX\":128,\"gridY\":128,\"type\":0},{\"gridX\":132,\"gridY\":136,\"type\":0},{\"gridX\":136,\"gridY\":144,\"type\":0},{\"gridX\":140,\"gridY\":152,\"type\":0},{\"gridX\":144,\"gridY\":160,\"type\":0},{\"gridX\":148,\"gridY\":168,\"type\":0},{\"gridX\":512,\"gridY\":64,\"type\":0},{\"gridX\":528,\"gridY\":64,\"type\":0},{\"gridX\":520,\"gridY\":64,\"type\":0},{\"gridX\":536,\"gridY\":56,\"type\":0},{\"gridX\":536,\"gridY\":48,\"type\":0},{\"gridX\":536,\"gridY\":24,\"type\":1},{\"gridX\":560,\"gridY\":36,\"type\":0},{\"gridX\":568,\"gridY\":36,\"type\":0},{\"gridX\":576,\"gridY\":36,\"type\":0},{\"gridX\":588,\"gridY\":64,\"type\":0},{\"gridX\":596,\"gridY\":68,\"type\":0},{\"gridX\":604,\"gridY\":64,\"type\":0},{\"gridX\":684,\"gridY\":64,\"type\":0},{\"gridX\":692,\"gridY\":68,\"type\":0},{\"gridX\":700,\"gridY\":64,\"type\":0},{\"gridX\":644,\"gridY\":36,\"type\":0},{\"gridX\":652,\"gridY\":36,\"type\":0},{\"gridX\":660,\"gridY\":36,\"type\":0},{\"gridX\":780,\"gridY\":60,\"type\":0},{\"gridX\":784,\"gridY\":68,\"type\":0},{\"gridX\":788,\"gridY\":76,\"type\":0},{\"gridX\":800,\"gridY\":108,\"type\":0},{\"gridX\":804,\"gridY\":116,\"type\":0},{\"gridX\":808,\"gridY\":124,\"type\":0},{\"gridX\":1004,\"gridY\":164,\"type\":1},{\"gridX\":868,\"gridY\":152,\"type\":0},{\"gridX\":876,\"gridY\":156,\"type\":0},{\"gridX\":8" << "84,\"gridY\":152,\"type\":0},{\"gridX\":1020,\"gridY\":104,\"type\":0},{\"gridX\":1028,\"gridY\":104,\"type\":0},{\"gridX\":1036,\"gridY\":104,\"type\":0},{\"gridX\":1068,\"gridY\":136,\"type\":0},{\"gridX\":1076,\"gridY\":144,\"type\":0},{\"gridX\":1084,\"gridY\":152,\"type\":0},{\"gridX\":1092,\"gridY\":144,\"type\":0},{\"gridX\":1100,\"gridY\":136,\"type\":0},{\"gridX\":1116,\"gridY\":120,\"type\":0},{\"gridX\":1108,\"gridY\":128,\"type\":0},{\"gridX\":1124,\"gridY\":112,\"type\":0},{\"gridX\":1044,\"gridY\":112,\"type\":0},{\"gridX\":1052,\"gridY\":120,\"type\":0},{\"gridX\":1060,\"gridY\":128,\"type\":0},{\"gridX\":1232,\"gridY\":112,\"type\":0},{\"gridX\":1240,\"gridY\":112,\"type\":0},{\"gridX\":1184,\"gridY\":116,\"type\":0},{\"gridX\":1180,\"gridY\":112,\"type\":0},{\"gridX\":1184,\"gridY\":108,\"type\":0},{\"gridX\":1188,\"gridY\":112,\"type\":0},{\"gridX\":1236,\"gridY\":108,\"type\":0},{\"gridX\":1236,\"gridY\":116,\"type\":0},{\"gridX\":1264,\"gridY\":112,\"type\":0},{\"gridX\":1268,\"gridY\":108,\"type\":0},{\"gridX\":1272,\"gridY\":112,\"type\":0},{\"gridX\":1268,\"gridY\":116,\"type\":0},{\"gridX\":1308,\"gridY\":112,\"type\":0},{\"gridX\":1312,\"gridY\":108,\"type\":0},{\"gridX\":1316,\"gridY\":112,\"type\":0},{\"gridX\":1312,\"gridY\":116,\"type\":0},{\"gridX\":1388,\"gridY\":144,\"type\":0},{\"gridX\":1380,\"gridY\":148,\"type\":0},{\"gridX\":1388,\"gridY\":152,\"type\":0},{\"gridX\":1396,\"gridY\":148,\"type\":0},{\"gridX\":1420,\"gridY\":128,\"type\":0},{\"gridX\":1428,\"gridY\":128,\"type\":0},{\"gridX\":1436,\"gridY\":128,\"type\":0},{\"gridX\":1456,\"gridY\":112,\"type\":0},{\"gridX\":1464,\"gridY\":120,\"type\":0},{\"gridX\":1476,\"gridY\":128,\"type\":0},{\"gridX\":1448,\"gridY\":120,\"type\":0},{\"gridX\":1484,\"gridY\":128,\"type\":0},{\"gridX\":1492,\"gridY\":128,\"type\":0},{\"gridX\":1508,\"gridY\":124,\"type\":0},{\"gridX\":1516,\"gridY\":120,\"type\":0},{\"gridX\":1524,\"gridY\":116,\"type\":0},{\"gridX\":1500,\"gridY\":128,\"type\":0},{\"gridX\":536,\"gridY\":64,\"type\":0},{\"gridX\":812,\"gridY\":132,\"type\":0}],\"jons\":[{\"gridX\":12,\"gridY\":100}],\"extraForegroundObjects\":[],\"foregroundCoverObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level12 *instance = new Chapter1Level12(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 13 ///

Chapter1Level13 * Chapter1Level13::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":13,\"midgrounds\":[{\"gridX\":89,\"gridY\":96,\"type\":2},{\"gridX\":8,\"gridY\":96,\"type\":1},{\"gridX\":57,\"gridY\":96,\"type\":0},{\"gridX\":111,\"gridY\":96,\"type\":6},{\"gridX\":300,\"gridY\":96,\"type\":0},{\"gridX\":556,\"gridY\":0,\"type\":5},{\"gridX\":643,\"gridY\":0,\"type\":4},{\"gridX\":618,\"gridY\":0,\"type\":5},{\"gridX\":668,\"gridY\":0,\"type\":5},{\"gridX\":877,\"gridY\":0,\"type\":4},{\"gridX\":891,\"gridY\":0,\"type\":5},{\"gridX\":924,\"gridY\":0,\"type\":4},{\"gridX\":742,\"gridY\":96,\"type\":1},{\"gridX\":806,\"gridY\":96,\"type\":2},{\"gridX\":804,\"gridY\":96,\"type\":0},{\"gridX\":874,\"gridY\":96,\"type\":6},{\"gridX\":954,\"gridY\":96,\"type\":7},{\"gridX\":980,\"gridY\":96,\"type\":7},{\"gridX\":954,\"gridY\":108,\"type\":7},{\"gridX\":980,\"gridY\":108,\"type\":7},{\"gridX\":946,\"gridY\":133,\"type\":8},{\"gridX\":954,\"gridY\":120,\"type\":7},{\"gridX\":980,\"gridY\":120,\"type\":7}],\"grounds\":[{\"gridX\":364,\"gridY\":0,\"type\":0},{\"gridX\":975,\"gridY\":0,\"type\":0},{\"gridX\":1097,\"gridY\":0,\"type\":0},{\"gridX\":372,\"gridY\":0,\"type\":1},{\"gridX\":983,\"gridY\":0,\"type\":1},{\"gridX\":1105,\"gridY\":0,\"type\":1},{\"gridX\":134,\"gridY\":0,\"type\":3},{\"gridX\":262,\"gridY\":0,\"type\":4},{\"gridX\":404,\"gridY\":0,\"type\":4},{\"gridX\":1014,\"gridY\":0,\"type\":4},{\"gridX\":1137,\"gridY\":0,\"type\":4},{\"gridX\":555,\"gridY\":50,\"type\":10},{\"gridX\":859,\"gridY\":50,\"type\":10},{\"gridX\":1184,\"gridY\":50,\"type\":10},{\"gridX\":563,\"gridY\":50,\"type\":11},{\"gridX\":595,\"gridY\":50,\"type\":12},{\"gridX\":867,\"gridY\":50,\"type\":12},{\"gridX\":1192,\"gridY\":50,\"type\":13},{\"gridX\":931,\"gridY\":50,\"type\":14},{\"gridX\":644,\"gridY\":48,\"type\":15},{\"gridX\":1305,\"gridY\":48,\"type\":15},{\"gridX\":652,\"gridY\":48,\"type\":16},{\"gridX\":1313,\"gridY\":48,\"type\":17},{\"gridX\":684,\"gridY\":48,\"type\":19},{\"gridX\":296,\"gridY\":90,\"type\":20},{\"gridX\":728,\"gridY\":90,\"type\":20},{\"gridX\":304,\"gridY\":90,\"type\":21},{\"gridX\":736,\"gridY\":90,\"type\":23},{\"gridX\":864,\"gridY\":90,\"type\":23},{\"gridX\":992,\"gridY\":90,\"type\":23},{\"gridX\":336,\"gridY\":90,\"type\":24},{\"gridX\":1119,\"gridY\":90,\"type\":24},{\"gridX\":440,\"gridY\":0,\"type\":25},{\"gridX\":1366,\"gridY\":0,\"type\":25},{\"gridX\":1574,\"gridY\":0,\"type\":25},{\"gridX\":1646,\"gridY\":0,\"type\":25},{\"gridX\":1716,\"gridY\":0,\"type\":25},{\"gridX\":1804,\"gridY\":0,\"type\":25},{\"gridX\":2330,\"gridY\":0,\"type\":25},{\"gridX\":448,\"gridY\":0,\"type\":27},{\"gridX\":0,\"gridY\":0,\"type\":28},{\"gridX\":1374,\"gridY\":0,\"type\":28},{\"gridX\":2338,\"gridY\":0,\"type\":28},{\"gridX\":2466,\"gridY\":0,\"type\":28},{\"gridX\":512,\"gridY\":0,\"type\":29},{\"gridX\":1724,\"gridY\":0,\"type\":29},{\"gridX\":1502,\"gridY\":0,\"type\":29},{\"gridX\":1812,\"gridY\":0,\"type\":29},{\"gridX\":1582,\"gridY\":0,\"type\":29},{\"gridX\":128,\"gridY\":0,\"type\":29},{\"gridX\":1654,\"gridY\":0,\"type\":29},{\"gridX\":2594,\"gridY\":0,\"type\":29}],\"pits\":[],\"exitGrounds\":[],\"holes\":[{\"gridX\":876,\"gridY\":80,\"type\":3},{\"gridX\":875,\"gridY\":32,\"type\":4}],\"foregroundObjects\":[{\"gridX\":64,\"gridY\":96,\"type\":52},{\"gridX\":64,\"gridY\":112,\"type\":52},{\"gridX\":64,\"gridY\":128,\"type\":52},{\"gridX\":254,\"gridY\":7,\"type\":21},{\"gridX\":316,\"gridY\":96,\"type\":52},{\"gridX\":316,\"gridY\":112,\"type\":52},{\"gridX\":316,\"gridY\":128,\"type\":52},{\"gridX\":316,\"gridY\":144,\"type\":52},{\"gridX\":316,\"gridY\":160,\"type\":52},{\"gridX\":316,\"gridY\":176,\"type\":52},{\"gridX\":316,\"gridY\":192,\"type\":51},{\"gridX\":332,\"gridY\":196,\"type\":50},{\"gridX\":348,\"gridY\":196,\"type\":50},{\"gridX\":364,\"gridY\":196,\"type\":50},{\"gridX\":388,\"gridY\":7,\"type\":21},{\"gridX\":223,\"gridY\":7,\"type\":27},{\"gridX\":449,\"gridY\":95,\"type\":35},{\"gridX\":506,\"gridY\":95,\"type\":35},{\"gridX\":478,\"gridY\":95,\"type\":35},{\"gridX\":409,\"gridY\":217,\"type\":34},{\"gridX\":425,\"gridY\":230,\"type\":34},{\"gridX\":414,\"gridY\":199,\"type\":34},{\"gridX\":371,\"gridY\":7,\"type\":26},{\"gridX\":399,\"gridY\":7,\"type\":25},{\"gridX\":404,\"gridY\":7,\"type\":25},{\"gridX\":736,\"gridY\":95,\"type\":35},{\"gridX\":64,\"gridY\":144,\"type\":52},{\"gridX\":64,\"gridY\":160,\"type\":51},{\"gridX\":734,\"gridY\":21,\"type\":4},{\"gridX\":730,\"gridY\":21,\"type\":3},{\"gridX\":748,\"gridY\":21,\"type\":5},{\"gridX\":798,\"gridY\":42,\"type\":4},{\"gridX\":795,\"gridY\":42,\"type\":3},{\"gridX\":812,\"gridY\":42,\"type\":5},{\"gridX\":442,\"gridY\":240,\"type\":34},{\"gridX\":48,\"gridY\":172,\"type\":50},{\"gridX\":32,\"gridY\":172,\"type\":50},{\"gridX\":16,\"gridY\":172,\"type\":50},{\"gridX\":0,\"gridY\":172,\"type\":50},{\"gridX\":872,\"gridY\":26,\"type\":4},{\"gridX\":869,\"gridY\":26,\"type\":3},{\"gridX\":942,\"gridY\":26,\"type\":5},{\"gridX\":886,\"gridY\":26,\"type\":4},{\"gridX\":900,\"gridY\":26,\"type\":4},{\"gridX\":914,\"gridY\":26,\"type\":4},{\"gridX\":928,\"gridY\":26,\"type\":4},{\"gridX\":916,\"gridY\":96,\"type\":24},{\"gridX\":948,\"gridY\":96,\"type\":24},{\"gridX\":980,\"gridY\":96,\"type\":24},{\"gridX\":1012,\"gridY\":96,\"type\":24},{\"gridX\":1044,\"gridY\":96,\"type\":24},{\"gridX\":942,\"gridY\":130,\"type\":47},{\"gridX\":946,\"gridY\":130,\"type\":48},{\"gridX\":962,\"gridY\":130,\"type\":48},{\"gridX\":978,\"gridY\":130,\"type\":48},{\"gridX\":994,\"gridY\":130,\"type\":49},{\"gridX\":1820,\"gridY\":92,\"type\":50},{\"gridX\":1852,\"gridY\":92,\"type\":50},{\"gridX\":1836,\"gridY\":92,\"type\":50},{\"gridX\":1868,\"gridY\":92,\"type\":50},{\"gridX\":1884,\"gridY\":92,\"type\":50},{\"gridX\":1900,\"gridY\":92,\"type\":50},{\"gridX\":1916,\"gridY\":92,\"type\":50},{\"gridX\":2052,\"gridY\":0,\"type\":52},{\"gridX\":2052,\"gridY\":16,\"type\":52},{\"gridX\":2052,\"gridY\":32,\"type\":52},{\"gridX\":2052,\"gridY\":48,\"type\":52},{\"gridX\":2052,\"gridY\":64,\"type\":51},{\"gridX\":2378,\"gridY\":96,\"type\":12},{\"gridX\":2068,\"gridY\":76,\"type\":50},{\"gridX\":2084,\"gridY\":76,\"type\":50},{\"gridX\":2100,\"gridY\":76,\"type\":50},{\"gridX\":2116,\"gridY\":76,\"type\":50},{\"gridX\":2132,\"gridY\":76,\"type\":50},{\"gridX\":2148,\"gridY\":76,\"type\":50},{\"gridX\":2164,\"gridY\":0,\"type\":52},{\"gridX\":2164,\"gridY\":16,\"type\":52},{\"gridX\":2164,\"gridY\":32,\"type\":52},{\"gridX\":2164,\"gridY\":48,\"type\":52},{\"gridX\":2164,\"gridY\":64,\"type\":51},{\"gridX\":2052,\"gridY\":104,\"type\":52},{\"gridX\":2052,\"gridY\":120,\"type\":51},{\"gridX\":2068,\"gridY\":132,\"type\":50},{\"gridX\":2084,\"gridY\":132,\"type\":50},{\"gridX\":2100,\"gridY\":132,\"type\":50},{\"gridX\":2116,\"gridY\":132,\"type\":50},{\"gridX\":2132,\"gridY\":132,\"type\":50},{\"gridX\":2148,\"gridY\":132,\"type\":50},{\"gridX\":2164,\"gridY\":120,\"type\":51},{\"gridX\":2164,\"gridY\":104,\"type\":52},{\"gridX\":2036,\"gridY\":119,\"type\":50},{\"gridX\":2020,\"gridY\":119,\"type\":50}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":2339,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":841,\"gridY\":96,\"type\":4},{\"gridX\":1049,\"gridY\":115,\"type\":3},{\"gridX\":619,\"gridY\":55,\"type\":9},{\"gridX\":986,\"gridY\":133,\"type\":11},{\"gridX\":2241,\"gridY\":84,\"type\":3},{\"gridX\":2290,\"gridY\":93,\"type\":3}],\"collectibles\":[{\"gridX\":107,\"gridY\":99,\"type\":1},{\"gridX\":802,\"gridY\":50,\"type\":1},{\"gridX\":739,\"gridY\":32,\"type\":0},{\"gridX\":388,\"gridY\":184,\"type\":0},{\"gridX\":388,\"gridY\":176,\"type\":0},{\"gridX\":388,\"gridY\":168,\"type\":0},{\"gridX\":388,\"gridY\":160,\"type\":0},{\"gridX\":388,\"gridY\":152,\"type\":0},{\"gridX\":388,\"gridY\":144,\"type\":0},{\"gridX\":388,\"gridY\":136,\"type\":0},{\"gridX\":152,\"gridY\":12,\"type\":0},{\"gridX\":160,\"gridY\":12,\"type\":0},{\"gridX\":168,\"gridY\":12,\"type\":0},{\"gridX" << "\":176,\"gridY\":12,\"type\":0},{\"gridX\":880,\"gridY\":59,\"type\":0},{\"gridX\":879,\"gridY\":35,\"type\":0},{\"gridX\":945,\"gridY\":36,\"type\":0},{\"gridX\":888,\"gridY\":35,\"type\":0},{\"gridX\":900,\"gridY\":36,\"type\":0},{\"gridX\":911,\"gridY\":35,\"type\":0},{\"gridX\":922,\"gridY\":35,\"type\":0},{\"gridX\":934,\"gridY\":36,\"type\":0},{\"gridX\":870,\"gridY\":59,\"type\":0},{\"gridX\":2112,\"gridY\":84,\"type\":1},{\"gridX\":1014,\"gridY\":13,\"type\":0},{\"gridX\":1025,\"gridY\":32,\"type\":0},{\"gridX\":1038,\"gridY\":45,\"type\":0},{\"gridX\":1055,\"gridY\":54,\"type\":0},{\"gridX\":1072,\"gridY\":54,\"type\":0},{\"gridX\":1080,\"gridY\":46,\"type\":0},{\"gridX\":1089,\"gridY\":31,\"type\":0},{\"gridX\":1099,\"gridY\":16,\"type\":0},{\"gridX\":1232,\"gridY\":60,\"type\":0},{\"gridX\":1246,\"gridY\":62,\"type\":0},{\"gridX\":1261,\"gridY\":62,\"type\":0},{\"gridX\":1274,\"gridY\":69,\"type\":0},{\"gridX\":1308,\"gridY\":85,\"type\":0},{\"gridX\":1323,\"gridY\":89,\"type\":0},{\"gridX\":1341,\"gridY\":91,\"type\":0},{\"gridX\":1373,\"gridY\":105,\"type\":0},{\"gridX\":1388,\"gridY\":106,\"type\":0},{\"gridX\":1404,\"gridY\":105,\"type\":0},{\"gridX\":1423,\"gridY\":108,\"type\":0},{\"gridX\":1443,\"gridY\":107,\"type\":0},{\"gridX\":1460,\"gridY\":107,\"type\":0},{\"gridX\":1488,\"gridY\":107,\"type\":0},{\"gridX\":1534,\"gridY\":134,\"type\":0},{\"gridX\":1543,\"gridY\":142,\"type\":0},{\"gridX\":1544,\"gridY\":135,\"type\":0},{\"gridX\":1608,\"gridY\":128,\"type\":0},{\"gridX\":1616,\"gridY\":139,\"type\":0},{\"gridX\":1619,\"gridY\":135,\"type\":0},{\"gridX\":1683,\"gridY\":130,\"type\":0},{\"gridX\":1689,\"gridY\":141,\"type\":0},{\"gridX\":1693,\"gridY\":134,\"type\":0},{\"gridX\":1759,\"gridY\":123,\"type\":0},{\"gridX\":1767,\"gridY\":133,\"type\":0},{\"gridX\":1770,\"gridY\":128,\"type\":0}],\"jons\":[{\"gridX\":29,\"gridY\":181}],\"extraForegroundObjects\":[],\"foregroundCoverObjects\":[{\"gridX\":44,\"gridY\":92,\"type\":2},{\"gridX\":93,\"gridY\":92,\"type\":1},{\"gridX\":94,\"gridY\":92,\"type\":3},{\"gridX\":304,\"gridY\":85,\"type\":0},{\"gridX\":844,\"gridY\":92,\"type\":2},{\"gridX\":895,\"gridY\":92,\"type\":3},{\"gridX\":747,\"gridY\":85,\"type\":0},{\"gridX\":994,\"gridY\":92,\"type\":1},{\"gridX\":969,\"gridY\":92,\"type\":3}],\"markers\":[{\"gridX\":380,\"gridY\":0,\"type\":0}]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level13 *instance = new Chapter1Level13(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 14 ///

Chapter1Level14 * Chapter1Level14::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":14,\"midgrounds\":[{\"gridX\":0,\"gridY\":96,\"type\":0},{\"gridX\":24,\"gridY\":96,\"type\":2},{\"gridX\":80,\"gridY\":96,\"type\":1},{\"gridX\":1396,\"gridY\":96,\"type\":2},{\"gridX\":1464,\"gridY\":96,\"type\":1},{\"gridX\":1372,\"gridY\":96,\"type\":0},{\"gridX\":1556,\"gridY\":96,\"type\":1},{\"gridX\":1508,\"gridY\":96,\"type\":2},{\"gridX\":1604,\"gridY\":96,\"type\":0},{\"gridX\":116,\"gridY\":96,\"type\":2},{\"gridX\":172,\"gridY\":96,\"type\":1},{\"gridX\":204,\"gridY\":96,\"type\":2},{\"gridX\":280,\"gridY\":96,\"type\":2},{\"gridX\":336,\"gridY\":96,\"type\":0},{\"gridX\":388,\"gridY\":96,\"type\":0},{\"gridX\":432,\"gridY\":96,\"type\":1},{\"gridX\":600,\"gridY\":96,\"type\":2},{\"gridX\":552,\"gridY\":96,\"type\":1},{\"gridX\":524,\"gridY\":96,\"type\":0},{\"gridX\":488,\"gridY\":96,\"type\":1},{\"gridX\":888,\"gridY\":96,\"type\":2},{\"gridX\":948,\"gridY\":96,\"type\":1},{\"gridX\":1128,\"gridY\":96,\"type\":2},{\"gridX\":1328,\"gridY\":96,\"type\":1},{\"gridX\":1096,\"gridY\":96,\"type\":1}],\"grounds\":[{\"gridX\":48,\"gridY\":50,\"type\":10},{\"gridX\":312,\"gridY\":50,\"type\":12},{\"gridX\":56,\"gridY\":50,\"type\":13},{\"gridX\":184,\"gridY\":50,\"type\":13},{\"gridX\":376,\"gridY\":50,\"type\":14},{\"gridX\":256,\"gridY\":48,\"type\":15},{\"gridX\":264,\"gridY\":48,\"type\":16},{\"gridX\":296,\"gridY\":48,\"type\":19},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":896,\"gridY\":90,\"type\":20},{\"gridX\":200,\"gridY\":90,\"type\":21},{\"gridX\":328,\"gridY\":90,\"type\":21},{\"gridX\":136,\"gridY\":90,\"type\":22},{\"gridX\":264,\"gridY\":90,\"type\":22},{\"gridX\":1288,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":1416,\"gridY\":90,\"type\":23},{\"gridX\":1544,\"gridY\":90,\"type\":23},{\"gridX\":392,\"gridY\":90,\"type\":23},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":1672,\"gridY\":90,\"type\":24},{\"gridX\":648,\"gridY\":90,\"type\":24}],\"pits\":[],\"exitGrounds\":[{\"gridX\":232,\"gridY\":88,\"type\":0},{\"gridX\":360,\"gridY\":88,\"type\":0}],\"holes\":[{\"gridX\":120,\"gridY\":80,\"type\":3},{\"gridX\":312,\"gridY\":80,\"type\":3}],\"foregroundObjects\":[{\"gridX\":1408,\"gridY\":96,\"type\":12},{\"gridX\":144,\"gridY\":96,\"type\":13},{\"gridX\":149,\"gridY\":96,\"type\":15},{\"gridX\":171,\"gridY\":96,\"type\":15},{\"gridX\":193,\"gridY\":96,\"type\":16},{\"gridX\":184,\"gridY\":55,\"type\":26},{\"gridX\":260,\"gridY\":75,\"type\":27},{\"gridX\":246,\"gridY\":55,\"type\":41},{\"gridX\":256,\"gridY\":96,\"type\":22},{\"gridX\":260,\"gridY\":96,\"type\":22},{\"gridX\":332,\"gridY\":96,\"type\":10},{\"gridX\":372,\"gridY\":55,\"type\":21},{\"gridX\":416,\"gridY\":224,\"type\":0},{\"gridX\":420,\"gridY\":224,\"type\":1},{\"gridX\":434,\"gridY\":224,\"type\":2},{\"gridX\":472,\"gridY\":204,\"type\":0},{\"gridX\":476,\"gridY\":204,\"type\":1},{\"gridX\":556,\"gridY\":124,\"type\":34},{\"gridX\":572,\"gridY\":120,\"type\":34},{\"gridX\":588,\"gridY\":120,\"type\":34},{\"gridX\":904,\"gridY\":96,\"type\":24},{\"gridX\":888,\"gridY\":100,\"type\":34},{\"gridX\":996,\"gridY\":95,\"type\":35},{\"gridX\":1036,\"gridY\":95,\"type\":35},{\"gridX\":1076,\"gridY\":95,\"type\":35},{\"gridX\":1188,\"gridY\":132,\"type\":0},{\"gridX\":1192,\"gridY\":132,\"type\":1},{\"gridX\":1206,\"gridY\":132,\"type\":2}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1376,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":208,\"gridY\":77,\"type\":7},{\"gridX\":276,\"gridY\":96,\"type\":5},{\"gridX\":208,\"gridY\":55,\"type\":11},{\"gridX\":300,\"gridY\":96,\"type\":5},{\"gridX\":472,\"gridY\":209,\"type\":6},{\"gridX\":416,\"gridY\":96,\"type\":5},{\"gridX\":516,\"gridY\":96,\"type\":11},{\"gridX\":608,\"gridY\":124,\"type\":3},{\"gridX\":688,\"gridY\":124,\"type\":3},{\"gridX\":768,\"gridY\":124,\"type\":3},{\"gridX\":848,\"gridY\":124,\"type\":3},{\"gridX\":1016,\"gridY\":96,\"type\":5},{\"gridX\":1056,\"gridY\":96,\"type\":5}],\"collectibles\":[{\"gridX\":160,\"gridY\":64,\"type\":0},{\"gridX\":144,\"gridY\":64,\"type\":0},{\"gridX\":152,\"gridY\":64,\"type\":0},{\"gridX\":184,\"gridY\":72,\"type\":0},{\"gridX\":192,\"gridY\":72,\"type\":0},{\"gridX\":200,\"gridY\":72,\"type\":0},{\"gridX\":256,\"gridY\":104,\"type\":0},{\"gridX\":260,\"gridY\":112,\"type\":0},{\"gridX\":264,\"gridY\":120,\"type\":0},{\"gridX\":316,\"gridY\":80,\"type\":0},{\"gridX\":316,\"gridY\":72,\"type\":0},{\"gridX\":124,\"gridY\":72,\"type\":0},{\"gridX\":124,\"gridY\":80,\"type\":0},{\"gridX\":124,\"gridY\":64,\"type\":0},{\"gridX\":316,\"gridY\":64,\"type\":0},{\"gridX\":276,\"gridY\":108,\"type\":0},{\"gridX\":284,\"gridY\":112,\"type\":0},{\"gridX\":292,\"gridY\":116,\"type\":0},{\"gridX\":372,\"gridY\":64,\"type\":0},{\"gridX\":372,\"gridY\":72,\"type\":0},{\"gridX\":372,\"gridY\":80,\"type\":0},{\"gridX\":416,\"gridY\":236,\"type\":0},{\"gridX\":424,\"gridY\":236,\"type\":0},{\"gridX\":432,\"gridY\":236,\"type\":0},{\"gridX\":444,\"gridY\":232,\"type\":0},{\"gridX\":452,\"gridY\":224,\"type\":0},{\"gridX\":460,\"gridY\":216,\"type\":0},{\"gridX\":316,\"gridY\":108,\"type\":0},{\"gridX\":428,\"gridY\":128,\"type\":0},{\"gridX\":432,\"gridY\":136,\"type\":0},{\"gridX\":436,\"gridY\":144,\"type\":0},{\"gridX\":476,\"gridY\":136,\"type\":0},{\"gridX\":376,\"gridY\":104,\"type\":0},{\"gridX\":380,\"gridY\":120,\"type\":0},{\"gridX\":384,\"gridY\":136,\"type\":0},{\"gridX\":388,\"gridY\":152,\"type\":0},{\"gridX\":392,\"gridY\":168,\"type\":0},{\"gridX\":396,\"gridY\":184,\"type\":0},{\"gridX\":400,\"gridY\":200,\"type\":0},{\"gridX\":404,\"gridY\":216,\"type\":0},{\"gridX\":460,\"gridY\":200,\"type\":1},{\"gridX\":424,\"gridY\":120,\"type\":0},{\"gridX\":472,\"gridY\":144,\"type\":0},{\"gridX\":480,\"gridY\":128,\"type\":0},{\"gridX\":316,\"gridY\":100,\"type\":0},{\"gridX\":316,\"gridY\":116,\"type\":0},{\"gridX\":540,\"gridY\":140,\"type\":0},{\"gridX\":532,\"gridY\":140,\"type\":0},{\"gridX\":548,\"gridY\":140,\"type\":0},{\"gridX\":592,\"gridY\":140,\"type\":1},{\"gridX\":336,\"gridY\":64,\"type\":0},{\"gridX\":344,\"gridY\":64,\"type\":0},{\"gridX\":352,\"gridY\":64,\"type\":0},{\"gridX\":628,\"gridY\":144,\"type\":0},{\"gridX\":636,\"gridY\":144,\"type\":0},{\"gridX\":664,\"gridY\":144,\"type\":0},{\"gridX\":620,\"gridY\":144,\"type\":0},{\"gridX\":672,\"gridY\":144,\"type\":0},{\"gridX\":680,\"gridY\":144,\"type\":0},{\"gridX\":716,\"gridY\":144,\"type\":0},{\"gridX\":700,\"gridY\":144,\"type\":0},{\"gridX\":708,\"gridY\":144,\"type\":0},{\"gridX\":752,\"gridY\":144,\"type\":0},{\"gridX\":760,\"gridY\":144,\"type\":0},{\"gridX\":744,\"gridY\":144,\"type\":0},{\"gridX\":796,\"gridY\":144,\"type\":0},{\"gridX\":788,\"gridY\":144,\"type\":0},{\"gridX\":780,\"gridY\":144,\"type\":0},{\"gridX\":840,\"gridY\":144,\"type\":0},{\"gridX\":824,\"gridY\":144,\"type\":0},{\"gridX\":832,\"gridY\":144,\"type\":0},{\"gridX\":1000,\"gridY\":132,\"type\":0},{\"gridX\":1008,\"gridY\":124,\"type\":0},{\"gridX\":1016,\"gridY\":116,\"type\":0},{\"gridX\":1032,\"gridY\":132,\"type\":0},{\"gridX\":1024,\"gridY\":124,\"type\":0},{\"gridX\":1048,\"gridY\":124,\"type\":0},{\"gridX\":1040,\"gridY\":132,\"type\":0},{\"gridX\":1056,\"gridY\":116,\"type\":0},{\"gridX\":1064,\"gridY\":124,\"type\":0},{\"gridX\":1072,\"gridY\":132,\"type\":0},{\"gridX\":1080,\"gridY\":132,\"type\":0},{\"gridX\":1088,\"gridY\":124,\"type\":0},{\"gridX\":1096,\"gridY\":116,\"type\":0},{\"gridX\":860,\"gridY\":144,\"type\":0},{\"gridX\":868,\"gridY\":144,\"type\":0},{\"gridX\":876,\"gridY\":144,\"type\":0},{\"gridX\":948,\"gridY\":108,\"type\":0},{\"gridX\":940,\"gridY\":108,\"type\":0},{\"gridX\":956,\"gridY\":108,\"type\":0},{\"gridX\":984,\"gridY\":116,\"type\":0},{\"gridX\":992,\"gridY\":124,\"type\":0},{\"gridX\":1288,\"gridY\":128,\"type\":1},{\"gridX\":1124,\"g" << "ridY\":104,\"type\":0},{\"gridX\":1132,\"gridY\":104,\"type\":0},{\"gridX\":1140,\"gridY\":104,\"type\":0},{\"gridX\":1196,\"gridY\":144,\"type\":0},{\"gridX\":1204,\"gridY\":144,\"type\":0},{\"gridX\":1188,\"gridY\":144,\"type\":0},{\"gridX\":1324,\"gridY\":112,\"type\":0},{\"gridX\":1316,\"gridY\":128,\"type\":0},{\"gridX\":1320,\"gridY\":120,\"type\":0},{\"gridX\":1328,\"gridY\":104,\"type\":0}],\"jons\":[{\"gridX\":16,\"gridY\":96}],\"extraForegroundObjects\":[{\"gridX\":1268,\"gridY\":92,\"type\":39}],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level14 *instance = new Chapter1Level14(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 15 ///

Chapter1Level15 * Chapter1Level15::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":15,\"midgrounds\":[{\"gridX\":1800,\"gridY\":96,\"type\":2},{\"gridX\":1856,\"gridY\":96,\"type\":2},{\"gridX\":1912,\"gridY\":96,\"type\":1},{\"gridX\":1956,\"gridY\":96,\"type\":0},{\"gridX\":1988,\"gridY\":96,\"type\":1},{\"gridX\":2028,\"gridY\":96,\"type\":0},{\"gridX\":0,\"gridY\":96,\"type\":1},{\"gridX\":40,\"gridY\":96,\"type\":0},{\"gridX\":68,\"gridY\":96,\"type\":2},{\"gridX\":268,\"gridY\":0,\"type\":3},{\"gridX\":176,\"gridY\":96,\"type\":2},{\"gridX\":148,\"gridY\":96,\"type\":0},{\"gridX\":232,\"gridY\":96,\"type\":1},{\"gridX\":272,\"gridY\":96,\"type\":0},{\"gridX\":492,\"gridY\":96,\"type\":0},{\"gridX\":548,\"gridY\":96,\"type\":0},{\"gridX\":1324,\"gridY\":0,\"type\":3},{\"gridX\":1372,\"gridY\":0,\"type\":3},{\"gridX\":1420,\"gridY\":0,\"type\":3},{\"gridX\":1468,\"gridY\":0,\"type\":3},{\"gridX\":1616,\"gridY\":0,\"type\":3},{\"gridX\":1680,\"gridY\":0,\"type\":3},{\"gridX\":1760,\"gridY\":96,\"type\":1},{\"gridX\":616,\"gridY\":96,\"type\":2},{\"gridX\":684,\"gridY\":96,\"type\":1},{\"gridX\":756,\"gridY\":96,\"type\":2},{\"gridX\":808,\"gridY\":96,\"type\":1},{\"gridX\":848,\"gridY\":96,\"type\":2},{\"gridX\":896,\"gridY\":96,\"type\":2},{\"gridX\":976,\"gridY\":96,\"type\":1},{\"gridX\":1020,\"gridY\":96,\"type\":1},{\"gridX\":1072,\"gridY\":96,\"type\":1},{\"gridX\":1108,\"gridY\":96,\"type\":1},{\"gridX\":1156,\"gridY\":96,\"type\":1},{\"gridX\":1212,\"gridY\":96,\"type\":2}],\"grounds\":[{\"gridX\":276,\"gridY\":0,\"type\":0},{\"gridX\":284,\"gridY\":0,\"type\":1},{\"gridX\":316,\"gridY\":0,\"type\":4},{\"gridX\":1444,\"gridY\":20,\"type\":5},{\"gridX\":1548,\"gridY\":20,\"type\":6},{\"gridX\":1452,\"gridY\":20,\"type\":7},{\"gridX\":1580,\"gridY\":20,\"type\":9},{\"gridX\":0,\"gridY\":50,\"type\":10},{\"gridX\":264,\"gridY\":50,\"type\":11},{\"gridX\":8,\"gridY\":50,\"type\":13},{\"gridX\":136,\"gridY\":50,\"type\":13},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":264,\"gridY\":90,\"type\":21},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":776,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":648,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":1288,\"gridY\":90,\"type\":24},{\"gridX\":320,\"gridY\":0,\"type\":25},{\"gridX\":1760,\"gridY\":0,\"type\":25},{\"gridX\":1768,\"gridY\":0,\"type\":26},{\"gridX\":456,\"gridY\":0,\"type\":27},{\"gridX\":328,\"gridY\":0,\"type\":28},{\"gridX\":1800,\"gridY\":0,\"type\":28},{\"gridX\":1928,\"gridY\":0,\"type\":28},{\"gridX\":2056,\"gridY\":0,\"type\":29}],\"pits\":[],\"exitGrounds\":[{\"gridX\":296,\"gridY\":48,\"type\":2},{\"gridX\":296,\"gridY\":88,\"type\":1},{\"gridX\":1516,\"gridY\":0,\"type\":3}],\"holes\":[{\"gridX\":224,\"gridY\":80,\"type\":3},{\"gridX\":276,\"gridY\":32,\"type\":4}],\"foregroundObjects\":[{\"gridX\":1792,\"gridY\":96,\"type\":12},{\"gridX\":128,\"gridY\":116,\"type\":0},{\"gridX\":132,\"gridY\":116,\"type\":1},{\"gridX\":146,\"gridY\":116,\"type\":2},{\"gridX\":180,\"gridY\":140,\"type\":0},{\"gridX\":184,\"gridY\":140,\"type\":1},{\"gridX\":198,\"gridY\":140,\"type\":2},{\"gridX\":184,\"gridY\":95,\"type\":35},{\"gridX\":244,\"gridY\":55,\"type\":27},{\"gridX\":244,\"gridY\":84,\"type\":30},{\"gridX\":308,\"gridY\":7,\"type\":21},{\"gridX\":296,\"gridY\":76,\"type\":34},{\"gridX\":296,\"gridY\":60,\"type\":34},{\"gridX\":320,\"gridY\":96,\"type\":6},{\"gridX\":424,\"gridY\":96,\"type\":6},{\"gridX\":452,\"gridY\":148,\"type\":34},{\"gridX\":444,\"gridY\":160,\"type\":34},{\"gridX\":436,\"gridY\":172,\"type\":34},{\"gridX\":428,\"gridY\":184,\"type\":34},{\"gridX\":420,\"gridY\":196,\"type\":34},{\"gridX\":412,\"gridY\":208,\"type\":34},{\"gridX\":404,\"gridY\":220,\"type\":34},{\"gridX\":396,\"gridY\":232,\"type\":34},{\"gridX\":388,\"gridY\":244,\"type\":34},{\"gridX\":360,\"gridY\":96,\"type\":24},{\"gridX\":396,\"gridY\":116,\"type\":34},{\"gridX\":412,\"gridY\":108,\"type\":34},{\"gridX\":816,\"gridY\":240,\"type\":34},{\"gridX\":816,\"gridY\":224,\"type\":34},{\"gridX\":816,\"gridY\":208,\"type\":34},{\"gridX\":816,\"gridY\":192,\"type\":34},{\"gridX\":816,\"gridY\":176,\"type\":34},{\"gridX\":816,\"gridY\":160,\"type\":34},{\"gridX\":888,\"gridY\":116,\"type\":0},{\"gridX\":892,\"gridY\":116,\"type\":1},{\"gridX\":906,\"gridY\":116,\"type\":1},{\"gridX\":934,\"gridY\":116,\"type\":2},{\"gridX\":944,\"gridY\":116,\"type\":34},{\"gridX\":960,\"gridY\":116,\"type\":34},{\"gridX\":976,\"gridY\":116,\"type\":34},{\"gridX\":920,\"gridY\":116,\"type\":1},{\"gridX\":876,\"gridY\":96,\"type\":19},{\"gridX\":1576,\"gridY\":27,\"type\":20}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1760,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":128,\"gridY\":96,\"type\":11},{\"gridX\":168,\"gridY\":96,\"type\":11},{\"gridX\":252,\"gridY\":96,\"type\":11},{\"gridX\":508,\"gridY\":96,\"type\":11},{\"gridX\":593,\"gridY\":96,\"type\":11},{\"gridX\":669,\"gridY\":96,\"type\":11},{\"gridX\":372,\"gridY\":124,\"type\":3},{\"gridX\":888,\"gridY\":96,\"type\":11},{\"gridX\":744,\"gridY\":96,\"type\":4},{\"gridX\":892,\"gridY\":121,\"type\":11},{\"gridX\":932,\"gridY\":96,\"type\":5},{\"gridX\":948,\"gridY\":96,\"type\":5},{\"gridX\":964,\"gridY\":96,\"type\":5},{\"gridX\":1080,\"gridY\":96,\"type\":11},{\"gridX\":1136,\"gridY\":96,\"type\":11},{\"gridX\":1184,\"gridY\":96,\"type\":11},{\"gridX\":1284,\"gridY\":96,\"type\":4}],\"collectibles\":[{\"gridX\":128,\"gridY\":132,\"type\":0},{\"gridX\":136,\"gridY\":128,\"type\":0},{\"gridX\":144,\"gridY\":132,\"type\":0},{\"gridX\":152,\"gridY\":140,\"type\":0},{\"gridX\":160,\"gridY\":148,\"type\":0},{\"gridX\":180,\"gridY\":152,\"type\":0},{\"gridX\":188,\"gridY\":152,\"type\":0},{\"gridX\":196,\"gridY\":152,\"type\":0},{\"gridX\":212,\"gridY\":144,\"type\":0},{\"gridX\":220,\"gridY\":136,\"type\":0},{\"gridX\":228,\"gridY\":128,\"type\":0},{\"gridX\":228,\"gridY\":116,\"type\":0},{\"gridX\":228,\"gridY\":104,\"type\":0},{\"gridX\":228,\"gridY\":80,\"type\":0},{\"gridX\":228,\"gridY\":72,\"type\":0},{\"gridX\":228,\"gridY\":64,\"type\":0},{\"gridX\":280,\"gridY\":68,\"type\":1},{\"gridX\":276,\"gridY\":36,\"type\":0},{\"gridX\":284,\"gridY\":36,\"type\":0},{\"gridX\":276,\"gridY\":28,\"type\":0},{\"gridX\":284,\"gridY\":28,\"type\":0},{\"gridX\":276,\"gridY\":20,\"type\":0},{\"gridX\":284,\"gridY\":20,\"type\":0},{\"gridX\":304,\"gridY\":20,\"type\":0},{\"gridX\":304,\"gridY\":28,\"type\":0},{\"gridX\":308,\"gridY\":68,\"type\":0},{\"gridX\":308,\"gridY\":80,\"type\":0},{\"gridX\":308,\"gridY\":124,\"type\":0},{\"gridX\":316,\"gridY\":132,\"type\":0},{\"gridX\":324,\"gridY\":140,\"type\":0},{\"gridX\":332,\"gridY\":140,\"type\":0},{\"gridX\":340,\"gridY\":140,\"type\":0},{\"gridX\":372,\"gridY\":136,\"type\":0},{\"gridX\":380,\"gridY\":136,\"type\":0},{\"gridX\":388,\"gridY\":136,\"type\":0},{\"gridX\":420,\"gridY\":132,\"type\":0},{\"gridX\":428,\"gridY\":132,\"type\":0},{\"gridX\":436,\"gridY\":132,\"type\":0},{\"gridX\":472,\"gridY\":128,\"type\":0},{\"gridX\":480,\"gridY\":128,\"type\":0},{\"gridX\":488,\"gridY\":128,\"type\":0},{\"gridX\":544,\"gridY\":132,\"type\":0},{\"gridX\":528,\"gridY\":132,\"type\":0},{\"gridX\":536,\"gridY\":132,\"type\":0},{\"gridX\":592,\"gridY\":136,\"type\":0},{\"gridX\":600,\"gridY\":136,\"type\":0},{\"gridX\":608,\"gridY\":136,\"type\":0},{\"gridX\":680,\"gridY\":140,\"type\":0},{\"gridX\":672,\"gridY\":140,\"type\":0},{\"gridX\":688,\"gridY\":140,\"type\":0},{\"gridX\":788,\"gridY\":192,\"type\":1},{\"gridX\":720,\"gridY\":140,\"type\":0},{\"gridX\":728,\"gridY\":148,\"type\":0},{\"gridX\":736,\"gridY\":156,\"type\":0},{\"gridX\":748,\"gridY\":156,\"type\":0},{\"gridX\":748,\"gridY\":148,\"type\":0},{\"gridX\":748,\"gridY\":140,\"type\":0},{\"gridX\":748,\"gridY\":132,\"type\":0},{\"gridX\":748,\"gridY\":124,\"type\":0" << "},{\"gridX\":748,\"gridY\":116,\"type\":0},{\"gridX\":788,\"gridY\":104,\"type\":0},{\"gridX\":804,\"gridY\":104,\"type\":0},{\"gridX\":796,\"gridY\":104,\"type\":0},{\"gridX\":848,\"gridY\":104,\"type\":0},{\"gridX\":856,\"gridY\":104,\"type\":0},{\"gridX\":864,\"gridY\":104,\"type\":0},{\"gridX\":980,\"gridY\":104,\"type\":1},{\"gridX\":1008,\"gridY\":104,\"type\":0},{\"gridX\":1016,\"gridY\":104,\"type\":0},{\"gridX\":1024,\"gridY\":104,\"type\":0},{\"gridX\":1056,\"gridY\":104,\"type\":0},{\"gridX\":1064,\"gridY\":104,\"type\":0},{\"gridX\":1072,\"gridY\":104,\"type\":0},{\"gridX\":1104,\"gridY\":104,\"type\":0},{\"gridX\":1112,\"gridY\":104,\"type\":0},{\"gridX\":1120,\"gridY\":104,\"type\":0},{\"gridX\":312,\"gridY\":20,\"type\":0},{\"gridX\":312,\"gridY\":28,\"type\":0},{\"gridX\":304,\"gridY\":36,\"type\":0},{\"gridX\":312,\"gridY\":36,\"type\":0},{\"gridX\":1152,\"gridY\":104,\"type\":0},{\"gridX\":1160,\"gridY\":104,\"type\":0},{\"gridX\":1168,\"gridY\":104,\"type\":0},{\"gridX\":1216,\"gridY\":104,\"type\":0},{\"gridX\":1224,\"gridY\":104,\"type\":0},{\"gridX\":1208,\"gridY\":104,\"type\":0},{\"gridX\":1556,\"gridY\":36,\"type\":0},{\"gridX\":1300,\"gridY\":100,\"type\":0},{\"gridX\":1304,\"gridY\":104,\"type\":0},{\"gridX\":1304,\"gridY\":84,\"type\":0},{\"gridX\":1304,\"gridY\":44,\"type\":0},{\"gridX\":1572,\"gridY\":36,\"type\":0},{\"gridX\":1304,\"gridY\":64,\"type\":0},{\"gridX\":1564,\"gridY\":36,\"type\":0},{\"gridX\":1336,\"gridY\":52,\"type\":0},{\"gridX\":1344,\"gridY\":52,\"type\":0},{\"gridX\":1352,\"gridY\":52,\"type\":0},{\"gridX\":1384,\"gridY\":48,\"type\":0},{\"gridX\":1392,\"gridY\":48,\"type\":0},{\"gridX\":1400,\"gridY\":48,\"type\":0},{\"gridX\":1432,\"gridY\":44,\"type\":0},{\"gridX\":1440,\"gridY\":44,\"type\":0},{\"gridX\":1448,\"gridY\":44,\"type\":0}],\"jons\":[{\"gridX\":24,\"gridY\":100}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level15 *instance = new Chapter1Level15(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 16 ///

Chapter1Level16 * Chapter1Level16::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":16,\"midgrounds\":[{\"gridX\":1780,\"gridY\":96,\"type\":2},{\"gridX\":1828,\"gridY\":96,\"type\":2},{\"gridX\":1892,\"gridY\":96,\"type\":1},{\"gridX\":1936,\"gridY\":96,\"type\":0},{\"gridX\":1960,\"gridY\":96,\"type\":1},{\"gridX\":2012,\"gridY\":96,\"type\":0},{\"gridX\":88,\"gridY\":96,\"type\":1},{\"gridX\":0,\"gridY\":96,\"type\":0},{\"gridX\":32,\"gridY\":96,\"type\":2},{\"gridX\":560,\"gridY\":0,\"type\":4},{\"gridX\":520,\"gridY\":0,\"type\":5},{\"gridX\":612,\"gridY\":0,\"type\":4},{\"gridX\":624,\"gridY\":0,\"type\":5},{\"gridX\":712,\"gridY\":0,\"type\":4},{\"gridX\":776,\"gridY\":0,\"type\":5},{\"gridX\":668,\"gridY\":0,\"type\":3},{\"gridX\":888,\"gridY\":0,\"type\":3},{\"gridX\":936,\"gridY\":0,\"type\":3},{\"gridX\":984,\"gridY\":0,\"type\":3},{\"gridX\":1032,\"gridY\":0,\"type\":3},{\"gridX\":1080,\"gridY\":0,\"type\":3},{\"gridX\":1216,\"gridY\":0,\"type\":3},{\"gridX\":1312,\"gridY\":0,\"type\":3},{\"gridX\":1372,\"gridY\":0,\"type\":3},{\"gridX\":1464,\"gridY\":0,\"type\":3},{\"gridX\":1536,\"gridY\":0,\"type\":3},{\"gridX\":1628,\"gridY\":0,\"type\":3},{\"gridX\":184,\"gridY\":0,\"type\":3},{\"gridX\":208,\"gridY\":96,\"type\":1},{\"gridX\":252,\"gridY\":96,\"type\":0},{\"gridX\":288,\"gridY\":96,\"type\":1},{\"gridX\":332,\"gridY\":96,\"type\":0},{\"gridX\":368,\"gridY\":96,\"type\":1},{\"gridX\":412,\"gridY\":96,\"type\":2},{\"gridX\":472,\"gridY\":96,\"type\":1},{\"gridX\":516,\"gridY\":96,\"type\":0},{\"gridX\":552,\"gridY\":96,\"type\":2},{\"gridX\":616,\"gridY\":96,\"type\":1},{\"gridX\":664,\"gridY\":96,\"type\":0},{\"gridX\":700,\"gridY\":96,\"type\":1},{\"gridX\":748,\"gridY\":96,\"type\":2},{\"gridX\":812,\"gridY\":96,\"type\":1},{\"gridX\":860,\"gridY\":96,\"type\":2},{\"gridX\":924,\"gridY\":96,\"type\":1},{\"gridX\":972,\"gridY\":96,\"type\":0},{\"gridX\":1012,\"gridY\":96,\"type\":1},{\"gridX\":1060,\"gridY\":96,\"type\":2},{\"gridX\":1124,\"gridY\":96,\"type\":1},{\"gridX\":1172,\"gridY\":96,\"type\":0},{\"gridX\":1212,\"gridY\":96,\"type\":1},{\"gridX\":1256,\"gridY\":96,\"type\":2},{\"gridX\":1316,\"gridY\":96,\"type\":1},{\"gridX\":1364,\"gridY\":96,\"type\":0},{\"gridX\":1400,\"gridY\":96,\"type\":2},{\"gridX\":1464,\"gridY\":96,\"type\":1},{\"gridX\":1508,\"gridY\":96,\"type\":1},{\"gridX\":1592,\"gridY\":96,\"type\":1},{\"gridX\":1556,\"gridY\":96,\"type\":0},{\"gridX\":1632,\"gridY\":96,\"type\":2},{\"gridX\":1684,\"gridY\":96,\"type\":0},{\"gridX\":1712,\"gridY\":96,\"type\":1}],\"grounds\":[{\"gridX\":224,\"gridY\":0,\"type\":0},{\"gridX\":692,\"gridY\":0,\"type\":0},{\"gridX\":1104,\"gridY\":0,\"type\":0},{\"gridX\":1264,\"gridY\":0,\"type\":0},{\"gridX\":1724,\"gridY\":0,\"type\":0},{\"gridX\":1412,\"gridY\":0,\"type\":0},{\"gridX\":1584,\"gridY\":0,\"type\":0},{\"gridX\":232,\"gridY\":0,\"type\":1},{\"gridX\":700,\"gridY\":0,\"type\":1},{\"gridX\":1144,\"gridY\":0,\"type\":1},{\"gridX\":1732,\"gridY\":0,\"type\":1},{\"gridX\":1112,\"gridY\":0,\"type\":1},{\"gridX\":732,\"gridY\":0,\"type\":2},{\"gridX\":264,\"gridY\":0,\"type\":3},{\"gridX\":392,\"gridY\":0,\"type\":3},{\"gridX\":520,\"gridY\":0,\"type\":3},{\"gridX\":648,\"gridY\":0,\"type\":4},{\"gridX\":1176,\"gridY\":0,\"type\":4},{\"gridX\":1272,\"gridY\":0,\"type\":4},{\"gridX\":1420,\"gridY\":0,\"type\":4},{\"gridX\":796,\"gridY\":0,\"type\":4},{\"gridX\":1592,\"gridY\":0,\"type\":4},{\"gridX\":0,\"gridY\":50,\"type\":10},{\"gridX\":504,\"gridY\":50,\"type\":10},{\"gridX\":768,\"gridY\":50,\"type\":11},{\"gridX\":136,\"gridY\":50,\"type\":11},{\"gridX\":704,\"gridY\":50,\"type\":12},{\"gridX\":512,\"gridY\":50,\"type\":12},{\"gridX\":576,\"gridY\":50,\"type\":13},{\"gridX\":8,\"gridY\":50,\"type\":13},{\"gridX\":168,\"gridY\":50,\"type\":14},{\"gridX\":800,\"gridY\":50,\"type\":14},{\"gridX\":540,\"gridY\":48,\"type\":15},{\"gridX\":772,\"gridY\":48,\"type\":16},{\"gridX\":548,\"gridY\":48,\"type\":16},{\"gridX\":708,\"gridY\":48,\"type\":17},{\"gridX\":580,\"gridY\":48,\"type\":18},{\"gridX\":804,\"gridY\":48,\"type\":19},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":1672,\"gridY\":90,\"type\":22},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":1288,\"gridY\":90,\"type\":23},{\"gridX\":1544,\"gridY\":90,\"type\":23},{\"gridX\":392,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1416,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":776,\"gridY\":90,\"type\":23},{\"gridX\":648,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":264,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":1760,\"gridY\":0,\"type\":25},{\"gridX\":1768,\"gridY\":0,\"type\":26},{\"gridX\":1800,\"gridY\":0,\"type\":28},{\"gridX\":1928,\"gridY\":0,\"type\":28},{\"gridX\":2056,\"gridY\":0,\"type\":29}],\"pits\":[],\"exitGrounds\":[{\"gridX\":1736,\"gridY\":88,\"type\":1}],\"holes\":[{\"gridX\":124,\"gridY\":73,\"type\":0},{\"gridX\":140,\"gridY\":73,\"type\":1},{\"gridX\":156,\"gridY\":73,\"type\":2}],\"foregroundObjects\":[{\"gridX\":1792,\"gridY\":96,\"type\":12},{\"gridX\":176,\"gridY\":96,\"type\":36},{\"gridX\":300,\"gridY\":16,\"type\":34},{\"gridX\":340,\"gridY\":16,\"type\":34},{\"gridX\":320,\"gridY\":76,\"type\":34},{\"gridX\":360,\"gridY\":76,\"type\":34},{\"gridX\":380,\"gridY\":16,\"type\":34},{\"gridX\":536,\"gridY\":70,\"type\":31},{\"gridX\":513,\"gridY\":84,\"type\":30},{\"gridX\":506,\"gridY\":55,\"type\":26},{\"gridX\":416,\"gridY\":7,\"type\":27},{\"gridX\":448,\"gridY\":7,\"type\":27},{\"gridX\":512,\"gridY\":7,\"type\":27},{\"gridX\":480,\"gridY\":7,\"type\":27},{\"gridX\":680,\"gridY\":16,\"type\":34},{\"gridX\":1192,\"gridY\":84,\"type\":30},{\"gridX\":1224,\"gridY\":84,\"type\":30},{\"gridX\":1256,\"gridY\":84,\"type\":30},{\"gridX\":1288,\"gridY\":84,\"type\":30},{\"gridX\":1320,\"gridY\":84,\"type\":30},{\"gridX\":1352,\"gridY\":84,\"type\":30},{\"gridX\":1384,\"gridY\":84,\"type\":30},{\"gridX\":1416,\"gridY\":84,\"type\":30},{\"gridX\":1448,\"gridY\":84,\"type\":30},{\"gridX\":1480,\"gridY\":84,\"type\":30},{\"gridX\":1512,\"gridY\":84,\"type\":30},{\"gridX\":1544,\"gridY\":84,\"type\":30},{\"gridX\":1576,\"gridY\":84,\"type\":30},{\"gridX\":1608,\"gridY\":84,\"type\":30},{\"gridX\":1640,\"gridY\":84,\"type\":30},{\"gridX\":1672,\"gridY\":84,\"type\":30},{\"gridX\":1704,\"gridY\":84,\"type\":30},{\"gridX\":1748,\"gridY\":7,\"type\":21},{\"gridX\":228,\"gridY\":48,\"type\":34},{\"gridX\":248,\"gridY\":44,\"type\":34},{\"gridX\":268,\"gridY\":40,\"type\":34},{\"gridX\":263,\"gridY\":7,\"type\":25},{\"gridX\":268,\"gridY\":7,\"type\":25}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1760,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":180,\"gridY\":77,\"type\":7},{\"gridX\":276,\"gridY\":7,\"type\":6},{\"gridX\":296,\"gridY\":77,\"type\":7},{\"gridX\":316,\"gridY\":7,\"type\":6},{\"gridX\":336,\"gridY\":77,\"type\":7},{\"gridX\":356,\"gridY\":7,\"type\":6},{\"gridX\":376,\"gridY\":77,\"type\":7},{\"gridX\":396,\"gridY\":7,\"type\":6},{\"gridX\":520,\"gridY\":55,\"type\":6},{\"gridX\":648,\"gridY\":37,\"type\":7},{\"gridX\":732,\"gridY\":7,\"type\":6},{\"gridX\":1168,\"gridY\":7,\"type\":6},{\"gridX\":1264,\"gridY\":7,\"type\":6},{\"gridX\":1412,\"gridY\":7,\"type\":6},{\"gridX\":1584,\"gridY\":7,\"type\":6}],\"collectibles\":[{\"gridX\":192,\"gridY\":32,\"type\":0},{\"gridX\":184,\"gridY\":64,\"type\":0},{\"gridX\":188,\"gridY\":48,\"type\":0},{\"gridX\":196,\"gridY\":16,\"type\":1},{\"gridX\":252,\"gridY\":24,\"type\":0},{\"gridX\":268,\"gridY\":24,\"type\":0},{\"gridX\":260,\"gridY\":24,\"type\":0},{\"gridX\":128,\"gridY\":104,\"type\":0},{\"gridX\":136,\"gridY\":104,\"type\":0},{\"gridX\":144,\"gridY\":104,\"type\":0},{\"gridX\":128,\"gridY\":112,\"type\":0},{\"gridX\":136,\"gridY\":112,\"type\":0},{\"gridX\":128,\"gridY\":120,\"type\":0},{\"gridX\":144,\"gridY\":64,\"type\":0},{\"gridX\":152,\"gridY" << "\":64,\"type\":0},{\"gridX\":160,\"gridY\":64,\"type\":0},{\"gridX\":284,\"gridY\":32,\"type\":0},{\"gridX\":292,\"gridY\":48,\"type\":0},{\"gridX\":300,\"gridY\":64,\"type\":0},{\"gridX\":308,\"gridY\":48,\"type\":0},{\"gridX\":316,\"gridY\":32,\"type\":0},{\"gridX\":512,\"gridY\":64,\"type\":1},{\"gridX\":324,\"gridY\":32,\"type\":0},{\"gridX\":332,\"gridY\":48,\"type\":0},{\"gridX\":340,\"gridY\":64,\"type\":0},{\"gridX\":348,\"gridY\":48,\"type\":0},{\"gridX\":356,\"gridY\":32,\"type\":0},{\"gridX\":364,\"gridY\":32,\"type\":0},{\"gridX\":372,\"gridY\":48,\"type\":0},{\"gridX\":380,\"gridY\":64,\"type\":0},{\"gridX\":388,\"gridY\":48,\"type\":0},{\"gridX\":396,\"gridY\":32,\"type\":0},{\"gridX\":404,\"gridY\":32,\"type\":0},{\"gridX\":412,\"gridY\":48,\"type\":0},{\"gridX\":420,\"gridY\":64,\"type\":0},{\"gridX\":540,\"gridY\":16,\"type\":0},{\"gridX\":524,\"gridY\":24,\"type\":0},{\"gridX\":532,\"gridY\":20,\"type\":0},{\"gridX\":596,\"gridY\":16,\"type\":0},{\"gridX\":580,\"gridY\":16,\"type\":0},{\"gridX\":588,\"gridY\":16,\"type\":0},{\"gridX\":500,\"gridY\":36,\"type\":0},{\"gridX\":508,\"gridY\":32,\"type\":0},{\"gridX\":516,\"gridY\":28,\"type\":0},{\"gridX\":648,\"gridY\":16,\"type\":0},{\"gridX\":656,\"gridY\":20,\"type\":0},{\"gridX\":664,\"gridY\":24,\"type\":0},{\"gridX\":672,\"gridY\":28,\"type\":0},{\"gridX\":680,\"gridY\":32,\"type\":0},{\"gridX\":688,\"gridY\":28,\"type\":0},{\"gridX\":696,\"gridY\":24,\"type\":0},{\"gridX\":652,\"gridY\":28,\"type\":1},{\"gridX\":732,\"gridY\":32,\"type\":0},{\"gridX\":724,\"gridY\":16,\"type\":0},{\"gridX\":728,\"gridY\":24,\"type\":0},{\"gridX\":736,\"gridY\":40,\"type\":0},{\"gridX\":744,\"gridY\":40,\"type\":0},{\"gridX\":748,\"gridY\":32,\"type\":0},{\"gridX\":752,\"gridY\":24,\"type\":0},{\"gridX\":756,\"gridY\":16,\"type\":0},{\"gridX\":804,\"gridY\":16,\"type\":0},{\"gridX\":808,\"gridY\":24,\"type\":0},{\"gridX\":812,\"gridY\":32,\"type\":0},{\"gridX\":816,\"gridY\":40,\"type\":0},{\"gridX\":836,\"gridY\":44,\"type\":0},{\"gridX\":820,\"gridY\":48,\"type\":0},{\"gridX\":844,\"gridY\":48,\"type\":0},{\"gridX\":852,\"gridY\":52,\"type\":0},{\"gridX\":860,\"gridY\":56,\"type\":0},{\"gridX\":888,\"gridY\":56,\"type\":0},{\"gridX\":896,\"gridY\":56,\"type\":0},{\"gridX\":904,\"gridY\":56,\"type\":0},{\"gridX\":936,\"gridY\":52,\"type\":0},{\"gridX\":944,\"gridY\":52,\"type\":0},{\"gridX\":952,\"gridY\":52,\"type\":0},{\"gridX\":984,\"gridY\":48,\"type\":0},{\"gridX\":992,\"gridY\":48,\"type\":0},{\"gridX\":1000,\"gridY\":48,\"type\":0},{\"gridX\":1040,\"gridY\":44,\"type\":0},{\"gridX\":1032,\"gridY\":44,\"type\":0},{\"gridX\":1048,\"gridY\":44,\"type\":0},{\"gridX\":1080,\"gridY\":40,\"type\":0},{\"gridX\":1088,\"gridY\":40,\"type\":0},{\"gridX\":1096,\"gridY\":40,\"type\":0},{\"gridX\":1224,\"gridY\":40,\"type\":0},{\"gridX\":1232,\"gridY\":40,\"type\":0},{\"gridX\":1216,\"gridY\":40,\"type\":0},{\"gridX\":1320,\"gridY\":48,\"type\":0},{\"gridX\":1328,\"gridY\":48,\"type\":0},{\"gridX\":1312,\"gridY\":48,\"type\":0},{\"gridX\":1380,\"gridY\":40,\"type\":0},{\"gridX\":1544,\"gridY\":52,\"type\":0},{\"gridX\":1372,\"gridY\":40,\"type\":0},{\"gridX\":1388,\"gridY\":40,\"type\":0},{\"gridX\":1464,\"gridY\":64,\"type\":0},{\"gridX\":1472,\"gridY\":64,\"type\":0},{\"gridX\":1480,\"gridY\":64,\"type\":0},{\"gridX\":1552,\"gridY\":52,\"type\":0},{\"gridX\":1536,\"gridY\":52,\"type\":0},{\"gridX\":1636,\"gridY\":56,\"type\":0},{\"gridX\":1656,\"gridY\":56,\"type\":0},{\"gridX\":1628,\"gridY\":56,\"type\":0},{\"gridX\":1644,\"gridY\":56,\"type\":0}],\"jons\":[{\"gridX\":20,\"gridY\":100}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level16 *instance = new Chapter1Level16(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 17 ///

Chapter1Level17 * Chapter1Level17::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":17,\"midgrounds\":[{\"gridX\":1912,\"gridY\":96,\"type\":2},{\"gridX\":1956,\"gridY\":96,\"type\":2},{\"gridX\":2036,\"gridY\":96,\"type\":1},{\"gridX\":2008,\"gridY\":96,\"type\":0},{\"gridX\":2076,\"gridY\":96,\"type\":1},{\"gridX\":2120,\"gridY\":96,\"type\":0},{\"gridX\":80,\"gridY\":96,\"type\":1},{\"gridX\":0,\"gridY\":96,\"type\":0},{\"gridX\":20,\"gridY\":96,\"type\":2},{\"gridX\":184,\"gridY\":96,\"type\":0},{\"gridX\":360,\"gridY\":96,\"type\":2},{\"gridX\":432,\"gridY\":96,\"type\":2},{\"gridX\":224,\"gridY\":96,\"type\":1},{\"gridX\":496,\"gridY\":96,\"type\":2},{\"gridX\":1216,\"gridY\":96,\"type\":2},{\"gridX\":1272,\"gridY\":96,\"type\":1},{\"gridX\":1300,\"gridY\":96,\"type\":0},{\"gridX\":1320,\"gridY\":96,\"type\":2},{\"gridX\":1372,\"gridY\":96,\"type\":1},{\"gridX\":1412,\"gridY\":96,\"type\":0},{\"gridX\":1456,\"gridY\":96,\"type\":2},{\"gridX\":1436,\"gridY\":96,\"type\":0},{\"gridX\":1676,\"gridY\":96,\"type\":2},{\"gridX\":1624,\"gridY\":96,\"type\":2},{\"gridX\":1564,\"gridY\":96,\"type\":1},{\"gridX\":1516,\"gridY\":96,\"type\":1},{\"gridX\":1780,\"gridY\":96,\"type\":2},{\"gridX\":1824,\"gridY\":96,\"type\":2},{\"gridX\":1868,\"gridY\":96,\"type\":2}],\"grounds\":[{\"gridX\":692,\"gridY\":0,\"type\":0},{\"gridX\":836,\"gridY\":0,\"type\":0},{\"gridX\":984,\"gridY\":0,\"type\":0},{\"gridX\":700,\"gridY\":0,\"type\":1},{\"gridX\":844,\"gridY\":0,\"type\":1},{\"gridX\":992,\"gridY\":0,\"type\":1},{\"gridX\":732,\"gridY\":0,\"type\":4},{\"gridX\":876,\"gridY\":0,\"type\":4},{\"gridX\":1024,\"gridY\":0,\"type\":4},{\"gridX\":488,\"gridY\":20,\"type\":5},{\"gridX\":588,\"gridY\":20,\"type\":5},{\"gridX\":1060,\"gridY\":20,\"type\":5},{\"gridX\":496,\"gridY\":20,\"type\":6},{\"gridX\":1068,\"gridY\":20,\"type\":6},{\"gridX\":1132,\"gridY\":20,\"type\":6},{\"gridX\":1292,\"gridY\":20,\"type\":6},{\"gridX\":596,\"gridY\":20,\"type\":7},{\"gridX\":1196,\"gridY\":20,\"type\":7},{\"gridX\":528,\"gridY\":20,\"type\":9},{\"gridX\":660,\"gridY\":20,\"type\":9},{\"gridX\":1324,\"gridY\":20,\"type\":9},{\"gridX\":556,\"gridY\":50,\"type\":10},{\"gridX\":1204,\"gridY\":50,\"type\":11},{\"gridX\":1236,\"gridY\":50,\"type\":12},{\"gridX\":820,\"gridY\":50,\"type\":13},{\"gridX\":948,\"gridY\":50,\"type\":13},{\"gridX\":1076,\"gridY\":50,\"type\":13},{\"gridX\":564,\"gridY\":50,\"type\":13},{\"gridX\":692,\"gridY\":50,\"type\":13},{\"gridX\":1300,\"gridY\":50,\"type\":14},{\"gridX\":552,\"gridY\":48,\"type\":15},{\"gridX\":1200,\"gridY\":48,\"type\":16},{\"gridX\":560,\"gridY\":48,\"type\":18},{\"gridX\":688,\"gridY\":48,\"type\":18},{\"gridX\":816,\"gridY\":48,\"type\":18},{\"gridX\":944,\"gridY\":48,\"type\":18},{\"gridX\":1072,\"gridY\":48,\"type\":18},{\"gridX\":1232,\"gridY\":48,\"type\":19},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":1384,\"gridY\":90,\"type\":21},{\"gridX\":1288,\"gridY\":90,\"type\":22},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":1416,\"gridY\":90,\"type\":23},{\"gridX\":1544,\"gridY\":90,\"type\":23},{\"gridX\":648,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":392,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":2056,\"gridY\":90,\"type\":23},{\"gridX\":264,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":1672,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1928,\"gridY\":90,\"type\":23},{\"gridX\":776,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":1800,\"gridY\":88,\"type\":23},{\"gridX\":2184,\"gridY\":90,\"type\":24}],\"pits\":[],\"exitGrounds\":[{\"gridX\":1260,\"gridY\":0,\"type\":3},{\"gridX\":1100,\"gridY\":0,\"type\":3},{\"gridX\":1164,\"gridY\":0,\"type\":3},{\"gridX\":1352,\"gridY\":88,\"type\":0}],\"holes\":[{\"gridX\":472,\"gridY\":73,\"type\":0},{\"gridX\":488,\"gridY\":73,\"type\":1},{\"gridX\":504,\"gridY\":73,\"type\":1},{\"gridX\":520,\"gridY\":73,\"type\":1},{\"gridX\":536,\"gridY\":73,\"type\":2}],\"foregroundObjects\":[{\"gridX\":1920,\"gridY\":96,\"type\":12},{\"gridX\":128,\"gridY\":95,\"type\":35},{\"gridX\":152,\"gridY\":140,\"type\":34},{\"gridX\":152,\"gridY\":124,\"type\":34},{\"gridX\":292,\"gridY\":95,\"type\":35},{\"gridX\":308,\"gridY\":95,\"type\":35},{\"gridX\":276,\"gridY\":95,\"type\":35},{\"gridX\":252,\"gridY\":132,\"type\":34},{\"gridX\":408,\"gridY\":96,\"type\":24},{\"gridX\":400,\"gridY\":116,\"type\":0},{\"gridX\":404,\"gridY\":116,\"type\":1},{\"gridX\":418,\"gridY\":116,\"type\":2},{\"gridX\":560,\"gridY\":95,\"type\":35},{\"gridX\":576,\"gridY\":95,\"type\":35},{\"gridX\":592,\"gridY\":95,\"type\":35},{\"gridX\":608,\"gridY\":95,\"type\":35},{\"gridX\":624,\"gridY\":95,\"type\":35},{\"gridX\":640,\"gridY\":95,\"type\":35},{\"gridX\":656,\"gridY\":95,\"type\":35},{\"gridX\":672,\"gridY\":95,\"type\":35},{\"gridX\":688,\"gridY\":95,\"type\":35},{\"gridX\":704,\"gridY\":95,\"type\":35},{\"gridX\":720,\"gridY\":95,\"type\":35},{\"gridX\":736,\"gridY\":95,\"type\":35},{\"gridX\":752,\"gridY\":95,\"type\":35},{\"gridX\":768,\"gridY\":95,\"type\":35},{\"gridX\":784,\"gridY\":95,\"type\":35},{\"gridX\":800,\"gridY\":95,\"type\":35},{\"gridX\":816,\"gridY\":95,\"type\":35},{\"gridX\":832,\"gridY\":95,\"type\":35},{\"gridX\":848,\"gridY\":95,\"type\":35},{\"gridX\":864,\"gridY\":95,\"type\":35},{\"gridX\":880,\"gridY\":95,\"type\":35},{\"gridX\":896,\"gridY\":95,\"type\":35},{\"gridX\":912,\"gridY\":95,\"type\":35},{\"gridX\":928,\"gridY\":95,\"type\":35},{\"gridX\":944,\"gridY\":95,\"type\":35},{\"gridX\":960,\"gridY\":95,\"type\":35},{\"gridX\":976,\"gridY\":95,\"type\":35},{\"gridX\":992,\"gridY\":95,\"type\":35},{\"gridX\":1008,\"gridY\":95,\"type\":35},{\"gridX\":1024,\"gridY\":95,\"type\":35},{\"gridX\":1040,\"gridY\":95,\"type\":35},{\"gridX\":1056,\"gridY\":95,\"type\":35},{\"gridX\":1072,\"gridY\":95,\"type\":35},{\"gridX\":1088,\"gridY\":95,\"type\":35},{\"gridX\":1104,\"gridY\":95,\"type\":35},{\"gridX\":1120,\"gridY\":95,\"type\":35},{\"gridX\":1136,\"gridY\":95,\"type\":35},{\"gridX\":1152,\"gridY\":95,\"type\":35},{\"gridX\":1168,\"gridY\":95,\"type\":35},{\"gridX\":1184,\"gridY\":95,\"type\":35},{\"gridX\":1200,\"gridY\":95,\"type\":35},{\"gridX\":556,\"gridY\":75,\"type\":27},{\"gridX\":588,\"gridY\":75,\"type\":27},{\"gridX\":744,\"gridY\":44,\"type\":29},{\"gridX\":1012,\"gridY\":44,\"type\":30},{\"gridX\":800,\"gridY\":44,\"type\":30},{\"gridX\":888,\"gridY\":44,\"type\":30},{\"gridX\":965,\"gridY\":44,\"type\":30},{\"gridX\":1348,\"gridY\":52,\"type\":3},{\"gridX\":1352,\"gridY\":52,\"type\":4},{\"gridX\":1366,\"gridY\":52,\"type\":5},{\"gridX\":1176,\"gridY\":44,\"type\":28},{\"gridX\":1180,\"gridY\":44,\"type\":28},{\"gridX\":1276,\"gridY\":84,\"type\":30},{\"gridX\":1320,\"gridY\":27,\"type\":41},{\"gridX\":1476,\"gridY\":96,\"type\":24},{\"gridX\":1508,\"gridY\":96,\"type\":24},{\"gridX\":1540,\"gridY\":96,\"type\":24},{\"gridX\":1572,\"gridY\":96,\"type\":24}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1888,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":184,\"gridY\":96,\"type\":4},{\"gridX\":260,\"gridY\":96,\"type\":11},{\"gridX\":336,\"gridY\":96,\"type\":6},{\"gridX\":412,\"gridY\":121,\"type\":11},{\"gridX\":556,\"gridY\":37,\"type\":7},{\"gridX\":688,\"gridY\":37,\"type\":7},{\"gridX\":868,\"gridY\":7,\"type\":11},{\"gridX\":859,\"gridY\":7,\"type\":11},{\"gridX\":1024,\"gridY\":7,\"type\":11},{\"gridX\":1348,\"gridY\":57,\"type\":6},{\"gridX\":1460,\"gridY\":96,\"type\":11},{\"gridX\":1436,\"gridY\":96,\"type\":11},{\"gridX\":1708,\"gridY\":96,\"type\":6}],\"collectibles\":[{\"gridX\":140,\"gridY\":140,\"type\":0},{\"gridX\":140,\"gridY\":128,\"type\":0},{\"gridX\":140,\"gridY\":116,\"type\":0},{\"gridX\":140,\"gridY\":104,\"type\":0},{\"gridX\":160,\"gridY\":104,\"type\":0},{\"gridX\":168,\"gridY\":104,\"type\":0},{\"gridX\":176,\"gridY\":104,\"type\":0},{\"gridX\":256,\"gridY\":164,\"type\":1},{\"" << "gridX\":216,\"gridY\":128,\"type\":0},{\"gridX\":224,\"gridY\":132,\"type\":0},{\"gridX\":232,\"gridY\":136,\"type\":0},{\"gridX\":320,\"gridY\":128,\"type\":0},{\"gridX\":328,\"gridY\":120,\"type\":0},{\"gridX\":336,\"gridY\":112,\"type\":0},{\"gridX\":344,\"gridY\":116,\"type\":0},{\"gridX\":348,\"gridY\":124,\"type\":0},{\"gridX\":352,\"gridY\":132,\"type\":0},{\"gridX\":356,\"gridY\":140,\"type\":0},{\"gridX\":360,\"gridY\":148,\"type\":0},{\"gridX\":464,\"gridY\":108,\"type\":0},{\"gridX\":472,\"gridY\":108,\"type\":0},{\"gridX\":480,\"gridY\":108,\"type\":0},{\"gridX\":408,\"gridY\":172,\"type\":0},{\"gridX\":416,\"gridY\":156,\"type\":0},{\"gridX\":412,\"gridY\":164,\"type\":0},{\"gridX\":420,\"gridY\":148,\"type\":0},{\"gridX\":424,\"gridY\":140,\"type\":0},{\"gridX\":428,\"gridY\":132,\"type\":0},{\"gridX\":432,\"gridY\":124,\"type\":0},{\"gridX\":480,\"gridY\":64,\"type\":0},{\"gridX\":480,\"gridY\":76,\"type\":0},{\"gridX\":480,\"gridY\":52,\"type\":0},{\"gridX\":480,\"gridY\":40,\"type\":0},{\"gridX\":500,\"gridY\":36,\"type\":0},{\"gridX\":508,\"gridY\":36,\"type\":0},{\"gridX\":516,\"gridY\":36,\"type\":0},{\"gridX\":588,\"gridY\":36,\"type\":0},{\"gridX\":596,\"gridY\":36,\"type\":0},{\"gridX\":604,\"gridY\":36,\"type\":0},{\"gridX\":644,\"gridY\":36,\"type\":0},{\"gridX\":652,\"gridY\":36,\"type\":0},{\"gridX\":660,\"gridY\":36,\"type\":0},{\"gridX\":696,\"gridY\":16,\"type\":0},{\"gridX\":704,\"gridY\":16,\"type\":0},{\"gridX\":712,\"gridY\":16,\"type\":0},{\"gridX\":720,\"gridY\":16,\"type\":0},{\"gridX\":728,\"gridY\":16,\"type\":0},{\"gridX\":776,\"gridY\":16,\"type\":0},{\"gridX\":792,\"gridY\":16,\"type\":0},{\"gridX\":784,\"gridY\":16,\"type\":0},{\"gridX\":892,\"gridY\":24,\"type\":0},{\"gridX\":900,\"gridY\":24,\"type\":0},{\"gridX\":908,\"gridY\":24,\"type\":0},{\"gridX\":940,\"gridY\":20,\"type\":0},{\"gridX\":948,\"gridY\":20,\"type\":0},{\"gridX\":956,\"gridY\":20,\"type\":0},{\"gridX\":984,\"gridY\":16,\"type\":0},{\"gridX\":992,\"gridY\":16,\"type\":0},{\"gridX\":1000,\"gridY\":16,\"type\":0},{\"gridX\":1060,\"gridY\":36,\"type\":0},{\"gridX\":1036,\"gridY\":24,\"type\":0},{\"gridX\":1068,\"gridY\":36,\"type\":0},{\"gridX\":1076,\"gridY\":36,\"type\":0},{\"gridX\":1032,\"gridY\":16,\"type\":0},{\"gridX\":1040,\"gridY\":32,\"type\":0},{\"gridX\":1104,\"gridY\":36,\"type\":0},{\"gridX\":1112,\"gridY\":36,\"type\":0},{\"gridX\":1120,\"gridY\":36,\"type\":0},{\"gridX\":1176,\"gridY\":36,\"type\":0},{\"gridX\":1168,\"gridY\":36,\"type\":0},{\"gridX\":1184,\"gridY\":36,\"type\":0},{\"gridX\":1300,\"gridY\":64,\"type\":1},{\"gridX\":1264,\"gridY\":36,\"type\":0},{\"gridX\":1272,\"gridY\":36,\"type\":0},{\"gridX\":1280,\"gridY\":36,\"type\":0},{\"gridX\":1216,\"gridY\":36,\"type\":0},{\"gridX\":1224,\"gridY\":36,\"type\":0},{\"gridX\":1232,\"gridY\":36,\"type\":0},{\"gridX\":1324,\"gridY\":36,\"type\":0},{\"gridX\":1328,\"gridY\":44,\"type\":0},{\"gridX\":1332,\"gridY\":52,\"type\":0},{\"gridX\":1332,\"gridY\":60,\"type\":0},{\"gridX\":1336,\"gridY\":68,\"type\":0},{\"gridX\":1340,\"gridY\":76,\"type\":0},{\"gridX\":1348,\"gridY\":76,\"type\":0},{\"gridX\":1356,\"gridY\":76,\"type\":0},{\"gridX\":1456,\"gridY\":144,\"type\":0},{\"gridX\":1464,\"gridY\":144,\"type\":0},{\"gridX\":1472,\"gridY\":144,\"type\":0},{\"gridX\":1504,\"gridY\":140,\"type\":0},{\"gridX\":1512,\"gridY\":140,\"type\":0},{\"gridX\":1520,\"gridY\":140,\"type\":0},{\"gridX\":1552,\"gridY\":136,\"type\":0},{\"gridX\":1568,\"gridY\":136,\"type\":0},{\"gridX\":1560,\"gridY\":136,\"type\":0},{\"gridX\":1764,\"gridY\":128,\"type\":1},{\"gridX\":1600,\"gridY\":132,\"type\":0},{\"gridX\":1608,\"gridY\":132,\"type\":0},{\"gridX\":1616,\"gridY\":132,\"type\":0},{\"gridX\":840,\"gridY\":16,\"type\":0},{\"gridX\":848,\"gridY\":16,\"type\":0},{\"gridX\":856,\"gridY\":16,\"type\":0},{\"gridX\":480,\"gridY\":28,\"type\":0}],\"jons\":[{\"gridX\":20,\"gridY\":100}],\"extraForegroundObjects\":[{\"gridX\":1744,\"gridY\":92,\"type\":39}],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level17 *instance = new Chapter1Level17(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 18 ///

Chapter1Level18 * Chapter1Level18::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":18,\"midgrounds\":[{\"gridX\":1780,\"gridY\":96,\"type\":2},{\"gridX\":1708,\"gridY\":96,\"type\":2},{\"gridX\":1672,\"gridY\":96,\"type\":1},{\"gridX\":1756,\"gridY\":96,\"type\":0},{\"gridX\":1872,\"gridY\":96,\"type\":1},{\"gridX\":1844,\"gridY\":96,\"type\":0},{\"gridX\":0,\"gridY\":96,\"type\":1},{\"gridX\":40,\"gridY\":96,\"type\":0},{\"gridX\":68,\"gridY\":96,\"type\":2},{\"gridX\":256,\"gridY\":96,\"type\":1},{\"gridX\":308,\"gridY\":96,\"type\":2},{\"gridX\":360,\"gridY\":96,\"type\":0},{\"gridX\":412,\"gridY\":96,\"type\":1},{\"gridX\":452,\"gridY\":96,\"type\":2},{\"gridX\":504,\"gridY\":96,\"type\":0},{\"gridX\":540,\"gridY\":96,\"type\":2},{\"gridX\":636,\"gridY\":96,\"type\":1},{\"gridX\":684,\"gridY\":96,\"type\":2},{\"gridX\":732,\"gridY\":96,\"type\":1},{\"gridX\":784,\"gridY\":96,\"type\":0},{\"gridX\":1920,\"gridY\":96,\"type\":2},{\"gridX\":996,\"gridY\":96,\"type\":1},{\"gridX\":952,\"gridY\":96,\"type\":0},{\"gridX\":1080,\"gridY\":96,\"type\":2},{\"gridX\":1132,\"gridY\":96,\"type\":1},{\"gridX\":1612,\"gridY\":96,\"type\":0},{\"gridX\":1636,\"gridY\":96,\"type\":1},{\"gridX\":124,\"gridY\":96,\"type\":2},{\"gridX\":172,\"gridY\":96,\"type\":2}],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":0},{\"gridX\":8,\"gridY\":0,\"type\":3},{\"gridX\":136,\"gridY\":0,\"type\":3},{\"gridX\":264,\"gridY\":0,\"type\":3},{\"gridX\":392,\"gridY\":0,\"type\":3},{\"gridX\":520,\"gridY\":0,\"type\":3},{\"gridX\":648,\"gridY\":0,\"type\":3},{\"gridX\":776,\"gridY\":0,\"type\":3},{\"gridX\":904,\"gridY\":0,\"type\":3},{\"gridX\":1032,\"gridY\":0,\"type\":4},{\"gridX\":0,\"gridY\":50,\"type\":10},{\"gridX\":8,\"gridY\":50,\"type\":13},{\"gridX\":136,\"gridY\":50,\"type\":13},{\"gridX\":264,\"gridY\":50,\"type\":13},{\"gridX\":392,\"gridY\":50,\"type\":13},{\"gridX\":520,\"gridY\":50,\"type\":13},{\"gridX\":648,\"gridY\":50,\"type\":13},{\"gridX\":776,\"gridY\":50,\"type\":13},{\"gridX\":904,\"gridY\":50,\"type\":13},{\"gridX\":1032,\"gridY\":50,\"type\":14},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":1160,\"gridY\":90,\"type\":21},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":392,\"gridY\":90,\"type\":23},{\"gridX\":264,\"gridY\":90,\"type\":23},{\"gridX\":776,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":648,\"gridY\":90,\"type\":23},{\"gridX\":1192,\"gridY\":90,\"type\":24},{\"gridX\":1616,\"gridY\":0,\"type\":25},{\"gridX\":1624,\"gridY\":0,\"type\":28},{\"gridX\":1752,\"gridY\":0,\"type\":28},{\"gridX\":1880,\"gridY\":0,\"type\":28},{\"gridX\":2008,\"gridY\":0,\"type\":29}],\"pits\":[],\"exitGrounds\":[],\"holes\":[],\"foregroundObjects\":[{\"gridX\":1680,\"gridY\":96,\"type\":12},{\"gridX\":132,\"gridY\":96,\"type\":11},{\"gridX\":228,\"gridY\":96,\"type\":10},{\"gridX\":148,\"gridY\":96,\"type\":13},{\"gridX\":153,\"gridY\":96,\"type\":15},{\"gridX\":175,\"gridY\":96,\"type\":15},{\"gridX\":197,\"gridY\":96,\"type\":15},{\"gridX\":219,\"gridY\":96,\"type\":16},{\"gridX\":244,\"gridY\":96,\"type\":13},{\"gridX\":249,\"gridY\":96,\"type\":14},{\"gridX\":260,\"gridY\":96,\"type\":16},{\"gridX\":436,\"gridY\":168,\"type\":1},{\"gridX\":450,\"gridY\":168,\"type\":2},{\"gridX\":432,\"gridY\":168,\"type\":0},{\"gridX\":304,\"gridY\":212,\"type\":34},{\"gridX\":304,\"gridY\":228,\"type\":34},{\"gridX\":304,\"gridY\":244,\"type\":34},{\"gridX\":304,\"gridY\":196,\"type\":34},{\"gridX\":360,\"gridY\":192,\"type\":34},{\"gridX\":392,\"gridY\":116,\"type\":0},{\"gridX\":396,\"gridY\":116,\"type\":1},{\"gridX\":410,\"gridY\":116,\"type\":2},{\"gridX\":440,\"gridY\":116,\"type\":0},{\"gridX\":444,\"gridY\":116,\"type\":1},{\"gridX\":458,\"gridY\":116,\"type\":2},{\"gridX\":536,\"gridY\":116,\"type\":0},{\"gridX\":540,\"gridY\":116,\"type\":1},{\"gridX\":554,\"gridY\":116,\"type\":2},{\"gridX\":612,\"gridY\":112,\"type\":0},{\"gridX\":616,\"gridY\":112,\"type\":1},{\"gridX\":630,\"gridY\":112,\"type\":2},{\"gridX\":760,\"gridY\":124,\"type\":0},{\"gridX\":764,\"gridY\":124,\"type\":1},{\"gridX\":778,\"gridY\":124,\"type\":2},{\"gridX\":708,\"gridY\":140,\"type\":0},{\"gridX\":712,\"gridY\":140,\"type\":1},{\"gridX\":726,\"gridY\":140,\"type\":2},{\"gridX\":464,\"gridY\":152,\"type\":34},{\"gridX\":560,\"gridY\":152,\"type\":34},{\"gridX\":636,\"gridY\":148,\"type\":34},{\"gridX\":732,\"gridY\":148,\"type\":34},{\"gridX\":784,\"gridY\":148,\"type\":34},{\"gridX\":809,\"gridY\":96,\"type\":10},{\"gridX\":824,\"gridY\":128,\"type\":0},{\"gridX\":828,\"gridY\":128,\"type\":1},{\"gridX\":842,\"gridY\":128,\"type\":2},{\"gridX\":936,\"gridY\":128,\"type\":0},{\"gridX\":940,\"gridY\":128,\"type\":1},{\"gridX\":954,\"gridY\":128,\"type\":2},{\"gridX\":1064,\"gridY\":96,\"type\":7},{\"gridX\":1200,\"gridY\":128,\"type\":0},{\"gridX\":1204,\"gridY\":128,\"type\":1},{\"gridX\":1218,\"gridY\":128,\"type\":2},{\"gridX\":1220,\"gridY\":144,\"type\":0},{\"gridX\":1224,\"gridY\":144,\"type\":1},{\"gridX\":1236,\"gridY\":144,\"type\":2}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1648,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":208,\"gridY\":108,\"type\":3},{\"gridX\":244,\"gridY\":108,\"type\":3},{\"gridX\":292,\"gridY\":96,\"type\":6},{\"gridX\":432,\"gridY\":173,\"type\":6},{\"gridX\":448,\"gridY\":96,\"type\":8},{\"gridX\":496,\"gridY\":96,\"type\":8},{\"gridX\":536,\"gridY\":121,\"type\":9},{\"gridX\":580,\"gridY\":96,\"type\":9},{\"gridX\":608,\"gridY\":117,\"type\":10},{\"gridX\":636,\"gridY\":96,\"type\":10},{\"gridX\":688,\"gridY\":96,\"type\":11},{\"gridX\":708,\"gridY\":145,\"type\":11},{\"gridX\":760,\"gridY\":129,\"type\":12},{\"gridX\":764,\"gridY\":96,\"type\":12},{\"gridX\":980,\"gridY\":96,\"type\":5},{\"gridX\":1048,\"gridY\":96,\"type\":5},{\"gridX\":1200,\"gridY\":132,\"type\":4},{\"gridX\":1220,\"gridY\":149,\"type\":4}],\"collectibles\":[{\"gridX\":148,\"gridY\":116,\"type\":0},{\"gridX\":164,\"gridY\":116,\"type\":0},{\"gridX\":180,\"gridY\":116,\"type\":0},{\"gridX\":172,\"gridY\":124,\"type\":0},{\"gridX\":156,\"gridY\":124,\"type\":0},{\"gridX\":188,\"gridY\":124,\"type\":0},{\"gridX\":196,\"gridY\":116,\"type\":0},{\"gridX\":204,\"gridY\":124,\"type\":0},{\"gridX\":212,\"gridY\":116,\"type\":0},{\"gridX\":220,\"gridY\":124,\"type\":0},{\"gridX\":228,\"gridY\":116,\"type\":0},{\"gridX\":236,\"gridY\":124,\"type\":0},{\"gridX\":244,\"gridY\":116,\"type\":0},{\"gridX\":252,\"gridY\":124,\"type\":0},{\"gridX\":260,\"gridY\":116,\"type\":0},{\"gridX\":140,\"gridY\":124,\"type\":0},{\"gridX\":132,\"gridY\":116,\"type\":0},{\"gridX\":296,\"gridY\":116,\"type\":0},{\"gridX\":300,\"gridY\":124,\"type\":0},{\"gridX\":304,\"gridY\":132,\"type\":0},{\"gridX\":308,\"gridY\":140,\"type\":0},{\"gridX\":312,\"gridY\":148,\"type\":0},{\"gridX\":316,\"gridY\":156,\"type\":0},{\"gridX\":320,\"gridY\":164,\"type\":0},{\"gridX\":416,\"gridY\":192,\"type\":1},{\"gridX\":360,\"gridY\":104,\"type\":0},{\"gridX\":344,\"gridY\":104,\"type\":0},{\"gridX\":352,\"gridY\":104,\"type\":0},{\"gridX\":392,\"gridY\":128,\"type\":0},{\"gridX\":400,\"gridY\":128,\"type\":0},{\"gridX\":408,\"gridY\":128,\"type\":0},{\"gridX\":440,\"gridY\":104,\"type\":0},{\"gridX\":448,\"gridY\":104,\"type\":0},{\"gridX\":456,\"gridY\":104,\"type\":0},{\"gridX\":488,\"gridY\":104,\"type\":0},{\"gridX\":496,\"gridY\":104,\"type\":0},{\"gridX\":504,\"gridY\":104,\"type\":0},{\"gridX\":536,\"gridY\":104,\"type\":0},{\"gridX\":544,\"gridY\":104,\"type\":0},{\"gridX\":552,\"gridY\":104,\"type\":0},{\"gridX\":584,\"gridY\":104,\"type\":0},{\"gridX\":592,\"gridY\":104,\"type\":0},{\"gridX\":600,\"gridY\":104,\"type\":0},{\"gridX\":632,\"gridY\":104,\"type\":0},{\"gridX\":640,\"gridY\":104,\"type\":0},{\"gridX\":648,\"gridY\":104,\"type\":0},{\"gridX\":680,\"gridY\":104,\"type\":0},{\"gridX\":688,\"gridY\":104,\"type\":0},{\"gridX\":696,\"gridY\":10" << "4,\"type\":0},{\"gridX\":728,\"gridY\":104,\"type\":0},{\"gridX\":736,\"gridY\":104,\"type\":0},{\"gridX\":744,\"gridY\":104,\"type\":0},{\"gridX\":776,\"gridY\":104,\"type\":0},{\"gridX\":784,\"gridY\":104,\"type\":0},{\"gridX\":792,\"gridY\":104,\"type\":0},{\"gridX\":1204,\"gridY\":140,\"type\":1},{\"gridX\":888,\"gridY\":132,\"type\":1},{\"gridX\":824,\"gridY\":140,\"type\":0},{\"gridX\":832,\"gridY\":140,\"type\":0},{\"gridX\":840,\"gridY\":140,\"type\":0},{\"gridX\":980,\"gridY\":112,\"type\":0},{\"gridX\":984,\"gridY\":120,\"type\":0},{\"gridX\":988,\"gridY\":128,\"type\":0},{\"gridX\":992,\"gridY\":136,\"type\":0},{\"gridX\":996,\"gridY\":144,\"type\":0},{\"gridX\":1040,\"gridY\":120,\"type\":0},{\"gridX\":1044,\"gridY\":112,\"type\":0},{\"gridX\":1036,\"gridY\":128,\"type\":0},{\"gridX\":1032,\"gridY\":136,\"type\":0},{\"gridX\":1028,\"gridY\":144,\"type\":0},{\"gridX\":1000,\"gridY\":152,\"type\":0},{\"gridX\":1128,\"gridY\":104,\"type\":0},{\"gridX\":1088,\"gridY\":116,\"type\":0},{\"gridX\":1172,\"gridY\":112,\"type\":0},{\"gridX\":1064,\"gridY\":120,\"type\":0},{\"gridX\":1136,\"gridY\":104,\"type\":0},{\"gridX\":1180,\"gridY\":128,\"type\":0},{\"gridX\":1120,\"gridY\":104,\"type\":0},{\"gridX\":1072,\"gridY\":124,\"type\":0},{\"gridX\":1024,\"gridY\":152,\"type\":0},{\"gridX\":1168,\"gridY\":104,\"type\":0},{\"gridX\":1056,\"gridY\":116,\"type\":0},{\"gridX\":1080,\"gridY\":120,\"type\":0},{\"gridX\":1176,\"gridY\":120,\"type\":0},{\"gridX\":1184,\"gridY\":136,\"type\":0},{\"gridX\":1244,\"gridY\":164,\"type\":0},{\"gridX\":1252,\"gridY\":164,\"type\":0},{\"gridX\":1236,\"gridY\":164,\"type\":0},{\"gridX\":1308,\"gridY\":152,\"type\":0},{\"gridX\":1316,\"gridY\":152,\"type\":0},{\"gridX\":1324,\"gridY\":152,\"type\":0},{\"gridX\":1380,\"gridY\":140,\"type\":0},{\"gridX\":1388,\"gridY\":140,\"type\":0},{\"gridX\":1396,\"gridY\":140,\"type\":0},{\"gridX\":1452,\"gridY\":124,\"type\":0},{\"gridX\":1460,\"gridY\":124,\"type\":0},{\"gridX\":1468,\"gridY\":124,\"type\":0},{\"gridX\":1540,\"gridY\":108,\"type\":0},{\"gridX\":1532,\"gridY\":108,\"type\":0},{\"gridX\":1524,\"gridY\":108,\"type\":0},{\"gridX\":1612,\"gridY\":108,\"type\":0},{\"gridX\":1604,\"gridY\":100,\"type\":0},{\"gridX\":1620,\"gridY\":100,\"type\":0}],\"jons\":[{\"gridX\":20,\"gridY\":100}],\"extraForegroundObjects\":[{\"gridX\":860,\"gridY\":95,\"type\":39},{\"gridX\":892,\"gridY\":95,\"type\":39}],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level18 *instance = new Chapter1Level18(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 19 ///

Chapter1Level19 * Chapter1Level19::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":19,\"midgrounds\":[{\"gridX\":1536,\"gridY\":96,\"type\":2},{\"gridX\":1644,\"gridY\":96,\"type\":1},{\"gridX\":1680,\"gridY\":96,\"type\":0},{\"gridX\":1708,\"gridY\":96,\"type\":1},{\"gridX\":1760,\"gridY\":96,\"type\":0},{\"gridX\":64,\"gridY\":96,\"type\":1},{\"gridX\":108,\"gridY\":96,\"type\":0},{\"gridX\":4,\"gridY\":96,\"type\":2},{\"gridX\":208,\"gridY\":96,\"type\":2},{\"gridX\":268,\"gridY\":96,\"type\":0},{\"gridX\":308,\"gridY\":96,\"type\":2},{\"gridX\":364,\"gridY\":96,\"type\":1},{\"gridX\":404,\"gridY\":96,\"type\":0},{\"gridX\":428,\"gridY\":0,\"type\":4},{\"gridX\":396,\"gridY\":0,\"type\":5},{\"gridX\":368,\"gridY\":0,\"type\":3},{\"gridX\":292,\"gridY\":0,\"type\":3},{\"gridX\":436,\"gridY\":96,\"type\":1},{\"gridX\":476,\"gridY\":96,\"type\":2},{\"gridX\":548,\"gridY\":96,\"type\":1},{\"gridX\":788,\"gridY\":96,\"type\":0},{\"gridX\":832,\"gridY\":0,\"type\":3},{\"gridX\":744,\"gridY\":96,\"type\":0},{\"gridX\":820,\"gridY\":96,\"type\":2},{\"gridX\":880,\"gridY\":96,\"type\":1},{\"gridX\":872,\"gridY\":0,\"type\":5},{\"gridX\":896,\"gridY\":0,\"type\":4},{\"gridX\":1156,\"gridY\":0,\"type\":3},{\"gridX\":1212,\"gridY\":0,\"type\":3},{\"gridX\":1252,\"gridY\":0,\"type\":3},{\"gridX\":1312,\"gridY\":0,\"type\":3},{\"gridX\":984,\"gridY\":96,\"type\":2},{\"gridX\":1044,\"gridY\":96,\"type\":2},{\"gridX\":1104,\"gridY\":96,\"type\":2},{\"gridX\":1164,\"gridY\":96,\"type\":2},{\"gridX\":1224,\"gridY\":96,\"type\":2},{\"gridX\":1284,\"gridY\":96,\"type\":2},{\"gridX\":1348,\"gridY\":96,\"type\":1},{\"gridX\":1588,\"gridY\":96,\"type\":0},{\"gridX\":1608,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":128,\"gridY\":0,\"type\":0},{\"gridX\":312,\"gridY\":0,\"type\":0},{\"gridX\":928,\"gridY\":0,\"type\":0},{\"gridX\":320,\"gridY\":0,\"type\":1},{\"gridX\":936,\"gridY\":0,\"type\":1},{\"gridX\":1032,\"gridY\":0,\"type\":1},{\"gridX\":968,\"gridY\":0,\"type\":2},{\"gridX\":1064,\"gridY\":0,\"type\":2},{\"gridX\":136,\"gridY\":0,\"type\":3},{\"gridX\":264,\"gridY\":0,\"type\":4},{\"gridX\":352,\"gridY\":0,\"type\":4},{\"gridX\":1128,\"gridY\":0,\"type\":4},{\"gridX\":384,\"gridY\":50,\"type\":10},{\"gridX\":864,\"gridY\":50,\"type\":10},{\"gridX\":872,\"gridY\":50,\"type\":11},{\"gridX\":392,\"gridY\":50,\"type\":12},{\"gridX\":904,\"gridY\":50,\"type\":14},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":392,\"gridY\":90,\"type\":21},{\"gridX\":1352,\"gridY\":90,\"type\":21},{\"gridX\":968,\"gridY\":90,\"type\":22},{\"gridX\":1288,\"gridY\":90,\"type\":22},{\"gridX\":776,\"gridY\":90,\"type\":22},{\"gridX\":1416,\"gridY\":90,\"type\":23},{\"gridX\":264,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":1672,\"gridY\":90,\"type\":23},{\"gridX\":1544,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":840,\"gridY\":90,\"type\":23},{\"gridX\":1800,\"gridY\":90,\"type\":24},{\"gridX\":448,\"gridY\":0,\"type\":25},{\"gridX\":712,\"gridY\":0,\"type\":27},{\"gridX\":456,\"gridY\":0,\"type\":28},{\"gridX\":584,\"gridY\":0,\"type\":28}],\"pits\":[],\"exitGrounds\":[{\"gridX\":424,\"gridY\":88,\"type\":1},{\"gridX\":1384,\"gridY\":88,\"type\":0}],\"holes\":[{\"gridX\":196,\"gridY\":80,\"type\":3},{\"gridX\":248,\"gridY\":80,\"type\":3},{\"gridX\":816,\"gridY\":80,\"type\":3},{\"gridX\":868,\"gridY\":73,\"type\":0},{\"gridX\":884,\"gridY\":73,\"type\":1},{\"gridX\":900,\"gridY\":73,\"type\":1},{\"gridX\":916,\"gridY\":73,\"type\":1},{\"gridX\":932,\"gridY\":73,\"type\":2}],\"foregroundObjects\":[{\"gridX\":1536,\"gridY\":96,\"type\":12},{\"gridX\":152,\"gridY\":96,\"type\":6},{\"gridX\":132,\"gridY\":96,\"type\":13},{\"gridX\":137,\"gridY\":96,\"type\":14},{\"gridX\":148,\"gridY\":96,\"type\":16},{\"gridX\":212,\"gridY\":96,\"type\":13},{\"gridX\":217,\"gridY\":96,\"type\":15},{\"gridX\":239,\"gridY\":96,\"type\":16},{\"gridX\":248,\"gridY\":64,\"type\":3},{\"gridX\":252,\"gridY\":64,\"type\":4},{\"gridX\":280,\"gridY\":64,\"type\":5},{\"gridX\":266,\"gridY\":64,\"type\":4},{\"gridX\":216,\"gridY\":76,\"type\":34},{\"gridX\":368,\"gridY\":84,\"type\":30},{\"gridX\":348,\"gridY\":7,\"type\":41},{\"gridX\":388,\"gridY\":55,\"type\":26},{\"gridX\":276,\"gridY\":152,\"type\":34},{\"gridX\":272,\"gridY\":168,\"type\":34},{\"gridX\":268,\"gridY\":184,\"type\":34},{\"gridX\":280,\"gridY\":136,\"type\":34},{\"gridX\":284,\"gridY\":120,\"type\":34},{\"gridX\":288,\"gridY\":104,\"type\":34},{\"gridX\":264,\"gridY\":200,\"type\":34},{\"gridX\":256,\"gridY\":232,\"type\":34},{\"gridX\":260,\"gridY\":216,\"type\":34},{\"gridX\":440,\"gridY\":55,\"type\":19},{\"gridX\":360,\"gridY\":44,\"type\":3},{\"gridX\":364,\"gridY\":44,\"type\":5},{\"gridX\":596,\"gridY\":95,\"type\":35},{\"gridX\":656,\"gridY\":95,\"type\":35},{\"gridX\":724,\"gridY\":95,\"type\":35},{\"gridX\":836,\"gridY\":76,\"type\":34},{\"gridX\":844,\"gridY\":68,\"type\":34},{\"gridX\":836,\"gridY\":60,\"type\":34},{\"gridX\":836,\"gridY\":44,\"type\":34},{\"gridX\":844,\"gridY\":52,\"type\":34},{\"gridX\":844,\"gridY\":36,\"type\":34},{\"gridX\":836,\"gridY\":28,\"type\":34},{\"gridX\":988,\"gridY\":48,\"type\":3},{\"gridX\":992,\"gridY\":48,\"type\":4},{\"gridX\":1006,\"gridY\":48,\"type\":5},{\"gridX\":952,\"gridY\":96,\"type\":36},{\"gridX\":1096,\"gridY\":44,\"type\":3},{\"gridX\":1100,\"gridY\":44,\"type\":4},{\"gridX\":1128,\"gridY\":44,\"type\":5},{\"gridX\":1156,\"gridY\":44,\"type\":3},{\"gridX\":1160,\"gridY\":44,\"type\":4},{\"gridX\":1174,\"gridY\":44,\"type\":5},{\"gridX\":1212,\"gridY\":44,\"type\":3},{\"gridX\":1216,\"gridY\":44,\"type\":4},{\"gridX\":1230,\"gridY\":44,\"type\":5},{\"gridX\":1252,\"gridY\":32,\"type\":3},{\"gridX\":1256,\"gridY\":32,\"type\":4},{\"gridX\":1270,\"gridY\":32,\"type\":5},{\"gridX\":1312,\"gridY\":32,\"type\":3},{\"gridX\":1316,\"gridY\":32,\"type\":4},{\"gridX\":1330,\"gridY\":32,\"type\":4},{\"gridX\":1344,\"gridY\":32,\"type\":5},{\"gridX\":1388,\"gridY\":60,\"type\":3},{\"gridX\":1392,\"gridY\":60,\"type\":4},{\"gridX\":1406,\"gridY\":60,\"type\":5},{\"gridX\":1392,\"gridY\":65,\"type\":41},{\"gridX\":1368,\"gridY\":44,\"type\":34},{\"gridX\":1416,\"gridY\":72,\"type\":34},{\"gridX\":1300,\"gridY\":20,\"type\":34},{\"gridX\":1428,\"gridY\":95,\"type\":35},{\"gridX\":1456,\"gridY\":95,\"type\":35},{\"gridX\":1484,\"gridY\":95,\"type\":35},{\"gridX\":1114,\"gridY\":44,\"type\":4}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1504,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":268,\"gridY\":69,\"type\":6},{\"gridX\":332,\"gridY\":7,\"type\":12},{\"gridX\":268,\"gridY\":96,\"type\":6},{\"gridX\":364,\"gridY\":49,\"type\":12},{\"gridX\":528,\"gridY\":96,\"type\":12},{\"gridX\":580,\"gridY\":96,\"type\":12},{\"gridX\":644,\"gridY\":96,\"type\":12},{\"gridX\":708,\"gridY\":96,\"type\":12},{\"gridX\":988,\"gridY\":53,\"type\":12},{\"gridX\":1096,\"gridY\":49,\"type\":12},{\"gridX\":1124,\"gridY\":7,\"type\":12}],\"collectibles\":[{\"gridX\":132,\"gridY\":104,\"type\":0},{\"gridX\":136,\"gridY\":112,\"type\":0},{\"gridX\":140,\"gridY\":120,\"type\":0},{\"gridX\":144,\"gridY\":128,\"type\":0},{\"gridX\":148,\"gridY\":136,\"type\":0},{\"gridX\":160,\"gridY\":132,\"type\":0},{\"gridX\":168,\"gridY\":136,\"type\":0},{\"gridX\":176,\"gridY\":140,\"type\":0},{\"gridX\":184,\"gridY\":144,\"type\":0},{\"gridX\":200,\"gridY\":136,\"type\":0},{\"gridX\":200,\"gridY\":128,\"type\":0},{\"gridX\":200,\"gridY\":120,\"type\":0},{\"gridX\":200,\"gridY\":112,\"type\":0},{\"gridX\":200,\"gridY\":104,\"type\":0},{\"gridX\":240,\"gridY\":80,\"type\":1},{\"gridX\":216,\"gridY\":16,\"type\":0},{\"gridX\":224,\"gridY\":16,\"type\":0},{\"gridX\":232,\"gridY\":16,\"type\":0},{\"gridX\":260,\"gridY\":16,\"type\":0},{\"gridX\":264,\"gridY\":24,\"type\":0},{\"gridX\":268,\"gridY\":32,\"type\":0},{\"gridX\":272,\"gridY\":40,\"type\":0},{\"gridX\":276,\"gridY\":4" << "8,\"type\":0},{\"gridX\":292,\"gridY\":40,\"type\":0},{\"gridX\":288,\"gridY\":48,\"type\":0},{\"gridX\":300,\"gridY\":40,\"type\":0},{\"gridX\":308,\"gridY\":48,\"type\":0},{\"gridX\":316,\"gridY\":56,\"type\":0},{\"gridX\":336,\"gridY\":68,\"type\":0},{\"gridX\":344,\"gridY\":68,\"type\":0},{\"gridX\":352,\"gridY\":68,\"type\":0},{\"gridX\":384,\"gridY\":64,\"type\":0},{\"gridX\":392,\"gridY\":64,\"type\":0},{\"gridX\":400,\"gridY\":64,\"type\":0},{\"gridX\":488,\"gridY\":104,\"type\":0},{\"gridX\":480,\"gridY\":104,\"type\":0},{\"gridX\":472,\"gridY\":104,\"type\":0},{\"gridX\":440,\"gridY\":100,\"type\":0},{\"gridX\":444,\"gridY\":112,\"type\":0},{\"gridX\":448,\"gridY\":124,\"type\":0},{\"gridX\":544,\"gridY\":124,\"type\":0},{\"gridX\":552,\"gridY\":132,\"type\":0},{\"gridX\":560,\"gridY\":140,\"type\":0},{\"gridX\":568,\"gridY\":148,\"type\":0},{\"gridX\":576,\"gridY\":140,\"type\":0},{\"gridX\":584,\"gridY\":132,\"type\":0},{\"gridX\":592,\"gridY\":124,\"type\":0},{\"gridX\":604,\"gridY\":124,\"type\":0},{\"gridX\":616,\"gridY\":132,\"type\":0},{\"gridX\":628,\"gridY\":124,\"type\":0},{\"gridX\":640,\"gridY\":116,\"type\":0},{\"gridX\":648,\"gridY\":124,\"type\":0},{\"gridX\":656,\"gridY\":132,\"type\":0},{\"gridX\":672,\"gridY\":148,\"type\":0},{\"gridX\":676,\"gridY\":156,\"type\":0},{\"gridX\":668,\"gridY\":140,\"type\":0},{\"gridX\":680,\"gridY\":164,\"type\":1},{\"gridX\":724,\"gridY\":124,\"type\":0},{\"gridX\":732,\"gridY\":124,\"type\":0},{\"gridX\":740,\"gridY\":124,\"type\":0},{\"gridX\":772,\"gridY\":116,\"type\":0},{\"gridX\":780,\"gridY\":116,\"type\":0},{\"gridX\":788,\"gridY\":116,\"type\":0},{\"gridX\":840,\"gridY\":20,\"type\":1},{\"gridX\":940,\"gridY\":16,\"type\":0},{\"gridX\":948,\"gridY\":16,\"type\":0},{\"gridX\":956,\"gridY\":16,\"type\":0},{\"gridX\":980,\"gridY\":16,\"type\":0},{\"gridX\":988,\"gridY\":16,\"type\":0},{\"gridX\":996,\"gridY\":16,\"type\":0},{\"gridX\":680,\"gridY\":128,\"type\":0},{\"gridX\":684,\"gridY\":116,\"type\":0},{\"gridX\":688,\"gridY\":104,\"type\":0},{\"gridX\":1020,\"gridY\":16,\"type\":0},{\"gridX\":1024,\"gridY\":24,\"type\":0},{\"gridX\":1028,\"gridY\":32,\"type\":0},{\"gridX\":1032,\"gridY\":40,\"type\":0},{\"gridX\":1036,\"gridY\":48,\"type\":0},{\"gridX\":1040,\"gridY\":56,\"type\":0},{\"gridX\":1076,\"gridY\":64,\"type\":0},{\"gridX\":1084,\"gridY\":64,\"type\":0},{\"gridX\":1092,\"gridY\":64,\"type\":0},{\"gridX\":1100,\"gridY\":60,\"type\":0},{\"gridX\":1108,\"gridY\":56,\"type\":0},{\"gridX\":1172,\"gridY\":56,\"type\":0},{\"gridX\":1156,\"gridY\":56,\"type\":0},{\"gridX\":1164,\"gridY\":56,\"type\":0},{\"gridX\":1212,\"gridY\":56,\"type\":0},{\"gridX\":1220,\"gridY\":56,\"type\":0},{\"gridX\":1228,\"gridY\":56,\"type\":0},{\"gridX\":1260,\"gridY\":44,\"type\":0},{\"gridX\":1252,\"gridY\":44,\"type\":0},{\"gridX\":1268,\"gridY\":44,\"type\":0},{\"gridX\":1316,\"gridY\":44,\"type\":0},{\"gridX\":1332,\"gridY\":44,\"type\":0},{\"gridX\":1324,\"gridY\":44,\"type\":0},{\"gridX\":1340,\"gridY\":44,\"type\":0},{\"gridX\":1456,\"gridY\":124,\"type\":0},{\"gridX\":1464,\"gridY\":124,\"type\":0},{\"gridX\":1448,\"gridY\":124,\"type\":0},{\"gridX\":1396,\"gridY\":72,\"type\":0},{\"gridX\":1388,\"gridY\":72,\"type\":0},{\"gridX\":1380,\"gridY\":72,\"type\":0}],\"jons\":[{\"gridX\":20,\"gridY\":104}],\"extraForegroundObjects\":[],\"markers\":[]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level19 *instance = new Chapter1Level19(text.c_str());
    
    return instance;
}

/// Chapter 1 Level 20 ///

Chapter1Level20 * Chapter1Level20::getInstance()
{
    std::stringstream ss;
    
    ss << "{\"world\":1,\"level\":20,\"midgrounds\":[{\"gridX\":1816,\"gridY\":96,\"type\":2},{\"gridX\":1872,\"gridY\":96,\"type\":2},{\"gridX\":1928,\"gridY\":96,\"type\":1},{\"gridX\":1972,\"gridY\":96,\"type\":0},{\"gridX\":2004,\"gridY\":96,\"type\":1},{\"gridX\":2048,\"gridY\":96,\"type\":0},{\"gridX\":20,\"gridY\":96,\"type\":0},{\"gridX\":0,\"gridY\":96,\"type\":0},{\"gridX\":40,\"gridY\":96,\"type\":0},{\"gridX\":60,\"gridY\":96,\"type\":0},{\"gridX\":80,\"gridY\":96,\"type\":0},{\"gridX\":100,\"gridY\":96,\"type\":0},{\"gridX\":136,\"gridY\":96,\"type\":2},{\"gridX\":204,\"gridY\":96,\"type\":1},{\"gridX\":332,\"gridY\":96,\"type\":2},{\"gridX\":304,\"gridY\":96,\"type\":0},{\"gridX\":244,\"gridY\":96,\"type\":0},{\"gridX\":400,\"gridY\":96,\"type\":1},{\"gridX\":376,\"gridY\":96,\"type\":0},{\"gridX\":412,\"gridY\":96,\"type\":1},{\"gridX\":456,\"gridY\":96,\"type\":2},{\"gridX\":732,\"gridY\":96,\"type\":2},{\"gridX\":788,\"gridY\":96,\"type\":1},{\"gridX\":732,\"gridY\":0,\"type\":3},{\"gridX\":664,\"gridY\":0,\"type\":3},{\"gridX\":676,\"gridY\":0,\"type\":3},{\"gridX\":484,\"gridY\":0,\"type\":3},{\"gridX\":748,\"gridY\":0,\"type\":4},{\"gridX\":756,\"gridY\":0,\"type\":5},{\"gridX\":652,\"gridY\":96,\"type\":2},{\"gridX\":1016,\"gridY\":96,\"type\":2},{\"gridX\":1132,\"gridY\":96,\"type\":0},{\"gridX\":1160,\"gridY\":96,\"type\":1},{\"gridX\":1208,\"gridY\":96,\"type\":2},{\"gridX\":1268,\"gridY\":96,\"type\":1},{\"gridX\":1308,\"gridY\":96,\"type\":0},{\"gridX\":1336,\"gridY\":96,\"type\":1},{\"gridX\":1376,\"gridY\":96,\"type\":2},{\"gridX\":1436,\"gridY\":96,\"type\":1},{\"gridX\":1476,\"gridY\":96,\"type\":2},{\"gridX\":1252,\"gridY\":0,\"type\":3},{\"gridX\":1204,\"gridY\":0,\"type\":3},{\"gridX\":1164,\"gridY\":0,\"type\":3},{\"gridX\":1116,\"gridY\":0,\"type\":3},{\"gridX\":1068,\"gridY\":0,\"type\":3},{\"gridX\":1548,\"gridY\":96,\"type\":2},{\"gridX\":1648,\"gridY\":96,\"type\":0},{\"gridX\":1716,\"gridY\":96,\"type\":0},{\"gridX\":1784,\"gridY\":96,\"type\":0}],\"grounds\":[{\"gridX\":512,\"gridY\":0,\"type\":0},{\"gridX\":768,\"gridY\":0,\"type\":0},{\"gridX\":1292,\"gridY\":0,\"type\":0},{\"gridX\":1412,\"gridY\":0,\"type\":0},{\"gridX\":776,\"gridY\":0,\"type\":1},{\"gridX\":1300,\"gridY\":0,\"type\":1},{\"gridX\":1420,\"gridY\":0,\"type\":1},{\"gridX\":520,\"gridY\":0,\"type\":3},{\"gridX\":648,\"gridY\":0,\"type\":4},{\"gridX\":808,\"gridY\":0,\"type\":4},{\"gridX\":1332,\"gridY\":0,\"type\":4},{\"gridX\":1452,\"gridY\":0,\"type\":4},{\"gridX\":1060,\"gridY\":20,\"type\":5},{\"gridX\":1360,\"gridY\":20,\"type\":5},{\"gridX\":1476,\"gridY\":20,\"type\":5},{\"gridX\":1400,\"gridY\":20,\"type\":6},{\"gridX\":1516,\"gridY\":20,\"type\":7},{\"gridX\":1068,\"gridY\":20,\"type\":8},{\"gridX\":1196,\"gridY\":20,\"type\":9},{\"gridX\":1432,\"gridY\":20,\"type\":9},{\"gridX\":256,\"gridY\":50,\"type\":10},{\"gridX\":736,\"gridY\":50,\"type\":10},{\"gridX\":1312,\"gridY\":50,\"type\":10},{\"gridX\":1400,\"gridY\":50,\"type\":10},{\"gridX\":744,\"gridY\":50,\"type\":11},{\"gridX\":1320,\"gridY\":50,\"type\":11},{\"gridX\":1440,\"gridY\":50,\"type\":11},{\"gridX\":392,\"gridY\":50,\"type\":12},{\"gridX\":264,\"gridY\":50,\"type\":13},{\"gridX\":1472,\"gridY\":50,\"type\":14},{\"gridX\":456,\"gridY\":50,\"type\":14},{\"gridX\":1352,\"gridY\":50,\"type\":14},{\"gridX\":732,\"gridY\":48,\"type\":15},{\"gridX\":740,\"gridY\":48,\"type\":16},{\"gridX\":772,\"gridY\":48,\"type\":19},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":776,\"gridY\":90,\"type\":21},{\"gridX\":1480,\"gridY\":90,\"type\":21},{\"gridX\":840,\"gridY\":90,\"type\":22},{\"gridX\":1416,\"gridY\":90,\"type\":22},{\"gridX\":520,\"gridY\":90,\"type\":23},{\"gridX\":1288,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":90,\"type\":23},{\"gridX\":648,\"gridY\":90,\"type\":23},{\"gridX\":1160,\"gridY\":90,\"type\":23},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":904,\"gridY\":90,\"type\":23},{\"gridX\":392,\"gridY\":90,\"type\":23},{\"gridX\":1032,\"gridY\":90,\"type\":23},{\"gridX\":264,\"gridY\":90,\"type\":23},{\"gridX\":1536,\"gridY\":0,\"type\":25},{\"gridX\":1544,\"gridY\":0,\"type\":26},{\"gridX\":2024,\"gridY\":0,\"type\":27},{\"gridX\":1576,\"gridY\":0,\"type\":27},{\"gridX\":1640,\"gridY\":0,\"type\":28},{\"gridX\":1768,\"gridY\":0,\"type\":28},{\"gridX\":1896,\"gridY\":0,\"type\":28},{\"gridX\":2088,\"gridY\":0,\"type\":29}],\"pits\":[],\"exitGrounds\":[{\"gridX\":808,\"gridY\":88,\"type\":0},{\"gridX\":1368,\"gridY\":0,\"type\":3},{\"gridX\":1484,\"gridY\":0,\"type\":3},{\"gridX\":1512,\"gridY\":88,\"type\":1},{\"gridX\":1408,\"gridY\":48,\"type\":2}],\"holes\":[{\"gridX\":332,\"gridY\":80,\"type\":3},{\"gridX\":1060,\"gridY\":73,\"type\":0},{\"gridX\":1076,\"gridY\":73,\"type\":1},{\"gridX\":1092,\"gridY\":73,\"type\":2},{\"gridX\":1328,\"gridY\":32,\"type\":4},{\"gridX\":1440,\"gridY\":32,\"type\":4}],\"foregroundObjects\":[{\"gridX\":1808,\"gridY\":96,\"type\":12},{\"gridX\":240,\"gridY\":160,\"type\":0},{\"gridX\":244,\"gridY\":160,\"type\":1},{\"gridX\":258,\"gridY\":160,\"type\":2},{\"gridX\":296,\"gridY\":160,\"type\":0},{\"gridX\":300,\"gridY\":160,\"type\":1},{\"gridX\":314,\"gridY\":160,\"type\":2},{\"gridX\":344,\"gridY\":152,\"type\":0},{\"gridX\":348,\"gridY\":152,\"type\":1},{\"gridX\":362,\"gridY\":152,\"type\":2},{\"gridX\":204,\"gridY\":96,\"type\":24},{\"gridX\":236,\"gridY\":96,\"type\":24},{\"gridX\":304,\"gridY\":96,\"type\":11},{\"gridX\":264,\"gridY\":116,\"type\":0},{\"gridX\":268,\"gridY\":116,\"type\":1},{\"gridX\":282,\"gridY\":116,\"type\":2},{\"gridX\":268,\"gridY\":96,\"type\":24},{\"gridX\":236,\"gridY\":148,\"type\":34},{\"gridX\":244,\"gridY\":136,\"type\":34},{\"gridX\":284,\"gridY\":152,\"type\":34},{\"gridX\":268,\"gridY\":152,\"type\":34},{\"gridX\":352,\"gridY\":96,\"type\":13},{\"gridX\":357,\"gridY\":96,\"type\":15},{\"gridX\":379,\"gridY\":96,\"type\":15},{\"gridX\":401,\"gridY\":96,\"type\":15},{\"gridX\":423,\"gridY\":96,\"type\":15},{\"gridX\":445,\"gridY\":96,\"type\":15},{\"gridX\":467,\"gridY\":96,\"type\":15},{\"gridX\":489,\"gridY\":96,\"type\":15},{\"gridX\":511,\"gridY\":96,\"type\":15},{\"gridX\":533,\"gridY\":96,\"type\":15},{\"gridX\":555,\"gridY\":96,\"type\":15},{\"gridX\":577,\"gridY\":96,\"type\":15},{\"gridX\":599,\"gridY\":96,\"type\":15},{\"gridX\":621,\"gridY\":96,\"type\":15},{\"gridX\":643,\"gridY\":96,\"type\":15},{\"gridX\":665,\"gridY\":96,\"type\":15},{\"gridX\":687,\"gridY\":96,\"type\":15},{\"gridX\":709,\"gridY\":96,\"type\":15},{\"gridX\":742,\"gridY\":96,\"type\":16},{\"gridX\":731,\"gridY\":96,\"type\":14},{\"gridX\":520,\"gridY\":148,\"type\":34},{\"gridX\":520,\"gridY\":164,\"type\":34},{\"gridX\":520,\"gridY\":180,\"type\":34},{\"gridX\":520,\"gridY\":132,\"type\":34},{\"gridX\":520,\"gridY\":196,\"type\":34},{\"gridX\":520,\"gridY\":212,\"type\":34},{\"gridX\":520,\"gridY\":228,\"type\":34},{\"gridX\":520,\"gridY\":244,\"type\":34},{\"gridX\":360,\"gridY\":84,\"type\":30},{\"gridX\":360,\"gridY\":55,\"type\":27},{\"gridX\":524,\"gridY\":44,\"type\":34},{\"gridX\":540,\"gridY\":44,\"type\":34},{\"gridX\":564,\"gridY\":76,\"type\":34},{\"gridX\":576,\"gridY\":60,\"type\":34},{\"gridX\":588,\"gridY\":76,\"type\":34},{\"gridX\":608,\"gridY\":16,\"type\":34},{\"gridX\":628,\"gridY\":76,\"type\":34},{\"gridX\":736,\"gridY\":75,\"type\":27},{\"gridX\":736,\"gridY\":84,\"type\":30},{\"gridX\":772,\"gridY\":7,\"type\":25},{\"gridX\":740,\"gridY\":44,\"type\":28},{\"gridX\":716,\"gridY\":84,\"type\":29},{\"gridX\":728,\"gridY\":70,\"type\":31},{\"gridX\":728,\"gridY\":64,\"type\":31},{\"gridX\":804,\"gridY\":7,\"type\":21},{\"gridX\":848,\"gridY\":95,\"type\":35},{\"gridX\":896,\"gridY\":95,\"type\":35},{\"gridX\":848,\"gridY\":148,\"type\":0},{\"gridX\":852,\"gridY\":148,\"type\":1},{\"gridX\":866,\"gridY\":148,\"type\":2},{\"gridX\":692,\"gridY\":116,\"type\":0},{\"gridX\":696,\"gridY\":116,\"type\":1},{\"gridX\":710,\"gridY\":116,\"type\":1},{\"gridX\":724,\"gridY\":116,\"type\":1},{\"gridX\"" << ":738,\"gridY\":116,\"type\":2},{\"gridX\":1112,\"gridY\":96,\"type\":36},{\"gridX\":1000,\"gridY\":96,\"type\":7},{\"gridX\":1052,\"gridY\":96,\"type\":8},{\"gridX\":568,\"gridY\":7,\"type\":26},{\"gridX\":916,\"gridY\":96,\"type\":19},{\"gridX\":924,\"gridY\":96,\"type\":22},{\"gridX\":932,\"gridY\":96,\"type\":22},{\"gridX\":940,\"gridY\":96,\"type\":22},{\"gridX\":948,\"gridY\":96,\"type\":22},{\"gridX\":956,\"gridY\":96,\"type\":22},{\"gridX\":1188,\"gridY\":27,\"type\":41},{\"gridX\":1196,\"gridY\":84,\"type\":30},{\"gridX\":1248,\"gridY\":48,\"type\":4},{\"gridX\":1244,\"gridY\":48,\"type\":3},{\"gridX\":1262,\"gridY\":48,\"type\":4},{\"gridX\":1276,\"gridY\":48,\"type\":5},{\"gridX\":1524,\"gridY\":27,\"type\":20},{\"gridX\":1560,\"gridY\":96,\"type\":17},{\"gridX\":1544,\"gridY\":96,\"type\":18},{\"gridX\":768,\"gridY\":96,\"type\":10},{\"gridX\":1584,\"gridY\":96,\"type\":11},{\"gridX\":1612,\"gridY\":96,\"type\":6},{\"gridX\":1416,\"gridY\":27,\"type\":41},{\"gridX\":1680,\"gridY\":96,\"type\":6},{\"gridX\":1749,\"gridY\":96,\"type\":6},{\"gridX\":1128,\"gridY\":160,\"type\":34},{\"gridX\":1136,\"gridY\":172,\"type\":34},{\"gridX\":1144,\"gridY\":184,\"type\":34},{\"gridX\":1152,\"gridY\":196,\"type\":34},{\"gridX\":1160,\"gridY\":208,\"type\":34},{\"gridX\":1168,\"gridY\":220,\"type\":34},{\"gridX\":1176,\"gridY\":232,\"type\":34},{\"gridX\":1184,\"gridY\":244,\"type\":34}],\"midBossForegroundObjects\":[],\"endBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":1776,\"gridY\":96}],\"endBossSnakes\":[],\"enemies\":[{\"gridX\":124,\"gridY\":96,\"type\":8},{\"gridX\":144,\"gridY\":96,\"type\":9},{\"gridX\":168,\"gridY\":96,\"type\":10},{\"gridX\":184,\"gridY\":96,\"type\":6},{\"gridX\":344,\"gridY\":157,\"type\":6},{\"gridX\":264,\"gridY\":121,\"type\":6},{\"gridX\":465,\"gridY\":77,\"type\":7},{\"gridX\":584,\"gridY\":7,\"type\":6},{\"gridX\":604,\"gridY\":77,\"type\":7},{\"gridX\":624,\"gridY\":7,\"type\":6},{\"gridX\":644,\"gridY\":77,\"type\":7},{\"gridX\":668,\"gridY\":20,\"type\":3},{\"gridX\":708,\"gridY\":8,\"type\":3},{\"gridX\":688,\"gridY\":77,\"type\":7},{\"gridX\":868,\"gridY\":96,\"type\":6},{\"gridX\":848,\"gridY\":153,\"type\":6},{\"gridX\":1032,\"gridY\":96,\"type\":5},{\"gridX\":1132,\"gridY\":27,\"type\":11},{\"gridX\":1120,\"gridY\":27,\"type\":12},{\"gridX\":1312,\"gridY\":55,\"type\":4}],\"collectibles\":[{\"gridX\":140,\"gridY\":104,\"type\":0},{\"gridX\":132,\"gridY\":104,\"type\":0},{\"gridX\":148,\"gridY\":104,\"type\":0},{\"gridX\":212,\"gridY\":156,\"type\":0},{\"gridX\":216,\"gridY\":164,\"type\":0},{\"gridX\":200,\"gridY\":132,\"type\":0},{\"gridX\":204,\"gridY\":140,\"type\":0},{\"gridX\":208,\"gridY\":148,\"type\":0},{\"gridX\":240,\"gridY\":172,\"type\":0},{\"gridX\":248,\"gridY\":172,\"type\":0},{\"gridX\":256,\"gridY\":172,\"type\":0},{\"gridX\":320,\"gridY\":168,\"type\":0},{\"gridX\":312,\"gridY\":172,\"type\":0},{\"gridX\":304,\"gridY\":172,\"type\":0},{\"gridX\":328,\"gridY\":104,\"type\":0},{\"gridX\":320,\"gridY\":112,\"type\":0},{\"gridX\":312,\"gridY\":120,\"type\":0},{\"gridX\":284,\"gridY\":128,\"type\":1},{\"gridX\":336,\"gridY\":80,\"type\":0},{\"gridX\":336,\"gridY\":72,\"type\":0},{\"gridX\":336,\"gridY\":64,\"type\":0},{\"gridX\":368,\"gridY\":68,\"type\":0},{\"gridX\":376,\"gridY\":68,\"type\":0},{\"gridX\":384,\"gridY\":68,\"type\":0},{\"gridX\":424,\"gridY\":64,\"type\":0},{\"gridX\":432,\"gridY\":64,\"type\":0},{\"gridX\":416,\"gridY\":64,\"type\":0},{\"gridX\":476,\"gridY\":64,\"type\":0},{\"gridX\":484,\"gridY\":56,\"type\":0},{\"gridX\":468,\"gridY\":64,\"type\":0},{\"gridX\":540,\"gridY\":56,\"type\":1},{\"gridX\":592,\"gridY\":32,\"type\":0},{\"gridX\":596,\"gridY\":40,\"type\":0},{\"gridX\":600,\"gridY\":48,\"type\":0},{\"gridX\":604,\"gridY\":56,\"type\":0},{\"gridX\":608,\"gridY\":64,\"type\":0},{\"gridX\":612,\"gridY\":56,\"type\":0},{\"gridX\":616,\"gridY\":48,\"type\":0},{\"gridX\":620,\"gridY\":40,\"type\":0},{\"gridX\":624,\"gridY\":32,\"type\":0},{\"gridX\":632,\"gridY\":32,\"type\":0},{\"gridX\":636,\"gridY\":40,\"type\":0},{\"gridX\":640,\"gridY\":48,\"type\":0},{\"gridX\":644,\"gridY\":56,\"type\":0},{\"gridX\":648,\"gridY\":64,\"type\":0},{\"gridX\":652,\"gridY\":56,\"type\":0},{\"gridX\":656,\"gridY\":48,\"type\":0},{\"gridX\":660,\"gridY\":40,\"type\":0},{\"gridX\":664,\"gridY\":32,\"type\":0},{\"gridX\":676,\"gridY\":32,\"type\":0},{\"gridX\":680,\"gridY\":40,\"type\":0},{\"gridX\":684,\"gridY\":48,\"type\":0},{\"gridX\":688,\"gridY\":56,\"type\":0},{\"gridX\":692,\"gridY\":64,\"type\":0},{\"gridX\":732,\"gridY\":28,\"type\":0},{\"gridX\":740,\"gridY\":28,\"type\":0},{\"gridX\":748,\"gridY\":28,\"type\":0},{\"gridX\":780,\"gridY\":24,\"type\":0},{\"gridX\":788,\"gridY\":24,\"type\":0},{\"gridX\":796,\"gridY\":24,\"type\":0},{\"gridX\":812,\"gridY\":48,\"type\":0},{\"gridX\":816,\"gridY\":64,\"type\":0},{\"gridX\":820,\"gridY\":80,\"type\":0},{\"gridX\":976,\"gridY\":128,\"type\":1},{\"gridX\":828,\"gridY\":104,\"type\":0},{\"gridX\":832,\"gridY\":120,\"type\":0},{\"gridX\":836,\"gridY\":136,\"type\":0},{\"gridX\":712,\"gridY\":128,\"type\":0},{\"gridX\":720,\"gridY\":128,\"type\":0},{\"gridX\":728,\"gridY\":128,\"type\":0},{\"gridX\":1048,\"gridY\":108,\"type\":0},{\"gridX\":1056,\"gridY\":112,\"type\":0},{\"gridX\":1064,\"gridY\":108,\"type\":0},{\"gridX\":1092,\"gridY\":36,\"type\":0},{\"gridX\":1076,\"gridY\":36,\"type\":0},{\"gridX\":1084,\"gridY\":36,\"type\":0},{\"gridX\":1164,\"gridY\":36,\"type\":0},{\"gridX\":1172,\"gridY\":36,\"type\":0},{\"gridX\":1180,\"gridY\":36,\"type\":0},{\"gridX\":1204,\"gridY\":68,\"type\":0},{\"gridX\":1212,\"gridY\":68,\"type\":0},{\"gridX\":1220,\"gridY\":68,\"type\":0},{\"gridX\":1252,\"gridY\":60,\"type\":0},{\"gridX\":1260,\"gridY\":60,\"type\":0},{\"gridX\":1268,\"gridY\":60,\"type\":0},{\"gridX\":1320,\"gridY\":64,\"type\":0},{\"gridX\":1328,\"gridY\":64,\"type\":0},{\"gridX\":1336,\"gridY\":64,\"type\":0},{\"gridX\":1380,\"gridY\":36,\"type\":0},{\"gridX\":1388,\"gridY\":36,\"type\":0},{\"gridX\":1372,\"gridY\":36,\"type\":0},{\"gridX\":1440,\"gridY\":64,\"type\":0},{\"gridX\":1432,\"gridY\":64,\"type\":0},{\"gridX\":1424,\"gridY\":64,\"type\":0},{\"gridX\":1652,\"gridY\":124,\"type\":0},{\"gridX\":1660,\"gridY\":120,\"type\":0},{\"gridX\":1668,\"gridY\":124,\"type\":0},{\"gridX\":1720,\"gridY\":124,\"type\":0},{\"gridX\":1728,\"gridY\":120,\"type\":0},{\"gridX\":1736,\"gridY\":124,\"type\":0},{\"gridX\":1504,\"gridY\":36,\"type\":0},{\"gridX\":1496,\"gridY\":36,\"type\":0},{\"gridX\":1488,\"gridY\":36,\"type\":0}],\"jons\":[{\"gridX\":16,\"gridY\":104}],\"extraForegroundObjects\":[{\"gridX\":960,\"gridY\":95,\"type\":39}],\"markers\":[{\"gridX\":2133,\"gridY\":0,\"type\":1}]}";
    
    static std::string text = ss.str();
    
    static Chapter1Level20 *instance = new Chapter1Level20(text.c_str());
    
    return instance;
}
