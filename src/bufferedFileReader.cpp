#include <bufferedFileReader.h>

std::mutex HDD_Avail;

/* This constructor should not be used
 */
Buffered_FileReader::Buffered_FileReader()
{
}

/* Correct constructor. Initial values are set
 */
Buffered_FileReader::Buffered_FileReader(std::string path, size_t size) : m_path(path), m_bufferSize(size)
{
	m_fs.open(m_path.c_str(), std::ifstream::in | std::ifstream::binary);

	if (!(m_fs))
	{
		std::cerr << "ERROR FileReader::FileReader - Couldnt open file: " << path << std::endl;
		exit(EXIT_FAILURE);
	}
	//Get filesize
	m_fs.seekg(0,m_fs.end);
	m_fileSize = m_fs.tellg();
	m_fs.seekg(0,m_fs.beg);

	m_loadBuffer = new char[m_bufferSize];
	m_workBuffer = new char[m_bufferSize];

	m_elements = m_loadedElements = 0;
	m_globalAdress = 0;
	reloadBuffer();
}

/* Getter for the number of elements
 */
size_t Buffered_FileReader::getNumberOfElements()
{
	return m_elements;
}

/* Getter for the size of the buffer
 */
size_t Buffered_FileReader::getBufferSize()
{
	return m_bufferSize;
}

/* Getter for the buffer
 */
char *Buffered_FileReader::getBuffer()
{
	return m_workBuffer;
}

/* Getter for the path to the file
 */
std::string Buffered_FileReader::getPath()
{
	return m_path;
}

/* Getter for the size of the file
 */
size_t Buffered_FileReader::getFileSize()
{
	return m_fileSize;
}

/* Getter for the global adress of the file
 */
size_t Buffered_FileReader::getGlobalAdress()
{
	return m_globalAdress;
}

/* Function to swith buffers and load new part into loadbuffer
 */
bool Buffered_FileReader::reloadBuffer()
{
	//Lambda function
	auto lambda = [&] () -> bool
	{
		HDD_Avail.lock();
		if (m_fs.is_open())
		{
			m_fs.read((char *)m_loadBuffer, m_bufferSize);
			m_loadedElements = m_fs.gcount();
		}
		HDD_Avail.unlock();
		m_localMtx.unlock();
		if (m_loadedElements == 0)
			return false;
		return true;
	};

	//Swap buffers
	m_localMtx.lock();
	char *tmp = m_loadBuffer;
	m_loadBuffer = m_workBuffer;
	m_workBuffer = tmp;
	m_globalAdress += m_elements;
	m_elements = m_loadedElements;

	//Reload buffer async
	m_threadSync = std::async(std::launch::async, lambda);
	if (m_elements == 0)
		return false;
	return true;
}

/* Function to reset the bufferedFileReader
 */
void Buffered_FileReader::reset()
{
	m_localMtx.lock();
	m_fs.clear();
	m_fs.seekg(0, m_fs.beg);
	m_elements = m_loadedElements = 0;
	m_globalAdress = 0;
	m_localMtx.unlock();
	reloadBuffer();
}

/* Function to close the file after reading is done
 */
void Buffered_FileReader::closeFile()
{
	m_localMtx.lock();
	if (m_fs.is_open())
	{
		m_fs.close();
	}
	m_localMtx.unlock();
}
