
#include "Socket.h"

#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3
#define PACKETSIZE		  300
#define CONTENTSIZE		  10

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS

#include <winsock2.h>
#pragma comment( lib, "wsock32.lib" )

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#else

#error unknown platform!

#endif
Socket::Socket()
{
	socket = 0;
}

Socket::~Socket()
{
	Close();
}

bool Socket::Open(unsigned short port)
{
	assert(!IsOpen());

	// create socket

	socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (socket <= 0)
	{
		printf("failed to create socket\n");
		socket = 0;
		return false;
	}

	// bind to port

	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((unsigned short)port);

	if (bind(socket, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
	{
		printf("failed to bind socket\n");
		Close();
		return false;
	}

	// set non-blocking io

#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

	int nonBlocking = 1;
	if (fcntl(socket, F_SETFL, O_NONBLOCK, nonBlocking) == -1)
	{
		printf("failed to set non-blocking socket\n");
		Close();
		return false;
	}

#elif PLATFORM == PLATFORM_WINDOWS

	DWORD nonBlocking = 1;
	if (ioctlsocket(socket, FIONBIO, &nonBlocking) != 0)
	{
		printf("failed to set non-blocking socket\n");
		Close();
		return false;
	}

#endif

	return true;
}

void Socket::Close()
{
	if (socket != 0)
	{
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
		close(socket);
#elif PLATFORM == PLATFORM_WINDOWS
		closesocket(socket);
#endif
		socket = 0;
	}
}

bool Socket::IsOpen() const
{
	return socket != 0;
}

bool Socket::Send(const Address& destination, const void* data, int size)
{
	assert(data);
	assert(size > 0);

	if (socket == 0)
		return false;

	assert(destination.GetAddress() != 0);
	assert(destination.GetPort() != 0);

	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(destination.GetAddress());
	address.sin_port = htons((unsigned short)destination.GetPort());

	int sent_bytes = sendto(socket, (const char*)data, size, 0, (sockaddr*)&address, sizeof(sockaddr_in));

	return sent_bytes == size;
}

int Socket::Receive(Address& sender, void* data, int size)
{
	assert(data);
	assert(size > 0);

	if (socket == 0)
		return false;

#if PLATFORM == PLATFORM_WINDOWS
	typedef int socklen_t;
#endif

	sockaddr_in from;
	socklen_t fromLength = sizeof(from);

	int received_bytes = recvfrom(socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength);

	if (received_bytes <= 0)
		return 0;

	unsigned int address = ntohl(from.sin_addr.s_addr);
	unsigned short port = ntohs(from.sin_port);

	sender = Address(address, port);

	return received_bytes;
}