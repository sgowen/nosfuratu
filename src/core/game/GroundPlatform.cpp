//
//  GroundPlatform.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 9/4/15.
//  Copyright (c) 2015 Gowen Game Dev. All rights reserved.
//

#include "GroundPlatform.h"

GroundPlatform* GroundPlatform::create(float x, float y, int type)
{
    switch ((GroundPlatformType) type)
    {
        case GroundPlatformType::GroundPlatformType_GrassDefault:
            return new GroundPlatformGrassDefault(x, y);
        case GroundPlatformType::GroundPlatformType_GrassEndLeft:
            return new GroundPlatformGrassEndLeft(x, y);
        case GroundPlatformType::GroundPlatformType_GrassCenter:
            return new GroundPlatformGrassCenter(x, y);
        case GroundPlatformType::GroundPlatformType_GrassEndRight:
            return new GroundPlatformGrassEndRight(x, y);
        case GroundPlatformType::GroundPlatformType_CaveEndLeft:
            return new GroundPlatformCaveEndLeft(x, y);
        case GroundPlatformType::GroundPlatformType_CaveCenter:
            return new GroundPlatformCaveCenter(x, y);
        case GroundPlatformType::GroundPlatformType_CaveEndRight:
        default:
            return new GroundPlatformCaveEndRight(x, y);
    }
}

GroundPlatform::GroundPlatform(float x, float y, float width, float height, GroundPlatformType type) : LandPhysicalEntity(x, y, width, height), m_type(type)
{
    updateBounds();
}

void GroundPlatform::updateBounds()
{
    m_bounds->setHeight(getHeight());
    
    PhysicalEntity::updateBounds();
    
    m_bounds->setHeight(getHeight() * 0.80f);
}

GroundPlatformType GroundPlatform::getEnumType()
{
    return m_type;
}

int GroundPlatform::getType()
{
    return m_type;
}

GroundSoundType GroundPlatform::getGroundSoundType()
{
    return m_type == GroundPlatformType_CaveEndLeft || m_type == GroundPlatformType_CaveCenter || m_type == GroundPlatformType_CaveEndRight ? GROUND_SOUND_CAVE : GROUND_SOUND_GRASS;
}