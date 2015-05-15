#include <raidSystem.h>

/* This constructor should not be used
 */
RaidSystem::RaidSystem()
{
	std::cerr << "Wrong Constructor for Raid system." << std::endl;
	exit(EXIT_FAILURE);
}

/* Correct constructor. Initial values are set
 */
RaidSystem::RaidSystem(FileHandler *fileHandler)
{
	handle = fileHandler;
	raidSystem = Raid_unknown;
	stripeSize = -1;
	lostImages = -1;
	raidSystem = Raid_unknown;
	srand(time(NULL));
}

/* Getter for the RAID type
 */
Raid RaidSystem::getRaid()
{
	return raidSystem;
}

/* Setter for the RAID type
 */
void RaidSystem::setRaid(int i)
{
	if (i == 0)
		raidSystem = Raid0;
	if (i == 1)
		raidSystem = Raid1;
	if (i == 5)
		raidSystem = Raid5_user;
}

/* Getter for the stripesize
 */
size_t RaidSystem::getStripeSize()
{
	return stripeSize;
}

/* Setter for the stripesize
 */
void RaidSystem::setStripeSize(int i)
{
	stripeSize = i*1024;
}

/* Setter for the number of lost images
 */
void RaidSystem::setLostImages(int i)
{
	lostImages = i;
}

/* Dunction which recovers the lost image
 */
bool RaidSystem::recoverLostImage()
{
	//Check cases, which are not permitted
	if (lostImages == 0)
	{
		std::cout << "\tNo lost disc to recover." << std::endl;
		return true;
	}
	else if (lostImages > 1)
	{
		std::cerr << "\tToo many lost discs to recover." << std::endl;
		return false;
	}
	//Start handler for each image
	size_t maxBuf=128*1024;
	std::vector<FileReader *> inFiles = handle->getInFiles();
	for (unsigned int j = 0; j < inFiles.size(); ++j)
	{
		inFiles.at(j)->setBlockSize(maxBuf);
		inFiles.at(j)->reset();
	}
	std::cout << handle->getFileWriter()->getPath() << std::endl;
	//Check, if the right RAID-level is estimated. Should only be RAID5 with ONE missing image.
	if (raidSystem == Raid5_incomplete)
	{
		size_t blocklength = inFiles.at(0)->getCurrentBlockSize();
		double written = 0.0f;
		size_t runs=0;
		char *buf = new char[maxBuf];
		//Loop until all datablocks are written
		while (true)
		{
			for (unsigned int j = 0; j < inFiles.size(); ++j)
			{
				if (inFiles.at(j)->newBlock() == false)
				{
					handle->getFileWriter()->closeFile();
					std::cout << std::endl;
					for (unsigned int t = 0; t < inFiles.size(); ++t)
						inFiles.at(t)->reset();
					return true;
				}
			}
			blocklength = inFiles.at(0)->getCurrentBlockSize();
			//Set buffer to zero
			for (unsigned int i = 0; i < blocklength; ++i)
			{
				buf[i] = 0;
			}
			++runs;
			//Iterate over all images and read a block
			for (size_t j = 0; j < inFiles.size(); ++j)
			{
				FileReader *reader = inFiles.at(j);
				char *read = reader->getBlock();
				//Calculate XOR to get the block of the lost image
				for (unsigned int i = 0; i < blocklength; ++i)
				{
					buf[i] = buf[i]^read[i];
				}
			}
			//Write calculated block to file
			handle->getFileWriter()->writeToFile(buf,blocklength);
			//Make progress-output
			written += blocklength/1024.0f/1024.0f;
			printf("\r%f MB written", written);
			fflush(stdout);
			//Get new blocks and stop if file is fully written
		}
		//Close files
		std::cout << std::endl;
		handle->getFileWriter()->closeFile();
		for (unsigned int j = 0; j < inFiles.size(); ++j)
			inFiles.at(j)->reset();
		return true;
	}
	for (unsigned int j = 0; j < inFiles.size(); ++j)
		inFiles.at(j)->reset();
	return false;
}

/* The main-function where we interpret the different parameters and call
 * the neccessary functions and classes.
 */
bool RaidSystem::raidCheck(std::string path)
{
	bool found = false;
	//No RAID level is chosen from the user
	handle->setBlockSize(CHECKSIZE);
	if (raidSystem == Raid_unknown)
	{
		std::cout << "Starting with Raid check." << std::endl;
		//Calling easyCheck() to estimate RAID level
		found = easyCheck();
		if (found == false)
		{
			//Something went wrong
			std::cout << "No valid Raid system found!" << std::endl;
			return false;
		}
		else
		{
			while (true)
			{
				//RAID level found
				std::string answer;
				std::cout << "Do you want to accept this? [y/n] ";
				std::getline(std::cin, answer);
				if (answer == "n" || answer == "No" || answer == "N")
				{
					std::cout << "Raid system can not be estimated automatically. Must be handed with the program start." << std::endl;
					return false;
				}
				else if (answer == "y" || answer == "Yes" || answer == "Y")
				{
					break;
				}
			}
		}
	}
	else if (raidSystem == Raid5_user)
	{
		//RAID5 was chosen

		//It was not set, if an image is lost
		if (lostImages == -1)
		{
			std::cout << "User input: Raid 5! Checking for lost discs or if it's a Raid 1 system." << std::endl;
			//Check if user was right
			found = easyCheck();
			if (found == false)
			{
				if (raidSystem == Raid1)
				{
					//User was wrong, we think it's RAID1.
					std::cerr << "User input wrong. Estimated Raid 1 system!" << std::endl;
					return false;
				}
				else
				{
					//User was right with RAID5, but an image is lost.
					std::cout << "Discs missing. Will recover disc later!" << std::endl;
					raidSystem = Raid5_incomplete;
				}
				found = true;
			}
			else
			{
				//Complete RAID5 was found. No image must be recovered
				std::cout << "Raid 5 system is complete. No missing discs indicated!" << std::endl;
			}
		}
		else if (lostImages == 0)
		{
			//User says that no image is missing
			raidSystem = Raid5_complete;
			found = true;
		}
		else
		{
			//User says that at least one image is missing
			raidSystem = Raid5_incomplete;
			found = true;
		}
	}
	else if (raidSystem == Raid1)
	{
		//User says it's RAID1
		found = true;
	}
	else if (raidSystem == Raid0)
	{
		//User says it's RAID0

		if (lostImages > 0)
		{
			//User says an image is missing. That can't be recovered. We have to abort.
			std::cerr << "User specifies Raid 0 system with lost discs. Cannot be recovered!" << std::endl;
			return false;
		}
		found = true;
	}

	//Few final checks
	if (found == false)
	{
		//We haven't found one fitting RAID-level.
		std::cerr << "No valid Raid system found! Have to abort!" << std::endl;
		return false;
	}
	if (raidSystem == Raid5_incomplete)
	{
		//RAID5 was found with a missing image. Is recovered here.
		std::cout << "Incomplete Raid 5 system. Will recover disc and mount it internally." << std::endl;
		recoverLostImage();
		handle->addImage(handle->getFileWriter()->getPath());
	}

	//Stripesize estimation
	if (stripeSize == -1)
	{
		//User hasn't specified a stripesize. We has to determine it on our own.
		std::cout << "No Stripesize specified! Trying to calculate Stripesize now." << std::endl;
		found = calculateStripeSize();
	}
	else
	{
		//User specified stripesize
		std::cout << "Using Stripesize specified by user!" << std::endl;
		found = true;
	}

	//Output if stripesize estimation went fine.
	if (found == true)
	{
		std::cout << "Stripesize: " << stripeSize/1024 << "KB" << std::endl;
	}
	else
	{
		std::cerr << "No valid Stripesize found. User should estimate it on its own, or disc could not be recovered." << std::endl;
		return false;
	}
	//For further operations, set blocksize to stripesize.
	handle->setBlockSize(stripeSize);

	//Stripemap estimation
	std::cout << "Trying to estimate the Stripemap." << std::endl;
	if (raidSystem == Raid5_user || raidSystem == Raid5_incomplete || raidSystem == Raid5_complete)
	{
		//RAID5 needs a stripemap and has parity information. That's the call with 'true'
		stripeMap = handle->estimateStripeMap(true);
	}
	else if (raidSystem == Raid0)
	{
		//RAID0 needs a stripemap and has no parity information. That's the call with 'false'
		stripeMap = handle->estimateStripeMap(false);
	}
	else
	{
		//RAID1 needs no stripemap. We're done here.
		std::cout << "No need to estimate Stripemap because of Raid 1 system." << std::endl;
		found = true;
	}

	//Final checks if stripemap estimation went fine
	if (stripeMap.size() == 0 && !(raidSystem == Raid1))
	{
		std::cerr << "No valid Stripemap found. Have to abort Recovery." << std::endl;
		return false;
	}
	else if (raidSystem == Raid1)
	{}
	else
	{
		//If everything went fine, the whole imagefile is created
		std::cout << "Rebuild the imagefile out of the Raid discs." << std::endl;
		found = buildDataImage(path);
	}

	//If nothing bad has happened, the results are printed.
	if (found == true)
	{
		printAllInfos();
	}
	std::vector<FileReader *> inFiles = handle->getInFiles();
	for (unsigned int j = 0; j < inFiles.size(); ++j)
	{
		inFiles.at(j)->closeFile();
	}
	return found;
}

/* Helpfunction to print information
 */
void RaidSystem::printAllInfos()
{
	std::cout << std::endl;
	std::cout << "All information compact: " << std::endl << std::endl;
	std::vector<FileReader *> inFiles = handle->getInFiles();
	std::cout << "Found Raid system: ";
	//Print RAID-level
	if (raidSystem == Raid_unknown)
	{
		std::cout << "No valid found";
	}
	else if (raidSystem == Raid0)
	{
		std::cout << "Raid 0";
	}
	else if (raidSystem == Raid1)
	{
		std::cout << "Raid 1";
	}
	else if (raidSystem == Raid5_incomplete)
	{
		std::cout << "Raid 5 - with one disc missing";
	}
	else if (raidSystem == Raid5_complete)
	{
		std::cout << "Raid 5";
	}
	std::cout << std::endl;
	//Print other information
	std::cout << "Stripe/Chunk-size:\t" << stripeSize/1024 << "KB" << std::endl;
	std::cout << "Device order:" << std::endl;
	for (unsigned int j = 0; j < inFiles.size(); ++j)
	{
		std::cout << std::to_string(j+1) << ") " << inFiles.at(j)->getPath() << std::endl;
	}

	//Print device order
	std::cout << std::setw(10)<< "Device:   ";
	for (unsigned int j = 0; j < inFiles.size(); ++j)
	{
		std::cout << std::setw(4) << std::to_string(j+1);
	}
	std::cout << std::endl;
	std::cout << std::endl;

	//Print stripemap
	for (auto &in : stripeMap)
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
	std::cout << std::endl;
}

/* This function is used to make the easyCheck(). We test here, if 'in' equals zero.
 * If yes, then true is returned.
 */
bool RaidSystem::checkForNull(char *in, size_t size)
{
	for (size_t i = 0; i < size; ++i)
	{
		if (in[i] > 0)
			return false;
	}
	return true;
}

/* This function checks if two blocks are equal. If (buf XOR in) == 0 then true is returned.
 */
bool RaidSystem::checkForEqual(char *buf, char *in, size_t size)
{
	unsigned int count = 0;
	for (size_t i = 0; i < size; ++i)
	{
		if (in[i] != buf[i])
			++count;
	}
	//A few errors are tolerated
	if (count > (size/1000))
		return false;

	return true;
}

/* This function estimates the RAID-level of the given images.
 */
bool RaidSystem::easyCheck()
{
	//Initial buffer allocations
	char buf1[CHECKSIZE];
	char buf5[CHECKSIZE];
	char *checkAgainstMe;
	char *in;
	unsigned int raid1=0, raid5=0, misses=0;

	//Starting fileReader for each image
	std::vector<FileReader *> inFiles = handle->getInFiles();

	//Check if enough images are specified
	if(inFiles.size() < 2)
	{
		std::cout << "\tThere are too few devices for a Raid system." << std::endl;
		return false;
	}
	//Check if images are too broken
	if (handle->findGoodBlock() == false)
	{
		std::cerr << "\tNo valid readable block found! Disc too broken!" << std::endl;
		return false;
	}

	for (int j = 0; j < CHECKSIZE; ++j)
	{
		buf1[j] = 0;
	}

	std::cout << "\tTesting 1.5 million blocks to check if a valid Raid-version can be estimated." << std::endl;

	//Iterate over many blocks and trying to estimate the RAID-level
	for (size_t i = 0; i < inFiles.size(); ++i)
	{
		inFiles.at(i)->setBlockSize(CHECKSIZE);
	}
	std::cout << "| ";
	for (size_t count=0; count < 1500000; ++count)
	{
		if ((count%5000) == 0)
			std::cout << "* ";
		//Get a block from the first image
		checkAgainstMe = inFiles.at(0)->getBlock();
		int raid1_miss = 0;
		int raid1_hit = 0;
		for (int j = 0; j < CHECKSIZE; ++j)
		{
			buf5[j] = checkAgainstMe[j];
		}
		//Get blocks from the other images
		for (unsigned int j = 1; j < inFiles.size(); ++j)
		{
			in = inFiles.at(j)->getBlock();
			//Calculate XOR
			for (int x = 0; x < CHECKSIZE; ++x)
			{
				buf1[x] = checkAgainstMe[x]^in[x];
				buf5[x] = buf5[x]^in[x];
			}
			//If XOR is in buf1 is not zero, then we have no RAID1
			if (checkForNull(buf1, CHECKSIZE)==false)
			{
				++raid1_miss;
				for (int x = 0; x < CHECKSIZE; ++x)
				{
					buf1[x] = 0;
				}
			}
			else
			{
				++raid1_hit;
			}
		}
		if (raid1_miss == 0)
		{
			//Blocks are equal, we have a RAID1 hit
			++raid1;
		}
		//else if (checkForEqual(buf5,checkAgainstMe+startAdress,CHECKSIZE) == true)
		else if (checkForNull(buf5, CHECKSIZE)==true)
		{
			//block0 = block1 XOR block2 XOR ...
			//There is Parity information, that's why we assume RAID5
			++raid5;
		}
		else
		{
			++misses;
		}
		if (handle->findGoodBlock() == false)
		{
			std::cout << "\tCouldnt find enough testable blocks. Trying to estimate with heuristics." << std::endl;
			break;
		}
	}
	std::cout << std::endl;

	//Final outputs
	std::cout << "\tMirrored blocks:\t\t: " << std::setw(6) << raid1 << "\t(Raid1)"<< std::endl;
	std::cout << "\tBlocks with parity info\t\t: " << std::setw(6) << raid5 << "\t(Raid5)"<< std::endl;
	std::cout << "\tBlocks without relevant info\t: "       << std::setw(6) << misses << std::endl;
	handle->reset();

	//Trying to estimate the RAID-level from the results above
	if (raid1 > (misses+raid5)*1.5)
	{
		//Enough RAID1 hits to evaluate this to RAID1
		raidSystem = Raid1;
		std::cout << "Found a Raid 1 system." << std::endl;
		return true;
	}
	else if (raid5 > (misses+raid1)*1.5 && inFiles.size() >= 3)
	{
		//Enough RAID5 hits and enough images to evaluate this to RAID5
		raidSystem = Raid5_complete;
		std::cout << "Found a complete Raid 5 system." << std::endl;
		return true;
	}

	//See if you can find two identical partitiontables at the beginning of two images out of all.
	std::vector<char *> tables;
	for (unsigned int i = 0; i < inFiles.size(); ++i)
	{
		char *buf = inFiles.at(i)->getBlock();
		if (checkForNull(buf, 512)==false)
			tables.push_back(buf);
	}
	//If this can be found, it's RAID5 with one missing image (incomplete).
	if (tables.size() == 2)
	{
		if (checkForEqual(tables.at(0),tables.at(1),512)==true)
		{
			lostImages = 1;
			raidSystem = Raid5_incomplete;
			std::cout << "Found a mirrored partition table. Has to be an incomplete Raid 5 system." << std::endl;
			handle->reset();
			return true;
		}
	}
	//Otherwise it can still be RAID5-incomplete or RAID0
	if (raid1 < (100*misses/(inFiles.size()+1)) && (raid1+raid5) > 0.007*(float)misses)
	{
		//At this point it could only be RAID0 or RAID5.
		//There are enough hits for identical blocks and parity blocks => RAID5
		lostImages = 1;
		raidSystem = Raid5_incomplete;
		std::cout << "Some mirrored blocks and some parity blocks found. It's probably an incomplete Raid 5 system." << std::endl;
		handle->reset();
		return true;
	}
	else if (raid1 >= (100*misses/(inFiles.size()+1)))
	{
		//Still too much identical blocks found. This has to be RAID1
		raidSystem = Raid1;
		std::cout << "Above average mirrored blocks found. Has to be a Raid 1 system." << std::endl;
		return true;
	}
	else
	{
		//Now it can only be RAID0
		raidSystem = Raid0;
		std::cout << "Too few mirrored blocks or parity infos found. Has to be a Raid 0 system." << std::endl;
		return true;
	}
	return false;
}

/* This function calls the stripesize calculation in the fileHandler.
 */
bool RaidSystem::calculateStripeSize()
{
	int ret = handle->estimateStripeSize();
	if (ret == -1)
		return false;
	this->stripeSize = ret;
	return true;
}

/* In this function the whole data image is restored from all given images.
 */
bool RaidSystem::buildDataImage(std::string path)
{
	//Start readers for every image and one writer for the lost image which has to be recovered
	std::vector<FileReader *> inFiles = handle->getInFiles();
	char *in;
	FileWriter *writeMe = handle->getFileWriter();
	writeMe->setPath(path);
	handle->reset();

	for (unsigned int j = 0; j < inFiles.size(); ++j)
	{
		inFiles.at(j)->setBlockSize(stripeSize);
		inFiles.at(j)->reset();
	}

	//Parity buffer for check parity plausibility
	char *parity = new char[stripeSize]{};
	size_t block = 0;
	size_t written = 0;
	size_t finished = 0;

	//Will break when dataImage is built
	while (true)
	{
		//Iterate stripeMap-rows
		for (size_t i = 0; i < stripeMap.size(); ++i)
		{
			for (auto &readers : inFiles)
			{
				if (readers->newBlock()==false)
				{
					++finished;
				}
			}
			//Break-case when everything is done
			if (finished == inFiles.size())
			{
				printf("\r%12zu MB written\t%12fMB/s", written/1024/1024, writeMe->getTransferRate());
				fflush(stdout);
				writeMe->closeFile();
				return true;
			}
			int runner=0;
			//Re-iterate over stripemap-colums, maximum of #colums
			for (size_t worstCase = 0; worstCase < stripeMap.at(i).size(); ++worstCase)
			{
				//Iterate over stripeMap-columns
				for (size_t j = 0; j < stripeMap.at(i).size(); ++j)
				{
					//Right position at stripeMap -> write it to outfile
					//WorstCase loop forces that we don't miss anything
					if (stripeMap.at(i).at(j).dataBlock == runner)
					{
						in = inFiles.at(j)->getBlock();
						//Calculation parity
						for (size_t k = 0; k < inFiles.at(j)->getCurrentBlockSize(); ++k)
						{
							parity[k] = parity[k] ^ in[k];
						}

						//If not parity, write to outfile
						if (stripeMap.at(i).at(j).isParity == false)
						{
							writeMe->writeToFile(in, inFiles.at(j)->getCurrentBlockSize());
							written += inFiles.at(j)->getCurrentBlockSize();
						}
						else
						{
							if (checkForNull(parity, inFiles.at(0)->getCurrentBlockSize()) == false)
							{
								std::cout << "Parity Error at block: " << block << std::endl;
							}
							for (int k = 0; k < stripeSize; ++k)
							{
								parity[k] = 0;
							}
						}
						++runner;
					}
				}
			}
			//Updated output so we don't have to stare against a blinking cursor
			printf("\r%12f MB written\t%12fMB/s", (double)written/1024.f/1024.f, writeMe->getTransferRate());
			fflush(stdout);

			++block;
		}
	}

	//Shouldn't reach this, if valid Raidsystem & Stripesize & Stripemap
	return false;
}
