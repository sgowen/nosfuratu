﻿#pragma once

#include "StepTimer.h"
#include "DeviceResources.h"
#include "Direct3DGameScreen.h"

#include <vector>
#include <thread>

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
		void onTouchDown(float screenX, float screenY);
		void onTouchDragged(float screenX, float screenY);
		void onTouchUp(float screenX, float screenY);
		bool handleOnBackPressed();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		std::unique_ptr<Direct3DGameScreen> m_gameScreen;

		std::unique_ptr<MediaEnginePlayer> m_mediaPlayer;
		std::vector<GameSound> m_sounds;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		std::vector<std::thread> m_threads;

		int m_iRequestedAction;

		void Update();
		bool Render();

		void handleSound();
		void handleMusic();

		void playSound(int soundId, bool isLoop = false);
		void stopSound(int soundId);

		void saveLevel(int requestedAction);
		void loadLevel(int requestedAction);
		void markLevelAsCompleted(int requestedAction);
		void sendLevelStats();
        void showMessage(int requestedAction);

		Platform::String^ getLevelName(int requestedAction);

		int calcWorld(int requestedAction);

		int calcLevel(int requestedAction);
        
        int calcGoldenCarrotsFlag(int requestedAction);

		void displayToast(Platform::String^ message);
	};
}