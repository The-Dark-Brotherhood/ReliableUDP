#pragma once
#include <assert.h>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <algorithm>
#include <functional>
#include <cstring> // for memcpy

// packet queue to store information about sent and received packets sorted in sequence order
	//  + we define ordering using the "sequence_more_recent" function, this works provided there is a large gap when sequence wrap occurs

struct PacketData
{
	unsigned int sequence;			// packet sequence number
	float time;					    // time offset since packet was sent or received (depending on context)
	int size;						// packet size in bytes
};

inline bool sequence_more_recent(unsigned int s1, unsigned int s2, unsigned int max_sequence)
{
	auto half_max = max_sequence / 2;
	return (
		((s1 > s2) && (s1 - s2 <= half_max))
		||
		((s2 > s1) && (s2 - s1 > half_max))
		);
}