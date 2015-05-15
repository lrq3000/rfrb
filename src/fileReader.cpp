#include <fileReader.h>

/* This constructor should not be used
 */
FileReader::FileReader() : Buffered_FileReader()
{
	offset=0;
}

/* Correct constructor. Initial values are set and file is opened
 */
FileReader::FileReader(std::string inPath, size_t size) : Buffered_FileReader(inPath, size)
{
	blockSize = BLOCKSIZE;
	block = getBuffer();
	offset=0;
}

/* Getter for block
 */
char *FileReader::getBlock()
{
	return &getBuffer()[offset];
}

/* Getter for the file path
 */
std::string FileReader::getPath()
{
	return Buffered_FileReader::getPath();
}

/* Getter for the size of the file
 */
size_t FileReader::getFileSize()
{
	return Buffered_FileReader::getFileSize();
}

/* Setter for file offset
 */
void FileReader::setOffset(size_t off)
{
	offset = off - getGlobalAdress();
	while (static_cast<size_t>(offset)+blockSize > getNumberOfElements())
	{
		reloadBuffer();
		offset = off - getGlobalAdress();
	}
	this->readSize = std::min(blockSize, getNumberOfElements()-offset);
	block = &getBuffer()[offset];
}

//getter for previously set blocksize
size_t FileReader::getBlockSize()
{
	return blockSize;
}

/* Getter for current blocksize
 */
size_t FileReader::getCurrentBlockSize()
{
	return readSize;
}

/* Setter for blocksize
 */
void FileReader::setBlockSize(size_t blockS)
{
	this->blockSize=blockS;
	this->readSize = std::min(blockSize, getNumberOfElements()-offset);
}

/* Function for calculating the entropy of a block
 */
float FileReader::calcEntropyOfCurrentBlock()
{
	float ret = 0.0f;
	unsigned int possibleVals[256]={};
	for (unsigned int i = 0; i < readSize; ++i)
	{
		possibleVals[(unsigned char)block[i]] += 1;
	}
	float tmp;
	if (possibleVals[0] == readSize)
		return 0.0f;
	for (unsigned int i = 0; i < 256; ++i)
	{
		tmp = (float)possibleVals[i]/(float)readSize;
		if (tmp>0)
			ret += -tmp * std::log2(tmp);
	}
	return ret;
}

/* This function checks if the block is empty. If yes, true is returned
 */
bool FileReader::emptyBlock()
{
	char *tmp = getBuffer();
	for (size_t i = 0; i < getNumberOfElements(); ++i)
		if (tmp[i] > 0)
			return false;
	return true;
}

/* Searches for the first non empty block and returns the adress if one is found
 */
int64_t FileReader::findFirstNonemptyBlock()
{
	while (newBlock()==true)
	{
		for (size_t j = 0; j < readSize; ++j)
		{
			if (block[j] > 0)
			{
				return (int64_t)getCurrentAdress();
			}
		}
	}
	return -1;
}

/* Searches for the first empty block and returns the adress if one is found.
 */
int64_t FileReader::findFirstEmptyBlock()
{
	while (newBlock()==true)
	{
		bool zero = true;
		for (size_t j = 0; j < readSize; ++j)
		{
			if (block[j] > 0)
			{
				zero = false;
				break;
			}
		}
		if (zero == true)
			return (int64_t)getCurrentAdress();
	}
	return -1;
}

/* Function to skip a special amount of buffers
 */
bool FileReader::skipInputBuffer(int NumOfBuffers)
{
	for (int i = 0; i < NumOfBuffers; ++i)
		reloadBuffer();
	return true;
}

/* The two buffers are switched in this function.
 * It also reloads a buffer to 'loadBuffer', using threads.
 */
bool FileReader::reloadBuffer()
{
	bool ret = Buffered_FileReader::reloadBuffer();
	offset = 0;
	block = Buffered_FileReader::getBuffer();
	readSize = std::min(blockSize, Buffered_FileReader::getNumberOfElements()-blockSize);
	return ret;
}

/* This function loads a block from the 'workBuffer' to 'block'
 */
bool FileReader::newBlock()
{
	offset += readSize;
	readSize = std::min(getNumberOfElements(), std::min(blockSize, getNumberOfElements()-offset));
	if (readSize == 0)
	{
		bool ret = reloadBuffer();
		block = getBuffer();
		return ret;
	}
	block = &getBuffer()[offset];
	return true;
}

/* Function to reset the fileReader
 */
void FileReader::reset()
{
	Buffered_FileReader::reset();
	offset = 0;
	readSize = std::min(blockSize, Buffered_FileReader::getNumberOfElements());
	block = Buffered_FileReader::getBuffer();
}

/* Function to close the file after reading is done
 */
void FileReader::closeFile()
{
	Buffered_FileReader::closeFile();
}

/* Helpfunction to print a block
 */
void FileReader::printBlock()
{
	for (unsigned int i = 0; i < readSize; i = i+16)
	{
		for (int j = 0; j < 16; ++j)
		{
			if (block[i+j] == 0)
			{
				std::cout << ".";
			}
			else
				std::cout << block[i+j];
		}
		std::cout << std::endl;
	}
}

/* Helpfunctions for the position in a file
 */
std::string FileReader::getRelativePos()
{
	size_t currentPos = getCurrentAdress();
	double blubb = (double)currentPos*100.0f/(double)getFileSize();
	char t[128];
	sprintf(t,"%16lu/%lu", currentPos, getFileSize());
	std::string out = std::string(t) + "\t\t"+std::to_string(blubb)+"%";
	return out;
}

/* Helpfunctions for the position in a file (adress)
 */
size_t FileReader::getCurrentAdress()
{
	return offset+getGlobalAdress();
}
