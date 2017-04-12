//
//  Direct3DFadeScreenGpuProgramWrapper.cpp
//  nosfuratu
//
//  Created by Stephen Gowen on 10/6/16.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#include "pch.h"

#include "Direct3DFadeScreenGpuProgramWrapper.h"

#include "Direct3DTextureProgram.h"
#include "DeviceResources.h"
#include "Direct3DManager.h"
#include "GpuTextureWrapper.h"

Direct3DFadeScreenGpuProgramWrapper::Direct3DFadeScreenGpuProgramWrapper() : TransitionGpuProgramWrapper(),
m_program(new Direct3DTextureProgram(L"FramebufferToScreenVertexShader.cso", L"FadeScreenTexturePixelShader.cso"))
{
	m_program->createConstantBuffer(&m_progressConstantBuffer);
	m_program->createConstantBuffer(&m_isWindowsMobileConstantBuffer);
    
    m_isWindowsMobile = D3DManager->isWindowsMobile();
}

Direct3DFadeScreenGpuProgramWrapper::~Direct3DFadeScreenGpuProgramWrapper()
{
    m_isWindowsMobileConstantBuffer.Reset();
    m_progressConstantBuffer.Reset();
    
    delete m_program;
}

void Direct3DFadeScreenGpuProgramWrapper::bind()
{
    D3DManager->useScreenBlending();
    
	m_program->bindShaders();
    
    m_program->bindNormalSamplerState();

    DX::DeviceResources* deviceResources = Direct3DManager::getDeviceResources();
    
	// tell the GPU which texture to use
	deviceResources->GetD3DDeviceContext()->PSSetShaderResources(1, 1, &m_to->texture);

	deviceResources->GetD3DDeviceContext()->PSSetConstantBuffers(0, 1, m_progressConstantBuffer.GetAddressOf());
    deviceResources->GetD3DDeviceContext()->PSSetConstantBuffers(1, 1, m_isWindowsMobileConstantBuffer.GetAddressOf());

	// send time elapsed to video memory
	deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_progressConstantBuffer.Get(), 0, 0, &m_fProgress, 0, 0);
    
    int isWindowsMobile = m_isWindowsMobile ? 1 : 0;
    deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_isWindowsMobileConstantBuffer.Get(), 0, 0, &isWindowsMobile, 0, 0);

	m_program->mapVertices();
}

void Direct3DFadeScreenGpuProgramWrapper::unbind()
{
	DX::DeviceResources* deviceResources = Direct3DManager::getDeviceResources();

	ID3D11ShaderResourceView *pSRV[1] = { NULL };
	deviceResources->GetD3DDeviceContext()->PSSetShaderResources(1, 1, pSRV);
}
