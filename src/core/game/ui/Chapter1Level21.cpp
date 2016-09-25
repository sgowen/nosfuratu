//
//  Chapter1Level21.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 3/16/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#include "GameScreenLevels.h"
#include "State.h"
#include "GameScreen.h"
#include "EntityUtils.h"
#include "Vector2D.h"
#include "Game.h"

void Chapter1Level21::enter(GameScreen* gs)
{
    Level::enter(gs);
    
    for (std::vector<Hole*>::iterator i = m_game->getHoles().begin(); i != m_game->getHoles().end(); i++)
    {
        if ((*i)->getType() == HoleType_Grass)
        {
            m_hole = (*i);
            break;
        }
    }
    
    if (m_game->getEndBossSnakes().size() > 0)
    {
        m_endBossSnake = m_game->getEndBossSnakeP();
    }
    
    m_game->getCountHissWithMina().faceLeft();
    
    Jon& jon = m_game->getJon();
    jon.setUserActionPrevented(true);
}

void Chapter1Level21::exit(GameScreen* gs)
{
    m_hole = nullptr;
    
    Level::exit(gs);
}

void Chapter1Level21::update(GameScreen* gs)
{
    Level::update(gs);
    
    if (m_game->getJons().size() == 0
        || !m_endBossSnake
        || !m_hole)
    {
        return;
    }
    
    Jon& jon = m_game->getJon();
    
    if (m_endBossSnake->getState() == EndBossSnakeState_Sleeping)
    {
        if (jon.getPosition().getX() > m_hole->getPosition().getX())
        {
            jon.getPosition().setX(m_hole->getPosition().getX());
        }
    }
}

void Chapter1Level21::updateCamera(GameScreen* gs, float paddingX, bool ignoreY, bool instant)
{
    if (m_isChaseCamActivated)
    {
        gs->m_renderer->updateCameraToFollowJon(*m_game, m_batPanel.get(), gs->m_fDeltaTime, paddingX, true, ignoreY, instant);
    }
    else
    {
        Level::updateCamera(gs, paddingX, ignoreY, instant);
    }
}

void Chapter1Level21::additionalRenderingBeforeHud(GameScreen* gs)
{
    gs->m_renderer->renderEndBossSnake(*m_endBossSnake);
}

bool Chapter1Level21::isInSlowMotionMode()
{
    return false;
}

Chapter1Level21::Chapter1Level21(const char* json) : Level(json),
m_endBossSnake(nullptr),
m_hole(nullptr),
m_isChaseCamActivated(false)
{
    // Empty
}

/// Chapter 1 Level 21 ///

Chapter1Level21 * Chapter1Level21::getInstance()
{
    static Chapter1Level21 *instance = new Chapter1Level21("{\"world\":1,\"level\":21,\"midgrounds\":[],\"grounds\":[{\"gridX\":0,\"gridY\":0,\"type\":0},{\"gridX\":8,\"gridY\":0,\"type\":3},{\"gridX\":136,\"gridY\":0,\"type\":3},{\"gridX\":264,\"gridY\":0,\"type\":3},{\"gridX\":392,\"gridY\":0,\"type\":3},{\"gridX\":520,\"gridY\":0,\"type\":3},{\"gridX\":648,\"gridY\":0,\"type\":3},{\"gridX\":776,\"gridY\":0,\"type\":3},{\"gridX\":904,\"gridY\":0,\"type\":3},{\"gridX\":1032,\"gridY\":0,\"type\":3},{\"gridX\":1160,\"gridY\":0,\"type\":3},{\"gridX\":1288,\"gridY\":0,\"type\":3},{\"gridX\":1416,\"gridY\":0,\"type\":3},{\"gridX\":148,\"gridY\":48,\"type\":10},{\"gridX\":156,\"gridY\":48,\"type\":13},{\"gridX\":284,\"gridY\":48,\"type\":13},{\"gridX\":412,\"gridY\":48,\"type\":13},{\"gridX\":540,\"gridY\":48,\"type\":13},{\"gridX\":144,\"gridY\":48,\"type\":15},{\"gridX\":152,\"gridY\":48,\"type\":18},{\"gridX\":280,\"gridY\":48,\"type\":18},{\"gridX\":408,\"gridY\":48,\"type\":18},{\"gridX\":536,\"gridY\":48,\"type\":18},{\"gridX\":664,\"gridY\":48,\"type\":19},{\"gridX\":0,\"gridY\":90,\"type\":20},{\"gridX\":8,\"gridY\":90,\"type\":23},{\"gridX\":136,\"gridY\":88,\"type\":23},{\"gridX\":264,\"gridY\":88,\"type\":23},{\"gridX\":392,\"gridY\":88,\"type\":23},{\"gridX\":520,\"gridY\":88,\"type\":23},{\"gridX\":648,\"gridY\":88,\"type\":23},{\"gridX\":776,\"gridY\":88,\"type\":23},{\"gridX\":904,\"gridY\":88,\"type\":23},{\"gridX\":1032,\"gridY\":88,\"type\":23},{\"gridX\":1160,\"gridY\":88,\"type\":23},{\"gridX\":1288,\"gridY\":88,\"type\":23},{\"gridX\":1416,\"gridY\":88,\"type\":23}],\"pits\":[],\"exitGrounds\":[],\"holes\":[{\"gridX\":128,\"gridY\":80,\"type\":3}],\"foregroundObjects\":[{\"gridX\":200,\"gridY\":132,\"type\":0},{\"gridX\":204,\"gridY\":132,\"type\":1},{\"gridX\":218,\"gridY\":132,\"type\":1},{\"gridX\":232,\"gridY\":132,\"type\":2},{\"gridX\":1536,\"gridY\":96,\"type\":12},{\"gridX\":148,\"gridY\":75,\"type\":27},{\"gridX\":148,\"gridY\":84,\"type\":30},{\"gridX\":184,\"gridY\":75,\"type\":27},{\"gridX\":184,\"gridY\":84,\"type\":30},{\"gridX\":220,\"gridY\":75,\"type\":27},{\"gridX\":220,\"gridY\":84,\"type\":30}],\"midBossForegroundObjects\":[],\"countHissWithMinas\":[{\"gridX\":200,\"gridY\":137}],\"enemies\":[],\"collectibles\":[],\"jons\":[{\"gridX\":24,\"gridY\":108}],\"extraForegroundObjects\":[],\"markers\":[]}");
    
    return instance;
}
