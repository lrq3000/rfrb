#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <future>
#include <map>
#include <thread>
#include <iomanip>
#include <dirent.h>
#include <string.h>

#include <fileReader.h>
#include <defines.h>
#include <fileWriter.h>

class FileHandler
{
	private:
		/* Member variables which handle the reading and writing of a file
		 */
		std::string inFolder;
		std::string outFolder;
		std::vector<FileReader*> inFiles;
		FileWriter *writer;
		size_t mStripeSize;
	
		/* Function which determines the important parts of the file, where we can build our heuristic for the stripemap.
		 */
		std::vector<std::vector<std::pair<bool, std::vector<size_t>>>> buildMapHeuristic(std::vector<std::vector<Block>> &, std::vector<size_t> &);

		/* Function to manually define the stripemap
		 */
		void userStripeMapInteraction(std::vector<std::vector<mapEntry>> &map);

	public:
		/* Constructors
		 */
		FileHandler();
		FileHandler(std::string out);
		FileHandler(std::string inputFolder, std::string outputFolder);
	
		/* Setter for block size
		 */
		void setBlockSize(size_t blocksize);

		/* Getter for the fileReaders, which hold the input files
		 */
		std::vector<FileReader*> &getInFiles();

		/* Getter for a specific fileReader
		 */
		FileReader *getFileReader(size_t num);

		/* Getter for the fileWriter
		 */
		FileWriter *getFileWriter();

		/* Add a recovered file image (Raid5_incomplete)
		 */
		void addImage(std::string path);

		/* Function to find a block with data in it
		 */
		bool findGoodBlock();

		/* To reload the buffers in the fileReaders
		 */
		bool reloadBuffers();

		/* Resets the fileReaders
		 */
		void reset();

		/* Function to estimate the stripesize in kilo byte.
		 */
		int estimateStripeSize();

		/* Function to estimate the stripemap.
		 */
		std::vector<std::vector<mapEntry>> estimateStripeMap(bool isRaid5);

};

#endif

