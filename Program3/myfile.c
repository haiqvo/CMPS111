#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include "mydisk.h"

//int writefile(disk_t disk, int block, unsigned char *databuf)
void print_disk(disk_t, int);

struct block_address
{
	int block;
	int offset;
	int address;
};
typedef struct block_address* address;

address make_address(int add, int block_size)
{
	address ret = malloc(sizeof(struct block_address));
	ret->address = add;
	int mod = add%block_size;
	ret->block = add/block_size;
	if(mod) ret->block++;
	ret->offset = mod;
	return ret;
}

int char_num(int n, unsigned char** array)
{
	int count = 0;
	printf("Entering char_num with %d\n", n);
	unsigned char* temp = malloc(sizeof(unsigned char)*256);
	printf("count:%d\n",count++);//0
	int i = 0;
	while(n)
	{
		unsigned int mod = n%10;
		n = n/10;
		mod = mod + 48;
		assert(mod<256);
		temp[i++] = (unsigned char)mod;
	}
	printf("count:%d\n",count++);//1
	*array = malloc(sizeof(unsigned char)*i);
	int j = 0;
	printf("count:%d\n",count++);//2
	for(i--;i >= 0; i--)
	{
		printf("i:%d\n",i);
		printf("j:%d\n",j);
		*array[j++] = temp[i];
	}
	printf("count:%d\n",count++);//3
	free(temp);
	printf("Ending char_num with ");
	for(i = 0; i < j; i++) printf("%c",*array[i]);
	printf("count:%d\n",count++);//4
	printf("\n");
	return j;
}

int write_array(disk_t disk, address add, unsigned char* array)
{
	printf("Entering write_array\nArray: %s\n", array);
	unsigned char* databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, add->block, databuf);
	int i = 0;
	int j = add->offset;
	int limit = disk->block_size;
	int block = add->block;
	while(1)
	{
		databuf[j++] = array[i++];
		if(array[i] == '\0') 
		{
			writeblock(disk, block, databuf);
			return 0;
		}
		if(j == limit)
		{
			writeblock(disk, block++, databuf);
			limit = limit + disk->block_size;
			j = 0;
			if(block > disk->size) return -1;
			readblock(disk, block, databuf);
		}
	}
}

int init_fsys(disk_t disk, int size)
{
	unsigned char* disk_size;
	int len = char_num(size, &disk_size);
	address free_blocks = make_address(len, disk->block_size);
	//int mod = size%disk->block_size;
	int free_size = size;
	int free_block_count = size/disk->block_size + 1;

	address root = make_address(free_blocks->address + free_size, disk->block_size);
	address data = make_address(root->address + 1, disk->block_size);

	//initialize disk as \0
	unsigned char* databuf = malloc(sizeof(unsigned char)*disk->block_size);
	int i;
	for(i = 0; i < disk->block_size; i++)
	{
		databuf[i] = '\0';
	}
	for(i = 0; i < size; i++)
	{
		writeblock(disk, i, databuf);
	}

	free(databuf);//realloc	
	databuf = malloc(sizeof(unsigned char)*free_size);
	

	databuf[0] = 'i';
	printf("disk_size:%p\n", disk_size);
	for(i = 0; i < len; i++)
	{
		printf("i:%d\n",i);
		databuf[i] = disk_size[i];
	}
	free(disk_size);
	databuf[i++] = '\n';
	databuf[i++] = '1';
	databuf[i++] = '\n';
	unsigned char* root_block;
	len = char_num(1 + free_block_count, &root_block);
	int j;
	for(j = 0; j < len; j++)
	{
		databuf[i++] = root_block[j];
	}
	free(root_block);
	databuf[i++] = '\n';
	unsigned char* data_block;
	len = char_num(1 + free_block_count + 1, &data_block);
	for(j = 0; j < len; j++)
	{
		databuf[i++] = data_block[j];
	}
	write_array(disk, make_address(0, disk->block_size), databuf);
	
	for(i = 0; i < free_block_count + 2; i++)//free block map start
	{
		databuf[i] = '1';
	}
	for(j = i; j < free_size; j++)
	{
		databuf[j] = '0';
	}
	databuf[j] = '\0';//free block map end
	printf("databuf: %s\n", databuf);
	int ret = write_array(disk, make_address(1, disk->block_size), databuf);
	if(ret) printf("ret: %d\n", ret);
	return 0;
}
/*int init_fsys(disk_t disk)
{
	int disk_size = disk->size;
	int free_blocks = 1;
	int free_size = disk_size/disk->block_size;
	int root = free_blocks + free_size;
	int data = root+1;
	
	unsigned char *databuf = malloc(disk->block_size);
	
	
	writeblock(disk, root, "\0");//empty root
	
	int i;//bytemap
	databuf[0] = '1';//superblock
	for(i = 1; i < free_size + 1; i++)//freeblocks
	{
		databuf[i] = '1';
	}
	databuf[i++] = '1';//root
	databuf[i++] = '1';//data
	for(; i < disk->block_size; i++)
	{
		databuf[i] = '0';
	}
	writeblock(disk, free_blocks, databuf);
	if(disk->block_size < free_size)
	{
		free(databuf);
		databuf = calloc(disk->block_size, sizeof(char));
		i = 
		while()
		{
		}
	}
}*/

void print_disk(disk_t disk, int disk_size)
{
	unsigned char* databuf = malloc(disk->block_size);

	int i;
	for(i = 0; i < disk_size; i++)
	{
		readblock(disk, i, databuf);

		printf("block: %d\n",i);
		int j;
		for(j = 0; j < disk->block_size; j++)
		{
			printf("%c", databuf[j]);
		}
		printf("\n");
	}
}

int main(int argc, char** argv)
{
	char *disk_name;
	int disk_size;
	disk_t disk;
	unsigned char *databuf;
	int i, j;
	
	if(argc != 3)
	{
		printf("Usage: myfile <disk_name> <disk_size> (in blocks)\n");
		exit(-1);
	}
	
	disk_name = (char *)argv[1];
	disk_size = atoi((char*)argv[2]);
	
	createdisk(disk_name, disk_size);
	printf("Disk %s created with size %d\n", disk_name, disk_size);
	disk = opendisk(disk_name);
	
	init_fsys(disk, disk_size);	

	//databuf = malloc(disk->block_size);

	print_disk(disk, disk_size);
	exit(0);
}
