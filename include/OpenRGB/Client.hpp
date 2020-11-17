//======================================================================================================================
// Project: OpenRGB - C++ SDK
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  1.11.2020
// Description: OpenRGB network client
//======================================================================================================================

#ifndef OPENRGB_CLIENT_INCLUDED
#define OPENRGB_CLIENT_INCLUDED


#include "Color.hpp"
#include "DeviceInfo.hpp"

#include <string>  // client name
#include <memory>  // unique_ptr<Socket>
#include <chrono>  // timeout

namespace own {
	class TcpClientSocket;
}

// HACK: This definition needs to be copied here from SystemErrorInfo.hpp, because that header is now in a submodule,
// and it isn't visible to the library users that add this to their include directories
#ifdef _WIN32
	using system_error_t = uint32_t;  // should be DWORD but let's not include the whole windows.h just because of this
#else
	using system_error_t = int;
#endif


namespace orgb {


//======================================================================================================================

/** All the possible ways how the connect operation can end up. */
enum class ConnectStatus
{
	Success,               ///< The operation was successful.
	NetworkingInitFailed,  ///< Operation failed because underlying networking system could not be initialized. Call getLastSystemError() for more info.
	AlreadyConnected,      ///< Connect operation failed because the socket is already connected. Call disconnect() first.
	HostNotResolved,       ///< The hostname you entered could not be resolved to IP address. Call getLastSystemError() for more info.
	ConnectFailed,         ///< Could not connect to the target server, either it's down or the port is closed. Call getLastSystemError() for more info.
	SendNameFailed,        ///< Failed to send the client name to the server. Call getLastSystemError() for more info.
	OtherError             ///< Other system error. Call getLastSystemError() for more info.
};

/** All the possible ways how a request can end up. */
enum class RequestStatus
{
	Success,               ///< The request was succesful.
	NotConnected,          ///< Request failed because the client is not connected. Call connect(...) first.
	SendRequestFailed,     ///< Failed to send the request message.
	ConnectionClosed,      ///< Server has closed the connection.
	NoReply,               ///< No reply has arrived from the server in given timeout. In case this happens too often, you may try to increase the timeout.
	ReceiveError,          ///< There has been some other error while trying to receive a reply. Call getLastSystemError() for more info.
	InvalidReply           ///< The reply from the server is invalid.
};

/** All the possible results of a check whether the locally stored device list is out of date */
enum class UpdateStatus
{
	UpToDate,              ///< The current device list seems up to date.
	OutOfDate,             ///< Server has sent a notification message indicating that the device list has changed. Call requestDeviceList() again.
	ConnectionClosed,      ///< Server has closed the connection.
	UnexpectedMessage,     ///< Server has sent some other kind of message that we didn't expect.
	CantRestoreSocket,     ///< Error has occured while trying to restore socket to its original state and the socket has been closed. Call getLastSystemError() for more info. This should never happen, but one never knows.
	OtherError             ///< Other system error. Call getLastSystemError() for more info.
};

/** Result and output of a device list request */
struct DeviceListResult
{
	RequestStatus status;
	DeviceList devices;
};


//======================================================================================================================
/** OpenRGB network client. Use this to communicate with the OpenRGB service in order to set colors on your RGB devices. */

class Client
{

 public:

	/** Creates a client of specified or default name. Does not connect anywhere yet. */
	Client( const std::string & clientName = "orgb::Client" );

	~Client();

	/** Connects to the OpenRGB server and announces our client name. */
	ConnectStatus connect( const std::string & host = "127.0.0.1", uint16_t port = 6742 );

	/** Closes connection to the server. */
	void disconnect();

	bool isConnected() const;

	/** Sets a timeout for receiving request answers. */
	bool setTimeout( std::chrono::milliseconds timeout );

	/** Queries the server for information about all its RGB devices. */
	DeviceListResult requestDeviceList();

	/** Checks if the device list you downloaded earlier via requestDeviceList() hasn't been changed on the server.
	  * In case it has been changed, you need to call requestDeviceList() again. */
	UpdateStatus checkForDeviceUpdates();

	// TODO: seems currently unfinished on the server side
	//RequestStatus modifyMode( const Mode & mode );

	/** Changes the color mode of a device to "Direct".
	  * This needs to be called before any of the methods below, but with at least few milliseconds delay. */
	RequestStatus switchToDirectMode( const Device & device );

	/** Sets one unified color for the whole device. */
	RequestStatus setDeviceColor( const Device & device, Color color );

	/** Sets a color for a particular zone of a device. */
	RequestStatus setZoneColor( const Zone & zone, Color color );

	/** Resizes a zone of leds, if the device supports it. */
	RequestStatus setZoneSize( const Zone & zone, uint32_t newSize );

	/** Sets a color for one selected LED. */
	RequestStatus setColorOfSingleLED( const LED & led, Color color );

	/** Call this if your requests keep failing and you don't know why. */
	system_error_t getLastSystemError() const;
	std::string getLastSystemErrorStr( system_error_t errorCode ) const;
	std::string getLastSystemErrorStr() const;

 private: // helpers

	template< typename Message, typename ... ConstructorArgs >
	bool sendMessage( ConstructorArgs ... args );

	template< typename Message >
	struct RecvResult
	{
		RequestStatus status;
		Message message;
	};
	template< typename Message >
	RecvResult< Message > awaitMessage();

	UpdateStatus hasUpdateMessageArrived();

 private:

	std::string _clientName;

	// pointer so that we don't have to include the TcpSocket and all its OS dependancies here
	std::unique_ptr< own::TcpClientSocket > _socket;

	bool _isDeviceListOutOfDate;

};


//======================================================================================================================


} // namespace orgb


#endif // OPENRGB_CLIENT_INCLUDED
