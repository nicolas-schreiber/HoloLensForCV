#include "pch.h"
#include "DesktopStreamer.h"

namespace HoloLensForCV
{
		: _writeInProgress(false)
	{
		// Initialize stream socket listener and beginning 
		// looking for connections
		_listener = ref new Windows::Networking::Sockets::StreamSocketListener();

		_listener->ConnectionReceived +=
			ref new Windows::Foundation::TypedEventHandler<
			Windows::Networking::Sockets::StreamSocketListener^,
			Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^>(
				this,
				&DesktopStreamer::OnConnection);

		_listener->Control->KeepAlive = true;

		// Don't limit traffic to an address or an adapter.
		Concurrency::create_task(_listener->BindServiceNameAsync(serviceName)).then(
			[this](Concurrency::task<void> previousTask)
		{
			try
			{
				// Try getting an exception.
				previousTask.get();
			}
			catch (Platform::Exception^ exception)
			{
#if DBG_ENABLE_ERROR_LOGGING
				dbg::trace(
					L"DesktopStreamer::DesktopStreamer: %s",
					exception->Message->Data());
#endif /* DBG_ENABLE_ERROR_LOGGING */
			}
		});
	}
}

