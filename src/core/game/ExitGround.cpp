//
//  ExitGround.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 2/8/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#include "ExitGround.h"

#include "EntityUtils.h"
#include "GameConstants.h"

ExitGround* ExitGround::create(int gridX, int gridY, int type)
{
    switch ((ExitGroundType)type)
    {
        case ExitGroundType_GrassWithCaveSmallExitMid:
            return new GrassWithCaveSmallExitMid(gridX);
        case ExitGroundType_GrassWithCaveSmallExitEnd:
            return new GrassWithCaveSmallExitEnd(gridX);
        case ExitGroundType_CaveSmallExit:
            return new CaveSmallExit(gridX);
            
        case ExitGroundType_CaveDeepSmallWaterfall:
            return new CaveDeepSmallWaterfall(gridX);
    }
    
    assert(false);
}

ExitGround::ExitGround(int gridX, int gridY, int gridWidth, int gridHeight, float boundsHeight, bool hasCover, ExitGroundType type, GroundSoundType groundSoundType) : GridLockedPhysicalEntity(gridX, gridY, gridWidth, gridHeight, 0, 0, 1, boundsHeight), m_type(type), m_groundSoundType(groundSoundType), m_exitCover(nullptr)
{
    updateBounds();
    
    if (hasCover)
    {
        ExitGroundCoverType t = m_type == ExitGroundType_GrassWithCaveSmallExitMid || m_type == ExitGroundType_GrassWithCaveSmallExitEnd ? ExitGroundCoverType_Grass : ExitGroundCoverType_Cave;
        m_exitCover = new ExitGroundCover(m_position->getX(), m_position->getY(), m_fWidth, m_fHeight, t);
    }
}

void ExitGround::update(float deltaTime)
{
    PhysicalEntity::update(deltaTime);
    
    if (m_exitCover)
    {
        m_exitCover->update(deltaTime);
        m_exitCover->getPosition().set(getPosition());
        m_exitCover->updateBounds();
        
        if (m_exitCover->isRequestingDeletion())
        {
            delete m_exitCover;
            m_exitCover = nullptr;
        }
    }
}

bool ExitGround::isEntityLanding(PhysicalEntity* entity, float deltaTime)
{
    float jonVelocityY = entity->getVelocity().getY();
    
    if (jonVelocityY <= 0)
    {
        if (OverlapTester::doRectanglesOverlap(entity->getMainBounds(), getMainBounds()))
        {
            float jonBottomY = entity->getMainBounds().getBottom();
            float jonLeftX = entity->getMainBounds().getLeft();
            float jonRightX = entity->getMainBounds().getRight();
            float jonYDelta = fabsf(jonVelocityY * deltaTime);
            
            float itemTop = getMainBounds().getTop();
            float padding = itemTop * .01f;
            padding += jonYDelta;
            float itemTopReq = itemTop - padding;
            float pitEntranceLeft = getMainBounds().getLeft() + getMainBounds().getWidth() * 0.2421875f;
            float pitEntranceRight = getMainBounds().getLeft() + getMainBounds().getWidth() * 0.71875f;
            
            bool isLanding = jonBottomY >= itemTopReq;
            if (!hasCover())
            {
                isLanding = jonRightX < pitEntranceLeft || jonLeftX > pitEntranceRight;
            }
            
            if (isLanding)
            {
				entity->placeOn(itemTop);
                
                Jon *jon;
                if ((jon = dynamic_cast<Jon *>(entity)))
                {
                    jon->setGroundSoundType(getGroundSoundType());
                }
                
                return true;
            }
        }
    }
    
    return false;
}

bool ExitGround::isJonBlockedAbove(Jon &jon, float deltaTime)
{
    float entityVelocityY = jon.getVelocity().getY();
    
    if (entityVelocityY > 0 && OverlapTester::doRectanglesOverlap(jon.getMainBounds(), getMainBounds()))
    {
        if (hasCover())
		{
			if (entityVelocityY > 10.0f)
			{
				m_exitCover->triggerHit();

				return false;
			}
			else
			{
				jon.getPosition().sub(0, jon.getVelocity().getY() * deltaTime);
				jon.updateBounds();

				return true;
			}
		}
    }
    
    return false;
}

bool ExitGround::canObjectBePlacedOn()
{
    return true;
}

bool ExitGround::hasCover()
{
    return m_exitCover ? true : false;
}

ExitGroundCover& ExitGround::getExitCover()
{
    return *m_exitCover;
}

ExitGroundType ExitGround::getType()
{
    return m_type;
}

GroundSoundType ExitGround::getGroundSoundType()
{
    return m_groundSoundType;
}

bool CaveDeepSmallWaterfall::isEntityLanding(PhysicalEntity* entity, float deltaTime)
{
    float jonVelocityY = entity->getVelocity().getY();
    
    if (jonVelocityY <= 0)
    {
        if (OverlapTester::doRectanglesOverlap(entity->getMainBounds(), getMainBounds()))
        {
            float jonBottomY = entity->getMainBounds().getBottom();
            float jonLeftX = entity->getMainBounds().getLeft();
            float jonRightX = entity->getMainBounds().getRight();
            float jonYDelta = fabsf(jonVelocityY * deltaTime);
            
            float itemTop = getMainBounds().getTop();
            float padding = itemTop * .01f;
            padding += jonYDelta;
            float itemTopReq = itemTop - padding;
            float pitEntranceLeft = getMainBounds().getLeft() + getMainBounds().getWidth() * 0.328125f;
            float pitEntranceRight = getMainBounds().getLeft() + getMainBounds().getWidth() * 0.75f;
            
            if (jonBottomY >= itemTopReq && (jonRightX < pitEntranceLeft || jonLeftX > pitEntranceRight))
            {
				entity->placeOn(itemTop);
                
                Jon *jon;
                if ((jon = dynamic_cast<Jon *>(entity)))
                {
                    jon->setGroundSoundType(getGroundSoundType());
                }
                
                return true;
            }
            else
            {
                entity->getAcceleration().setY(GAME_GRAVITY * 2);
            }
        }
    }
    
    return false;
}

bool CaveDeepSmallWaterfall::canObjectBePlacedOn()
{
    return false;
}
