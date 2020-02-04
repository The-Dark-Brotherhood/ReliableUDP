#pragma once
#include <assert.h>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <algorithm>
#include <functional>
#include <cstring> // for memcpy


class Connection
{
public:

	enum Mode
	{
		None,
		Client,
		Server
	};

	Connection(unsigned int protocolId, float timeout)
	{
		this->protocolId = protocolId;
		this->timeout = timeout;
		mode = None;
		running = false;
		ClearData();
	}

	virtual ~Connection()
	{
		if (IsRunning())
			Stop();
	}

	//Start the connection by opening a socket
	//set running to true
	bool Start(int port)
	{
		assert(!running);
		printf("start connection on port %d\n", port);
		if (!socket.Open(port))
			return false;
		running = true;
		OnStart();
		return true;
	}

	//Close the connection
	void Stop()
	{
		assert(running);
		printf("stop connection\n");
		bool connected = IsConnected();
		ClearData();
		socket.Close();
		running = false;
		if (connected)
			OnDisconnect();
		OnStop();
	}

	bool IsRunning() const
	{
		return running;
	}

	//check if connected, clear data, if connected, call 
	//on disconnect to clear data/reset
	void Listen()
	{
		printf("server listening for connection\n");
		bool connected = IsConnected();
		ClearData();
		if (connected)
			OnDisconnect();
		mode = Server;
		state = Listening;
	}

	void Connect(const Address& address)
	{
		printf("client connecting to %d.%d.%d.%d:%d\n",
			address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort());
		bool connected = IsConnected();
		ClearData();
		if (connected)
			OnDisconnect();
		mode = Client;
		state = Connecting;
		this->address = address;
	}

	bool IsConnecting() const
	{
		return state == Connecting;
	}

	bool ConnectFailed() const
	{
		return state == ConnectFail;
	}

	bool IsConnected() const
	{
		return state == Connected;
	}

	bool IsListening() const
	{
		return state == Listening;
	}

	Mode GetMode() const
	{
		return mode;
	}

	virtual void Update(float deltaTime)
	{
		assert(running);
		timeoutAccumulator += deltaTime;
		if (timeoutAccumulator > timeout)
		{
			if (state == Connecting)
			{
				printf("connect timed out\n");
				ClearData();
				state = ConnectFail;
				OnDisconnect();
			}
			else if (state == Connected)
			{
				printf("connection timed out\n");
				ClearData();
				if (state == Connecting)
					state = ConnectFail;
				OnDisconnect();
			}
		}
	}

	virtual bool SendPacket(const unsigned char data[], int size)
	{
		assert(running);
		if (address.GetAddress() == 0)
			return false;
		unsigned char packet[PACKETSIZE];
		packet[0] = (unsigned char)(protocolId >> 24);
		packet[1] = (unsigned char)((protocolId >> 16) & 0xFF);
		packet[2] = (unsigned char)((protocolId >> 8) & 0xFF);
		packet[3] = (unsigned char)((protocolId) & 0xFF);
		std::memcpy(&packet[4], data, size);
		return socket.Send(address, packet, size + 4);
	}

	virtual int ReceivePacket(unsigned char data[], int size)
	{
		assert(running);
		unsigned char packet[PACKETSIZE];
		Address sender;
		int bytes_read = socket.Receive(sender, packet, size + 4);
		if (bytes_read == 0)
			return 0;
		if (bytes_read <= 4)
			return 0;
		if (packet[0] != (unsigned char)(protocolId >> 24) ||
			packet[1] != (unsigned char)((protocolId >> 16) & 0xFF) ||
			packet[2] != (unsigned char)((protocolId >> 8) & 0xFF) ||
			packet[3] != (unsigned char)(protocolId & 0xFF))
			return 0;
		if (mode == Server && !IsConnected())
		{
			printf("server accepts connection from client %d.%d.%d.%d:%d\n",
				sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort());
			state = Connected;
			address = sender;
			OnConnect();
		}
		if (sender == address)
		{
			if (mode == Client && state == Connecting)
			{
				printf("client completes connection with server\n");
				state = Connected;
				OnConnect();
			}
			timeoutAccumulator = 0.0f;
			memcpy(data, &packet[4], bytes_read - 4);
			return bytes_read - 4;
		}
		return 0;
	}

	//constant get size:4
	int GetHeaderSize() const
	{
		return 4;
	}

protected:

	virtual void OnStart() {}
	virtual void OnStop() {}
	virtual void OnConnect() {}
	virtual void OnDisconnect() {}

private:
	//set state to disconnected
	//timeout accumulator to 0
	void ClearData()
	{
		state = Disconnected;
		timeoutAccumulator = 0.0f;
		address = Address();
	}

	enum State
	{
		Disconnected,
		Listening,
		Connecting,
		ConnectFail,
		Connected
	};

	unsigned int protocolId;
	float timeout;

	bool running;
	Mode mode;
	State state;
	Socket socket;
	float timeoutAccumulator;
	Address address;
};