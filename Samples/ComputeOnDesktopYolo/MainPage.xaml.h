#pragma once

#include "MainPage.g.h"

namespace ComputeOnDesktopYolo
{
	public ref class MainPage sealed
	{
	public:
		MainPage();

		void ConnectSocket_Click();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void ReceiverLoop(
			HoloLensForCV::SensorFrameReceiver^ receiver);

		void OnFrameReceived(
			HoloLensForCV::SensorFrame^ sensorFrame);


		// Variables
		// Model file name and instances of classes to define model inputs and outputs.
		Platform::String^ _dataSocketName = L"12345";
		HoloLensForCV::DesktopStreamer^ _dataStreamer;

		bool _isInitialized = false;
		Platform::String^ modelFileName = "tinyYoloV2_12.onnx";
		const Windows::AI::MachineLearning::LearningModelDeviceKind _kModelDeviceKind = Windows::AI::MachineLearning::LearningModelDeviceKind::Default;

		YoloModelIO::Model^ _model = nullptr;
		YoloModelIO::Input^ _input = ref new YoloModelIO::Input();
		YoloModelIO::Output^ _output = ref new YoloModelIO::Output();

		int _canvasActualWidth;
		int _canvasActualHeight;

		// Requirements for drawing
		Windows::UI::Xaml::Media::SolidColorBrush^ _lineBrushYellow = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Yellow);
		Windows::UI::Xaml::Media::SolidColorBrush^ _lineBrushGreen = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Green);
		Windows::UI::Xaml::Media::SolidColorBrush^ _fillBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Transparent);
		double _lineThickness = 2.0;

		// New intances of bounding box and yolo parser files.
		// Equivalent of IList<T> is IVector<T>
		YoloRuntime::WinMlParserInterop^ _parser = ref new YoloRuntime::WinMlParserInterop();

		// Methods
		// Set the size of the canvas to the camera preview size.
		void GetCameraSize();

		// Asynchronously load the onnx model file using input string.
		concurrency::task<void> LoadModelAsync();

		// Draw overlays onto the canvas rendering
		void DrawOverlays(Windows::Media::VideoFrame^ inputImage, Windows::Foundation::Collections::IVector<YoloRuntime::BoundingBox^>^ boxes);
		void DrawYoloBoundingBox(YoloRuntime::BoundingBox^ box, Windows::UI::Xaml::Controls::Canvas^ overlayCanvas);

	};
}
