/* 
    File: my_allocator.cpp
*/
#include "BuddyAllocator.h"
#include "Helper.cpp"
#include <iostream>
#include <math.h>
using namespace std;

BuddyAllocator::BuddyAllocator(int _basic_block_size, int _total_memory_length)
{ // Creates the framework for a buddy system memory allocator and reserves a portion of memory for its use
	basic_block_size = next_power_of_2(_basic_block_size);
	total_size = next_power_of_2(_total_memory_length);

	if (total_size < sizeof(BlockHeader) + basic_block_size)
	{
		printf("ERROR: Total size is not large enough to accommodate the basic block size, changing total size!\n");
		total_size = next_power_of_2(sizeof(BlockHeader) + basic_block_size);
	}

	printf("Base block size now %u\n", basic_block_size); //DEBUG
	printf("Total size now %u\n", total_size); //DEBUG

	// create new array of LinkedLists of size max_depth + 1 (max_depth is max indexable depth)
	max_depth = log2(total_size / basic_block_size);
	free_list = new LinkedList[max_depth + 1];

	// Allocate a new block of size total_size
	base_addr = new char[total_size];

	// Add BlockHeader to beginning of block and insert into free_list
	BlockHeader* base_head = (BlockHeader*) base_addr;
	base_head->block_size = total_size;
	base_head->free = true;
	free_list[max_depth].insert(base_head);
}

BuddyAllocator::~BuddyAllocator()
{
	// Delete dynamically allocated memory
	delete[] base_addr;
	delete[] free_list;
}

char* BuddyAllocator::alloc(int _length)
{ // Returns a pointer to the beginning of a usable portion of memory of a specified size
	uint free_block_size = 0;
	
	//printf("\nALLOC\n"); //DEBUG

	// Find actual length needed
	//printf("%i bytes requested\n", _length); //DEBUG
	_length = next_power_of_2(_length + sizeof(BlockHeader));
	//printf("(%i actual)\n", _length); //DEBUG

	// Check if input is valid
	if (_length <= basic_block_size)
	{
		_length = basic_block_size;
	}
	else if(_length > total_size)
	{
		printf("ERROR: Cannot allocate block larger than %u bytes!\n", (unsigned) (total_size - sizeof(BlockHeader)));
		printf("(%i bytes requested)\n", _length);
		return nullptr;
	}

	uint depth = log2(_length / basic_block_size);

	// Find large enough block on the free list starting at the smallest
	for(uint i = depth; i <= max_depth; ++i)
	{
		if (free_list[i].get_size() > 0)
		{
			free_block_size = free_list[i].get_head()->block_size;
			break;
		}
	}

	// If there was no large enough block on the free list
	if (free_block_size == 0)
	{
		printf("ERROR: Could not find large enough free block!\n");
		printf("(%u bytes requested)\n", _length);
		printf("Current free blocks:\n");
		debug();
		return nullptr;
	}
	
	uint free_block_level = log2(free_block_size / basic_block_size);

	BlockHeader* block = free_list[free_block_level].get_head();
	
	// Whittle down block until it is the same size as _length (which is a power of two)
	while (block->block_size > _length)
	{
		block = split(block);
		--free_block_level;
	}

	// Remove allocated block from the free list
	free_list[free_block_level].remove(block);
	block->free = false;

	//printf("Allocating block of size %u\n", block->block_size);
	//printf("Current free blocks:\n");
	//debug(); //DEBUG

	// Return the beginning of the block/the end of the metadata
	return (char*) block + sizeof(BlockHeader);
}

int BuddyAllocator::free(char* _a)
{ // Frees a block allocated by alloc()
	//printf("\nFREE\n"); //DEBUG

	// Recover the metadata stashed away by alloc
	BlockHeader* addr = (BlockHeader*) (_a - sizeof(BlockHeader));

	// Add the header back to the free list
	uint depth = log2(addr->block_size / basic_block_size);
	free_list[depth].insert(addr);
	addr->free = true;

	//printf("About to free block of size %u\n", addr->block_size);
	//printf("Current free blocks:\n");
	//debug(); //DEBUG

	// If there is more than one block in the free list, check if merge is needed
	while(free_list[depth].get_size() > 1 && depth < max_depth)
	{
		BlockHeader* buddy = getbuddy(addr);

		if(buddy->free && arebuddies(addr, buddy))
		{
			addr = merge(addr, buddy);
			++depth;
		}
		else
			break;
	}

	//printf("Freed block!\n");
	//printf("Current free blocks:\n");
	//debug(); //DEBUG
	return 1;
}

BlockHeader* BuddyAllocator::getbuddy(BlockHeader* addr)
{ // given a block address, this function returns the address of its buddy
	return (BlockHeader*) ((((char*) addr - base_addr) ^ addr->block_size) + base_addr);
}

bool BuddyAllocator::arebuddies(BlockHeader* block1, BlockHeader* block2)
{ // checks whether the two blocks are buddies are not and makes sure they are the same size
	if (getbuddy(block1) != block2 || block2->block_size != block1->block_size)
		return false;
	return true;
}

BlockHeader* BuddyAllocator::merge(BlockHeader* block1, BlockHeader* block2)
{ /* This function merges the two blocks returns the beginning address of the merged block.
     (note that either block1 can be to the left of block2, or the other way around)
  */
 	//printf("\nMERGE\n"); //DEBUG

 	if (block2 < block1)
	{
		swap(block1, block2);
	}

	if (block1->block_size != block2->block_size)
	{
		printf("ERROR: Cannot merge blocks of different sizes!\n");
		return nullptr;
	}

	uint depth = log2(block1->block_size / basic_block_size);

	// Remove the blocks from the current free list, and add the first block to the next free list
	free_list[depth].remove(block1);
	free_list[depth].remove(block2);

	block1->block_size *= 2;

	free_list[depth + 1].insert(block1);
	
	return block1;
}

BlockHeader* BuddyAllocator::split(BlockHeader* block)
{ /* Splits the given block by putting a new header halfway through the block.
     Also, the original header needs to be corrected.
  */
 	//printf("\nSPLIT\n"); //DEBUG

 	uint depth = log2(block->block_size / basic_block_size);

	if (depth < 1)
	{
		printf("ERROR: Cannot split beyond basic block size!\n");
		return nullptr;
	}

	// Remove block from free list
 	free_list[depth].remove(block);

 	block->block_size /= 2;

	// Add a new BlockHeader in the center of the block
	BlockHeader* new_header = (BlockHeader*) ((char*) block + block->block_size);
	new_header->block_size = block->block_size;
	new_header->free = true;

	// Insert both BlockHeaders into the previous free list
	free_list[depth - 1].insert(block);
	free_list[depth - 1].insert(new_header);
 	return new_header;
}

void BuddyAllocator::debug()
{ // Prints each level size and number of free blocks of each size
	for (uint i = 0; i <= max_depth; ++i)
	{
		printf("%u: %u\n", (unsigned) (basic_block_size * pow(2, i)), free_list[i].get_size());
	}
}
