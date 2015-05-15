#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <fstream>
#include <unistd.h>
#include <sys/time.h>

class FileWriter
{
	private:
		/* Member variables for opening the file
		 */
		std::ofstream fs;
		std::string outPath;

		/* Member variables for time measuring and performance evaluation
		 */
		double startTime;
		double endTime;
		size_t written;

		/* Member variables for writing to the file
		 */
		size_t bufferSize;
		bool init;
		size_t pos;
		size_t toWrite;

		/* Member variables for threads and synchronization
		 */
		std::mutex localMtx;
		std::future<void> futureObj;

		/* Two buffers for the async writing
		 */
		char *workBuffer;
		char *writeBuffer;

		/* This function is called from writeToFile().
		 * The write() function is called using threads.
		 */
		bool writeAsync();

		/* In this function, the writing really takes place.
		 */
		bool write();
		
		/* Helpfunction for time measuring
	 	 */
		double timeU();

	public:
		/* Constructors
		 */
		FileWriter();
		FileWriter(std::string path,size_t bufferSize);

		/* Destructor
		 */
		~FileWriter();

		/* Function to get the write performance
		 */
		double getTransferRate();
		
		/* Function which writes 'buf' to the file
		 */
		bool writeToFile(char *buf, size_t size);

		/* Function to close the file after writing is done
		 */
		bool closeFile();

		/* Getter and setter for the output path
		 */
		std::string getPath();
		void setPath(std::string path);

};

#endif
