﻿//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace NosFURatu;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
#if defined(WINAPI_FAMILY)
	#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
		using namespace Windows::Phone::UI::Input;
	#elif WINAPI_FAMILY == WINAPI_FAMILY_APP
		#if WINAPI_PARTITION_PHONE_APP
			using namespace Windows::Phone::UI::Input;
		#endif
	#endif
#endif
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;
using namespace Microsoft::Advertising::WinRT::UI;
using namespace Windows::System::Profile;

DirectXPage::DirectXPage():
	m_windowVisible(true),
	m_isPointerPressed(false),
	m_coreInput(nullptr)
{
	InitializeComponent();

	ApplicationView^ view = ApplicationView::GetForCurrentView();

#if defined(WINAPI_FAMILY)
	#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
		view->SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);
	#elif WINAPI_FAMILY == WINAPI_FAMILY_APP
		#if WINAPI_PARTITION_PHONE_APP
			if (Windows::Foundation::Metadata::ApiInformation::IsMethodPresent("Windows.UI.ViewManagement.ApplicationView", "TryEnterFullScreenMode"))
			{
				view->TryEnterFullScreenMode();
			}
		#endif
	#endif
#endif

	// Register event handlers for page lifecycle.
	CoreWindow^ window = Window::Current->CoreWindow;

	window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &DirectXPage::onKeyDown);

	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

	currentDisplayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged += ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged += ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);

#if defined(WINAPI_FAMILY)
	#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
		HardwareButtons::BackPressed += ref new EventHandler<BackPressedEventArgs ^>(this, &NosFURatu::DirectXPage::OnBackPressed);
	#elif WINAPI_FAMILY == WINAPI_FAMILY_APP
		#if WINAPI_PARTITION_PHONE_APP
			if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.Phone.UI.Input.HardwareButtons"))
			{
				HardwareButtons::BackPressed += ref new EventHandler<BackPressedEventArgs ^>(this, &NosFURatu::DirectXPage::OnBackPressed);
			}
		#endif
	#endif
#endif

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
	m_deviceResources->SetSwapChainPanel(swapChainPanel);

	// Register our SwapChainPanel to get independent input pointer events
	auto workItemHandler = ref new WorkItemHandler([this] (IAsyncAction ^)
	{
		// The CoreIndependentInputSource will raise pointer events for the specified device types on whichever thread it's created on.
		m_coreInput = swapChainPanel->CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen
			);

		// Register for pointer events, which will be raised on the background thread.
		m_coreInput->PointerPressed += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerPressed);
		m_coreInput->PointerMoved += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerMoved);
		m_coreInput->PointerReleased += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerReleased);

		// Begin processing input messages as they're delivered.
		m_coreInput->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	});

	// Run task on a dedicated high priority background thread.
	m_inputLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

	m_main = std::unique_ptr<NosFURatuMain>(new NosFURatuMain(this, m_deviceResources));
	m_main->StartRenderLoop();

	m_interstitialAd = ref new InterstitialAd();
	m_interstitialAd->AdReady += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &NosFURatu::DirectXPage::OnAdReady);
	m_interstitialAd->Completed += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &NosFURatu::DirectXPage::OnAdCompleted);
	m_interstitialAd->Cancelled += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &NosFURatu::DirectXPage::OnAdCancelled);
	m_interstitialAd->ErrorOccurred += ref new Windows::Foundation::EventHandler<Microsoft::Advertising::WinRT::UI::AdErrorEventArgs ^>(this, &NosFURatu::DirectXPage::OnAdError);

	RequestInterstitialAd();
}

DirectXPage::~DirectXPage()
{
	// Stop rendering and processing events on destruction.
	m_main->StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->Trim();

	// Stop rendering when the app is suspended.
	m_main->StopRenderLoop();

	// Put code to save app state here.
}

// Loads the current state of the app for resume events.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Put code to load app state here.

	// Start rendering when the app is resumed.
	m_main->StartRenderLoop();
}

void DirectXPage::RequestInterstitialAd()
{
	Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Low, ref new Windows::UI::Core::DispatchedHandler([this]()
	{
		Platform::String^ myAppId;
		Platform::String^ myAdUnitId;

#if defined(_DEBUG)
		myAppId = L"d25517cb-12d4-4699-8bdc-52040c712cab";
		myAdUnitId = L"11389925";
#else
	bool isMobile;
#if defined(WINAPI_FAMILY)
	#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
		isMobile = true;
	#elif WINAPI_FAMILY == WINAPI_FAMILY_APP
		#if WINAPI_PARTITION_PHONE_APP
			AnalyticsVersionInfo^ api = AnalyticsInfo::VersionInfo;
			isMobile = api->DeviceFamily->Equals("Windows.Mobile");
		#else
			isMobile = false;
		#endif
	#endif
#endif
		if (isMobile)
		{
			myAppId = L"98231ab8-4983-4702-91a9-5e5fc1b139b7";
			myAdUnitId = L"11666621";
		}
		else
		{
			myAppId = L"661a0da5-63ee-4506-b900-dd3631302c5b";
			myAdUnitId = L"11666622";
		}
#endif

		m_interstitialAd->RequestAd(AdType::Video, myAppId, myAdUnitId);
	}));
}

void DirectXPage::DisplayInterstitialAdIfAvailable()
{
	Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Low, ref new Windows::UI::Core::DispatchedHandler([this]()
	{
		if (InterstitialAdState::Ready == m_interstitialAd->State)
		{
			m_main->StopRenderLoop();
			m_interstitialAd->Show();
		}
	}));
}

// Window event handlers.

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
	if (m_windowVisible)
	{
		m_main->StartRenderLoop();
	}
	else
	{
		m_main->StopRenderLoop();
	}
}

// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->ValidateDevice();
}

void DirectXPage::OnPointerPressed(Object^ sender, PointerEventArgs^ e)
{
	m_main->onTouchDown(e->CurrentPoint->Position.X, e->CurrentPoint->Position.Y);
	m_isPointerPressed = true;
}

void DirectXPage::OnPointerMoved(Object^ sender, PointerEventArgs^ e)
{
	if (m_isPointerPressed)
	{
		m_main->onTouchDragged(e->CurrentPoint->Position.X, e->CurrentPoint->Position.Y);
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerEventArgs^ e)
{
	m_main->onTouchUp(e->CurrentPoint->Position.X, e->CurrentPoint->Position.Y);
	m_isPointerPressed = false;
}

void DirectXPage::onKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ e)
{
	// Pass on Gamepad/Keyboard events as pseudo touch events, haha

	bool jump = e->VirtualKey == Windows::System::VirtualKey::W;
	bool transform = e->VirtualKey == Windows::System::VirtualKey::S;
	bool left = e->VirtualKey == Windows::System::VirtualKey::Left;
	bool up = e->VirtualKey == Windows::System::VirtualKey::Up;
	bool right = e->VirtualKey == Windows::System::VirtualKey::Right;
	bool down = e->VirtualKey == Windows::System::VirtualKey::Down;

#if defined(WINAPI_FAMILY)
	#if WINAPI_FAMILY == WINAPI_FAMILY_APP
		#if WINAPI_PARTITION_PHONE_APP
			if (!jump) jump = e->VirtualKey == Windows::System::VirtualKey::GamepadA;
			if (!transform) transform = e->VirtualKey == Windows::System::VirtualKey::GamepadX;
			if (!left) left = e->VirtualKey == Windows::System::VirtualKey::GamepadLeftThumbstickLeft;
			if (!up) up = e->VirtualKey == Windows::System::VirtualKey::GamepadLeftThumbstickUp;
			if (!right) right = e->VirtualKey == Windows::System::VirtualKey::GamepadLeftThumbstickRight;
			if (!down) down = e->VirtualKey == Windows::System::VirtualKey::GamepadLeftThumbstickDown;
		#endif
	#endif
#endif

	if (jump)
	{
        m_main->onTouchDown(300, 300);
        m_isPointerPressed = true;
        
		m_main->onTouchUp(300, 300);
		m_isPointerPressed = false;
	}
	else if (transform)
	{
		m_main->onTouchDown(300, 300);
		m_isPointerPressed = true;
	}
	else if (left)
	{
		m_main->onTouchDown(500, 500);
		m_isPointerPressed = true;
        
		m_main->onTouchDragged(500, 500);
		m_main->onTouchDragged(0, 500);
		m_main->onTouchUp(0, 500);
		m_isPointerPressed = false;
	}
	else if (up)
	{
		m_main->onTouchDown(500, 500);
		m_isPointerPressed = true;
        
		m_main->onTouchDragged(500, 500);
		m_main->onTouchDragged(500, 0);
		m_main->onTouchUp(500, 0);
		m_isPointerPressed = false;
	}
	else if (right)
	{
		m_main->onTouchDown(500, 500);
		m_isPointerPressed = true;
        
		m_main->onTouchDragged(500, 500);
		m_main->onTouchDragged(900, 500);
		m_main->onTouchUp(900, 500);
		m_isPointerPressed = false;
	}
	else if (down)
	{
		m_main->onTouchDown(500, 500);
		m_isPointerPressed = true;
        
		m_main->onTouchDragged(500, 500);
		m_main->onTouchDragged(500, 900);
		m_main->onTouchUp(500, 900);
		m_isPointerPressed = false;
	}
}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetLogicalSize(e->NewSize);
	m_main->CreateWindowSizeDependentResources();
}

#if defined(WINAPI_FAMILY)
	#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
		void DirectXPage::OnBackPressed(Platform::Object^ sender, BackPressedEventArgs^ args)
		{
			args->Handled = m_main->handleOnBackPressed();
		}
	#elif WINAPI_FAMILY == WINAPI_FAMILY_APP
		#if WINAPI_PARTITION_PHONE_APP
			void DirectXPage::OnBackPressed(Platform::Object^ sender, BackPressedEventArgs^ args)
			{
				args->Handled = m_main->handleOnBackPressed();
			}
		#endif
	#endif
#endif

void DirectXPage::OnAdReady(Platform::Object^ sender, Platform::Object^ args)
{
	// Empty
}

void DirectXPage::OnAdCompleted(Platform::Object^ sender, Platform::Object^ args)
{
	m_main->StartRenderLoop();
	RequestInterstitialAd();
}

void DirectXPage::OnAdCancelled(Platform::Object^ sender, Platform::Object^ args)
{
	m_main->StartRenderLoop();
	RequestInterstitialAd();
}

void DirectXPage::OnAdError(Platform::Object^ sender, Microsoft::Advertising::WinRT::UI::AdErrorEventArgs^ args)
{
	m_main->StartRenderLoop();
	RequestInterstitialAd();
}