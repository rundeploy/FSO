
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

//#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 6



/*
Teste
*/
struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};
struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
};
union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};
int fs_format(){
	return 0;
}
void fs_debug()
{
	union fs_block block;

	disk_read(0,block.data);
	
	printf("superblock:\n");
	//if (1/* true */)
		//printf("Magic number is valid\n");

	printf("    %d blocks on disk\n",block.super.nblocks);
	printf("    %d blocks for inodes\n",block.super.ninodeblocks);
	printf("    %d inodes total\n",block.super.ninodes);

	int numberofinodes = block.super.ninodeblocks;

	for (int i = 1; i <= numberofinodes; i++){
		disk_read(i,block.data);

		for (int j = 0; j < INODES_PER_BLOCK; j++){
			if(block.inode[j].isvalid == 1){
				printf("Inode %d: \n", j);
				printf("    size: %d\n",block.inode[j].size);
				printf("    Blocks:");
				for (int k = 0; k < POINTERS_PER_INODE ; k++){
					if (block.inode[j].direct[k] != 0){
						printf(" %d", block.inode[j].direct[k]);
					}
				}
				printf("\n");
			}
		}
	}
}

int fs_mount()
{
	return 0;
}

/*

*/
int fs_create()
{
	return 0;
}

/*

*/
int fs_delete( int inumber )
{
	return 0;
}

/*

*/
int fs_getsize( int inumber )
{
	return -1;
}

/*

*/
int fs_read( int inumber, char *data, int length, int offset )
{
	return 0;
}

/*

*/
int fs_write( int inumber, const char *data, int length, int offset )
{
	return 0;
}
