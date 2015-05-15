# rfrb
Mirror of the "Raid faster - recover better" (rfrb) tool by Sabine Seufert and Christian Zoubek, is a recovery tool for different RAID levels (RAID 0, RAID 1, RAID 5) to automatically estimate parameters used by the raidcontroller like the raidsystem, stripesize and the corresponding stripemap.

Below is the original description from the original webpage "Forensic RAID Recovery" at https://www1.cs.fau.de/content/forensic-raid-recovery:

RAIDs (Redundant Arrays of Independent Disks) are a good way to prevent data loss in case of hardware defects like a broken harddisc, while at the same time improving I/O performance. However, due to the introduction of another abstraction layer (i.e. the RAID layer) between the harddiscs and the operating system, it becomes harder to reconstruct the filesystem data from the set of disks in case the RAID controller fails, as data is distributed among the disks. A similar case occurs in the field of forensic computing (or IT forensics), where accessing data on previously seized and imaged hard disks is the basis for many investigations. The challenge here is to recover the RAID system from the single disk images, hereby verifying redundancy information or reconstructing failed or missing disks. 

In the course of the lecture 'Forensic Hacks' at Friedrich-Alexander-University by Dr.-Ing. Andreas Dewald, we - Sabine Seufert and Christian Zoubek -  implemented a recovery tool for different RAID levels (RAID 0, RAID 1, RAID 5). Hereby, the goal was to automatically estimate parameters used by the raidcontroller like the raidsystem, stripesize and the corresponding stripemap. 

"Raid faster - recover better" (rfrb v1.0.0) provides such a tool that uses several heuristics to determine those parameters. We further put a focus on performance to increase read-/write-throughput to ensure that even big images can be recovered in a reasonable time.  
