﻿//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "Direct3DManager.h"

using namespace NosFURatu;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
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

DirectXPage::DirectXPage(): m_coreInput(nullptr), m_isPointerPressed(false)
{
	InitializeComponent();

	ApplicationView^ view = ApplicationView::GetForCurrentView();
	view->TryEnterFullScreenMode();

	// Register event handlers for page lifecycle.
	CoreWindow^ window = Window::Current->CoreWindow;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged += 
		ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged +=
		ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);

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

	m_gameScreen = std::unique_ptr<Direct3DGameScreen>(new Direct3DGameScreen(m_deviceResources.get()));
	m_gameScreen->onResume();

	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);

	StartRenderLoop();
}

DirectXPage::~DirectXPage()
{
	// Stop rendering and processing events on destruction.
	StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_deviceResources->Trim();

	// Stop rendering when the app is suspended.
	StopRenderLoop();

	// Put code to save app state here.
}

// Loads the current state of the app for resume events.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Put code to load app state here.

	// Start rendering when the app is resumed.
	StartRenderLoop();
}

// Window event handlers.

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	if (args->Visible)
	{
		StartRenderLoop();
	}
	else
	{
		StopRenderLoop();
	}
}

// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_deviceResources->SetDpi(sender->LogicalDpi);
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_deviceResources->ValidateDevice();
}

void DirectXPage::OnPointerPressed(Object^ sender, PointerEventArgs^ args)
{
	m_gameScreen->onTouch(DOWN, args->CurrentPoint->RawPosition.X, args->CurrentPoint->RawPosition.Y);

	m_isPointerPressed = true;
}

void DirectXPage::OnPointerMoved(Object^ sender, PointerEventArgs^ args)
{
	if (m_isPointerPressed)
	{
		m_gameScreen->onTouch(DRAGGED, args->CurrentPoint->RawPosition.X, args->CurrentPoint->RawPosition.Y);
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerEventArgs^ args)
{
	m_gameScreen->onTouch(UP, args->CurrentPoint->RawPosition.X, args->CurrentPoint->RawPosition.Y);

	m_isPointerPressed = false;
}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_deviceResources->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_deviceResources->SetLogicalSize(e->NewSize);
	m_gameScreen->onScreenSizeChanged(m_deviceResources->GetLogicalSize().Width, m_deviceResources->GetLogicalSize().Height);
}

void DirectXPage::StartRenderLoop()
{
	// If the animation render loop is already running then do not start another thread.
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
	{
		return;
	}

	// Create a task that will be run on a background thread.
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
	{
		// Calculate the updated frame and render once per vertical blanking interval.
		while (action->Status == AsyncStatus::Started)
		{
			critical_section::scoped_lock lock(m_criticalSection);
			Update();
			if (Render())
			{
				m_deviceResources->Present();
			}
		}
	});

	// Run task on a dedicated high priority background thread.
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void DirectXPage::StopRenderLoop()
{
	m_renderLoopWorker->Cancel();
}

// Updates the application state once per frame.
void DirectXPage::Update()
{
	switch (m_gameScreen->getRequestedAction())
	{
	case REQUESTED_ACTION_UPDATE:
		// Update scene objects.
		m_timer.Tick([&]()
		{
			m_gameScreen->update(m_timer.GetElapsedSeconds());
		});
		break;
	case REQUESTED_ACTION_LEVEL_EDITOR_SAVE:
		saveLevel();
		m_gameScreen->clearRequestedAction();
		break;
	case REQUESTED_ACTION_LEVEL_EDITOR_LOAD:
		loadLevel();
		m_gameScreen->clearRequestedAction();
		break;
	default:
		break;
	}
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool DirectXPage::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	m_gameScreen->render();
	m_gameScreen->handleMusic();
	m_gameScreen->handleSound();

	return true;
}

void DirectXPage::saveLevel()
{
	const char *level_json = LevelEditor::getInstance()->save();

	StorageFile^ file = NosfuratuFile;
	if (file != nullptr)
	{
		std::string s(level_json);
		std::wstring ws;
		ws.assign(s.begin(), s.end());
		String^ levelJson = ref new Platform::String(ws.c_str());
		if (levelJson != nullptr && !levelJson->IsEmpty())
		{
			create_task(FileIO::WriteTextAsync(file, levelJson)).then([this, file, levelJson](task<void> task)
			{
				try
				{
					task.get();
				}
				catch (COMException^ ex)
				{
					// TODO, update UI to reflect that an error has occurred
				}
			});
		}
		else
		{
			// TODO, update UI to reflect that an error has occurred
		}
	}
	else
	{
		create_task(KnownFolders::PicturesLibrary->CreateFileAsync(Filename, CreationCollisionOption::ReplaceExisting)).then([this](StorageFile^ file)
		{
			NosfuratuFile = file;
			saveLevel();
		});
	}
}

void DirectXPage::loadLevel()
{
	StorageFile^ file = NosfuratuFile;
	if (file != nullptr)
	{
		create_task(FileIO::ReadTextAsync(file)).then([this, file](task<String^> task)
		{
			try
			{
				String^ fileContent = task.get();
				if (fileContent != nullptr)
				{
					std::wstring fooW(fileContent->Begin());
					std::string fooA(fooW.begin(), fooW.end());
					const char *levelContent = fooA.c_str();
					LevelEditor::getInstance()->load(levelContent);
				}
				else
				{
					// TODO, update UI to reflect that an error has occurred
				}
			}
			catch (COMException^ ex)
			{
				// TODO, update UI to reflect that an error has occurred
			}
		});
	}
	else
	{
		create_task(KnownFolders::PicturesLibrary->CreateFileAsync(Filename, CreationCollisionOption::OpenIfExists)).then([this](StorageFile^ file)
		{
			NosfuratuFile = file;
			loadLevel();
		});
	}
}