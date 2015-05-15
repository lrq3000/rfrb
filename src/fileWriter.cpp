#include <fileWriter.h>

extern std::mutex mtx;

/* This constructor should not be used
 */
FileWriter::FileWriter()
{
	std::cerr << "Wrong FileWriter constructor is called!" << std::endl;
}

/* Correct constructor. Initial values are set
 */
FileWriter::FileWriter(std::string path, size_t bufferS)
{
	outPath = path;
	init = true;
	bufferSize = bufferS;
	workBuffer = new char[bufferSize];
	writeBuffer = new char[bufferSize];
	pos = 0;
	startTime = endTime = 0.0f;
	written = 0;
}

/* Destructor
 */
FileWriter::~FileWriter()
{
	fs.close();
}

/* Function to get the write performance
 */
double FileWriter::getTransferRate()
{
	double tmp = endTime - startTime;
	tmp = written/(double)tmp;
	tmp /= 1024.0f;
	tmp *= 1000.0f;
	tmp /= 1024.0f;
	tmp *= 1000.0f;
	return tmp;
}

/* Function which writes 'buf' to the file
 */
bool FileWriter::writeToFile(char *buf, size_t size)
{
	//Buffer is written async
	size_t counter = 0;
	while (counter < size)
	{
		if (pos >= bufferSize)
		{
			//If the first buffer should be written, the file has to be opened
			if (init == true)
			{
				startTime = endTime = timeU();
				fs.open(outPath.c_str(), std::fstream::out);
				if (!(fs))
				{
					std::cerr << "ERROR FileWriter::FileWriter - Couldnt open file: " << outPath << std::endl;
					exit(EXIT_FAILURE);
				}
				init = false;
			}
			if (writeAsync() == false)
			{
				std::cerr << "Something went terribly wrong! " << std::endl;
				return false;
			}
			continue;
		}
		workBuffer[pos] = buf[counter];
		++pos;
		++counter;
	}
	return true;
}

/* Function to close the file after writing is done
 */
bool FileWriter::closeFile()
{
	//Check if file is open
	if (fs.is_open())
	{
		writeAsync();
		localMtx.lock();
		fs.close();
		localMtx.unlock();
		//Using Unix, the hashsums are calculated
#ifndef WINDOWS
		std::string sys = "md5sum ";
		sys += outPath;
		std::cout << std::endl;
		std::cout << "Hash5 for " << outPath << ":" << std::endl;
		if (system(sys.c_str()))
		{}
		std::cout << std::endl;
#endif
	}
	return true;
}

/* Getter for the output path
 */
std::string FileWriter::getPath()
{
	return outPath;
}

/* Setter for the output path
 */
void FileWriter::setPath(std::string path)
{
	closeFile();
	pos = 0;
	outPath = path;
	init = true;
}

/* This function is called from writeToFile().
 * The write() function is called using threads.
 */
bool FileWriter::writeAsync()
{
	auto lambda = [&] () -> void
	{
		write();
		localMtx.unlock();
	};

	localMtx.lock();
	char *tmp = writeBuffer;
	writeBuffer = workBuffer;
	workBuffer = tmp;
	toWrite = pos;
	pos = 0;
	//Threads are started with the lambda function
	futureObj = std::async(std::launch::async, lambda);
	return true;
}

/* In this function, the writing really takes place.
 */
bool FileWriter::write()
{
	//If file is valid, write
	mtx.lock();
	if(fs.is_open() && fs.good())
	{
		fs.write(writeBuffer, toWrite);
		written += toWrite;
		endTime = timeU();
	}
	mtx.unlock();

	if (fs.bad())
	{
		std::cerr << "Writer is bad" << std::endl;
		return false;
	}
	return true;
}

/* Helpfunction for time measuring
 */
double FileWriter::timeU()
{
	double ret;
	struct timeval a;
	gettimeofday(&a, NULL);
	ret = a.tv_usec + a.tv_sec*1000 *1000;
	return ret;
}
