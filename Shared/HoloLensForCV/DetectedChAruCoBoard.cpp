#include "pch.h"
#include "DetectedChArUcoBoard.h"

namespace HoloLensForCV
{
	DetectedChArUcoBoard::DetectedChArUcoBoard(
			_In_ Windows::Foundation::Numerics::float3 position,
			_In_ Windows::Foundation::Numerics::float3 rotation,
			_In_ Windows::Foundation::Numerics::float4x4 cameraToWorldUnity,
			_In_ Windows::Foundation::Collections::IVector<DetectedArUcoMarker^>^ detectedArUcoMarkers)
	{
		// Set the position, rotation and cam to world transform
		// properties of current marker
		DetectedArUcoMarkers = detectedArUcoMarkers;
		Position = position;
		Rotation = rotation;
		CameraToWorldUnity = cameraToWorldUnity;
	}
}