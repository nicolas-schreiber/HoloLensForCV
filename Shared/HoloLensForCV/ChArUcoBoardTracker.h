#pragma once
#include "DetectedChArUcoBoard.h"
#include "DetectedArUcoMarker.h"

#include <opencv2/aruco.hpp>
#include <opencv2/core.hpp>

namespace HoloLensForCV
{
	// ChArUcoBoardTracker runtime class
	public ref class ChArUcoBoardTracker sealed
	{
	public:
		ChArUcoBoardTracker(
            int squaresX,
            int squaresY,
            float squareSize,
            float markerSize,
			int dictId,
			Windows::Perception::Spatial::SpatialCoordinateSystem^ unitySpatialCoodinateSystem);

		DetectedChArUcoBoard^ DetectChArUcoBoardInFrame(SensorFrame^ pvSensorFrame);

	private:
		// Cached parameters for aruco marker detection
        int _squaresX;
        int _squaresY;
        float _squareSize;
		float _markerSize;
		int _dictId;


		// Spatial coordinate system from unity
		Windows::Perception::Spatial::SpatialCoordinateSystem^ _unitySpatialCoordinateSystem;

	};
}


