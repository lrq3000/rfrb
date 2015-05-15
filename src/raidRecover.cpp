#include <raidRecover.h>

/* This constructor should not be used
 */
RaidRecover::RaidRecover()
{
	std::cerr << "Wrong Constructor" << std::endl;
}

/* Correct constructor. Initial values are set
 */
RaidRecover::RaidRecover(std::string inPath, std::string outPath)
{
	outP = outPath;
	inP = inPath;
	handle = new FileHandler(inPath, outPath);
	system = new RaidSystem(handle);
}

/* Correct constructor. Initial values are set
 */
RaidRecover::RaidRecover(std::vector<std::string> paths, std::string outPath)
{
	outP = outPath;
	handle = new FileHandler(outPath);
	for (size_t i = 0; i < paths.size(); ++i)
	{
		handle->addImage(paths.at(i));
	}
	system = new RaidSystem(handle);
}

/* Setter for input parameter of stripesize
 */
void RaidRecover::setStripeSize(int i)
{
	system->setStripeSize(i);
}

/* Setter for input parameter of lost images
 */
void RaidRecover::setLostImages(int i)
{
	system->setLostImages(i);
}

/* Setter for input parameter of RAID-level
 */
void RaidRecover::setRaid(int i)
{
	system->setRaid(i);
}

/* Run method which calls the raidSystem-class
 */
void RaidRecover::run()
{
	//Set name for recovered dataImage
	std::string o = outP+"dataImage.dd";

	//Start with analysing the case und recover lost data (if possible)
	bool found = system->raidCheck(o);
	std::cout << std::endl;

	//Check if everything went fine
	if (found==true)
		std::cout << "Recovery successful!" << std::endl;
	else
		std::cout << "Recovery was aborted!" << std::endl;
}
