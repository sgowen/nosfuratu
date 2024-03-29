//
//  NosFURatuMain.h
//  nosfuratu
//
//  Created by Stephen Gowen on 4/20/17.
//  Copyright (c) 2017 Noctis Games. All rights reserved.
//

#pragma once

#include "StepTimer.h"
#include "DeviceResources.h"

#include <vector>
#include <thread>

class MainScreen;

namespace NosFURatu
{
	class NosFURatuMain : public DX::IDeviceNotify
	{
	public:
		NosFURatuMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~NosFURatuMain();
		void CreateWindowSizeDependentResources();
		void StartRenderLoop();
		void StopRenderLoop();
		MainScreen* getMainScreen();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

		int m_iRequestedAction;

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		MainScreen* m_screen;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		void Update();
		bool Render();
	};
}
