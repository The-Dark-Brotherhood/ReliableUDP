/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility> 

#include "Net.h"
#include "FlowControl.h"

//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

void MakeFileFromPacket(unsigned char packet[]);


int main(int argc, char* argv[])
{
	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;

	if (argc >= 2)
	{
		int a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
	}

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;

	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)
		connection.Connect(address);
	else
		connection.Listen();

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;

	//--> Relevant Info --//
	unsigned int packetCounter = 0;
	string filepath = "C:\\Users\\Ggurgel8686\\source\\repos\\SET\\huge.png";
	FILE* fileToSend = NULL;
	fileToSend = fopen(filepath.c_str(), "rb");

	while (true)
	{
		// update flow control

		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}

		// send and receive packets

		sendAccumulator += DeltaTime;

		//----- Sending Packets -----//
		while (connection.IsConnected() && sendAccumulator > 1.0f / sendRate)
		{
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));
			unsigned char bytesRead = 0;

			//--> OUR CODE 
			bytesRead = fread(packet, 1, CONTENTSIZE - 1, fileToSend);  // I changed the read bytes to remove the space for the extra info(footer)
			if (bytesRead != 0)	// Packet with content
			{
				packet[200] = bytesRead;
				// Send Packet
				connection.SendPacket(packet, sizeof(packet));
				sendAccumulator -= 1.0f / sendRate;
			}

			// Close file and exit loop when the last packet was already sent 
			if (bytesRead < CONTENTSIZE - 1 || bytesRead == 0)
			{
				fclose(fileToSend);
			}
		}

		//----- Receiving Packets -----//
		char bytesRead = 0;
		while (true)
		{
			//CHANGEMADE
			unsigned char packet[PACKETSIZE] = "";
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read <= 0)
				break;
			else
			{
				MakeFileFromPacket(packet);
			}
		}
	

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		net::wait(DeltaTime);
	}

	ShutdownSockets();

	return 0;
}

void MakeFileFromPacket(unsigned char packet[])
{
	ofstream newFile;
	int bytesToWrite = (int)packet[200];

	// Write to file
	newFile.open(".\\crapola.txt", ios::out | ios::binary | ios::app);
	newFile.write((char*)packet, bytesToWrite);
	newFile.close();
}
