#include "BuddyAllocator.h"
#include <iostream>

typedef unsigned int uint;
using namespace std;

uint next_power_of_2(uint n)
{ // Rounds a positive integer up to the next power of two 
	--n;
	for (uint i = 1; i <= 16; i *= 2)
	{
		n |= n >> i;
	}
	return ++n;
}

void swap(BlockHeader*& block1, BlockHeader*& block2)
{ // Swaps two BlockHeaders
	BlockHeader* temp = block2;
	block2 = block1;
	block1 = temp;
}