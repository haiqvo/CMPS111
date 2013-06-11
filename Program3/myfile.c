/* MyFile.c
 * 
 * Justin Yeo, Hai Vo, Erik Swedberg 
 * 
 * The file that uses the mydisk files and uses it to make a simple file system stucture.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "mydisk.h"

void print_disk(disk_t, int);

struct block_address
{
	int block;
	int offset;
	int address;
};
typedef struct block_address* address;


// a debug print statement
void print_address(address add)
{
	printf("block: %d\n", add->block);
	printf("offset: %d\n", add->offset);
	printf("address: %d\n", add->address);
}

//create a address 
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

//The struct for the file entry
struct file_entry
{
	char c0;//the name of the file
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

//the super block 
struct super_block
{
	int size;
	address free_map;
	address root;
	address data;
};
typedef struct super_block* super;

//initializing the super block
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

//use to write a array into the address location in the disk
int write_array(disk_t disk, address add, unsigned char* array)
{
	unsigned char* databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, add->block, databuf);
	int i = 0;
	int j = add->offset;
	int limit = disk->block_size;
	int block = add->block;
	while(1)
	{
		databuf[j++] = array[i++];
		if(array[i] == '\0') //check if array is not already filled
		{
			writeblock(disk, block, databuf);
			return 0;
		}
		if(j == limit)//check is the size of the block has been reached
		{
			writeblock(disk, block++, databuf);
			limit = limit + disk->block_size;
			j = 0;
			if(block > disk->size) return -1;
			readblock(disk, block, databuf);
		}
	}
}

//get a free blocks and set them with  a value
void set_free_blocks(disk_t disk, address free_map, int block, int block_count, char set)
{
	address temp = make_address(free_map->address + block - 1, disk->block_size);
	unsigned char* databuf = malloc(sizeof(unsigned char)*block_count);
	int i;
	for(i = 0; i < block_count; i++)
	{
		databuf[i] = set;
	}
	write_array(disk, temp, databuf);
}

//Set the blocks with a value
void set_block(disk_t disk, int block, char set)
{
	super sup = read_super(disk);
	address temp = make_address(sup->free_map->address + block, disk->block_size);
	unsigned char* databuf = malloc(sizeof(unsigned char)*2);
	databuf[0] = set;
	databuf[1] = '\0';
	write_array(disk, temp, databuf);
	free(databuf);
}

//find a free block map using the free block map
int get_free_block(disk_t disk)
{
	super sup = read_super(disk);
	int free_block_count = sup->size/disk->block_size + 1;
	unsigned char* databuf = malloc(sizeof(unsigned char)*disk->block_size);
	int i;
	for(i = sup->free_map->block; i < free_block_count + sup->free_map->block; i++)
	{
		readblock(disk, i, databuf);
		int j;
		for(j = 0; j < disk->block_size; j++)
		{
			if(databuf[j] == '0')//looks at the free block map and see if it is zero
			{
				j += (i-sup->free_map->block)*disk->block_size;				
				set_block(disk, j, '1');//set the byte to unfree(1)
				return j;
			}
		}
	}
	return -1;	
} 

//initialize the file system with a set size
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
		databuf[i] = '\0';//setting it to null
	}
	for(i = 0; i < size; i++)
	{
		writeblock(disk, i, databuf);//making all the block initialize
	}


	//superblock
	readblock(disk, 0, databuf);// making the super block
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
	for(j = i; j < free_size; j++)//filling at the other bytes to 0
	{
		databuf[j] = '0';
	}
	databuf[j] = '\0';//free block map end
	int ret = write_array(disk, free_blocks, databuf);

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

	root_entry = malloc(sizeof(struct file_entry));// making the file entry for root
	strcpy(&(root_entry->c0), "root");
	root_entry->directory = true;
	root_entry->inode = root->block;
	root_entry->size = 0;
	return 0;
}

/*void print_block(disk_t disk, unsigned char* databuf)
{
	int i;
	for(i = 0; i < 
}*/

//a debug print statement
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

//use to get the data for a block by the name
file fetch(disk_t disk, char* name, file* databuf)
{
	int i;
	for(i = 0; i < disk->block_size/sizeof(struct file_entry); i++)
	{
		file ret = &((*databuf)[i]);
		if(strcmp(&(ret->c0), "\0") == 0) //if there is no name founded return null
		{
			return NULL;
		}
		if(strcmp(&(ret->c0), name) == 0)//once name found return block
		{
			readblock(disk, ret->inode, (unsigned char*)(*databuf));
			return ret;
		}
	}
}

//goes though the path to file the file to fetch
file opendir(disk_t disk, char* directory, file* databuf)
{	
	char* path = strdup(directory);
	char* dir;
	file op = root_entry;
	if(strcmp(path, "/") == 0) dir = NULL;//if the path is just the root
	else dir = strtok(path, "/");

	while(dir != NULL)
	{
		op = fetch(disk, dir, databuf);
		dir = strtok(NULL, "/");//continue down the path
	}
	return op;
}

//looks for empty file entry
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

//creates a directory
int makdir(disk_t disk, char* name, char* path)
{
	super sup = read_super(disk);
	file databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, sup->root->block, (unsigned char*)(databuf));
	file op = opendir(disk, path, &databuf);
	if(op == NULL) return -1;

	int index = find_empty_entry(disk, &databuf);// looks for a empty spot
	if(index >= 0)// if spot open create a directory
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

//create a file
int createfile(disk_t disk, char* name, char* path, int size, file* input)
{
	file inputbuf = *input;
	super sup = read_super(disk);
	file databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, sup->root->block, (unsigned char*)(databuf));
	file op = opendir(disk, path, &databuf);
	if(op == NULL) return -1;

	int index = find_empty_entry(disk, &databuf);// looks for a spot
	if(index >= 0)// create file
	{
		file new = &databuf[index];
		int strlength = strlen(name);
		if(strlength > 8){
			name[8] = '\0';
		}
		strcpy(&new->c0, name);
		new->directory = false;
		int iblock = get_free_block(disk);
		new->inode = iblock;
		int block;
		if(op != NULL) block = op->inode;
		else block = sup->root->block;
		writeblock(disk, block, (unsigned char*)databuf);//directory written back

		//create inode

		free(databuf);
		databuf = malloc(sizeof(unsigned char)*disk->block_size);
		((inode)databuf)->size = size;
		int* array = &(((inode)databuf)->blockarray);
		int i;
		for(i = 0; i < size; i++)
		{
			array[i] = get_free_block(disk);
			writeblock(disk, array[i], ((unsigned char*)inputbuf)+i*disk->block_size);
		}
		writeblock(disk, iblock, (unsigned char*)databuf);
		return 1;
	}
	return -1;
}

//uses as a function to read files
file readfile(disk_t disk, char* path)
{
	int j;
	super sup = read_super(disk);
	file databuf = malloc(sizeof(unsigned char)*disk->block_size);
	readblock(disk, sup->root->block, (unsigned char*)(databuf));
	file op = opendir(disk, path, &databuf);//go to the directory in the path
	if(op == NULL){ 
		return NULL;
	}
	inode inodebuf = (inode)databuf;
	databuf = malloc(sizeof(unsigned char)*disk->block_size*inodebuf->size);
	int* array = &inodebuf->blockarray;
	int i;
	for(i = 0; i < inodebuf->size; i++)// go though the blocks and read  the file.
	{
		readblock(disk, array[i], ((unsigned char*)databuf) + i*disk->block_size);
	}
	return databuf;
}

/*
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

	//printf("\nChecking makdir\n");
	//printf("\tMaking test\n");
	//makdir(disk, "test", "/");
	//printf("\tMaking alsotest\n");
	//makdir(disk, "alsotest", "/");
	//print_disk(disk, disk_size);
	//exit(0);

	printf("\nCheking file created\n");
	file filebuf = malloc(disk->block_size*2);
	for(i = 0; i < disk->block_size; i++)
	{
		((unsigned char*)filebuf)[i] = 't';
	}
	for(; i < disk->block_size*2-145; i++)
	{
		((unsigned char*)filebuf)[i] = 'v';
	}

	createfile(disk, "tester", "/", 2, &filebuf);
	print_disk(disk, disk->size);

	printf("Reading file\n");
	unsigned char* red = (unsigned char*)readfile(disk, "/tester/");
	printf("\tPrinting file\n");
	for(i = 0; i < disk->block_size*2; i++)
	{
		printf("%c", red[i]);
	}
	printf("\n");
}
*/
