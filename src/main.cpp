#include <fstream>
#include <cstdlib>
#include <iostream>

#include <raidRecover.h>

/*
 *
 * "Raid faster - recover better"(v1.0.0) will recover/rebuild a damaged raidsystem
 * from remaining images. It handles Raid-0, Raid-1 and Raid-5.
 * Copyright (C) 2015 crisbi - Christian Zoubek, Sabine Seufert
 * resulting from the lecture ‚Forensic Hacks‘ in 2014 at
 * Friedrich-Alexander-University (FAU) Erlangen held by Dr.-Ing Andreas Dewald
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

int main(int argc, char *argv[])
{
	//Check the Parameters
	std::string inPath;
	std::vector<std::string> paths;
	std::string outPath;
	int raidVersion = 6, stripeSize = -1, lostImages = -1;

	//Too few arguments
	if (argc < 5)
	{
		std::cout << "\"Raid faster - recover better\"(v0.8.0) will recover/rebuild a damaged raidsystem " << std::endl;
		std::cout << "from remaining images. It handles Raid-0, Raid-1 and Raid-5." << std::endl;
		std::cout << "Copyright (C) 2014 crisbi - Christian Zoubek, Sabine Seufert" << std::endl << std::endl;
		std::cerr << "Try: './rfrb [-P PathToImageFolder] [-o PathToOutputFolder] ([-r #RaidVersion] [-s #Stripesize] [-n #NumberOfLostImages])'" << std::endl;
		std::cerr << "or './rfrb [-p PathToImage1] [-p PathToImage2] ... [-o PathToOutputFolder] ([-r #RaidVersion] [-s #Stripesize] [-n #NumberOfLostImages])'" << std::endl;
		return 1;
	}

	bool in = false;
	bool out = false;
	for (int i = 1; i < argc-1; i=i+1)
	{
		std::string input = argv[i];
		if (input.compare("-h") == 0)
		{
			std::cout << "\"Raid faster - recover better\"(v0.8.0) will recover/rebuild a damaged raidsystem " << std::endl;
			std::cout << "from remaining images. It handles Raid-0, Raid-1 and Raid-5." << std::endl;
			std::cout << "Copyright (C) 2014 crisbi - Christian Zoubek, Sabine Seufert" << std::endl << std::endl;
			std::cerr << "Usage 1: './rfrb [-P PathToImageFolder] [-o PathToOutputFolder] ([-r #RaidVersion] [-s #Stripesize] [-n #NumberOfLostImages])'" << std::endl;
			std::cerr << "Usage 2: './rfrb [-p PathToImage1] [-p PathToImage2] ... [-o PathToOutputFolder] ([-r #RaidVersion] [-s #Stripesize] [-n #NumberOfLostImages])'" << std::endl;
			return 1;
		}
		if (input.compare("-P") == 0)
		{
			inPath = argv[i+1];
			in = true;
			std::cout << "Input Folder: " << inPath << std::endl;
		}
		else if (input.compare("-p") == 0)
		{
			paths.push_back(argv[i+1]);
			in = true;
			std::cout << "image: " << argv[i+1] << std::endl;
		}
		else if (input.compare("-o") == 0)
		{
			outPath = argv[i+1];
			out = true;
			std::cout << "Output Folder: " << outPath << std::endl;
		}
		else if (input.compare("-r") == 0)
		{
			raidVersion = atoi(argv[i+1]);
			std::cout << "RaidVersion: " << raidVersion << std::endl;
		}
		else if (input.compare("-s") == 0)
		{
			stripeSize = atoi(argv[i+1]);
			std::cout << "Stripesize: " << stripeSize << std::endl;
		}
		else if (input.compare("-n") == 0)
		{
			lostImages = atoi(argv[i+1]);
			std::cout << "Lost discs: " << lostImages << std::endl;
		}
		else
		{
			continue;
		}
	}

	//Check if input is valid
	if (in == true && out == true && ((inPath.size() > 0 && paths.empty()) || (paths.size()>0 && inPath == "")))
	{
		std::cout << "Valid Input" << std::endl;
	} else
	{
		std::cout << "Not a valid Input. Please try again." << std::endl;
		std::cout << "Please use: './rfrb [-P PathToImageFolder] [-o PathToOutput-Folder]'" << std::endl;
		std::cout << "or: './rfrb [-p PathToImage1] [-p PathToImage2] [-o PathToOutput-Folder]'" << std::endl;
		return 1;
	}

	RaidRecover *raidR;
	if (paths.empty())
	{
		if (!(inPath.at(inPath.size()-1) == '/'))
			inPath+="/";
		if (!(outPath.at(outPath.size()-1) == '/'))
			outPath += "/";
		raidR = new RaidRecover(inPath, outPath);
	}
	else
	{
		raidR = new RaidRecover(paths, outPath);
	}

	//Check if a valid RAID-level was set
	if (raidVersion != 0 && raidVersion != 1 && raidVersion != 5)
	{}
	else
	{
		raidR->setRaid(raidVersion);
	}

	//Stripesize was set
	if (stripeSize >= 0)
	{
		raidR->setStripeSize(stripeSize);
	}

	//Number of lost Images was set
	if (lostImages >= 0)
	{
		raidR->setLostImages(lostImages);
	}

	std::cout << "Interpreted parameters. Now starting to analyse the Raid system." << std::endl;

	//Call the RAID-recovery
	raidR->run();

	return  0;
}

