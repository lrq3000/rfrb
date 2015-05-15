#ifndef RAIDSYSTEM_H
#define RAIDSYSTEM_H

#include <vector>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>

#include <fileHandler.h>
#include <defines.h>

/* These defines should make the code readable.
 */
enum Raid
{
	Raid0,
	Raid1,
	Raid5_user,	//This one is only set by user
	Raid5_incomplete,
	Raid5_complete,
	Raid_unknown
};

class RaidSystem
{
	private:
		FileHandler *handle;

		/* If raidsystem, stripe or lostCount is set, we dont need to test for it
		 * initial values are Raid_unknown, -1, -1
		 */
		Raid raidSystem;

		/* Different member variables
		 */
		int stripeSize;
		std::vector<std::vector<mapEntry>> stripeMap;
		int lostImages;

		/* This function is used to make the easyCheck(). We test here, if 'in' equals null.
		 * If yes, true is returned.
		 */
		bool checkForNull(char *in, size_t size);

		/* This function checks if two blocks are equal. If (buf XOR in) == 0 then true is returned.
		 */
		bool checkForEqual(char *buf, char *in, size_t size);

		/* This function estimates the RAID-level of the given images.
		 */
		bool easyCheck();

		/* This function calls the stripesize calculation in the fileHandler.
		 */
		bool calculateStripeSize();

		/* In this function the whole data image is restored from all given images.
		 */
		bool buildDataImage(std::string path);

	public:
		/* Constructors
		 */
		RaidSystem();
		RaidSystem(FileHandler *fileHandler);

		/* Getter and setter for the RAID-level
		 */
		Raid getRaid();
		void setRaid(int i);

		/* Getter and setter for the stripesize
		 */
		size_t getStripeSize();
		void setStripeSize(int i);

		/* Setter for the number of lost images and the function which recovers it.
		 */
		void setLostImages(int i);
		bool recoverLostImage();

		/* The main-function where we interpret the different parameters and call
		 * the neccessary functions and classes.
		 */
		bool raidCheck(std::string path);

		/* Helpfunction to print information
		 */
		void printAllInfos();

};

#endif
