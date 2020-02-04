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
#include <conio.h>
#include <ctype.h>

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

/* -------------------------------------------------------------
* Function: checkFileExists()
* Description: This function is used to check if a file exists(by
*								attempting to read from it)
* Parameters: char* file -> name of file to try to open.
* Returns: 0 if failed, 1 if successful
* ------------------------------------------------------------ */
int checkFileExists(char* file)
{
	int ret = 0;
	FILE* fp = NULL;
	if (fp = fopen(file, "r"))
	{
		ret = 1;
		fclose(fp);
	}
	return ret;
}
//https://stackoverflow.com/questions/8233842/how-to-check-if-directory-exist-using-c-and-winapi/8233867
bool dirExists(char* dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}
int main(int argc, char* argv[])
{
	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;
	printf("Which mode would you like to run in?\n[S]erver or [C]lient?\n");

	while (true)
	{
		char input = _getche();
		if ((input == 'S') || (input == 's'))
		{
			mode = Server;
			printf("\nRunning in Server mode.\n");
			break;
		}
		else if ((input == 'C') || (input == 'c'))
		{
			mode = Client;
			printf("\nRunning in Client mode.\n");
			break;
		}
		else
		{
			printf("\nUnrecognized input. Please enter S for Server or C for Client.\n");
		}
	}
	char buffer[100] = "";

	if (mode == Client)
	{
		printf("Please enter the IP address you would like to connect to.\n");
		fgets(buffer, 25, stdin);
		int a, b, c, d;
		sscanf(buffer, "%d.%d.%d.%d", &a, &b, &c, &d);
		
	}
	printf("Please enter the port you would like to connect to.\n");
	fgets(buffer, 100, stdin);
	int port = atoi(buffer);
	/*if (argc >= 2)
	{
		int a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
	}*/
	FILE* fileToSend = NULL;
	if (mode == Server)
	{
		while (true)
		{
			printf("Please enter the path of the file you would like to send\n");
		
			fgets(buffer, 100, stdin);
			char* pos;
			if ((pos = strchr(buffer, '\n')) != NULL)
				*pos = '\0';
			if (!checkFileExists(buffer))
			{
				printf("File does not exist\n");

			}
			else
			{
				break;
			}
		}
		fileToSend = fopen(buffer, "rb");
	}
	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	//const int port = mode == Server ? ServerPort : atoi(buffer);

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
	
	//--> Relevant Info --//'
	if (mode == Server)
	{
		string filepath = "C:\\Users\\Ggurgel8686\\source\\repos\\SET\\file.txt";		
		fileToSend = fopen(filepath.c_str(), "rb");
	}

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
		while (sendAccumulator > 1.0f / sendRate)
		{
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));
			unsigned char bytesRead = 0;

			// Build filename with extension packet 
			//if (packetCounter == 0)
			//{

			//}
			if (mode == Server && connected && !connection.IsConnected())
			{

				//--> OUR CODE 
				bytesRead = fread(packet, 1, CONTENTSIZE - 1, fileToSend);  // I changed the read bytes to remove the space for the extra info(footer)
				if (bytesRead != 0)	// Packet with content
				{
					packet[200] = bytesRead;
				}

				// Close file and exit loop when the last packet was already sent 
				if (bytesRead < CONTENTSIZE - 1 || bytesRead == 0)
				{
					fclose(fileToSend);
				}
			}

			// Send Packet
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;
		}

		//----- Receiving Packets -----//
		char bytesRead = 0;
		while (true)
		{
			//CHANGEMADE
			unsigned char packet[PACKETSIZE] = "";
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
				if (bytes_read > 0)
				{
					MakeFileFromPacket(packet);
				}
				else
				{
					break;
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


	int packetRead = (int)packet[200];

	// Write to file
	newFile.open(".\\crapola.png", ios::out | ios::binary | ios::app);
	newFile.write((char*)packet, packetRead);
	newFile.close();
}
