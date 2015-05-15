#ifndef DEFINES_H
#define DEFINES_H

#include <vector>

#define BUFLENGTH (1024*1024*1024)

#define BLOCKSIZE (512)
#define CHECKSIZE (BLOCKSIZE)

/* Struct for an entry in the stripemap
 */
typedef struct mapEntry
{
	bool isParity;
	int dataBlock;
	mapEntry(bool a, int b) : isParity(a), dataBlock(b)
	{}
} mapEntry;

/* Struct for a datablock, so that the tool can work on it
 */
typedef struct Block
{
	std::vector<float> entropies;
	size_t startAdress;
	bool zeroFirst;
	size_t endAdress;
	bool zeroLast;
	Block(size_t a, bool c, size_t b, bool d) : startAdress(a), zeroFirst(c), endAdress(b), zeroLast(d)
	{}
	Block(){}
} Block;

#endif
