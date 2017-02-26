//
//  OpenGLPointTransScreenGpuProgramWrapper.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 7/28/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#include "OpenGLPointTransScreenGpuProgramWrapper.h"

#include "OpenGLManager.h"
#include "macros.h"
#include "Vector2D.h"
#include "GpuTextureWrapper.h"
#include "AssetUtil.h"

OpenGLPointTransScreenGpuProgramWrapper::OpenGLPointTransScreenGpuProgramWrapper() : PointTransitionGpuProgramWrapper()
{
    m_program = OpenGLPointTransitionProgram::build(AssetUtil::buildProgramFromAssets("trans_screen_circle_open_shader.vsh", "trans_screen_circle_open_shader.fsh"));
}

OpenGLPointTransScreenGpuProgramWrapper::~OpenGLPointTransScreenGpuProgramWrapper()
{
    glDeleteProgram(m_program.program);
}

void OpenGLPointTransScreenGpuProgramWrapper::bind()
{
    OGLManager->useScreenBlending();
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_to->texture);
    
    glUseProgram(m_program.program);
    
    glUniformMatrix4fv(m_program.u_mvp_matrix_location, 1, GL_FALSE, (GLfloat*)OGLManager->getViewProjectionMatrix());
    glUniform1f(m_program.u_center_x_unit_location, m_center->getX());
    glUniform1f(m_program.u_center_y_unit_location, m_center->getY());
    glUniform1i(m_program.u_from_location, 0);
    glUniform1i(m_program.u_to_location, 1);
    glUniform1f(m_program.u_progress_location, m_fProgress);
    
    glGenBuffers(1, &OGLManager->getSbVboObject());
    glBindBuffer(GL_ARRAY_BUFFER, OGLManager->getSbVboObject());
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * OGLManager->getTextureVertices().size(), &OGLManager->getTextureVertices()[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(m_program.a_position_location, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 9, BUFFER_OFFSET(0));
    
    glEnableVertexAttribArray(m_program.a_position_location);
}

void OpenGLPointTransScreenGpuProgramWrapper::unbind()
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glDeleteBuffers(1, &OGLManager->getSbVboObject());
    
    glUseProgram(0);
}
