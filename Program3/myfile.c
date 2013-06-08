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
	//print_address(ret);//
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
	char cn;
	bool directory;
	int inode;
	int size;
};
typedef struct file_entry* file;

file root_entry;

struct directory_block
{
	file* files;
};
typedef struct directory_block* directory;

struct i_node
{
	int size;
	int blockarray;
};
typedef struct i_node* inode;

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

void set_block(disk_t disk, int block, char set)
{
	super sup = read_super(disk);
	address temp = make_address(sup->free_map->address + block - 1, disk->block_size);
	unsigned char* databuf = &set;
	write_array(disk, temp, databuf);
}

int get_free_block(disk_t disk)
{
	printf(">>>get_free_block\n");
	super sup = read_super(disk);
	int free_block_count = sup->size/disk->block_size + 1;
	unsigned char* databuf = malloc(sizeof(unsigned char)*disk->block_size*free_block_count);
	int i;
	for(i = 0; i < free_block_count; i++)
	{
		readblock(disk, i, databuf + disk->block_size*i);
		printf("ii:%d\n",i);		
		int j;
		for(j = disk->block_size*i; j < free_block_count*disk->block_size; j++)
		{
			printf("%c",databuf[j]);
		}
		printf("\n");
	}
	for(i = 0; i < free_block_count*disk->block_size; i++)
	{					
		if(databuf[i] == '0') 
		{
			set_block(disk, i, '1');
			return i;
		}
	}
	printf("\n");
	return -1;
} 

int init_fsys(disk_t disk, int size)
{
	address free_blocks = make_address(disk->block_size + 1, disk->block_size);
	//int mod = size%disk->block_size;
	int free_size = size;
	int free_block_count = size/disk->block_size + 1;

	address root = make_address(free_blocks->address + disk->block_size, disk->block_size);
	root_entry = malloc(sizeof(struct file_entry));
	strcpy(&(root_entry->c0), "root");
	root_entry->directory = true;
	root_entry->inode = root->block;
	root_entry->size = 0;
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
	
	//init root
	int file_count = disk->block_size/sizeof(struct file_entry);
	file filebuf = calloc(file_count, sizeof(struct file_entry));
	filebuf = realloc(filebuf, sizeof(unsigned char)*disk->block_size);
	for(i = 0; i < disk->block_size/sizeof(struct file_entry); i++)
	{
		filebuf[i].c0 = '\0';
		filebuf[i].c1 = '\0';
		filebuf[i].c2 = '\0';
		filebuf[i].c3 = '\0';
		filebuf[i].c4 = '\0';
		filebuf[i].c5 = '\0';
		filebuf[i].c6 = '\0';
		filebuf[i].c7 = '\0';
		filebuf[i].cn = '\0';
		filebuf[i].directory = true;
		filebuf[i].inode = -1;
		filebuf[i].size = -1;
	}
	writeblock(disk, root->block, (unsigned char*)databuf);
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

file fetch(disk_t disk, char* name, file* databuf)
{
	int i;
	for(i = 0; i < disk->block_size/sizeof(struct file_entry); i++)
	{
		file ret = &((*databuf)[i]);
		if(strcmp(&(ret->c0), "\0")) return NULL;
		if(strcmp(&(ret->c0), name))
		{
			readblock(disk, ret->inode, (unsigned char*)(*databuf));
			return ret;
		}
	}
}

file opendir(disk_t disk, char* path, file* databuf)
{
	char* dir;
	file op = root_entry;
	if(strcmp(path, "/")) dir = NULL;
	else dir = strtok(path, "/");

	while(dir != NULL)
	{
		op = fetch(disk, dir, databuf);
		dir = strtok(NULL, "/");
	}
	return op;
}

int find_empty_entry(disk_t disk, file* directory)
{	
	file databuf = *directory;
	int i;
	for(i = 0; i < disk->block_size/sizeof(struct file_entry); i++)
	{
		file temp = &databuf[i];
		if(strcmp(&(temp->c0), "\0"))//empty file slot found
		{
			return i;
		}
	}
	return -1;
}

int makdir(disk_t disk, char* name, char* path)
{
	super sup = read_super(disk);
	file databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, sup->root->block, (unsigned char*)(databuf));
	file op = opendir(disk, path, &databuf);
	if(op == NULL) return -1;
	
	int index = find_empty_entry(disk, &databuf);
	if(index >= 0)
	{
		file new = &databuf[index];
		strcpy(&new->c0, name);
		new->directory = true;
		int block = get_free_block(disk);
		new->inode = block;
		int wblock = 0;
		if(op != NULL) wblock = op->inode;
		else wblock = sup->root->block;
		writeblock(disk, wblock, (unsigned char*)databuf);
		return block;
	}
	return -1;
}

int createfile(disk_t disk, char* name, char* path, int size, file* input)
{
	file inputbuf = *input;
	super sup = read_super(disk);
	file databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, sup->root->block, (unsigned char*)(databuf));
	file op = opendir(disk, path, &databuf);
	if(op == NULL) return -1;

	int index = find_empty_entry(disk, &databuf);
	printf("index: %d\n", index);
	if(index >= 0)
	{
		file new = &databuf[index];
		strcpy(&new->c0, name);
		new->directory = false;
		int block = get_free_block(disk);
		new->inode = block;
		int wblock = 0;
		if(op != NULL) wblock = op->inode;
		else wblock = sup->root->block;
		writeblock(disk, wblock, (unsigned char*)databuf);

		//create inode

		free(databuf);
		databuf = malloc(sizeof(unsigned char)*disk->block_size);
		((inode)databuf)->size = size;
		int* array = &(((inode)databuf)->blockarray);
		int i;
		for(i = 0; i < size; i++)
		{
			array[i] = get_free_block(disk);
			file filebuf = malloc(sizeof(unsigned char)*disk->block_size);
			memcpy(filebuf, inputbuf + i*disk->block_size, disk->block_size);
			printf("array[i]: %d\n", array[i]);			
			writeblock(disk, array[i], (unsigned char*)filebuf);
		}
		return 1;
	}
	return -1;
}

file readfile(disk_t disk, char* path)
{
	super sup = read_super(disk);
	file databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, sup->root->block, (unsigned char*)(databuf));
	file op = opendir(disk, path, &databuf);
	if(op == NULL) return NULL;
	else if(op->directory) return NULL;
	
	inode inodebuf = (inode)databuf;
	databuf = malloc(sizeof(unsigned char)*disk->block_size*inodebuf->size);
	
	int* array = &inodebuf->blockarray;
	int i;
	for(i = 0; i < inodebuf->size; i++)
	{
		readblock(disk, array[i], (unsigned char*)(databuf + i*disk->block_size));
	}
	return databuf;
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
	
	printf("\nCheking file created\n");
	file filebuf = malloc(disk->block_size*2);
	for(i = 0; i < disk->block_size*2; i++)
	{
		((unsigned char*)filebuf)[i] = 't';
	}

	createfile(disk, "tester", "/", 2, &filebuf);
	
	printf("Reading file\n");
	unsigned char* red = (unsigned char*)readfile(disk, "/tester");
	printf("\tPrinting file\n");
	for(i = 0; i < disk->block_size*2; i++)
	{
		printf("I:%d\n",i);
		printf("\t%c", red[i]);
	}
	printf("\n");
}
