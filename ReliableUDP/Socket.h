#pragma once
#include <assert.h>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <algorithm>
#include <functional>
#include <cstring> // for memcpy
#include "Address.h"

class Socket
{
public:

	Socket();

	~Socket();

	bool Open(unsigned short port);

	void Close();

	bool IsOpen() const;

	bool Send(const Address& destination, const void* data, int size);

	int Receive(Address& sender, void* data, int size);

private:

	int socket;
};