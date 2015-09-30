//
//  Ground.h
//  nosfuratu
//
//  Created by Stephen Gowen on 9/5/15.
//  Copyright (c) 2015 Gowen Game Dev. All rights reserved.
//

#ifndef __nosfuratu__Ground__
#define __nosfuratu__Ground__

#include "PhysicalEntity.h"
#include "GroundType.h"
#include "EntityAnchor.h"
#include "GroundSize.h"

#include <vector>

class Ground : public PhysicalEntity
{
public:
    static void createGrassWithCave(std::vector<Ground>& grounds, float x, GroundSize gs, int length);
    
    static void createGrassWithoutCave(std::vector<Ground>& grounds, float x, GroundSize gs, int length);
    
    static void createCave(std::vector<Ground>& grounds, float x, GroundSize gs, int length);
    
    static void createCaveRaised(std::vector<Ground>& grounds, float x, GroundSize gs, int length);
    
    static Ground create(float x, GroundType groundType);
    
    Ground(float x, float width, float height, GroundType groundType, float boundsHeightFactor, float y = 0, EntityAnchor anchor = ANCHOR_BOTTOM);
    
    virtual void updateBounds();
    
    GroundType getGroundType();
    
private:
    static void create(std::vector<Ground>& grounds, float x, int length, GroundType typeLeft, GroundType typeCenter, GroundType typeRight);
    
    GroundType m_groundType;
    float m_fBoundsHeightFactor;
};

#endif /* defined(__nosfuratu__Ground__) */
