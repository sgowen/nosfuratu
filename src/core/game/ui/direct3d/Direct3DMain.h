//
// Direct3DMain.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "GamePad.h"
#include "Audio.h"
#include "MainScreen.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Direct3DMain : public DX::IDeviceNotify
{
public:
    Direct3DMain();
	~Direct3DMain();

	// Initialization and management
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
	void Initialize(HWND window, int width, int height);
#else
	void Initialize(IUnknown* window, int width, int height, float dpi, DXGI_MODE_ROTATION rotation);
#endif
	
	void OnNewAudioDevice();

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
	void OnWindowSizeChanged(int width, int height);
#else
	void OnWindowSizeChanged(int width, int height, float dpi, DXGI_MODE_ROTATION rotation);
	void ValidateDevice();
#endif
    
    // Properties
    void GetDefaultSize( int& width, int& height ) const;

	MainScreen* getMainScreen();
	int getRequestedAction();
	int getScreenRequestedAction();

private:
	// Device resources.
	std::unique_ptr<DX::DeviceResources>    m_deviceResources;

	// Rendering loop timer.
	DX::StepTimer                           m_timer;

	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;
	std::unique_ptr<DirectX::GamePad> m_gamePad;
	DirectX::GamePad::ButtonStateTracker m_buttons;
	std::unique_ptr<DirectX::AudioEngine> m_audEngine;

	std::vector<std::unique_ptr<DirectX::SoundEffect>> m_sounds;

	std::unique_ptr<DirectX::SoundEffect> m_music;
	std::unique_ptr<DirectX::SoundEffectInstance> m_musicLoop;

	MainScreen* m_screen;

	float m_fDPI;
	int m_iRequestedAction;
	int m_iScreenRequestedAction;
	bool m_isPointerPressed;
	bool m_retryAudio;

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

	void beginPixEvent(PCWSTR pFormat, DX::DeviceResources* deviceResources = nullptr);
	void endPixEvent(DX::DeviceResources* deviceResources = nullptr);

	void handleSound();
	void handleMusic();
	void playSound(int soundId, bool isLooping);
	void playSound(int soundId);
	void stopSound(int soundId);
	void stopAllSounds();
	void stopAllLoopingSounds();

	void initSoundEngine();
	void loadSound(const wchar_t* waveFileName);
	void loadMusic(const wchar_t* waveFileName);

	void unlockLevel(int requestedAction);
	void markLevelAsCompleted(int requestedAction);
	void submitScoreOnline(int requestedAction);
	void setCutsceneViewedFlag(int requestedAction);
	void sendSaveData();

	int calcWorld(int requestedAction);
	int calcLevel(int requestedAction);
};
