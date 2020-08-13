#pragma once

#include "DesktopStreamerHeader.h"
namespace HoloLensForCV
{
	// Collect bounding box information and stream from PC to connected HoloLens.
	// Using sensor frame streaming server as a reference.
	public ref class DesktopStreamer sealed
	{
	public:
		DesktopStreamer(Platform::String^ serviceName);

	private:
		bool _writeInProgress;
	};
}

