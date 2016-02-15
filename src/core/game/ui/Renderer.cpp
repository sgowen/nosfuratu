//
//  Renderer.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 2/22/14.
//  Copyright (c) 2014 Gowen Game Dev. All rights reserved.
//

#include "Renderer.h"
#include "SpriteBatcher.h"
#include "RectangleBatcher.h"
#include "LineBatcher.h"
#include "TextureRegion.h"
#include "Assets.h"
#include "PhysicalEntity.h"
#include "GameConstants.h"
#include "Vector2D.h"
#include "Rectangle.h"
#include "Game.h"
#include "BackButton.h"
#include "LevelEditorButton.h"
#include "LevelEditorActionsPanel.h"
#include "LevelEditorEntitiesPanel.h"
#include "TrashCan.h"
#include "LevelSelectorPanel.h"
#include "GpuProgramWrapper.h"
#include "TransitionGpuProgramWrapper.h"
#include "SinWaveTextureGpuProgramWrapper.h"
#include "SnakeDeathTextureGpuProgramWrapper.h"
#include "ShockwaveTextureGpuProgramWrapper.h"
#include "TransDeathGpuProgramWrapper.h"
#include "FramebufferRadialBlurGpuProgramWrapper.h"
#include "CollectibleItem.h"

#include <math.h>
#include <sstream>
#include <iomanip>

Renderer::Renderer() : m_fStateTime(0), m_iFramebufferIndex(0), m_iRadialBlurDirection(RADIAL_BLUR_DIRECTION_LEFT), m_areMenuTexturesLoaded(false), m_areWorld1TexturesLoaded(false), m_areShadersLoaded(false), m_sinWaveTextureProgram(nullptr), m_snakeDeathTextureProgram(nullptr), m_shockwaveTextureGpuProgramWrapper(nullptr), m_framebufferToScreenGpuProgramWrapper(nullptr), m_framebufferTintGpuProgramWrapper(nullptr)
{
    int x = 0;
    int y = 0;
    int glyphWidth = 64;
    int glyphHeight = 73;
    int textureSize = TEXTURE_SIZE_4096;
    
    if (Assets::getInstance()->isUsingCompressedTextureSet())
    {
        x /= 2.0;
        y /= 2.0;
        glyphWidth /= 2.0;
        glyphHeight /= 2.0;
        textureSize /= 2.0;
    }
    
    m_font = std::unique_ptr<Font>(new Font(x, y, 16, glyphWidth, glyphHeight, textureSize, textureSize));
    m_camBounds = std::unique_ptr<Rectangle>(new Rectangle(0, 0, CAM_WIDTH, CAM_HEIGHT));
    m_camPosAcceleration = std::unique_ptr<Vector2D>(new Vector2D(0, 0));
    m_camPosVelocity = std::unique_ptr<Vector2D>(new Vector2D(0, 0));
}

Renderer::~Renderer()
{
    cleanUp();
}

void Renderer::init(RendererType type)
{
    bool compressed = Assets::getInstance()->isUsingCompressedTextureSet();
    
    switch (type)
    {
        case RENDERER_TYPE_WORLD_1:
            if (!m_areWorld1TexturesLoaded)
            {
                m_jon = loadTexture(compressed ? "c_jon" : "jon");
                
                m_trans_death_shader_helper = loadTexture("trans_death_shader_helper");
                
                m_vampire = loadTexture(compressed ? "c_vampire" : "vampire");
                
                m_world_1_background_lower = loadTexture(compressed ? "c_world_1_background_lower" : "world_1_background_lower", 1);
                m_world_1_background_mid = loadTexture(compressed ? "c_world_1_background_mid" : "world_1_background_mid", 1);
                m_world_1_background_upper = loadTexture(compressed ? "c_world_1_background_upper" : "world_1_background_upper", 1);
                
                m_world_1_enemies = loadTexture(compressed ? "c_world_1_enemies" : "world_1_enemies", 1);
                
                m_world_1_ground = loadTexture(compressed ? "c_world_1_ground" : "world_1_ground");
                
                m_world_1_midground = loadTexture(compressed ? "c_world_1_midground" : "world_1_midground");
                
                m_world_1_objects = loadTexture(compressed ? "c_world_1_objects" : "world_1_objects");
                
                m_areWorld1TexturesLoaded = true;
            }
        case RENDERER_TYPE_MENU:
            if (!m_areMenuTexturesLoaded)
            {
                m_misc = loadTexture(compressed ? "c_misc" : "misc");
                
                m_areMenuTexturesLoaded = true;
            }
        default:
            break;
    }
    
    if (!m_areShadersLoaded)
    {
        loadShaderPrograms();
        
        m_areShadersLoaded = true;
    }
    
    if (m_framebuffers.size() == 0)
    {
        addFramebuffers();
    }
}

void Renderer::beginFrame()
{
    clearFramebufferWithColor(0, 0, 0, 1);
    
    setFramebuffer(0);
    
    clearFramebufferWithColor(0, 0, 0, 1);
}

void Renderer::setFramebuffer(int framebufferIndex)
{
    m_iFramebufferIndex = framebufferIndex;
    bindToOffscreenFramebuffer(framebufferIndex);
}

void Renderer::beginOpeningPanningSequence(Game& game)
{
    zoomIn();
    
    m_fStateTime = 0;
    
    updateCameraToFollowJon(game, 1337);
    
    m_camBounds->getLowerLeft().setX(getCamPosFarRight(game));
    m_camBounds->getLowerLeft().setY(game.getFarRightBottom());
    
    Jon& jon = game.getJon();
    float farLeft = jon.getPosition().getX() - CAM_WIDTH / 5;
    float farLeftBottom = jon.getPosition().getY() - jon.getHeight() / 4 * 3;
    
    float changeInX = farLeft - getCamPosFarRight(game);
    float changeInY = farLeftBottom - game.getFarRightBottom();
    
    if (changeInY > (GAME_HEIGHT / 6))
    {
        m_iRadialBlurDirection = RADIAL_BLUR_DIRECTION_TOP_LEFT;
    }
    else if (changeInY < -(GAME_HEIGHT / 6))
    {
        m_iRadialBlurDirection = RADIAL_BLUR_DIRECTION_BOTTOM_LEFT;
    }
    else
    {
        m_iRadialBlurDirection = RADIAL_BLUR_DIRECTION_LEFT;
    }
    
    m_camPosVelocity->set(changeInX, changeInY);
}

int Renderer::updateCameraToFollowPathToJon(Game& game, float deltaTime)
{
    m_fStateTime += deltaTime;
    
    if (m_fStateTime >= 5.0f && m_fStateTime <= 5.265f)
    {
        bool isComplete = false;
        float progress = (m_fStateTime - 5.0f) / 0.2f;
        if (progress > 1)
        {
            progress = 1;
            isComplete = true;
        }
        
        Jon& jon = game.getJon();
        float farLeft = jon.getPosition().getX() - CAM_WIDTH / 5;
        float farLeftBottom = jon.getPosition().getY() - jon.getHeight() / 4 * 3;
        
        float changeInX = farLeft - getCamPosFarRight(game);
        float changeInY = farLeftBottom - game.getFarRightBottom();
        
        m_camBounds->getLowerLeft().set(getCamPosFarRight(game) + changeInX * progress, game.getFarRightBottom() + changeInY * progress);

        if (m_camBounds->getLowerLeft().getX() < farLeft)
        {
            m_camBounds->getLowerLeft().setX(farLeft);
        }
        
        if ((m_camPosVelocity->getY() < 0 && m_camBounds->getLowerLeft().getY() < farLeftBottom)
            || (m_camPosVelocity->getY() > 0 && m_camBounds->getLowerLeft().getY() > farLeftBottom))
        {
            m_camBounds->getLowerLeft().setY(farLeftBottom);
        }
        
        if (isComplete)
        {
            updateCameraToFollowJon(game, 1337);
        }
        
        return isComplete ? 2 : 1;
    }
    
    return m_fStateTime >= 8.065f ? 3 : 0;
}

void Renderer::updateCameraToFollowJon(Game& game, float deltaTime)
{
    Jon& jon = game.getJon();
    m_camBounds->getLowerLeft().setX(jon.getPosition().getX() - CAM_WIDTH / 5);
    float jy = jon.getPosition().getY() - jon.getHeight() / 4 * 3;
    float jonHeightPlusPadding = jon.getHeight() * 1.5f;
    
    float regionBottomY;
    if (jon.getAbilityState() == ABILITY_BURROW)
    {
        regionBottomY = jy - 1.22451534f;
    }
    else if (jon.isFalling())
    {
        if (jy < 6.9979978125f)
        {
            regionBottomY = 0;
        }
        else if (jy < 12.76362286085f)
        {
            regionBottomY = 6.9979978125f;
        }
        else if (jy < 21.76362286085f)
        {
            regionBottomY = 12.76362286085f;
        }
        else
        {
            regionBottomY = 21.76362286085f;
        }
    }
    else
    {
        if (jy < (6.9979978125f - jonHeightPlusPadding))
        {
            regionBottomY = 0;
        }
        else if (jy < (12.76362286085 - jonHeightPlusPadding))
        {
            regionBottomY = 6.9979978125f;
        }
        else if (jy < (21.76362286085f - jonHeightPlusPadding))
        {
            regionBottomY = 12.76362286085f;
        }
        else
        {
            regionBottomY = 21.76362286085f;
        }
    }
    
    float camSpeed = regionBottomY - m_camBounds->getLowerLeft().getY();
    float camVelocityY = regionBottomY > m_camBounds->getLowerLeft().getY() ? camSpeed : regionBottomY == m_camBounds->getLowerLeft().getY() ? 0 : camSpeed * 4;
    m_camBounds->getLowerLeft().add(0, camVelocityY * deltaTime);
    
    if (camVelocityY > 0)
    {
        if (jon.getPhysicalState() != PHYSICAL_GROUNDED)
        {
            float newCamPos = jy + jonHeightPlusPadding - CAM_HEIGHT;
            if (newCamPos > m_camBounds->getLowerLeft().getY())
            {
                m_camBounds->getLowerLeft().setY(newCamPos);
            }
        }
        
        if (m_camBounds->getLowerLeft().getY() > regionBottomY)
        {
            m_camBounds->getLowerLeft().setY(regionBottomY);
        }
    }
    
    if (camVelocityY < 0)
    {
        if (m_camBounds->getLowerLeft().getY() < regionBottomY)
        {
            m_camBounds->getLowerLeft().setY(regionBottomY);
        }
    }
    
    if (m_camBounds->getLowerLeft().getY() < 0)
    {
        m_camBounds->getLowerLeft().setY(0);
    }
    
    if (m_camBounds->getLowerLeft().getY() > GAME_HEIGHT - CAM_HEIGHT)
    {
        m_camBounds->getLowerLeft().setY(GAME_HEIGHT - CAM_HEIGHT);
    }
    
    float farCamPos = getCamPosFarRight(game);
    if (m_camBounds->getLowerLeft().getX() > farCamPos)
    {
        m_camBounds->getLowerLeft().setX(farCamPos);
    }
}

void Renderer::moveCamera(float x)
{
    m_camBounds->getLowerLeft().add(x, 0);
}

void Renderer::zoomOut()
{
    m_camBounds->getLowerLeft().set(0, 0);
    m_camBounds->setWidth(ZOOMED_OUT_CAM_WIDTH);
    m_camBounds->setHeight(GAME_HEIGHT);
}

void Renderer::zoomIn()
{
    m_camBounds->getLowerLeft().set(0, 0);
    m_camBounds->setWidth(CAM_WIDTH);
    m_camBounds->setHeight(CAM_HEIGHT);
}

void Renderer::renderTitleScreen()
{
    /// Render Title Logo
    
    updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
    
    m_spriteBatcher->beginBatch();
    static float tlWidth = CAM_WIDTH * 2 / 3;
    static float tlHeight = tlWidth * 0.4921875f;
    
    int x = 0;
    int y = 1040;
    int regionWidth = 2048;
    int regionHeight = 1008;
    int textureSize = TEXTURE_SIZE_4096;
    
    if (Assets::getInstance()->isUsingCompressedTextureSet())
    {
        x /= 2.0;
        y /= 2.0;
        regionWidth /= 2.0;
        regionHeight /= 2.0;
        textureSize /= 2.0;
    }
    
    static TextureRegion tlTr = TextureRegion(x, y, regionWidth, regionHeight, textureSize, textureSize);
    m_spriteBatcher->drawSprite(CAM_WIDTH / 2, CAM_HEIGHT * 2 / 3, tlWidth, tlHeight, 0, tlTr);
    m_spriteBatcher->endBatch(*m_misc);
    
    m_spriteBatcher->beginBatch();
    
    static Color fontColor = Color(1, 1, 1, 1);
    static float fgWidth = CAM_WIDTH / 28;
    static float fgHeight = fgWidth * 1.140625f;
    
    float fontStartingY = CAM_HEIGHT * 2 / 5;
    
    {
        static std::string text = std::string("Swipe down to dig");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH / 2, fontStartingY -= fgHeight, fgWidth, fgHeight, fontColor, true);
    }
    
    {
        static std::string text = std::string("Swipe right to punch");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH / 2, fontStartingY -= fgHeight, fgWidth, fgHeight, fontColor, true);
    }
    
    {
        static std::string text = std::string("Hold screen to transform");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH / 2, fontStartingY -= fgHeight, fgWidth, fgHeight, fontColor, true);
    }
    
    fontStartingY -= fgHeight / 2;
    
    {
        static std::string text = std::string("Tap to begin");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH / 2, fontStartingY -= fgHeight, fgWidth, fgHeight, fontColor, true);
    }
    
    m_spriteBatcher->endBatch(*m_misc);
}

void Renderer::renderLoadingTextOnTitleScreen()
{
	updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
    
    m_spriteBatcher->beginBatch();
    
    static Color fontColor = Color(1, 1, 1, 1);
    static float fgWidth = CAM_WIDTH / 30;
    static float fgHeight = fgWidth * 1.140625f;
    
    {
        static std::string text = std::string("Loading...");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH - fgWidth / 2, fgHeight / 2, fgWidth, fgHeight, fontColor, false, true);
    }
    
    m_spriteBatcher->endBatch(*m_misc);
}

void Renderer::renderTitleScreenUi(LevelEditorButton& levelEditorButton)
{
    /// Render Level Editor Button
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntity(levelEditorButton, Assets::getInstance()->get(levelEditorButton));
    m_spriteBatcher->endBatch(*m_misc);
}

void Renderer::renderWorldMapScreenBackground()
{
    /// Render Title Logo
    
    updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
    
    m_spriteBatcher->beginBatch();
    
    int x = 2048;
    int y = 0;
    int regionWidth = 2048;
    int regionHeight = 766;
    int textureSize = TEXTURE_SIZE_4096;
    
    if (Assets::getInstance()->isUsingCompressedTextureSet())
    {
        x /= 2.0;
        y /= 2.0;
        regionWidth /= 2.0;
        regionHeight /= 2.0;
        textureSize /= 2.0;
    }
    
    static TextureRegion tr = TextureRegion(x, y, regionWidth, regionHeight, textureSize, textureSize);
    static Color bgColor = Color(1, 1, 1, 1);
    m_spriteBatcher->drawSprite(CAM_WIDTH / 2, CAM_HEIGHT / 2, CAM_WIDTH, CAM_HEIGHT, 0, bgColor, tr);
    m_spriteBatcher->endBatch(*m_misc);
}

void Renderer::renderWorldMapScreenUi(BackButton& backButton)
{
    /// Render Back Button
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntity(backButton, Assets::getInstance()->get(backButton));
    m_spriteBatcher->endBatch(*m_misc);
    
    m_spriteBatcher->beginBatch();
    
    static Color fontColor = Color(0, 0, 0, 1);
    static float fgWidth = CAM_WIDTH / 28;
    static float fgHeight = fgWidth * 1.140625f;
    
    m_highlightRectangleBatcher->beginBatch();
    
    float fontStartingY = CAM_HEIGHT - fgHeight;
    
    {
        Rectangle r = Rectangle(CAM_WIDTH / 2 - fgWidth * 8, fontStartingY - fgHeight / 2, fgWidth * 16, fgHeight);
        static Color c = Color(0, 0.4f, 1, 0.9f);
        m_highlightRectangleBatcher->renderRectangle(r, c);
        
        static std::string text = std::string("Select a Level:");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH / 2, fontStartingY, fgWidth, fgHeight, fontColor, true);
        fontStartingY -= fgHeight;
    }
    
    fontStartingY -= fgHeight * 8;
    
    {
        Rectangle r = Rectangle(0.6f, fontStartingY, CAM_WIDTH / 4, CAM_HEIGHT / 2);
        static Color c = Color(0, 0.4f, 1, 0.9f);
        m_highlightRectangleBatcher->renderRectangle(r, c);
        
        static std::string text = std::string("1");
        m_font->renderText(*m_spriteBatcher, text, 0.6f + CAM_WIDTH / 8, fontStartingY + CAM_HEIGHT / 4, fgWidth, fgHeight, fontColor, true);
    }
    
    {
        Rectangle r = Rectangle(0.6f + CAM_WIDTH / 3, fontStartingY, CAM_WIDTH / 4, CAM_HEIGHT / 2);
        static Color c = Color(0, 0.4f, 1, 0.9f);
        m_highlightRectangleBatcher->renderRectangle(r, c);
        
        static std::string text = std::string("2");
        m_font->renderText(*m_spriteBatcher, text, 0.6f + CAM_WIDTH / 3 + CAM_WIDTH / 8, fontStartingY + CAM_HEIGHT / 4, fgWidth, fgHeight, fontColor, true);
    }
    
    {
        Rectangle r = Rectangle(0.6f + CAM_WIDTH / 3 * 2, fontStartingY, CAM_WIDTH / 4, CAM_HEIGHT / 2);
        static Color c = Color(0, 0.4f, 1, 0.9f);
        m_highlightRectangleBatcher->renderRectangle(r, c);
        
        static std::string text = std::string("3");
        m_font->renderText(*m_spriteBatcher, text, 0.6f + CAM_WIDTH / 3 * 2 + CAM_WIDTH / 8, fontStartingY + CAM_HEIGHT / 4, fgWidth, fgHeight, fontColor, true);
    }
    
    m_highlightRectangleBatcher->endBatch();
    m_spriteBatcher->endBatch(*m_misc);
}

void Renderer::renderLoadingTextOnWorldMapScreen()
{
    updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
    
    m_spriteBatcher->beginBatch();
    
    static Color fontColor = Color(0, 0, 0, 1);
    static float fgWidth = CAM_WIDTH / 30;
    static float fgHeight = fgWidth * 1.140625f;
    
    {
        static std::string text = std::string("Loading...");
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH - fgWidth / 2, fgHeight / 2, fgWidth, fgHeight, fontColor, false, true);
    }
    
    m_spriteBatcher->endBatch(*m_misc);
}

void Renderer::renderWorld(Game& game)
{
    /// Render Background
    
    updateMatrix(0, m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getBackgroundUppers());
    m_spriteBatcher->endBatch(*m_world_1_background_upper, *m_backgroundTextureWrapper);
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getBackgroundMids());
    m_spriteBatcher->endBatch(*m_world_1_background_mid, *m_backgroundTextureWrapper);
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getBackgroundLowers());
    m_spriteBatcher->endBatch(*m_world_1_background_lower, *m_backgroundTextureWrapper);
    
    /// Render Midground
    
    updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getMidgrounds());
    m_spriteBatcher->endBatch(*m_world_1_midground);
    
    /// Render Exit Ground
    
    updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getExitGrounds());
    for (std::vector<ExitGround *>::iterator i = game.getExitGrounds().begin(); i != game.getExitGrounds().end(); i++)
    {
        renderPhysicalEntity(*(*i), Assets::getInstance()->get(*(*i)));
        if ((*i)->hasCover())
        {
            ExitGroundCover& egc = (*i)->getExitCover();
            renderPhysicalEntityWithColor(egc, Assets::getInstance()->get(egc), egc.getColor());
        }
    }
    m_spriteBatcher->endBatch(*m_world_1_midground);
    
    /// Render Background Midground Cover
    
    updateMatrix(0, m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getBackgroundMidgroundCovers());
    m_spriteBatcher->endBatch(*m_world_1_background_lower, *m_backgroundTextureWrapper);
    
    /// Render World
    
    updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getGrounds());
    m_spriteBatcher->endBatch(*m_world_1_ground);
    
    m_spriteBatcher->beginBatch();
    for (std::vector<Hole *>::iterator i = game.getHoles().begin(); i != game.getHoles().end(); i++)
    {
        renderPhysicalEntity(*(*i), Assets::getInstance()->get(*(*i)));
        if ((*i)->hasCover())
        {
            HoleCover& hc = (*i)->getHoleCover();
            renderPhysicalEntity(hc, Assets::getInstance()->get(hc));
        }
    }
    m_spriteBatcher->endBatch(*m_world_1_midground);
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntities(game.getCollectibleItems());
    renderPhysicalEntities(game.getForegroundObjects());
    m_spriteBatcher->endBatch(*m_world_1_objects);
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntitiesWithColor(game.getEnemies());
    m_spriteBatcher->endBatch(*m_world_1_enemies, *m_snakeDeathTextureProgram);
    
    m_spriteBatcher->beginBatch();
    for (std::vector<Enemy *>::iterator i = game.getEnemies().begin(); i != game.getEnemies().end(); i++)
    {
        if ((*i)->hasSpirit())
        {
            EnemySpirit& spirit = (*i)->getSpirit();
            renderPhysicalEntity(spirit, Assets::getInstance()->get(spirit));
        }
    }
    m_spriteBatcher->endBatch(*m_world_1_enemies);
}

void Renderer::renderJon(Game& game)
{
    if (game.getJons().size() > 0)
    {
        Jon& jon = game.getJon();
        bool isTransforming = jon.isTransformingIntoVampire() || jon.isRevertingToRabbit();
        bool isVampire = jon.isVampire();
        
        /// Render Jon Effects (e.g. Dust Clouds)
        
        m_spriteBatcher->beginBatch();
        for (std::vector<Jon *>::iterator i = game.getJons().begin(); i != game.getJons().end(); i++)
        {
            renderPhysicalEntitiesWithColor((*i)->getDustClouds());
        }
        m_spriteBatcher->endBatch((isVampire || jon.isRevertingToRabbit()) ? *m_vampire : *m_jon);
        
        /// Render Jon After Images
        
        m_spriteBatcher->beginBatch();
        for (std::vector<Jon *>::iterator i = jon.getAfterImages().begin(); i != jon.getAfterImages().end(); i++)
        {
            Jon* pItem = *i;
            Jon& item = *pItem;
            renderPhysicalEntityWithColor(item, Assets::getInstance()->get(item), item.getColor());
        }
        m_spriteBatcher->endBatch(*m_vampire);
        
        /// Render Jon
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntitiesWithColor(game.getJons());
        m_spriteBatcher->endBatch(isVampire || isTransforming ? *m_vampire : *m_jon);
    }
}

void Renderer::renderBounds(Game& game, int boundsLevelRequested)
{
    updateMatrix(m_camBounds->getLeft(), m_camBounds->getRight(), m_camBounds->getBottom(), m_camBounds->getTop());
    
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getMidgrounds());
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getExitGrounds());
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getGrounds());
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getHoles());
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getForegroundObjects());
	renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getEnemies());
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getCollectibleItems());
    renderBoundsForPhysicalEntities(*m_boundsRectangleBatcher, game.getJons());
    
    static Color gridColor = Color(1, 1, 1, 0.4f);
    
    int right = m_camBounds->getRight() / GRID_CELL_SIZE;
    int len = right;
    
    m_lineBatcher->beginBatch();
    
    for (int i = 0; i < len; i+= boundsLevelRequested)
    {
        float x = i * GRID_CELL_SIZE;
        m_lineBatcher->renderLine(x, 0, x, GAME_HEIGHT, gridColor);
    }
    
    for (int j = 0; j < 256; j+= boundsLevelRequested)
    {
        float y = j * GRID_CELL_SIZE;
        m_lineBatcher->renderLine(m_camBounds->getLeft(), y, m_camBounds->getRight(), y, gridColor);
    }
    
    m_lineBatcher->endBatch();
}

void Renderer::renderEntityHighlighted(PhysicalEntity& entity, Color& c)
{
	updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_highlightRectangleBatcher->beginBatch();
    renderHighlightForPhysicalEntity(entity, c);
    m_highlightRectangleBatcher->endBatch();
}

void Renderer::renderHud(Game& game, BackButton &backButton, int fps)
{
	updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
    
    /// Render Back Button
    
    m_spriteBatcher->beginBatch();

    renderPhysicalEntity(backButton, Assets::getInstance()->get(backButton));
    
    static Color fontColor = Color(1, 1, 1, 1);
    static float fgWidth = CAM_WIDTH / 24;
    static float fgHeight = fgWidth * 1.140625f;
    
    /// Render Play Time
    
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << game.getStateTime();
        std::string text = ss.str();
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH / 2, CAM_HEIGHT - fgHeight / 2, fgWidth, fgHeight, fontColor, true);
    }
    
    std::stringstream ss;
    ss << game.getNumTotalCarrots();
    std::string totalNumGoldenCarrots = ss.str();
    int offset = 1 + (int) totalNumGoldenCarrots.size();
    
    /// Render Num / Total Carrots
    
    {
        std::stringstream ss;
        ss << (game.getNumTotalCarrots() - game.getNumRemainingCarrots());
        std::string text = ss.str();
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH - (offset + text.size()) * fgWidth - fgWidth / 2, CAM_HEIGHT - fgHeight / 2, fgWidth, fgHeight, fontColor);
    }
    {
        std::stringstream ss;
        ss << "/";
        std::string text = ss.str();
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH - offset * fgWidth - fgWidth / 2, CAM_HEIGHT - fgHeight / 2, fgWidth, fgHeight, fontColor);
    }
    {
        std::stringstream ss;
        ss << game.getNumTotalCarrots();
        std::string text = ss.str();
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH - text.size() * fgWidth - fgWidth / 2, CAM_HEIGHT - fgHeight / 2, fgWidth, fgHeight, fontColor);
    }
    
    /// Render Num / Total Golden Carrots
    
    {
        std::stringstream ss;
        ss << (game.getNumTotalGoldenCarrots() - game.getNumRemainingGoldenCarrots()) << "/" << game.getNumTotalGoldenCarrots();
        std::string text = ss.str();
        m_font->renderText(*m_spriteBatcher, text, CAM_WIDTH - 3 * fgWidth - fgWidth / 2, CAM_HEIGHT - fgHeight - fgHeight / 2, fgWidth, fgHeight, fontColor);
    }

    m_spriteBatcher->endBatch(*m_misc);
    
    static CollectibleItem uiCarrot = Carrot(0, 0);
    static CollectibleItem uiGoldenCarrot = GoldenCarrot(0, 0);
    
    uiCarrot.getPosition().set(CAM_WIDTH - fgWidth / 2, CAM_HEIGHT - fgHeight / 2);
    uiCarrot.setWidth(fgWidth);
    uiCarrot.setHeight(fgHeight);
    
    uiGoldenCarrot.getPosition().set(CAM_WIDTH - fgWidth / 2, CAM_HEIGHT - fgHeight - fgHeight / 2);
    uiGoldenCarrot.setWidth(fgWidth);
    uiGoldenCarrot.setHeight(fgHeight);
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntity(uiCarrot, Assets::getInstance()->get(uiCarrot));
    renderPhysicalEntity(uiGoldenCarrot, Assets::getInstance()->get(uiGoldenCarrot));
    m_spriteBatcher->endBatch(*m_world_1_objects);

	{
		updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);

		std::stringstream ss;
		ss << fps << " FPS";
		std::string fps_string = ss.str();

		// Render FPS

		m_spriteBatcher->beginBatch();
		m_font->renderText(*m_spriteBatcher, fps_string, CAM_WIDTH / 4, CAM_HEIGHT - fgHeight / 2, fgWidth / 2, fgHeight / 2, fontColor, true);
		m_spriteBatcher->endBatch(*m_misc);
	}
}

void Renderer::renderLevelEditor(LevelEditorActionsPanel& leap, LevelEditorEntitiesPanel& leep, TrashCan& tc, LevelSelectorPanel& lsp)
{
	static Rectangle originMarker = Rectangle(0, 0, 0.1f, GAME_HEIGHT);
    static Color originMarkerColor = Color(0, 0, 0, 0.7f);
    
    updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_highlightRectangleBatcher->beginBatch();
    m_highlightRectangleBatcher->renderRectangle(originMarker, originMarkerColor);
    m_highlightRectangleBatcher->endBatch();
    
    updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
    
    /// Render Level Editor
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntity(leap, Assets::getInstance()->get(leap));
    renderPhysicalEntity(leep, Assets::getInstance()->get(leep));
    m_spriteBatcher->endBatch(*m_misc);
    
    updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    m_spriteBatcher->beginBatch();
    renderPhysicalEntity(tc, Assets::getInstance()->get(tc));
    m_spriteBatcher->endBatch(*m_misc);
    
    if (leep.isOpen())
    {
        updateMatrix(0, CAM_WIDTH, leep.getEntitiesCameraPos(), leep.getEntitiesCameraPos() + CAM_HEIGHT);
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntities(leep.getMidgrounds());
        renderPhysicalEntities(leep.getExitGrounds());
        renderPhysicalEntities(leep.getHoles());
        m_spriteBatcher->endBatch(*m_world_1_midground);
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntities(leep.getGrounds());
        m_spriteBatcher->endBatch(*m_world_1_ground);
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntities(leep.getCollectibleItems());
        renderPhysicalEntities(leep.getForegroundObjects());
        m_spriteBatcher->endBatch(*m_world_1_objects);
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntities(leep.getEnemies());
        m_spriteBatcher->endBatch(*m_world_1_enemies);
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntities(leep.getJons());
        m_spriteBatcher->endBatch(*m_jon);
    }
    
    if (lsp.isOpen())
    {
        updateMatrix(0, CAM_WIDTH, 0, CAM_HEIGHT);
        
        m_spriteBatcher->beginBatch();
        renderPhysicalEntity(lsp, Assets::getInstance()->get(lsp));
        m_spriteBatcher->endBatch(*m_misc);
        
        static Color fontColor = Color(1, 1, 1, 1);
        
        float fgWidth = lsp.getTextSize();
        float fgHeight = fgWidth * 1.140625f;
        
        m_spriteBatcher->beginBatch();
        {
            std::stringstream ss;
            ss << lsp.getWorld();
            std::string text = ss.str();
            m_font->renderText(*m_spriteBatcher, text, lsp.getWorldTextPosition().getX(), lsp.getWorldTextPosition().getY(), fgWidth, fgHeight, fontColor, false, true);
        }
        {
            std::stringstream ss;
            ss << lsp.getLevel();
            std::string text = ss.str();
            m_font->renderText(*m_spriteBatcher, text, lsp.getLevelTextPosition().getX(), lsp.getLevelTextPosition().getY(), fgWidth, fgHeight, fontColor, false, true);
        }
        m_spriteBatcher->endBatch(*m_misc);
    }
}

void Renderer::renderToSecondFramebufferWithShockwave(float centerX, float centerY, float timeElapsed, bool isTransforming)
{
    m_shockwaveTextureGpuProgramWrapper->configure(centerX, centerY, timeElapsed, isTransforming);
    
    updateMatrix(m_camBounds->getLowerLeft().getX(), m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth(), m_camBounds->getLowerLeft().getY(), m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight());
    
    setFramebuffer(1);
    
    float x = m_camBounds->getLowerLeft().getX() + m_camBounds->getWidth() / 2;
    float y = m_camBounds->getLowerLeft().getY() + m_camBounds->getHeight() / 2;
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(x, y, m_camBounds->getWidth(), m_camBounds->getHeight(), 0, TextureRegion(0, 0, 1, 1, 1, 1));
    m_spriteBatcher->endBatch(m_framebuffers.at(0), *m_shockwaveTextureGpuProgramWrapper);
}

void Renderer::renderToSecondFramebuffer(Game& game)
{
    setFramebuffer(1);
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(0, 0, 2, 2, 0, TextureRegion(0, 0, 1, 1, 1, 1));
    
    if (game.getJons().size() > 0)
    {
        Jon& jon = game.getJon();
        bool isVampire = jon.isVampire();
        m_spriteBatcher->endBatch(m_framebuffers.at(0), (isVampire || jon.isRevertingToRabbit()) ? *m_framebufferTintGpuProgramWrapper : *m_framebufferToScreenGpuProgramWrapper);
    }
    else
    {
        m_spriteBatcher->endBatch(m_framebuffers.at(0), *m_framebufferToScreenGpuProgramWrapper);
    }
}

void Renderer::renderToScreenWithTransDeathIn(float timeElapsed)
{
    /// Render the death transition to the screen
    
    m_transDeathInGpuProgramWrapper->configure(m_trans_death_shader_helper, timeElapsed);
    
    bindToScreenFramebuffer();
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(0, 0, 2, 2, 0, TextureRegion(0, 0, 1, 1, 1, 1));
    m_spriteBatcher->endBatch(m_framebuffers.at(m_iFramebufferIndex), *m_transDeathInGpuProgramWrapper);
}

void Renderer::renderToScreenWithTransDeathOut(float timeElapsed)
{
    /// Render the new game to the screen
    
    m_transDeathOutGpuProgramWrapper->configure(m_trans_death_shader_helper, timeElapsed);
    
    bindToScreenFramebuffer();
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(0, 0, 2, 2, 0, TextureRegion(0, 0, 1, 1, 1, 1));
    m_spriteBatcher->endBatch(m_framebuffers.at(m_iFramebufferIndex), *m_transDeathOutGpuProgramWrapper);
}

void Renderer::renderToScreenTransition(float progress)
{
    /// Render the screen transition to the screen
    
    m_transScreenGpuProgramWrapper->configure(&m_framebuffers.at(1), progress);
    
    bindToScreenFramebuffer();
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(0, 0, 2, 2, 0, TextureRegion(0, 0, 1, 1, 1, 1));
    m_spriteBatcher->endBatch(m_framebuffers.at(0), *m_transScreenGpuProgramWrapper);
}

void Renderer::renderToScreenWithRadialBlur()
{
    /// Render everything to the screen with a radial blur
    
    m_framebufferRadialBlurGpuProgramWrapper->configure(m_iRadialBlurDirection);
    
    bindToScreenFramebuffer();
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(0, 0, 2, 2, 0, TextureRegion(0, 0, 1, 1, 1, 1));
    m_spriteBatcher->endBatch(m_framebuffers.at(m_iFramebufferIndex), *m_framebufferRadialBlurGpuProgramWrapper);
}

void Renderer::renderToScreen()
{
	/// Render everything to the screen
    
    bindToScreenFramebuffer();
    
    m_spriteBatcher->beginBatch();
    m_spriteBatcher->drawSprite(0, 0, 2, 2, 0, TextureRegion(0, 0, 1, 1, 1, 1));
    m_spriteBatcher->endBatch(m_framebuffers.at(m_iFramebufferIndex), *m_framebufferToScreenGpuProgramWrapper);
}

void Renderer::cleanUp()
{
    if (m_areWorld1TexturesLoaded)
    {
        destroyTexture(*m_jon);
        
        destroyTexture(*m_trans_death_shader_helper);
        
        destroyTexture(*m_vampire);
        
        destroyTexture(*m_world_1_background_lower);
        destroyTexture(*m_world_1_background_mid);
        destroyTexture(*m_world_1_background_upper);
        
        destroyTexture(*m_world_1_enemies);
        
        destroyTexture(*m_world_1_ground);
        
        destroyTexture(*m_world_1_midground);
        
        destroyTexture(*m_world_1_objects);
        
        m_areWorld1TexturesLoaded = false;
    }
    
    if (m_areMenuTexturesLoaded)
    {
        destroyTexture(*m_misc);
        
        m_areMenuTexturesLoaded = false;
    }
    
    if (m_areShadersLoaded)
    {
        m_transScreenGpuProgramWrapper->cleanUp();
        m_sinWaveTextureProgram->cleanUp();
        m_backgroundTextureWrapper->cleanUp();
        m_snakeDeathTextureProgram->cleanUp();
        m_shockwaveTextureGpuProgramWrapper->cleanUp();
        m_transDeathInGpuProgramWrapper->cleanUp();
        m_transDeathOutGpuProgramWrapper->cleanUp();
        m_framebufferToScreenGpuProgramWrapper->cleanUp();
        m_framebufferTintGpuProgramWrapper->cleanUp();
        m_framebufferRadialBlurGpuProgramWrapper->cleanUp();
        
        m_areShadersLoaded = false;
    }
    
    m_framebuffers.clear();
}

Vector2D& Renderer::getCameraPosition()
{
    return m_camBounds->getLowerLeft();
}

#pragma mark private

void Renderer::renderPhysicalEntity(PhysicalEntity &pe, TextureRegion& tr)
{
	m_spriteBatcher->drawSprite(pe.getPosition().getX(), pe.getPosition().getY(), pe.getWidth(), pe.getHeight(), pe.getAngle(), tr);
}

void Renderer::renderPhysicalEntityWithColor(PhysicalEntity &pe, TextureRegion& tr, Color c)
{
	m_spriteBatcher->drawSprite(pe.getPosition().getX(), pe.getPosition().getY(), pe.getWidth(), pe.getHeight(), pe.getAngle(), c, tr);
}

void Renderer::renderBoundsForPhysicalEntity(PhysicalEntity &pe)
{
    static Color red = Color(1, 0, 0, 1);
    
    m_boundsRectangleBatcher->renderRectangle(pe.getBounds(), red);
}

void Renderer::renderHighlightForPhysicalEntity(PhysicalEntity &pe, Color &c)
{
    m_highlightRectangleBatcher->renderRectangle(pe.getBounds(), c);
}

float Renderer::getCamPosFarRight(Game &game)
{
    float farRight = game.getFarRight();
    float farCamPos = farRight - CAM_WIDTH;
    
    return farCamPos;
}