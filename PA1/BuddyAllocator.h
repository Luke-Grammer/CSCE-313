/* 
    File: my_allocator.h

    Original Author: R.Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 08/02/08

    Modified:

 */

#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_

#include <iostream>
using namespace std;
typedef unsigned int uint;

/* declare types as you need */

struct BlockHeader
{
	// decide what goes here
	// hint: obviously block size will go here
	BlockHeader* next = nullptr;
	uint block_size = 0;
	bool free = true;
};

class LinkedList
{
	// this is a special linked list that is made out of BlockHeaders. 
private:
	BlockHeader* head = nullptr;		// you need a head of the list
	uint size = 0;

public:
	void insert (BlockHeader* b)
	{	// adds a block to the front of a linked list
		BlockHeader* temp = head;
		head = b;
		b->next = temp;
		size++;
	}

	void remove (BlockHeader* b)
	{   // removes a particular block from the list

		if (size == 0)
		{
			cout << "ERROR: Attempt to remove BlockHeader from empty list!\n";
			return;
		}

		if (head == b)
		{ // If removing the head
			head = b->next;
			size--;
			return;
		}

		// Otherwise loop through list to find block
		BlockHeader* it = head;
		while(it->next != nullptr)
		{
			if (it->next == b)
			{
				it->next = b->next;
				size--;
				return;
			}
			it = it->next;
		}

		cout << "ERROR: Could not find memory block for list removal!\n";
	}

	BlockHeader* get_head()
	{ // Returns a pointer to the head of the list
		return head;
	}

	uint get_size()
	{ // Returns the size of the linked list
		return size;
	}
};


class BuddyAllocator
{
private:
	/* declare member variables as necessary */
	LinkedList* free_list = nullptr;
	char* base_addr = nullptr;
	uint basic_block_size = 0;
	uint total_size = 0;
	uint max_depth = 0;

private:
	/* private function you are required to implement
	 this will allow you and us to do unit test */
	
	BlockHeader* getbuddy(BlockHeader* addr); 
	// given a block address, this function returns the address of its buddy 
	
	bool arebuddies(BlockHeader* block1, BlockHeader* block2);
	// checks whether the two blocks are buddies are not

	BlockHeader* merge(BlockHeader* block1, BlockHeader* block2);
	// this function merges the two blocks returns the beginning address of the merged block
	// note that either block1 can be to the left of block2, or the other way around

	BlockHeader* split(BlockHeader* block);
	// splits the given block by putting a new header halfway through the block
	// also, the original header needs to be corrected


public:
	BuddyAllocator (int _basic_block_size, int _total_memory_length); 
	/* This initializes the memory allocator and makes a portion of 
	   ’_total_memory_length’ bytes available. The allocator uses a ’_basic_block_size’ as 
	   its minimal unit of allocation. The function returns the amount of 
	   memory made available to the allocator. If an error occurred, 
	   it returns 0. 
	*/ 

	~BuddyAllocator(); 
	/* Destructor that returns any allocated memory back to the operating system. 
	   There should not be any memory leakage (i.e., memory staying allocated).
	*/ 

	char* alloc(int _length); 
	/* Allocate _length number of bytes of free memory and returns the 
		address of the allocated portion. Returns 0 when out of memory. */ 

	int free(char* _a); 
	/* Frees the section of physical memory previously allocated 
	   using ’my_malloc’. Returns 0 if everything ok. */ 
   
	void debug();
	/* Mainly used for debugging purposes and running short test cases */
	/* This function should print how many free blocks of each size belong to the allocator
	at that point. The output format should be the following (assuming basic block size = 128 bytes):

	128: 5
	256: 0
	512: 3
	1024: 0
	....
	....
	 which means that at point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};

#endif 
