/* fileSysTest.c
 * 
 * Justin Yeo, Hai Vo, Erik Swedberg 
 * 
 * Test program to show off the myfile.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myfile.c"

int main(int argc, char** argv)
{
	char *disk_name;
	int disk_size;
	char *disk_test;
	char* file_name;
	char* path;
	disk_t disk;
	unsigned char *databuf;
	int i, j;
	char ch;
	FILE *fp;

	if(argc != 5)
	{
		printf("Usage: fileSysTest <disk_name> <disk_size> <file_name> <path>\n");
		exit(-1);
	}

	disk_name = (char *)argv[1];
	disk_test = (char *)argv[2];
	file_name = (char *)argv[3];
	path 	  = (char *)argv[4];
	disk_size = atoi(disk_test);
	
	fp = fopen(file_name, "r");
	fseek(fp, 0, SEEK_END);
	int pos = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *bytes = malloc(pos);
	fread(bytes, pos, 1, fp);

  	fclose(fp);

  	//initizalize the file System
  	printf("\n==========================================\n");
	createdisk(disk_name, disk_size);
	printf("Disk %s created with size %d\n", disk_name, disk_size);
	disk = opendisk(disk_name);

	init_fsys(disk, disk_size);	//initizalize the file System

	print_disk(disk, disk_size);

	//checking the super block
	printf("==========================================\n");
	printf("Checking superblock\n");
	databuf = malloc(disk->block_size);
	readblock(disk, 0, databuf);
	for(i = 0; i < 4; i++)
	{
		printf("%d:%d\n",i,((int*)databuf)[i]);
	}

	int size;
	if(pos%disk->block_size != 0){
		size = pos/disk->block_size + 1;
	}else{
		size = pos/disk->block_size;
	}
	printf("size: %d\n", disk->block_size);
	
	//inputing a file into the file system
	printf("\n==========================================\n");
	printf("Checking file created\n");
	file filebuf = malloc(disk->block_size*size);
	for(i=0; i<pos; i++){
		((unsigned char*)filebuf)[i] = bytes[i];
	}

	createfile(disk, file_name, path, size, &filebuf);
	print_disk(disk, disk->size);
	
	//reading back the file
	printf("==========================================\n");
	printf("Reading file\n");
	char * path_way = malloc(strlen(path)+strlen(file_name)+1);
	char * cut_name =  malloc(8);
	strcat(path_way, path);
	if(strlen(file_name)>8){
		strncpy(cut_name, file_name,8);
		strcat(path_way, cut_name);
	}else{
		strcat(path_way, file_name);
	}
	strcat(path_way, "/");
	unsigned char* red = (unsigned char*)readfile(disk, path_way);
	
	//printing it to stdout
	printf("==========================================\n");
	printf("\tPrinting file\n");
	for(i = 0; i < pos; i++)
	{
		//printf("Hello\n" );
		//printf("i: %d", i);
		printf("%c", red[i]);
	}
	printf("\n");
}
