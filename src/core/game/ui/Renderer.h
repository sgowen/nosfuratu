//
//  Renderer.h
//  nosfuratu
//
//  Created by Stephen Gowen on 2/22/14.
//  Copyright (c) 2014 Gowen Game Dev. All rights reserved.
//

#ifndef __nosfuratu__Renderer__
#define __nosfuratu__Renderer__

#include <memory>

class SpriteBatcher;
struct TextureWrapper;
class PhysicalEntity;
class TextureRegion;
class Game;
class Jon;
class GpuProgramWrapper;
class Vector2D;

struct Color;

#include "Assets.h"

#include <vector>

class Renderer
{
public:
    Renderer();
    
	void init();
    
    void updateCameraToFollowJon(Jon& jon, Game& game, float deltaTime);
    
    void render(Game& game);

    void cleanUp();
    
    Vector2D& getCameraPosition();
    
protected:
    std::unique_ptr<SpriteBatcher> m_spriteBatcher;
    
    std::unique_ptr<TextureWrapper> m_jon_ability;
    std::unique_ptr<TextureWrapper> m_jon;
    std::unique_ptr<TextureWrapper> m_vampire;
    std::unique_ptr<TextureWrapper> m_world_1_background;
    std::unique_ptr<TextureWrapper> m_world_1_foreground_more;
    std::unique_ptr<TextureWrapper> m_world_1_foreground;
    std::unique_ptr<TextureWrapper> m_framebuffer;

	bool m_areTexturesLoaded;
    
    virtual TextureWrapper* loadTexture(const char* textureName) = 0;
    
    virtual void updateMatrix(float left, float right, float bottom, float top) = 0;
    
    virtual void bindToOffscreenFramebuffer() = 0;
    
    virtual void beginFrame() = 0;
    
    virtual void clearFrameBufferWithColor(float r, float g, float b, float a) = 0;
    
    virtual void bindToScreenFramebuffer() = 0;
    
    virtual void endFrame() = 0;
    
    virtual GpuProgramWrapper& getFramebufferToScreenGpuProgramWrapper() = 0;
    
    virtual void destroyTexture(TextureWrapper& textureWrapper) = 0;
    
private:
    std::unique_ptr<Vector2D> m_camPos;
    
    template<typename T>
    void renderPhysicalEntities(std::vector<T>& items)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            renderPhysicalEntity(*i, Assets::get(*i));
        }
    }
    
    template<typename T>
    void renderPhysicalEntitiesWithColor(std::vector<T>& items)
    {
        for (typename std::vector<T>::iterator i = items.begin(); i != items.end(); i++)
        {
            renderPhysicalEntityWithColor(*i, Assets::get(*i), (*i).getColor());
        }
    }
    
    void renderPhysicalEntity(PhysicalEntity &go, TextureRegion tr);
    
    void renderPhysicalEntityWithColor(PhysicalEntity &go, TextureRegion tr, Color c);
    
    bool isGroundForegroundMore(Ground& g);
};

#endif /* defined(__nosfuratu__Renderer__) */