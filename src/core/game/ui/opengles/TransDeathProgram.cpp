//
//  TransDeathProgram.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 1/27/16.
//  Copyright (c) 2016 Gowen Game Dev. All rights reserved.
//

#include "TransDeathProgram.h"

TransDeathProgramStruct TransDeathProgram::build(GLuint program)
{
    return (TransDeathProgramStruct)
    {
        program,
        glGetAttribLocation(program, "a_Position"),
        glGetUniformLocation(program, "u_TextureUnit"),
        glGetUniformLocation(program, "u_TextureUnitGrayMap"),
        glGetUniformLocation(program, "u_TimeElapsed")
    };
}