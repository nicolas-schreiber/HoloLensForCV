#include "pch.h"
#include "MainPage.xaml.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

using namespace Concurrency;
using namespace Windows::AI::MachineLearning;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::Storage;

namespace ComputeOnDesktopYolo
{
	MainPage::MainPage()
	{
		InitializeComponent();
	}

	void MainPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^)
	{
		// Initialize the interop link
		_parser->InitializeInterop();

		// Initialize the size of the Yolo canvas
		GetCameraSize();

		// Load the model asynchronously
		LoadModelAsync();

		_isInitialized = true;
	}

	void MainPage::ConnectSocket_Click()
	{
		//
		// By default 'HostNameForConnect' is disabled and host name validation is not required. When enabling the
		// text box validating the host name is required since it was received from an untrusted source
		// (user input). The host name is validated by catching ArgumentExceptions thrown by the HostName
		// constructor for invalid input.
		//
		Windows::Networking::HostName^ hostName;

		try
		{
			hostName = ref new Windows::Networking::HostName(
				HostNameForConnect->Text);
		}
		catch (Platform::Exception^)
		{
			return;
		}

		Windows::Networking::Sockets::StreamSocket^ pvCameraSocket =
			ref new Windows::Networking::Sockets::StreamSocket();
		pvCameraSocket->Control->KeepAlive = true;

		// Create the sender to send data to device over specified socket
		_dataStreamer =
			ref new HoloLensForCV::DesktopStreamer(_dataSocketName);

#if DBG_ENABLE_VERBOSE_LOGGING
		dbg::trace(
			L"MainPage::ConnectSocket_Click: data sender created");
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

		//
		// Save the socket, so subsequent steps can use it.
		//
		concurrency::create_task(
			pvCameraSocket->ConnectAsync(hostName, _pvServiceName->Text)
		).then(
			[this, pvCameraSocket]()
		{
#if DBG_ENABLE_VERBOSE_LOGGING
			dbg::trace(
				L"MainPage::ConnectSocket_Click: server connection established");
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

			HoloLensForCV::SensorFrameReceiver^ receiver =
				ref new HoloLensForCV::SensorFrameReceiver(
					pvCameraSocket);

			ReceiverLoop(
				receiver);
		});
	}

	void MainPage::ReceiverLoop(
		HoloLensForCV::SensorFrameReceiver^ receiver)
	{
		concurrency::create_task(
			receiver->ReceiveAsync()).then(
				[this, receiver](
					concurrency::task<HoloLensForCV::SensorFrame^> sensorFrameTask)
		{
			HoloLensForCV::SensorFrame^ sensorFrame =
				sensorFrameTask.get();

#if DBG_ENABLE_VERBOSE_LOGGING
			dbg::trace(
				L"MainPage::ReceiverLoop: receiving a %ix%i image of type %i with timestamp %llu",
				sensorFrame->SoftwareBitmap->PixelWidth,
				sensorFrame->SoftwareBitmap->PixelHeight,
				(int32_t)sensorFrame->FrameType,
				sensorFrame->Timestamp);
#endif /* DBG_ENABLE_VERBOSE_LOGGING */

			OnFrameReceived(
				sensorFrame);

			Windows::UI::Core::CoreDispatcher^ uiThreadDispatcher =
				Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher;

			uiThreadDispatcher->RunAsync(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler(
					[this, sensorFrame]()
			{

				switch (sensorFrame->FrameType)
				{
				case HoloLensForCV::SensorType::PhotoVideo:
				{
					Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource^ imageSource =
						ref new Windows::UI::Xaml::Media::Imaging::SoftwareBitmapSource();

					concurrency::create_task(
						imageSource->SetBitmapAsync(sensorFrame->SoftwareBitmap)
					).then(
						[this, imageSource]()
					{
						_pvImage->Source = imageSource;

					}, concurrency::task_continuation_context::use_current());
				}
				break;

				default:
					throw new std::logic_error("invalid frame type");
				}
			}));

			ReceiverLoop(
				receiver);
		});
	}

	void MainPage::OnFrameReceived(
		HoloLensForCV::SensorFrame^ sensorFrame)
	{

		if (_isInitialized)
		{
			// From windows ML examples, MNIST cpp
			// Asynchronously evaluate incoming frames using Yolo framework
			VideoFrame^ inputFrame = VideoFrame::CreateWithSoftwareBitmap(sensorFrame->SoftwareBitmap);
			_input->image = ImageFeatureValue::CreateFromVideoFrame(inputFrame);
			create_task(this->_model->EvaluateAsync(_input))
				.then([this, inputFrame](YoloModelIO::Output^ output) {
				this->_output = output;

				// Convert tensor float output to vector image
				IVectorView<float>^ outputVector = _output->grid->GetAsVectorView();
				Windows::Foundation::Collections::IVector<YoloRuntime::BoundingBox^>^
					boxesVector = _parser->ParseOutputsInterop(outputVector, 0.5f);

				// Send the bounding boxes to HoloLens over socket
				// make sure there arent a million boxes by strict filtering
				// using non max suppression
				IVector<YoloRuntime::BoundingBox^>^ filteredBoxes =
					_parser->NonMaxSuppressInterop(boxesVector, 5, 0.3f);
				_dataStreamer->Send(filteredBoxes);

				// Draw yolo overlays on the frame
				DrawOverlays(inputFrame, filteredBoxes);
			});
		}
	}


	void MainPage::GetCameraSize()
	{
		_canvasActualWidth = (int)_pvImage->ActualWidth;
		_canvasActualHeight = (int)_pvImage->ActualHeight;
	}

	concurrency::task<void> MainPage::LoadModelAsync()
	{
		// just load the model one time.
		if (_model == nullptr)
		{
			std::wstringstream wStringstream;
			wStringstream << "Loading model..." << "\n";
			OutputDebugString(wStringstream.str().c_str());

			try
			{
				String^ filePath = "ms-appx:///Assets/" + modelFileName;

				// From Windows ML samples, MNIST cpp
				create_task(StorageFile::GetFileFromApplicationUriAsync(
					ref new Uri(filePath)))
					.then([](StorageFile^ modelFile) {
					return create_task(YoloModelIO::Model::CreateFromStreamAsync(
						modelFile,
						LearningModelDeviceKind::Default)); })
					.then([this](YoloModelIO::Model ^model) {
						this->_model = model;
					});
			}
			catch (const std::exception&)
			{
				std::wstringstream wStringstream2;
				wStringstream2 << "Error loading the model." << "\n";
				OutputDebugString(wStringstream2.str().c_str());
				_model = nullptr;
			}
		}
	}

	void MainPage::DrawOverlays(
		Windows::Media::VideoFrame^ inputImage,
		Windows::Foundation::Collections::IVector<YoloRuntime::BoundingBox^>^ boxes)
	{
		YoloCanvas->Children->Clear();
		if (boxes->Size <= 0) return;

		// Iterate across all boxes
		std::for_each(begin(boxes),
			end(boxes),
			[&](YoloRuntime::BoundingBox^ box)
		{
			DrawYoloBoundingBox(box, YoloCanvas);
		});
	}

	void MainPage::DrawYoloBoundingBox(
		YoloRuntime::BoundingBox^ box,
		Windows::UI::Xaml::Controls::Canvas^ overlayCanvas)
	{
		GetCameraSize();

		// process output boxes
		// find max of set
		// condition ? result_if_true : result_if_false
		// Check bounds
		int x = (int)box->X > 0.0f ? box->X : 0.0f;
		int y = (int)box->Y > 0.0f ? box->Y : 0.0f;
		int w = (int)(overlayCanvas->ActualWidth - x) <
			box->Width ? (overlayCanvas->ActualWidth - x) : box->Width;
		int h = (int)(overlayCanvas->ActualHeight - y) <
			box->Height ? (overlayCanvas->ActualHeight - y) : box->Height;

		// fit to current canvas and webcam size
		// 416 x 416 is size of tensor input
		x = _canvasActualWidth * x / 416;
		y = _canvasActualHeight * y / 416;
		w = _canvasActualWidth * w / 416;
		h = _canvasActualHeight * h / 416;

		auto rectStroke = box->Label == "person" ? _lineBrushGreen : _lineBrushYellow;

		auto r = ref new Windows::UI::Xaml::Shapes::Rectangle();
		r->Tag = box;
		r->Width = w;
		r->Height = h;
		r->Fill = _fillBrush;
		r->Stroke = rectStroke;
		r->StrokeThickness = _lineThickness;
		r->Margin = Thickness(x, y, 0, 0);

		auto tb = ref new TextBlock();
		tb->Margin = Thickness(x + 4, y + 4, 0, 0);
		tb->Text = box->Label + ": " + box->Confidence.ToString();
		tb->FontWeight = Windows::UI::Text::FontWeights::Bold;
		tb->Width = 126;
		tb->Height = 21;
		tb->HorizontalTextAlignment = TextAlignment::Center;

		auto textBack = ref new Windows::UI::Xaml::Shapes::Rectangle();
		textBack->Width = 134;
		textBack->Height = 29;
		textBack->Fill = rectStroke;
		textBack->Margin = Thickness(x, y, 0, 0);

		overlayCanvas->Children->Append(textBack);
		overlayCanvas->Children->Append(tb);
		overlayCanvas->Children->Append(r);
	}

}
