#ifndef _BUFFERED_FILEREADER_H
#define _BUFFERED_FILEREADER_H

#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <cmath>
#include <memory>
#include <unistd.h>

class Buffered_FileReader
{
	private:
		/* Member variables for reading the file
		 */
		std::ifstream m_fs;
		size_t m_fileSize;
		std::string m_path;

		/* Member variables to work on the file
		 */
		size_t m_bufferSize;
		size_t m_elements;		//number of elements in buffers
		size_t m_loadedElements;
		size_t m_globalAdress;

		/* Two buffers. One to work on, another to load into (for asynchronous load)
		 */
		char *m_loadBuffer;
		char *m_workBuffer;

		/* Thread-related
		 */
		std::future<bool> m_threadSync;
		std::mutex m_localMtx;

		inline void loadBuffer();

	public:
		/* Constructors
		 */
		Buffered_FileReader();
		Buffered_FileReader(std::string path, size_t size);

		/* Getter for Buffers
		 */
		size_t getNumberOfElements();
		size_t getBufferSize();
		char *getBuffer();

		/* Getter for file specific variables
		 */
		std::string getPath();
		size_t getFileSize();
		size_t getGlobalAdress();

		/* Function to swith buffers and load new part into loadbuffer
		 */
		bool reloadBuffer();

		/* Function to reset the bufferedFileReader
		 */
		void reset();

		/* Function to close the file after reading is done
		 */
		void closeFile();

};

#endif

