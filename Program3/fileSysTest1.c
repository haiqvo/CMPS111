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
		printf("Usage: myfile <disk_name> <disk_size> <file_name> <path> (in blocks)\n");
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
	printf("the number is %d\n", pos);
	fseek(fp, 0, SEEK_SET);

	char *bytes = malloc(pos);
	fread(bytes, pos, 1, fp);

  	fclose(fp);
  	printf("the last is: %c \n", bytes[pos-2]);
  	for(i=0; i<pos; i++){
  		printf("%c", bytes[i]);
	}



  	//exit(0);

	createdisk(disk_name, disk_size);
	printf("Disk %s created with size %d\n", disk_name, disk_size);
	disk = opendisk(disk_name);

	init_fsys(disk, disk_size);	

	print_disk(disk, disk_size);

	printf("\nChecking superblock\n");
	databuf = malloc(disk->block_size);
	readblock(disk, 0, databuf);
	for(i = 0; i < 4; i++)
	{
		printf("%d:%d\n",i,((int*)databuf)[i]);
	}

/*	printf("\nChecking makdir\n");
	printf("\tMaking test\n");
	makdir(disk, "test", "/");
	printf("\tMaking alsotest\n");
	makdir(disk, "alsotest", "/");
	print_disk(disk, disk_size);
	exit(0);*/

	int size;
	if(pos%disk->block_size != 0){
		size = pos/disk->block_size + 1;
	}else{
		size = pos/disk->block_size;
	}
	printf("size: %d\n", disk->block_size);
	
	printf("\nCheking file created\n");
	file filebuf = malloc(disk->block_size*size);
	for(i=0; i<pos; i++){
		((unsigned char*)filebuf)[i] = bytes[i];
	}

	for(i=0; i<pos; i++){
		printf("%c", ((unsigned char*)filebuf)[i]);
	}

	createfile(disk, "sample", path, size, &filebuf);
	print_disk(disk, disk->size);

	printf("Reading file\n");
	//char * path_way;
	//strcat(path_way, path);
	//strcat(path_way, file_name);
	//strcat(path_way, "/");
	unsigned char* red = (unsigned char*)readfile(disk, "/sample/");

	printf("\tPrinting file\n");
	for(i = 0; i < pos; i++)
	{
		//printf("Hello\n" );
		//printf("i: %d", i);
		printf("%c", red[i]);
	}
	printf("\n");
}
