//
//  Renderer.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 2/22/14.
//  Copyright (c) 2014 Gowen Game Dev. All rights reserved.
//

#include "Renderer.h"
#include "ResourceConstants.h"
#include "SpriteBatcher.h"
#include "TextureRegion.h"
#include "Assets.h"
#include "GameObject.h"
#include "GameConstants.h"
#include "Vector2D.h"
#include "Rectangle.h"

Renderer::Renderer()
{
    // TODO
}

void Renderer::renderBackground()
{
    m_spriteBatcher->beginBatch();
    
    static GameObject go = GameObject(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    
    renderGameObject(go, Assets::getBackground());
    
    m_spriteBatcher->endBatchWithTexture(*m_backgroundTexture);
}

void Renderer::renderGameObject(GameObject &go, TextureRegion tr)
{
    m_spriteBatcher->drawSprite(go.getPosition().getX(), go.getPosition().getY(), go.getWidth(), go.getHeight(), go.getAngle(), tr);
}