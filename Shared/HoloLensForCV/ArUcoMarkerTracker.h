#pragma once
#include "DetectedArUcoMarker.h"

namespace HoloLensForCV
{
	// ArUcoMarkerTracker runtime class
	public ref class ArUcoMarkerTracker sealed
	{
	public:
		ArUcoMarkerTracker(
			float markerSize,
			int dictId,
			Windows::Perception::Spatial::SpatialCoordinateSystem^ unitySpatialCoodinateSystem);

		Windows::Foundation::Collections::IVector<DetectedArUcoMarker^>^
			DetectArUcoMarkersInFrame(
				SensorFrame^ pvSensorFrame);

	private:
		// Cached parameters for aruco marker detection
		float _markerSize;
		int _dictId;

		// Vector of markers which can be passed across the WinRT interface
		Windows::Foundation::Collections::IVector<DetectedArUcoMarker^>^ _detectedArUcoMarkers;

		// Spatial coordinate system from unity
		Windows::Perception::Spatial::SpatialCoordinateSystem^ _unitySpatialCoordinateSystem;

	};
}


