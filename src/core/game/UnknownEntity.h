//
//  UnknownEntity.h
//  nosfuratu
//
//  Created by Stephen Gowen on 11/8/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#ifndef __nosfuratu__UnknownEntity__
#define __nosfuratu__UnknownEntity__

#include "PhysicalEntity.h"
#include "Vector2D.h"
#include "Rectangle.h"
#include "RTTI.h"

class UnknownEntity : public PhysicalEntity
{
    RTTI_DECL;
    
public:
    static UnknownEntity* create(std::string assetId, float x = 0, float y = 0);
    
    UnknownEntity(std::string assetId, float x, float y, float width, float height);
    
    void reset() { m_fStateTime = 0; }
    
    std::string& getAssetId() { return m_assetId; }
    
private:
    std::string m_assetId;
};

#endif /* defined(__nosfuratu__UnknownEntity__) */
