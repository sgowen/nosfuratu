//
//  EntityUtils.h
//  nosfuratu
//
//  Created by Stephen Gowen on 9/23/15.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#ifndef __nosfuratu__EntityUtils__
#define __nosfuratu__EntityUtils__

#include "OverlapTester.h"
#include "PhysicalEntity.h"
#include "GridLockedPhysicalEntity.h"
#include "NGRect.h"
#include "Vector2D.h"
#include "Jon.h"
#include "ForegroundObject.h"

#include <math.h>
#include <vector>

class PhysicalEntity;

class EntityUtils
{
public:
    static void attach(GridLockedPhysicalEntity& entity, GridLockedPhysicalEntity& to, int gridCellSizeScalar, bool leftOf, bool yCorrection)
    {
        if (leftOf)
        {
            float left = to.getMainBounds().getLeft();
            float x = left - entity.getMainBounds().getWidth() / 2;
            
            entity.getPosition().setX(x);
        }
        else
        {
            float right = to.getMainBounds().getLeft() + to.getMainBounds().getWidth();
            float x = right + entity.getMainBounds().getWidth() / 2;
            
            entity.getPosition().setX(x);
        }
        
        entity.updateBounds();
        
        if (yCorrection)
        {
            float top = entity.getMainBounds().getTop();
            float topTo = to.getMainBounds().getTop();
            float yDelta = topTo - top;
            entity.getPosition().add(0, yDelta);
            entity.updateBounds();
        }
    }
    
    static void placeOn(GridLockedPhysicalEntity& entity, GridLockedPhysicalEntity& on, int gridCellSizeScalar)
    {
		float top = on.getMainBounds().getTop();

		entity.getMainBounds().getLowerLeft().setY(top);

		entity.snapToGrid(gridCellSizeScalar);
    }
    
    static void placeUnder(GridLockedPhysicalEntity& entity, GridLockedPhysicalEntity& under, int gridCellSizeScalar)
    {
        float boundsHeight = entity.getMainBounds().getHeight();
        float bottom = under.getMainBounds().getBottom();
        float lowerLeft = bottom - boundsHeight;
        
        entity.getMainBounds().getLowerLeft().setY(lowerLeft);

		entity.snapToGrid(gridCellSizeScalar);
    }
    
    template<typename T>
    static bool isLanding(PhysicalEntity* entity, std::vector<T>& items, float deltaTime)
    {
        float entityVelocityY = entity->getVelocity().getY();
        
        if (entityVelocityY <= 0)
        {
			int highestPriority = 0;
			for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
			{
				if ((*i) == entity)
				{
					continue;
				}

				int priority = (*i)->getEntityLandingPriority();
				if (priority > highestPriority)
				{
					highestPriority = priority;
				}
			}

			for (int p = highestPriority; p >= 0; p--)
			{
				for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
				{
					if ((*i) == entity)
					{
						continue;
					}

					int priority = (*i)->getEntityLandingPriority();

					if (p == priority
						&& (*i)->isEntityLanding(entity, deltaTime))
					{
						return true;
					}
				}
			}
        }
        
        return false;
    }
    
    template<typename T>
    static bool isFallingThroughHole(PhysicalEntity* entity, std::vector<T>& items, float deltaTime)
    {
        if (entity->getVelocity().getY() <= 0)
        {
            for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
            {
                if (OverlapTester::doNGRectsOverlap(entity->getMainBounds(), (*i)->getMainBounds()))
                {
                    return (*i)->isCoverBreaking() || !(*i)->hasCover();
                }
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isFallingThroughPit(PhysicalEntity* entity, std::vector<T>& items, float deltaTime)
    {
        float entityLeft = entity->getMainBounds().getLeft();
        float entityRight = entity->getMainBounds().getRight();
        float entityBottom = entity->getMainBounds().getBottom();
        
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            float itemLeftX = (*i)->getMainBounds().getLeft();
            float itemRight = (*i)->getMainBounds().getRight();
            float itemTop = (*i)->getMainBounds().getTop() * 0.99f;
            
            if (OverlapTester::doNGRectsOverlap(entity->getMainBounds(), (*i)->getMainBounds()))
            {
                if (entityBottom < itemTop)
                {
                    return true;
                }
                
                if (entityLeft >= itemLeftX && entityRight <= itemRight)
                {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isBurrowingThroughHole(Jon& jon, std::vector<T>& items)
    {
        bool ret = false;
        if (jon.getVelocity().getY() <= 0)
        {
            for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
            {
                if (OverlapTester::doNGRectsOverlap(jon.getMainBounds(), (*i)->getMainBounds()))
                {
                    if ((*i)->triggerBurrow())
                    {
                        ret = true;
                    }
                }
            }
        }
        
        return ret;
    }
    
    template<typename T>
    static bool isBlockedOnRight(PhysicalEntity* entity, std::vector<T*>& items, float deltaTime)
    {
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
			if ((*i) == entity)
			{
				continue;
			}

            if ((*i)->isEntityBlockedOnRight(entity, deltaTime))
            {
                return true;
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isBlockedOnLeft(PhysicalEntity* entity, std::vector<T*>& items, float deltaTime)
    {
		for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
			if ((*i) == entity)
			{
				continue;
			}

			if ((*i)->isEntityBlockedOnLeft(entity, deltaTime))
			{
				return true;
			}
        }
        
        return false;
    }
    
    template<typename T>
    static bool isBlockedAbove(Jon& jon, std::vector<T>& items, float deltaTime)
    {
        float entityVelocityY = jon.getVelocity().getY();
        
        if (entityVelocityY > 0)
        {
            for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
            {
                if ((*i)->isJonBlockedAbove(jon, deltaTime))
                {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isHittingFromBelow(Jon& jon, std::vector<T>& items, float deltaTime)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            if ((*i)->isJonHittingFromBelow(jon, deltaTime))
            {
                return true;
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isHorizontallyHitting(Jon& jon, std::vector<T>& items, float deltaTime)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            if ((*i)->isJonHittingHorizontally(jon, deltaTime))
            {
                return true;
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isKilledByEnemy(PhysicalEntity& entity, std::vector<T>& items)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            if ((*i)->hasKilledJon())
            {
                return true;
            }
        }
        
        return false;
    }
    
    template<typename T>
    static bool isFallingIntoDeath(PhysicalEntity& entity, std::vector<T>& items)
    {
        float entityVelocityY = entity.getVelocity().getY();
        if (entityVelocityY < 0)
        {
            float originalY = entity.getPosition().getY();
            entity.getPosition().sub(0, entity.getVelocity().getY() * 0.5f);
            entity.updateBounds();
            
            float width = entity.getMainBounds().getWidth();
            float height = entity.getMainBounds().getHeight();
            float x = entity.getMainBounds().getLeft();
            float y = entity.getMainBounds().getBottom();
            
            NGRect tempBounds = NGRect(x, y, width, height);
            
            entity.getPosition().setY(originalY);
            entity.updateBounds();
            
            for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
            {
                if (OverlapTester::doNGRectsOverlap(entity.getMainBounds(), (*i)->getMainBounds()))
                {
                    if (tempBounds.getBottom() > (*i)->getMainBounds().getTop())
                    {
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
    
    template<typename T>
    static void updateBackgrounds(std::vector<T>& items, Vector2D& cameraPosition, float deltaTime)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            (*i)->update(cameraPosition, deltaTime);
        }
    }
    
    template<typename T>
    static void update(std::vector<T>& items, float deltaTime)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            (*i)->update(deltaTime);
        }
    }
    
    template<typename T>
    static void updateAndClean(std::vector<T*>& items, float deltaTime)
    {
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); )
        {
            (*i)->update(deltaTime);
            
            if ((*i)->isRequestingDeletion())
            {
                delete *i;
                i = items.erase(i);
            }
            else
            {
                i++;
            }
        }
    }
    
    template<typename T>
    static void addAll(std::vector<T>& itemsFrom, std::vector<GridLockedPhysicalEntity*>& itemsTo)
    {
        for (typename std::vector<T>::iterator i = itemsFrom.begin(); i != itemsFrom.end(); i++)
        {
            itemsTo.push_back((*i));
        }
    }
    
    template<typename T>
    static int isTouching(std::vector<T*>& items, Vector2D& touchPoint)
    {
        int last = ((int) items.size()) - 1;
        for (int i = last; i >= 0; i--)
        {
            T* item = items.at(i);
            float width = item->getWidth();
            float height = item->getHeight();
            float x = item->getPosition().getX() - width / 2;
            float y = item->getPosition().getY() - height / 2;
            
            NGRect tempBounds = NGRect(x, y, width, height);
            if (OverlapTester::isPointInNGRect(touchPoint, tempBounds))
            {
                return i;
            }
        }
        
        return -1;
    }
    
    template<typename T>
    static int indexOfOverlappingObjectThatCanBePlacedOn(PhysicalEntity* pe, std::vector<T>& items)
    {
        int index = 0;
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++, index++)
        {
            if ((*i) == pe)
            {
                continue;
            }
            
            if ((*i)->canObjectBePlacedOn()
				&& OverlapTester::doNGRectsOverlap(pe->getMainBounds(), (*i)->getMainBounds()))
            {
				return index;
            }
        }
        
        return -1;
    }
    
    template<typename T>
    static int indexOfOverlappingObjectThatCanBePlacedUnder(PhysicalEntity* pe, std::vector<T>& items)
    {
        int index = 0;
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++, index++)
        {
            if ((*i) == pe)
            {
                continue;
            }
            
            if ((*i)->canObjectBePlacedUnder() && OverlapTester::doNGRectsOverlap(pe->getMainBounds(), (*i)->getMainBounds()))
            {
                return index;
            }
        }
        
        return -1;
    }
    
    template<typename T>
    static void setGameToEntities(std::vector<T*>& items, Game* game)
    {
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            (*i)->setGame(game);
        }
    }
    
    template<typename T>
    static void copyAndOffset(std::vector<T*>& items, int beginGridX, int endGridX)
    {
        int gridSpacing = endGridX - beginGridX;
        float offset = gridSpacing * GRID_CELL_SIZE + GRID_CELL_SIZE / 3.0f;
        
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            if ((*i)->getGridX() >= endGridX)
            {
                (*i)->getPosition().add(offset, 0);
                (*i)->updateBounds();
                (*i)->snapToGrid(1);
            }
        }
        
		std::vector<T*> newItems;
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            if((*i)->getGridX() >= beginGridX && (*i)->getGridX() < endGridX)
            {
				newItems.push_back(T::create((*i)->getGridX() + gridSpacing, (*i)->getGridY(), (*i)->getType()));
            }
        }

		items.insert(items.end(), newItems.begin(), newItems.end());
    }
    
    template<typename T>
    static void offsetOnly(std::vector<T*>& items, int beginGridX, int endGridX)
    {
        int gridSpacing = endGridX - beginGridX;
        float offset = gridSpacing * GRID_CELL_SIZE + GRID_CELL_SIZE / 3.0f;
        
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            if ((*i)->getGridX() >= endGridX)
            {
                (*i)->getPosition().add(offset, 0);
                (*i)->updateBounds();
                (*i)->snapToGrid(1);
            }
        }
    }
    
    template<typename T>
    static void offsetAll(std::vector<T*>& items, int beginGridX, int endGridX)
    {
        int gridSpacing = endGridX - beginGridX;
        float offset = gridSpacing * GRID_CELL_SIZE + GRID_CELL_SIZE / 3.0f;
        
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            (*i)->getPosition().add(offset, 0);
            (*i)->updateBounds();
            (*i)->snapToGrid(1);
        }
    }
    
    template<typename T>
    static void offsetAllInRangeOpenEnd(std::vector<T*>& items, int beginGridX, int endGridX, int gridOffset)
    {
        float offset = gridOffset * GRID_CELL_SIZE + GRID_CELL_SIZE / 3.0f;
        
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            if ((*i)->getGridX() >= beginGridX && (*i)->getGridX() < endGridX)
            {
                (*i)->getPosition().add(offset, 0);
                (*i)->updateBounds();
                (*i)->snapToGrid(1);
            }
        }
    }

	template<typename T>
	static void offsetAllInRangeClosedEnd(std::vector<T*>& items, int beginGridX, int endGridX, int gridOffset)
	{
		float offset = gridOffset * GRID_CELL_SIZE + GRID_CELL_SIZE / 3.0f;

		for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
		{
			if ((*i)->getGridX() >= beginGridX && (*i)->getGridX() <= endGridX)
			{
				(*i)->getPosition().add(offset, 0);
				(*i)->updateBounds();
				(*i)->snapToGrid(1);
			}
		}
	}
    
    template<typename T>
    static int boxInAll(std::vector<T*>& items, float eX, float eY, float eWidth, float eHeight, int index)
    {
        float size = fminf(eWidth, eHeight);
        
        for (typename std::vector<T*>::iterator i = items.begin(); i != items.end(); i++)
        {
            T* item = *i;
            
            item->getPosition().set(eX, eY + (index++ * eHeight));
            item->setWidth(eWidth);
            item->setHeight(eHeight);
            
            if (item->getWidth() > item->getHeight())
            {
                item->setHeight(item->getHeight() / item->getWidth());
                item->setHeight(item->getHeight() * size);
                item->setWidth(size);
            }
            else
            {
                item->setWidth(item->getWidth() / item->getHeight());
                item->setWidth(item->getWidth() * size);
                item->setHeight(size);
            }
        }
        
        return index;
    }
    
private:
    // ctor, copy ctor, and assignment should be private in a Singleton
    EntityUtils();
    EntityUtils(const EntityUtils&);
    EntityUtils& operator=(const EntityUtils&);
};

#endif /* defined(__nosfuratu__EntityUtils__) */
