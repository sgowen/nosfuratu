//
//  Direct3DFramebufferToScreenGpuProgramWrapper.h
//  gowengamedev-framework
//
//  Created by Stephen Gowen on 9/7/15.
//  Copyright (c) 2015 Gowen Game Dev. All rights reserved.
//

#ifndef __gowengamedev__Direct3DFramebufferToScreenGpuProgramWrapper__
#define __gowengamedev__Direct3DFramebufferToScreenGpuProgramWrapper__

#include "GpuProgramWrapper.h"
#include "DeviceResources.h"

class Direct3DFramebufferToScreenGpuProgramWrapper : public GpuProgramWrapper
{
public:
	Direct3DFramebufferToScreenGpuProgramWrapper(const std::shared_ptr<DX::DeviceResources>& deviceResources);

	virtual void bind();

	virtual void unbind();

	virtual void cleanUp();

private:
	int m_iNumShadersLoaded;
	
	// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
};

#endif /* defined(__gowengamedev__Direct3DFramebufferToScreenGpuProgramWrapper__) */
