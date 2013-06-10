#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myfile.c"



int main(int argc, char** argv)
{
	char *disk_name;
	int disk_size;
	char *file_name;
	char *path;
	char ch;
	FILE *fp;
	disk_t disk;
	unsigned char *databuf;
	int i, j;
	
	if(argc != 3)
	{
		printf("Usage: fileSysTest2 <path> <file_name> \n");
		exit(-1);
	}
	
	disk = opendisk("SampleFileSys");
	readblock(disk, 0, databuf);
	printf("%s", databuf);
	//print_disk(disk, disk_size);
	path = (char *)argv[1];
	file_name = (char *)argv[2];

	fp = fopen(file_name, "r");
	i = 0;
	while( ( ch = fgetc(fp) ) != EOF ){
      ((unsigned char*)databuf)[i] =  ch;
      i++;
  	}

    fclose(fp);

    file filebuf = malloc(i);
    for(j = 0; j < i; j++)
	{
		((unsigned char*)filebuf)[j] = databuf[i];
	}

	

	

	if(createfile(disk, file_name, path, &filebuf, j)==1){
		printf("File Successful Created\n");
	}else{
		printf("File fail to create please check if it is the correct file name and the path exist\n");
	}





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
