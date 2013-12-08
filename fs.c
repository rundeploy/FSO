
#include "fs.h"
#include "disk.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 6
#define BLOCKSIZE 4096

struct fs_superblock {
	int magic; // magic number of the file system of the disk 
	int nblocks; // total of blocks of the disk
	int ninodeblocks; // number of blockes for inode table
	int ninodes; //total number of inodes on disc
};
struct fs_inode {
	int isvalid; // inode is free or not
	int size; // size of file from inode
	int direct[POINTERS_PER_INODE]; // points to the file celules (6 pointers max)
};
union fs_block {
	struct fs_superblock super; // superblock - first block of the disk
	struct fs_inode inode[INODES_PER_BLOCK]; // array of inodes of the block (128 inodes/block max)
	char data[DISK_BLOCK_SIZE]; // array of block of the disk
};

void inode_load( int inumber, struct fs_inode *inode );
void inode_save( int inumber, struct fs_inode *inode );

int ismounted = 0; // Mount/unmount disk. 0 - not mounted. 1 - mounted.
int *arrayOfInodes;

int *arrayofBlocks;

struct fs_superblock super;

int fs_format(){
	
	if(ismounted == 1){
		return 0;
	}

	union fs_block block;
 	int numberofblocks =  disk_size();
 	int nblockitable = numberofblocks *0.1;
 	int totalofinodes = 128*nblockitable;
 	
 	block.super.magic = FS_MAGIC;
 	block.super.nblocks = numberofblocks;
 	block.super.ninodeblocks = nblockitable;
 	block.super.ninodes = totalofinodes;
 	
 	disk_write(0,block.data);
 	
 	for (int i = 1; i <= nblockitable; i++){
		disk_read(i,block.data);
		for (int j = 0; j < INODES_PER_BLOCK; j++){
			block.inode[j].isvalid = 0;
		}
		disk_write(i,block.data);
	}
	return 1;
}

void fs_debug(){
	
	union fs_block block;
	disk_read(0,block.data);
	
	printf("superblock:\n");
	if(block.super.magic != FS_MAGIC){ // Verify if file system on disk is valid
		printf("Magic number is not valid\n");
	}else{ 
		printf("Magic number is valid\n");
		printf("    %d blocks on disk\n",block.super.nblocks);
		printf("    %d blocks for inodes\n",block.super.ninodeblocks);
		printf("    %d inodes total\n",block.super.ninodes);

		int nblockitable = block.super.ninodeblocks;

		for (int i = 1; i <= nblockitable; i++){
			disk_read(i,block.data);
			for (int j = 0; j < INODES_PER_BLOCK; j++){
				if(block.inode[j].isvalid == 1){
					printf("Inode %d: \n", INODES_PER_BLOCK*(i-1)+j);
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
}

int fs_mount(){
	if(ismounted == 1){
		return 0;
	}
	union fs_block block;
	disk_read(0,block.data);

	// Verify if file system on disk is valid
	if(block.super.magic != FS_MAGIC){
		return 0;
	}
	int numberofblocks =  disk_size();
	if(numberofblocks != block.super.nblocks){
		return 0;
	}

	arrayOfInodes = (int *)malloc(block.super.ninodes*sizeof(int));
	
	if(arrayOfInodes == NULL){
		return 0;
	}

	super.magic = block.super.magic;
	super.nblocks = block.super.nblocks;
	super.ninodeblocks = block.super.ninodeblocks;
	super.ninodes = block.super.ninodes;

	int nblockitable = block.super.ninodeblocks;
	
	for (int i = 1; i <= nblockitable; i++){
		disk_read(i,block.data);
		
		for (int j = 0; j < INODES_PER_BLOCK; j++){
			if (block.inode[j].isvalid == 0){
				arrayOfInodes[INODES_PER_BLOCK*(i-1)+j] = 0;
			}else{
				arrayOfInodes[INODES_PER_BLOCK*(i-1)+j] = 1;
			}
		}
	}

	ismounted = 1;
	return ismounted;
}

int fs_create(){ 
	if(ismounted != 1){
		return -1;
	}

	int ninodes = super.ninodes;
	struct fs_inode inode;

	for (int i = 0; i < ninodes; i++){
		if(arrayOfInodes[i] == 0){
			inode.isvalid = 1;
			inode.size = 0;
			for (int k = 0; k < POINTERS_PER_INODE ; k++){
				inode.direct[k] = 0;
			}
			arrayOfInodes[i] = 1;
			inode_save(i,&inode);		
			return i;
		}
	}
	return -1;
}

int fs_delete(int inumber){
	struct fs_inode inode;
	// disk must be mounted first to delete smthng
	if(ismounted !=1){
		return 0;
	}
	// if inode is greater than total of existing inodes - return error
	if (inumber >= super.ninodes){
		return 0;
	}
	// If inode isn't created - return error
	if (arrayOfInodes[inumber] ==  0){
		return 0; 
	}

	inode.isvalid = 0;
	inode.size = 0;
	for (int i = 0; i < POINTERS_PER_INODE; i++){
		inode.direct[i] = 0;
	}
	
	inode_save(inumber,&inode);
	arrayOfInodes[inumber] = 0;
	
	return 1;
}

int fs_getsize( int inumber ){
	struct fs_inode inode;
	inode_load(inumber, &inode);
	// disk must be mounted first to get the size
	if(ismounted != 1){
		printf("Disk is not mounted\n");
		return -1;
	}
	// if inode is greater than total of existing inodes - return error
	if (inumber >= super.ninodes){
		printf("Inode does not exist.\n");
		return -1;
	}
	// If inode isn't created - return error
	if (arrayOfInodes[inumber] ==  0){
		return -1; 
	}

	return inode.size;
}

void inode_load( int inumber, struct fs_inode *inode ) {
	union fs_block block;
	disk_read(inumber/INODES_PER_BLOCK+1,block.data);
	*inode = block.inode[inumber%INODES_PER_BLOCK];
} 

void inode_save( int inumber, struct fs_inode *inode ) {
	union fs_block block;
	disk_read(inumber/INODES_PER_BLOCK+1,block.data);
	block.inode[inumber%INODES_PER_BLOCK] = *inode;	
	disk_write(inumber/INODES_PER_BLOCK+1,block.data);
} 


int fs_read( int inumber, char *data, int length, int offset ){
	//quando o size passar do tamanho do file o size passa a ser o tamanho do file
	
	if (ismounted == 0){
		return -1;
	}
	
	if (inumber >= super.ninodes){
		return -1;
	}

	struct fs_inode inode;
	inode_load(inumber, &inode);

	int readbytes = 0;

	if(inode.size < offset){
		return -1;
	}
	
	if((offset + length) > inode.size){
		readbytes = inode.size - offset;
	}

	char bloco[BLOCKSIZE];

	int n = 0;
	int coffset = offset;

	while( n > readbytes){
		
		disk_read(inode.direct[coffset/BLOCKSIZE],bloco);
		int offsetintobloco = coffset % BLOCKSIZE;
		int srcaddr = bloco + offsetintobloco;
		int destaddr = data + coffset;
		int nbytecpy = BLOCKSIZE - offsetintobloco;
		if(nbytecpy > readbytes){
			nbytecpy = readbytes;
		}

		bcopy(&bloco,&data,readbytes);
		readbytes = readbytes - nbytecpy;
		coffset = coffset + nbytecpy;
	}
	
	return -1;
}

int fs_write( int inumber, const char *data, int length, int offset ){

	if (ismounted == 0){
		return -1;
	}
	
	if (inumber >= super.ninodes){
		return -1;
	}
	
	struct fs_inode inode;
	inode_load(inumber, &inode);




	return 0;
}