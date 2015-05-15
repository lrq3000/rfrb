#ifndef RAIDRECOVER_H
#define RAIDRECOVER_H

#include <iostream>
#include <string>

#include <fileHandler.h>
#include <raidSystem.h>

class RaidRecover
{
	private:
		/* Different member variables
		 */
		std::string outP, inP;
		FileHandler *handle;
		RaidSystem *system;

	public:
		/* Constructors
		 */
		RaidRecover();
		RaidRecover(std::string inPath, std::string outPath); //this should get a FileHandler....not make a new one
		RaidRecover(std::vector<std::string> paths, std::string outPath); //this should get a FileHandler....not make a new one

		/* Setter for input parameters
		 */
		void setStripeSize(int i);
		void setLostImages(int i);
		void setRaid(int i);

		/* Run method which calls the raidSystem-class
		 */
		void run();

};

#endif
