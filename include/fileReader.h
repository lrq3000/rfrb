#ifndef FILEREADER_H
#define FILEREADER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <mutex>
#include <thread>
#include <future>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <defines.h>
#include <bufferedFileReader.h>

class FileReader : private Buffered_FileReader
{
	private:
		/* Member variables for reading the file
		 */
		char *block;
		size_t readSize;
		int64_t offset;
		size_t blockSize;

	public:
		/* Constructors
		 */
		FileReader();
		FileReader(std::string inPath, size_t size);

		/* Getter for block
		 */
		char *getBlock();

		/* Getter for file specific variables
		 */
		std::string getPath();
		size_t getFileSize();

		/* Setter for file offset
		 */
		void setOffset(size_t in);

		/* Getter and setter for blocksize
		 */
		size_t getBlockSize();
		size_t getCurrentBlockSize();
		void setBlockSize(size_t blockS);

		/* Function for calculating the entropy of a block
		 */
		float calcEntropyOfCurrentBlock();

		/* This function checks if the block is empty. If yes, true is returned
		 */
		bool emptyBlock();

		/* Searches for the first non empty block and returns the adress if one is found
		 */
		int64_t findFirstNonemptyBlock();

		/* Searches for the first empty block and returns the adress if one is found.
		 */
		int64_t findFirstEmptyBlock();

		/* Function to skip a special amount of buffers
		 */
		bool skipInputBuffer(int NumOfBuffers);

		/* The two buffers are switched in this function.
		 * It also reloads a buffer to 'loadBuffer', using threads.
		 */
		bool reloadBuffer();

		/* This function loads a block from the 'workBuffer' to 'block'
		 */
		bool newBlock();

		/* Function to reset the fileReader
		 */
		void reset();

		/* Function to close the file after reading is done
		 */
		void closeFile();

		/* Helpfunction to print a block
		 */
		void printBlock();

		/* Helpfunctions for the position in a file
		 */
		std::string getRelativePos();
		size_t getCurrentAdress();

};

#endif
