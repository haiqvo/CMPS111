#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myfile.c"

int main(int argc, char** argv)
{
	char *disk_name;
	int disk_size;
	char *disk_test;
	disk_t disk;
	unsigned char *databuf;
	int i, j;
	
	if(argc != 2)
	{
		printf("Usage: fileSysTest1 <disk_size> (in blocks)\n");
		exit(-1);
	}
	
	disk_name = "SampleFileSys";
	disk_test = (char *)argv[1];
	disk_size = atoi(disk_test);

	createdisk(disk_name, disk_size);
	printf("Disk %s created with size %d\n", disk_name, disk_size);
	disk = opendisk(disk_name);

	init_fsys(disk, disk_size);	

	print_disk(disk, disk_size);

/*	printf("\nChecking makdir\n");
	printf("\tMaking test\n");
	makdir(disk, "test", "/");
	printf("\tMaking alsotest\n");
	makdir(disk, "alsotest", "/");
	print_disk(disk, disk_size);
	exit(0);
	
	//printf("\nCheking file created\n");
	file filebuf = malloc(disk->block_size*2);
	for(i = 0; i < disk->block_size; i++)
	{
		((unsigned char*)filebuf)[i] = 't';
	}
	for(; i < disk->block_size*2; i++)
	{
		((unsigned char*)filebuf)[i] = 'v';
	}

	createfile(disk, "tester", "/", 2, &filebuf);
	//print_disk(disk, disk->size);
	
	//printf("Reading file\n");
	unsigned char* red = (unsigned char*)readfile(disk, "/tester/");
	//printf("\tPrinting file\n");
	for(i = 0; i < disk->block_size*2; i++)
	{
		//printf("%c", red[i]);
	}
	//printf("\n");
	*/
}
