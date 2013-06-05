#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
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

void print_address(address add)
{
	printf("block: %d\n", add->block);
	printf("offset: %d\n", add->offset);
	printf("address: %d\n", add->address);
}
address make_address(int add, int block_size)
{
	address ret = malloc(sizeof(struct block_address));
	ret->address = add;
	int mod = add%block_size;
	ret->block = add/block_size - 1;
	if(mod) ret->block++;
	ret->offset = add - ret->block*block_size - 1;
	print_address(ret);//
	return ret;
}

struct super_block
{
	int size;
	address free_map;
	address root;
	address data;
};
typedef struct super_block* super;

/*void write_super(disk_t disk, int size)
{
	
}*/

super read_super(disk_t disk)
{
	super ret = malloc(sizeof(struct super_block));
	int* intbuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, 0, (unsigned char*)intbuf);
	ret->size = intbuf[0];
	ret->free_map = make_address(intbuf[1], disk->block_size);
	ret->root = make_address(intbuf[2], disk->block_size);
	ret->data = make_address(intbuf[3], disk->block_size);
	return ret;
}

struct file_entry
{
	char c0;
	char c1;
	char c2;
	char c3;
	char c4;
	char c5;
	char c6;
	char c7;
	bool directory;
	int inode;
};
typedef struct file_entry* file;

struct directory_block
{
	file* files;
};
typedef struct directory_block* directory;

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

void set_free_blocks(disk_t disk, address free_map, int block, int block_count, char set)
{
	address temp = make_address(free_map->address + block - 1, disk->block_size);
	print_address(temp);
	//int bufsize = block_count;
	//if(bufsize < disk->block_size) bufsize = block_size;
	unsigned char* databuf = malloc(sizeof(unsigned char)*block_count);
	int i;
	for(i = 0; i < block_count; i++)
	{
		databuf[i] = set;
	}
	write_array(disk, temp, databuf);
}

int init_fsys(disk_t disk, int size)
{
	address free_blocks = make_address(disk->block_size + 1, disk->block_size);
	//int mod = size%disk->block_size;
	int free_size = size;
	int free_block_count = size/disk->block_size + 1;

	address root = make_address(free_blocks->address + disk->block_size, disk->block_size);
	address data = make_address(root->address + disk->block_size, disk->block_size);

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


	//superblock
	readblock(disk, 0, databuf);
	((int*)databuf)[0] = size;
	((int*)databuf)[1] = free_blocks->address;
	((int*)databuf)[2] = root->address;
	((int*)databuf)[3] = data->address;
	writeblock(disk, 0, databuf);	
	//write_array(disk, make_address(0, disk->block_size), databuf);//superblock written

	free(databuf);//realloc	
	databuf = malloc(sizeof(unsigned char)*free_size);
	
	for(i = 0; i < free_block_count + 2; i++)//free block map start
	{
		databuf[i] = '1';
	}
	int j;
	for(j = i; j < free_size; j++)
	{
		databuf[j] = '0';
	}
	databuf[j] = '\0';//free block map end
	printf("databuf: %s\n", databuf);
	int ret = write_array(disk, free_blocks, databuf);
	if(ret) printf("ret: %d\n", ret);
	return 0;
}

void print_disk(disk_t disk, int disk_size)
{
	unsigned char* databuf = malloc(disk->block_size);
	if(disk_size > 10) disk_size = 10;
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


void makdir(disk_t disk, char* name, char* path)
{
	super sup = read_super(disk);
	char* dir = strtok(path, "/");
	while(dir != NULL)
	{
		file* databuf = malloc(sizeof(unsigned char)*disk->block_size);
		readblock(disk, sup->root->block, (unsigned char*)databuf);
		int i;
		for(i = 0; i < disk->block_size/sizeof(struct file_entry);i++)
		{
			file temp = databuf[i];
			if(temp == NULL);//do something
			if(strcmp(temp->name, dir))
			{
				readblock(disk, temp->inode, (unsigned char*)databuf);
				dir = strtok(NULL, "/");
			}
		}
	}
}

int main(int argc, char** argv)
{
	char *disk_name;
	int disk_size;
	char *disk_test;
	disk_t disk;
	unsigned char *databuf;
	int i, j;
	
	if(argc != 3)
	{
		printf("Usage: myfile <disk_name> <disk_size> (in blocks)\n");
		exit(-1);
	}
	
	disk_name = (char *)argv[1];
	disk_test = (char *)argv[2];
	disk_size = atoi(disk_test);

	createdisk(disk_name, disk_size);
	printf("Disk %s created with size %d\n", disk_name, disk_size);
	disk = opendisk(disk_name);

	//printf("%c \n", disk_test[1]);
	
	init_fsys(disk, disk_size);	


	print_disk(disk, disk_size);

	printf("Checking superblock\n");
	databuf = malloc(disk->block_size);
	readblock(disk, 0, databuf);
	for(i = 0; i < 4; i++)
	{
		printf("%d:%d\n",i,((int*)databuf)[i]);
	}

	printf("Checking set_free_blocks\n");
	set_free_blocks(disk, make_address(((int*)databuf)[1], disk->block_size), 4, 1, '1');
	print_disk(disk, disk_size);
	exit(0);
}
