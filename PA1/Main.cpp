#include "Ackerman.h"
#include "BuddyAllocator.h"
#include <unistd.h>

int main(int argc, char** argv)
{
	int basic_block_size = 128, memory_length = 128 * 1024 * 1024;

	int opt = 0;
	while ((opt = getopt(argc, argv, "b:s:")) != -1)
	{ // While options were received from getopt
		int n = atoi(optarg);
		switch (opt)
		{
			case 'b': // If block size is specified
				if (n < 1)
				{
					printf("ERROR: Block sizes must be strictly positive!\n");
					return 0;
				}
				basic_block_size = n;
				break;
			case 's': // If total size is specified
				if (n < 1)
				{
					printf("ERROR: Memory length must be strictly positive!\n");
					return 0;
				}
				memory_length = n;
				break;
			case '?': // If unknown, end the program (getopt produces its own error message)
				return 0;
		}
	}

	cout << "Block size = " << basic_block_size << endl;
	cout << "Total size = " << memory_length << endl;
	// create memory manager
	BuddyAllocator* allocator = new BuddyAllocator(basic_block_size, memory_length);

	// test memory manager
	Ackerman* am = new Ackerman();
	am->test(allocator); // this is the full-fledged test.
	delete am;

	// destroy memory manager
	delete allocator;
}
