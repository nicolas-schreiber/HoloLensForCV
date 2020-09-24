#pragma once
#include "DetectedArUcoMarker.h"

namespace HoloLensForCV
{
	// Class to hold important properties of the 
	// detected ArUco markers to send to Unity.
	public ref class DetectedChArUcoBoard sealed
	{
	public:
		DetectedChArUcoBoard(
			_In_ Windows::Foundation::Numerics::float3 position,
			_In_ Windows::Foundation::Numerics::float3 rotation,
			_In_ Windows::Foundation::Numerics::float4x4 cameraToWorldUnity,
			_In_ Windows::Foundation::Collections::IVector<DetectedArUcoMarker^>^ detectedArUcoMarkers);

		property Windows::Foundation::Collections::IVector<DetectedArUcoMarker^>^ DetectedArUcoMarkers;
		property Windows::Foundation::Numerics::float3 Position;
		property Windows::Foundation::Numerics::float3 Rotation;
		property Windows::Foundation::Numerics::float4x4 CameraToWorldUnity;
	};
}

