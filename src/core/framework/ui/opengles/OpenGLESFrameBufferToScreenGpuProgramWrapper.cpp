//
//  OpenGLESFrameBufferToScreenGpuProgramWrapper.cpp
//  gowengamedev-framework
//
//  Created by Stephen Gowen on 9/3/15.
//  Copyright (c) 2015 Gowen Game Dev. All rights reserved.
//

#include "OpenGLESFrameBufferToScreenGpuProgramWrapper.h"
#include "OpenGLESManager.h"
#include "macros.h"

OpenGLESFrameBufferToScreenGpuProgramWrapper::OpenGLESFrameBufferToScreenGpuProgramWrapper(FrameBufferToScreenProgramStruct program) : m_program(program)
{
    // Empty
}

void OpenGLESFrameBufferToScreenGpuProgramWrapper::bind()
{
    glUseProgram(m_program.program);
    
    glUniform1i(m_program.u_texture_unit_location, 0);
    
    glGenBuffers(1, &OGLESManager->sb_vbo_object);
    glBindBuffer(GL_ARRAY_BUFFER, OGLESManager->sb_vbo_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * OGLESManager->m_textureVertices.size(), &OGLESManager->m_textureVertices[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(m_program.a_position_location, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 9, BUFFER_OFFSET(0));
    
    glEnableVertexAttribArray(m_program.a_position_location);
}

void OpenGLESFrameBufferToScreenGpuProgramWrapper::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glDeleteBuffers(1, &OGLESManager->sb_vbo_object);
    
    glUseProgram(0);
}