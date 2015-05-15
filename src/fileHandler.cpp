#include <fileHandler.h>

static int maxBufferSize;
std::mutex mtx;

/* This constructor should not be used
 */
FileHandler::FileHandler()
{
	std::cerr << "Wrong FileHandler constructor is called!" << std::endl;
}

/* Correct constructor. Initial values are set
 */
FileHandler::FileHandler(std::string outputFolder)
{
	maxBufferSize = 16lu*1024lu*1024lu;
	writer = new FileWriter(outputFolder+"recoveredImage.dd",maxBufferSize);
}

/* Correct constructor. Initial values are set and reader and writer are initialized
 */
FileHandler::FileHandler(std::string inputFolder, std::string outputFolder)
{
	inFolder = inputFolder;
	outFolder = outputFolder;
	std::vector<std::string> imgs;

	//Get all files in the input folder for windows and unix
#ifdef WINDOWS
	HANDLE dir;
	WIN32_FIND_DATA file_data;

	if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Couldn't find any files in directory " << inputFolder << std::endl;
		exit(EXIT_FAILURE);
	}
	do {
		const string file_name = file_data.cFileName;
		const string full_file_name = directory + "/" + file_name;
		const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (file_name[0] == '.')
			continue;
		if (is_directory)
			continue;
		imgs.push_back(full_file_name);
	} while (FindNextFile(dir, &file_data));

	FindClose(dir);
#else
	DIR *dpdf;
	struct dirent *epdf;
	dpdf = opendir(inputFolder.c_str());
	if (dpdf != NULL)
	{
		while ((epdf=readdir(dpdf))!=NULL)
		{
			if (epdf->d_name[0] == '.')
				continue;
			if (epdf->d_type == DT_REG)
			{
				std::string tmp = inputFolder+epdf->d_name;
				std::cout << tmp << std::endl;
				imgs.push_back(tmp);
			}
		}
	}
#endif

	//Error
	if (imgs.empty())
	{
		std::cerr << "No valid directory or no valid datas in directory." << std::endl;
		exit(EXIT_FAILURE);
	}

	//Initial values are set
	maxBufferSize = (BUFLENGTH/(imgs.size()+0))/2*2;
	size_t realLength=0;
	size_t counter=0;
	while (maxBufferSize != 0)
	{
		maxBufferSize = maxBufferSize >> 1;
		++counter;
	}
	counter-=1;
	realLength = std::pow(2,counter);
	maxBufferSize = std::min(realLength, 16lu*1024lu*1024lu);
	std::cout << "Image-Buffers:\t" << maxBufferSize/1024/1024 << "MB" << std::endl;

	//Calculate MD5Sum, if it is not windows, and initialize the fileReaders and the fileWriter
	for (unsigned int i = 0; i < imgs.size(); ++i)
	{
#ifndef WINDOWS
		std::string sys = "md5sum ";
		sys += imgs.at(i);
		std::cout << std::endl;
		std::cout << "Hash5 for " << imgs.at(i) << ":" << std::endl;
		if (system(sys.c_str()))
		{}
		std::cout << std::endl;
#endif
		FileReader *a = new FileReader(imgs.at(i),maxBufferSize);
		inFiles.push_back(a);
	}
	writer = new FileWriter(outFolder+"recoveredImage.dd",maxBufferSize);
}

/* Setter for block size in all files
 */
void FileHandler::setBlockSize(size_t blocksize)
{
	for (size_t i = 0; i < inFiles.size(); ++i)
		inFiles.at(i)->setBlockSize(blocksize);
	mStripeSize = blocksize;
}

/* Getter for the fileReaders, which hold the input files
 */
std::vector<FileReader *> &FileHandler::getInFiles()
{
	return inFiles;
}

/* Getter for a specific fileReader
 */
FileReader * FileHandler::getFileReader(size_t num)
{
	if (num >= inFiles.size())
	{
		std::cerr << "ERROR in FileHandler::getFileReader - num smaller than available Images" << std::endl;
		return NULL;
	}
	return inFiles.at(num);
}

/* Getter for the fileWriter
 */
FileWriter * FileHandler::getFileWriter()
{
	return writer;
}

/* Add a recovered file image (Raid5_incomplete)
 */
void FileHandler::addImage(std::string path)
{
	if (inFiles.size()==0)
	{
		maxBufferSize = (BUFLENGTH/1+1)/2*2;
		maxBufferSize = maxBufferSize/BLOCKSIZE;
		maxBufferSize = maxBufferSize*BLOCKSIZE;
		maxBufferSize = std::min(maxBufferSize,256*1024*1024);
		FileReader *a = new FileReader(path,maxBufferSize);
		inFiles.push_back(a);
	}
	else
	{
		FileReader *a = new FileReader(path,maxBufferSize);
		inFiles.push_back(a);
	}
}

/* Function to find a block with data in it
 */
bool FileHandler::findGoodBlock()
{
	int64_t sample=inFiles.at(0)->findFirstNonemptyBlock();
	if (sample == -1)
	{
		std::cout << "Couldn't load any good blocks anymore." << std::endl;
		return false;
	}
	//Set offset on the other images
	for (size_t i = 0; i < inFiles.size(); ++i)
	{
		inFiles.at(i)->setOffset(sample);
	}
	return true;
}

/* To reload the buffers in the fileReaders
 */
bool FileHandler::reloadBuffers()
{
	for (unsigned int i = 0; i < inFiles.size(); ++i)
	{
		if (inFiles.at(i)->reloadBuffer()==false)
			return false;
	}
	return true;
}

/* Resets the fileReaders
 */
void FileHandler::reset()
{
	for (unsigned int i = 0; i < inFiles.size(); ++i)
	{
		inFiles.at(i)->reset();
	}
}

/* Function to estimate the stripesize in kilo byte
 */
int FileHandler::estimateStripeSize()
{
	//Lambda function, which is done by threads
	auto lambda = [&] (size_t id, size_t size, const float lowEdge, const float highEdge) -> std::vector<int>
	{
		//Initialization
		std::vector<std::pair<size_t, float>> entropies;
		FileReader *me = inFiles.at(id);
		const size_t CHECKS=800;
		int counters[12]={};
		float ent=0;
		std::vector<size_t> adressEdges;
		me->setBlockSize(size);
		const int checkSize = 16;
		me->reset();

		//Initial fill of vector with zero
		for (int i = 0; i < checkSize; ++i)
			entropies.push_back(std::pair<size_t,float>(0,0));

		//While loop until enough adressEdges are found
		size_t outputCounter = 0;
		while (adressEdges.size() < CHECKS)
		{
			size_t edgeCut = 0;
			size_t counter=0;
			size_t edgeAdress=0;
			bool low=false;
			bool high=false;
			do
			{
				if (me->newBlock() == false)
				{
					break;
				}
				if (outputCounter == 0)
				{
					std::string wr = "Thread ";
					wr += std::to_string(id);
					wr += " working ";
					printf("\r%s\t%f%%\t%s",wr.c_str(), static_cast<float>(adressEdges.size())*100.0f/static_cast<float>(CHECKS), me->getRelativePos().c_str());
					fflush(stdout);
				}
				outputCounter = (outputCounter + 1) % 25000;

				//Calculate entropy and check if the edges are good enough
				ent = me->calcEntropyOfCurrentBlock();
				if (ent <= lowEdge)
				{
					if (low == false)
					{
						if (high == true && counter >= checkSize)
						{
							++edgeCut;
						}
						else
						{
							edgeCut = 0;
						}
						low = true;
						high = false;
						edgeAdress = me->getCurrentAdress();
						counter = 0;
					}
					++counter;
				}
				else if (ent >= highEdge)
				{
					if (high == false)
					{
						if (low == true && counter >= checkSize)
						{
							++edgeCut;
						}
						else
						{
							edgeCut = 0;
						}
						high = true;
						low = false;
						edgeAdress = me->getCurrentAdress();
						counter = 0;
					}
					++counter;
				}
				else
				{
					low=false;
					high=false;
					edgeCut=0;
					counter=0;
				}
				if (counter == checkSize && edgeCut == 1)
					++edgeCut;
			} while (edgeCut != 2);

			//found an edge
			if (edgeCut == 2)
			{
				adressEdges.push_back(edgeAdress);
			}
			else //end of file
			{
				break;
			}
		}

		std::string wr = "Thread ";
		wr += std::to_string(id);
		wr += " working ";
		printf("\r%s\t%9d%%",wr.c_str(), 100);
		me->reset();

		//Sorting adress edges to possible stripesizes
		for (size_t i = 1; i < adressEdges.size(); ++i)
		{
			size_t in = (adressEdges.at(i) - adressEdges.at(i-1))/512;
			if (in % 4096 == 0)
				counters[0] += 1;
			else if (in % 2048 == 0)
				counters[1] += 1;
			else if (in % 1024 == 0)
				counters[2] += 1;
			else if (in % 512 == 0)
				counters[3] += 1;
			else if (in % 256 == 0)
				counters[4] += 1;
			else if (in % 128 == 0)
				counters[5] += 1;
			else if (in % 64 == 0)
				counters[6] += 1;
			else if (in % 32 == 0)
				counters[7] += 1;
			else if (in % 16 == 0)
				counters[8] += 1;
			else if (in % 8 == 0)
				counters[9] += 1;
			else if (in % 4 == 0)
				counters[10] += 1;
			else if (in % 2 == 0)
				counters[11] += 1;
		}
		std::vector<int> ret;
		for (int i = 0; i < 12; ++i)
		{
			ret.push_back(counters[i]);
		}
		return ret;
	}; //lambda function end

	//Initialization to run threads with above lambda function
	int stripeSize = -1;
	unsigned int NUM_THREADS = inFiles.size();
	std::cout << "Issuing Stripesize-calculating threads." << std::endl;
	int counters[12]={};

	std::vector<std::vector<int>> results;
	float lowEdge = 0.3f;
	float highEdge = 7.3f;
	int value[12] = {2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1};
	int index = 0;
	std::vector<std::future<std::vector<int>>> futureResults(inFiles.size());

	//Starting threads
	for (size_t tm = 0; tm < 12; ++tm)
		counters[tm]=0;
	for (size_t i = 0; i < NUM_THREADS; ++i)
	{
		futureResults[i] = std::async(std::launch::async, lambda, i, 512, lowEdge, highEdge);
	}

	//Getting results
	while (results.size() < inFiles.size())
	{
		for (size_t i = 0; i < NUM_THREADS; ++i)
		{
			std::vector<int> res = futureResults.at(i).get();
			results.push_back(res);
		}
	}
	for (unsigned int i=0; i < results.size();++i)
	{
		for (unsigned int j = 0; j < results.at(i).size(); ++j)
		{
			counters[j] += results.at(i).at(j);
		}
	}
	int ff=0;
	for (unsigned int i = 1; i < 12; ++i)
	{
		if (ff < counters[i])
		{
			ff = counters[i];
			index = i;
		}
	}
	//Print the results for the different stripesizes
	stripeSize = value[index];
	if (stripeSize == 2048)
	{
		std::cout << "Not one good block found. Need to abort." << std::endl;
		return -1;
	}
	while (true)
	{
		//Output and ask the user
		std::string answer;
		std::cout << "\nIt's probably:\t" << stripeSize << "KB"<< std::endl;
		std::cout << "It's just with simple heuristics. Would you like to continue with this result?" << std::endl;
		std::cout << "Answer with 'y' for 'yes', 'n' for 'no' or 'h' for 'hint' [y/n/h] ";
		std::getline(std::cin, answer);
		if (answer == "y")
		{
			stripeSize *= 1024;
			break;
		}
		else if (answer == "n")
		{
			std::cout << "With which Stripesize do you wish to continue? [1-1024] (in kB) ";
			std::cin >> stripeSize;
			stripeSize *= 1024;
			break;
		}
		else if (answer == "h")
		{
			int x = 0;
			std::cout << "\nFound possible blocksizes with following values (Higher value means supposable blocksize):\n";
			std::cout << ">1024:\t" << counters[x] << std::endl;
			++x;
			std::cout << "1024:\t" << counters[x] << std::endl;
			++x;
			std::cout << " 512:\t" << counters[x] << std::endl;
			++x;
			std::cout << " 256:\t" << counters[x] << std::endl;
			++x;
			std::cout << " 128:\t" << counters[x] << std::endl;
			++x;
			std::cout << "  64:\t" << counters[x] << std::endl;
			++x;
			std::cout << "  32:\t" << counters[x] << std::endl;
			++x;
			std::cout << "  16:\t" << counters[x] << std::endl;
			++x;
			std::cout << "   8:\t" << counters[x] << std::endl;
			++x;
			std::cout << "   4:\t" << counters[x] << std::endl;
			++x;
			std::cout << "   2:\t" << counters[x] << std::endl;
			++x;
			std::cout << "   1:\t" << counters[x] << std::endl;
			std::cerr << "Use Stripesize with high values. " << std::endl;
			std::cerr << "If automatically estimated Stripesize doesn't work correct. You could try the blocksize with the next lower value!" << std::endl;
			return -1;
		}
	}

	//If accepted, set block size to stripesize
	mStripeSize = stripeSize;
	return stripeSize;
}
//#endif

/* Function to estimate the stripemap.
 */
std::vector<std::vector<mapEntry>> FileHandler::estimateStripeMap(bool isRaid5)
{
	//Lambda function, which is done by threads
	auto lambda = [&] (size_t id) -> std::vector<Block>
	{
		//Initialization
		std::vector<Block> blocks;
		FileReader *me = inFiles.at(id);
		const size_t CHECKS = 400;
		me->reset();
		const int checkSize=1;
		bool checkGapSize = false;
		size_t startBlock=0,endBlock=0;
		bool zeroStart=false, zeroEnd=false;
		size_t blockSize = mStripeSize;
		size_t gap = mStripeSize - (checkSize * me->getBlockSize());
		gap -= (checkSize * me->getBlockSize());
		Block pushMe;
		size_t outputCounter = 0;
		while (true)
		{
			//Output
			if (outputCounter == 0)
			{
				std::string wr = "Thread ";
				wr += std::to_string(id);
				wr += " working \t";
				wr += me->getRelativePos();
				printf("\r%s\t%6lu",wr.c_str(), blocks.size());
				fflush(stdout);
			}
			outputCounter = (outputCounter + 1) % 25000;
			if (blocks.size() > CHECKS)
				break;

			//Error
			if (me->findFirstNonemptyBlock() == -1)
			{
				break;
			}

			//Get adress and check the gap
			size_t adress = me->getCurrentAdress();
			if (checkGapSize == true)
			{
				size_t tmp = adress;
				if (tmp >= endBlock-checkSize*me->getBlockSize())
				{
					pushMe.zeroFirst = zeroStart;
					pushMe.zeroLast = zeroEnd;
					pushMe.startAdress = startBlock/blockSize;
					pushMe.endAdress = endBlock/blockSize;
					blocks.push_back(pushMe);
					startBlock = adress;
				}
				if (startBlock % blockSize != 0)
				{
					startBlock -= me->getBlockSize();
					zeroStart=true;
				}
				else
				{
					zeroStart=false;
				}
			}
			else
			{
				startBlock = adress;
				if (startBlock % blockSize != 0)
				{
					startBlock -= me->getBlockSize();
					zeroStart=true;
				}
				else
				{
					zeroStart=false;
				}
			}

			//Calculate entropy and check if it's relevant
			pushMe.entropies.clear();
			pushMe.entropies.push_back(me->calcEntropyOfCurrentBlock());
			while (me->newBlock()==true)
			{
				//Output
				if (outputCounter == 0)
				{
					std::string wr = "Thread ";
					wr += std::to_string(id);
					wr += " working \t";
					wr += me->getRelativePos();
					printf("\r%s\t%6lu",wr.c_str(), blocks.size());
					fflush(stdout);
				}
				outputCounter = (outputCounter + 1) % 25000;
				if (blocks.size() > CHECKS)
					break;
				float f = me->calcEntropyOfCurrentBlock();
				if (f == 0.0f)
					break;
				pushMe.entropies.push_back(f);
			}

			endBlock = me->getCurrentAdress();
			if (endBlock % blockSize != 0)
			{
				endBlock += me->getBlockSize();
				zeroEnd=true;
			}
			else
			{
				zeroEnd=false;
			}
			if (endBlock - startBlock < checkSize*me->getBlockSize())
			{
				checkGapSize = false;
			}
			else
			{
				checkGapSize = true;
			}
		}
		me->reset();
		return blocks;
	}; //lambda function end

	//Initialization to start threads
	unsigned int NUM_THREADS = inFiles.size();
	std::vector<std::pair<size_t, float>> entropies;
	std::vector<std::future<std::vector<Block>>> futureResults(inFiles.size());
	std::vector<std::vector<Block>> results(inFiles.size());
	for (size_t i = 0; i < NUM_THREADS; ++i)
	{
		inFiles.at(i)->setBlockSize(mStripeSize);
		futureResults[i] = std::async(std::launch::async, lambda, i);
	}
	for (size_t i = 0; i < NUM_THREADS; ++i)
	{
		results.at(i) = futureResults.at(i).get();
		inFiles.at(i)->setBlockSize(mStripeSize);
	}
	std::cout << std::endl;
	std::cout << "\tEntropie calculation finished" << std::endl;

	std::vector<std::vector<std::pair<bool, std::vector<size_t>>>> map;
	map.resize(results.size());

	//Build the heuristic with the modulo operation and the block begins and ends
	std::vector<size_t> parityDist;
	std::vector<std::vector<std::pair<bool, std::vector<size_t>>>> entropyMat = buildMapHeuristic(results, parityDist);

	//Initializing the resulting stripingMap
	std::vector<std::vector<mapEntry>> stripingMap;
	std::vector<std::pair<size_t, size_t>> referenceCounters;
	std::pair<size_t,size_t> c;
	size_t revC = results.size()-2;
	if (isRaid5==false)
		revC++;
	for (size_t i = 0; i < results.size(); ++i)
	{
		referenceCounters.push_back(std::pair<size_t,size_t>(0,revC));
	}
	for (size_t j = 0; j < results.size(); ++j)
	{
		std::vector<mapEntry> b;
		for (size_t i = 0; i < results.size(); ++i)
		{
			if (isRaid5 == true)
			{
				if (parityDist.at(j) == i)
				{
					b.push_back(mapEntry(true,-2));
				}
				else
				{
					b.push_back(mapEntry(false, -1));
				}
			}
			else
			{
				b.push_back(mapEntry(false, -1));
			}
		}
		stripingMap.push_back(b);
		if (isRaid5==false)
			break;

	}

	std::vector<size_t> posValues;
	if (isRaid5 == true)
	{
		posValues = std::vector<size_t>(results.size(), results.size()-1);
	}
	else
	{
		posValues = std::vector<size_t>(1, results.size());
	}

	//Sort, if it is Raid0 for the later algorithm
	if (isRaid5 == false)
	{
		for (size_t i = 1; i < entropyMat.size();)
		{
			for (size_t j = 0; j < entropyMat.at(i).size(); ++j)
			{
				entropyMat.at(0).push_back(entropyMat.at(i).at(j));
			}
			entropyMat.erase(entropyMat.begin()+i);
		}
	}

	//*
	// * DEBUG OUTPUT - do not delete me
	// *
/*	std::cout << std::endl;
	for (auto in : entropyMat)
	{
		std::cout << "Begin" << std::endl;
		for (auto in2 : in)
		{
			if (in2.first == true)
			{
				for (auto in3 : in2.second)
				{
					std::cout << in3 << ";";
				}
				std::cout << " ";
			}
		}
		std::cout << std::endl;
		std::cout << "End" << std::endl;
		for (auto in2 : in)
		{
			if (in2.first == false)
			{
				for (auto in3 : in2.second)
				{
					std::cout << in3 << ";";
				}
				std::cout << " ";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
*/

	float probability = 1.0f;
	//left = 1, right = 2
	int leftRight = 0;
	//asymm = 1, symm = 2
	int asymm_symm = 0;

	//In this loop we determine the striping map out of the map heuristic, we built earlier
	while (probability > 0.5f)
	{
		size_t entropyMatRunner=0;
		//*
		// * DEBUG OUTPUT - do not delete me
		// *
/*		for (auto &in : stripingMap)
		{
			std::cout << std::setw(10) << " ";
			for (auto &it : in)
			{
				if (it.isParity)
				{
					std::cout << std::setw(4) << "P";
				}
				else
				{
					std::cout << std::setw(4) << std::to_string(it.dataBlock);
				}
			}
			std::cout << std::endl;
		}
		std::cout << std::endl << std::endl;
		std::cout << std::to_string(probability) << std::endl;
*/		//Debug output -end

		size_t lineC=0;
		for (auto &in : entropyMat)
		{
			//We have to rerun each vec at least all images times
			//Possible-vec is size of all images, each entry representing one image
			//pair.first for begin-block, pair.second for end-block
			std::vector<std::pair<size_t, size_t>> possible(results.size(), std::pair<size_t,size_t>(0,0));
			std::pair<size_t, size_t> occCounter(0,0);
			for (auto &moduloEntry : in)
			{
				if (moduloEntry.second.size()>0)
				{
					if (moduloEntry.first == true)
					{
						++occCounter.first;
					}
					else
					{
						++occCounter.second;
					}
				}
				for (auto &modVecEntry : moduloEntry.second)
				{
					if (moduloEntry.first == true)
					{
						++possible.at(modVecEntry).first;
					}
					else
					{
						++possible.at(modVecEntry).second;
					}
				}
			}

			//Check if the probability is high enough to determine the entry
			for (size_t entry = 0; entry < results.size(); ++entry)
			{
				short begin=-1;
				if (possible.at(entry).first == 0 && possible.at(entry).second == 0)
					continue;
				if (occCounter.second > occCounter.first)
				{
					if ((float)possible.at(entry).second/(float)occCounter.second >= probability)
					{
						begin = 0;
					}
				}
				else if (begin == -1 && occCounter.first>0)
				{
					if ((float)possible.at(entry).first/(float)occCounter.first >= probability)
					{
						begin = 1;
					}
				}
				if (begin != -1)
				{
					//We found something
					possible.at(entry).first = 0;
					possible.at(entry).second = 0;
					stripingMap.at(entropyMatRunner).at(entry).isParity = false;
					if (begin == 1)
					{
						stripingMap.at(entropyMatRunner).at(entry).dataBlock = referenceCounters.at(entropyMatRunner).first++;
					}
					else
					{
						stripingMap.at(entropyMatRunner).at(entry).dataBlock = referenceCounters.at(entropyMatRunner).second--;
					}
					--posValues.at(entropyMatRunner);

					//Evrything fine, kill each entry in line because we found an entry
					for (auto &moduloEntry : in)
					{
						if (posValues.at(entropyMatRunner) == 0)
						{
							moduloEntry.second.clear();
						}
						for (size_t burn = 0; burn < moduloEntry.second.size();)
						{
							if (moduloEntry.second.at(burn) == entry)
							{
								moduloEntry.second.erase(moduloEntry.second.begin()+burn);
							}
							else
							{
								++burn;
							}
						}
					}
					probability = 1.0f;
					break;
				}
			}
			if (posValues.at(entropyMatRunner) == 1)
			{
				for (size_t i = 0; i < stripingMap.at(entropyMatRunner).size(); ++i)
				{
					if (stripingMap.at(entropyMatRunner).at(i).dataBlock == -1)
					{
						stripingMap.at(entropyMatRunner).at(i).dataBlock = referenceCounters.at(entropyMatRunner).first++;
						posValues.at(entropyMatRunner) = 0;
						for (auto &moduloEntry : in)
							moduloEntry.second.clear();
						break;
					}
				}
			}
			++lineC;
			++entropyMatRunner;
		}
		probability -= 0.05f;

		//Another approach, only for Raid5; Depends on left-right and symm-asymm builds
		if (isRaid5 == true)
		{
			int curNum = -1;
			for (size_t i = 1; i < stripingMap.size(); ++i)
			{
				for (size_t j = 0; j < stripingMap.at(i).size(); ++j)
				{
					if (stripingMap.at(i).at(j).isParity == true)
					{
						//Get upper datablocknum
						int tmp = stripingMap.at(i-1).at(j).dataBlock;
						if (tmp != -1)
						{
							if (curNum == -1)
							{
								curNum = tmp;
							}
							else //we found a block
							{
								if (tmp > curNum)
								{
									asymm_symm = 1;
									leftRight = 2;
								}
								else if (tmp < curNum)
								{
									asymm_symm = 1;
									leftRight = 1;
								}
								else
								{
									asymm_symm = 2;
									if (tmp == 0)
									{
										leftRight = 2;
									}
									else if (tmp == (int)(inFiles.size()-1))
									{
										leftRight = 1;
									}
									else
									{
										std::cerr << "Error in stripingMap building algorithm" << std::endl;
										return std::vector<std::vector<mapEntry>>();
									}
								}
							}
						}
					}
				}
			}
		}
		if (leftRight != 0 && asymm_symm != 0)
			break;
	}

	//Error, should not happen
	if (leftRight == 0 && asymm_symm == 0 && isRaid5 == true)
	{
		std::cerr << "Couldnt determine stripemap type" << std::endl;
		return std::vector<std::vector<mapEntry>>();
	}

	//Build resulting Stripemap from the found information
	if (isRaid5 == true)
	{
		std::vector<size_t> order;
		if (leftRight == 2)
		{
			for (size_t i = 0; i < parityDist.size(); ++i)
			{
				order.push_back(parityDist.at(i));
			}
		}
		else
		{
			for (size_t i = 0; i < parityDist.size(); ++i)
			{
				order.push_back(parityDist.at(parityDist.size()-(i+1)));
			}
		}

		//Fill the stripemap
		for (size_t i = 0; i < stripingMap.size(); ++i)
		{
			size_t counter;
			if (asymm_symm == 1)
			{
				counter = 0;
			}
			else
			{
				if (leftRight == 1)
				{
					counter = i;
				}
				else
				{
					counter = inFiles.size()-(i-1);
				}
			}
			for (size_t j = 0; j < stripingMap.at(i).size(); ++j)
			{
				if (stripingMap.at(i).at(order.at(j)).isParity == false)
				{
					stripingMap.at(i).at(order.at(j)).dataBlock = counter;
					counter = (counter+1)%(inFiles.size()-1);
				}
			}
		}
	}
	//Else, ask user
	userStripeMapInteraction(stripingMap);
	return stripingMap;
}

/* Function which determines the important parts of the file, where we can build our heuristic for the stripemap.
 * The Outer vector is for modulo distribution, the inner vector of pairs for found zero blocks,
 * last vector is for all found zero blocks with bool to determine if it's a block end or a start.
 */
std::vector<std::vector<std::pair<bool, std::vector<size_t>>>>
FileHandler::buildMapHeuristic(std::vector<std::vector<Block>> &input, std::vector<size_t> &parityDist)
{
	//Initialization
	std::vector<std::vector<std::pair<bool, std::vector<size_t>>>> ret;
	ret.resize(inFiles.size());

	size_t maxCalls = input.at(0).size();
	std::vector<std::vector<Block>::iterator> iterators;
	for (auto &in : input)
	{
		iterators.push_back(in.begin());
		maxCalls = std::min(in.size(), maxCalls);
	}
	size_t curMin = -1;
	bool interestingStart=false;
	bool interestingEnd=false;
	std::vector<size_t> pushMe;

	std::vector<std::vector<size_t>> parityDistribution;
	for (size_t i = 0; i < inFiles.size(); ++i)
	{
		std::vector<size_t> tmp;
		for (size_t j = 0; j < inFiles.size(); ++j)
		{
			tmp.push_back(0);
		}
		parityDistribution.push_back(tmp);
	}
	while (maxCalls != 0)
	{
		maxCalls--;
		curMin = -1;
		for (auto &in : iterators)
		{
			curMin = std::min(curMin, (*in).startAdress);
		}
		interestingStart = true;
		pushMe.clear();
		size_t num=0;
		for (auto &in : iterators)
		{
			if ((*in).startAdress == curMin)
			{}
			else if ((*in).startAdress == curMin+1)
			{
				pushMe.push_back(num);
			}
			else
			{
				interestingStart=false;
			}
			++num;
		}
		if (interestingStart == true && (!(pushMe.empty())))
		{
			std::pair<bool, std::vector<size_t>> entry;
			entry = std::make_pair(true, pushMe);
			size_t modOp = (curMin) % inFiles.size();
			ret.at(modOp).push_back(entry);
		}

		//If an interesting start was found
		if (interestingStart == true)
		{
			size_t sAdress = curMin;
			curMin = -1;
			for (auto &in : iterators)
				curMin = std::min(curMin, (*in).entropies.size());
			for (size_t i = 1; i < curMin; ++i)
			{
				size_t pos=0;
				size_t par=0;
				float highest=0.0f;
				size_t en = (sAdress+i)%inFiles.size();
				for (auto &in : iterators)
				{
					if ((*in).startAdress == sAdress)
					{
						if ((*in).entropies.at(i) > highest)
						{
							highest = (*in).entropies.at(i);
							par = pos;
						}
					}
					else
					{
						if ((*in).entropies.at(i-1) > highest)
						{
							highest = (*in).entropies.at(i-1);
							par = pos;
						}
					}
					++pos;
				}
				parityDistribution.at(en).at(par) = parityDistribution.at(en).at(par)+1;
			}
		}

		curMin = -1;
		for (auto &in : iterators)
		{
			curMin = std::min(curMin, (*in).endAdress);
		}
		interestingEnd = true;
		pushMe.clear();
		num=0;
		for (auto &in : iterators)
		{
			if ((*in).endAdress == curMin)
			{
				pushMe.push_back(num);
				++in;
			}
			else if ((*in).endAdress == curMin+1)
			{
				++in;
			}
			else
			{
				interestingEnd=false;
			}
			++num;
		}

		//If an interesting end was found
		if (interestingEnd == true && pushMe.size()<inFiles.size())
		{
			std::pair<bool, std::vector<size_t>> entry;
			entry = std::make_pair(false, pushMe);
			size_t modOp = (curMin) % inFiles.size();
			ret.at(modOp).push_back(entry);
		}
	}

	//Build the hashMap
	std::map<std::vector<size_t>, size_t> hashMap;
	bool schlonz = true;
	for (size_t i = 0; i < ret.size();)
	{
		hashMap.clear();
		for (size_t j = 0; j < ret.at(i).size(); ++j)
		{
			if (ret.at(i).at(j).first == schlonz)
			{
				if (hashMap.find(ret.at(i).at(j).second) == hashMap.end())
				{
					hashMap.insert(std::pair<std::vector<size_t>, size_t>(ret.at(i).at(j).second, 0));
				}
				hashMap.at(ret.at(i).at(j).second) += 1;
			}
		}
		if (hashMap.size() == 1 && hashMap.begin()->first.size() > 1)
		{
			for (size_t j = 0; j < ret.at(i).size();)
			{
				if (ret.at(i).at(j).first == schlonz)
				{
					ret.at(i).erase(ret.at(i).begin()+j);
					continue;
				}
				++j;
			}
		}
		//Kill also all unnecessary entries
		for (std::map<std::vector<size_t>, size_t>::iterator j = hashMap.begin(); j != hashMap.end();)
		{
			bool killMe = false;
			for (std::map<std::vector<size_t>, size_t>::iterator x = j; x != hashMap.end();)
			{
				if (x == j)
				{
					++x;
					continue;
				}
				if (j->first.size() == x->first.size())
				{
					if (j->second >= x->second)
					{
						if (j->second == x->second)
							killMe = true;
						for (size_t kill = 0; kill < ret.at(i).size();)
						{
							if (ret.at(i).at(kill).first == schlonz && ret.at(i).at(kill).second == x->first)
							{
								ret.at(i).erase(ret.at(i).begin()+kill);
								continue;
							}
							++kill;
						}
						x = hashMap.erase(x);
						continue;
					}
					else
					{
						killMe = true;
					}
				}
				++x;
			}
			if (killMe == true)
			{
				for (size_t kill = 0; kill < ret.at(i).size();)
				{
					if (ret.at(i).at(kill).first == schlonz && ret.at(i).at(kill).second == j->first)
					{
						ret.at(i).erase(ret.at(i).begin()+kill);
						continue;
					}
					++kill;
				}
				j = hashMap.erase(j);
				continue;
			}
			++j;
		}
		if (schlonz == true)
		{
			schlonz = false;
		}
		else
		{
			schlonz = true;
			++i;
		}
	}

	//Parity distribution is estimated
	for (size_t i = 0; i < inFiles.size(); ++i)
	{
		size_t bestFit = 0;
		size_t highestScore=0;
		for (size_t j = 0; j < inFiles.size(); ++j)
		{
			//std::cout << parityDistribution.at(i).at(j) << "\t";
			if (parityDistribution.at(i).at(j) > highestScore)
			{
				highestScore = parityDistribution.at(i).at(j);
				bestFit = j;
			}
		}
		parityDist.push_back(bestFit);
		//std::cout << std::endl;
	}
	return ret;
}

/* Function to manually define the stripemap
*/
void FileHandler::userStripeMapInteraction(std::vector<std::vector<mapEntry>> &stripingMap)
{
	bool finished=false;
	std::string answer;
	while (finished == false)
	{
		std::cout << "Found following stripemap: " << std::endl << std::endl;
		for (auto &in : stripingMap)
		{
			std::cout << std::setw(10) << " ";
			for (auto &it : in)
			{
				if (it.isParity)
				{
					std::cout << std::setw(4) << "P";
				}
				else
				{
					std::cout << std::setw(4) << std::to_string(it.dataBlock);
				}
			}
			std::cout << std::endl;
		}
		std::cout << std::endl << std::endl;
		std::cout << "Everything ok? [y/n]" << std::endl;
		std::getline(std::cin,answer);
		if (answer == "y" || answer == "yes" || answer == "Y")
		{
			finished=true;
		}
		else
		{
			std::cout << "Which line to change?" << std::endl;
			std::getline(std::cin,answer);
			size_t line = std::atoll(answer.c_str())-1;
			std::vector<mapEntry> tmp;
			if (line >= stripingMap.size())
			{
				std::cout << "Line not found" << std::endl;
			}
			else
			{
				size_t pos=0;
				std::cout << "Insert line like '0 1 P'(RAID5) or '0 1 2'(RAID0)" << std::endl;
				std::getline(std::cin,answer);
				answer += " ";
				while (pos != std::string::npos)
				{
					pos = answer.find_first_of(' ',pos+1);
					if (pos == std::string::npos)
						break;
					std::string ent = answer.substr(pos-1, 1);
					if (ent == "P")
					{
						tmp.push_back(mapEntry(true,inFiles.size()-1));
					}
					else
					{
						size_t t = std::atoll(ent.c_str());
						if (t >= stripingMap.at(0).size())
						{
							std::cout << "Invalid line" << std::endl;
						}
						tmp.push_back(mapEntry(false, t));
					}
				}
				if (tmp.size() != stripingMap.at(0).size())
				{
					std::cout << "Invalid line" << std::endl;
				}
				else
				{
					stripingMap.at(line) = tmp;
				}
			}
		}
	}
}
