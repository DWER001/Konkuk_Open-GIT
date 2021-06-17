#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_LEN 4096
#define BYTE unsigned char // 1B

#define SIZE_INODE 256 // 256B
#define SIZE_BLOCK 4096 // 4KB
#define LEN_BLOCK 64 // 64-block
#define SIZE_PARTITION 262144 // 256KB


struct inode{
	unsigned int fsize;
	unsigned int blocks;
	unsigned int pointer[12];
	BYTE dummy[200]; // Fill 256B
};


BYTE* disk = NULL;  // Disk Area

BYTE* g_iBmap = NULL; // disk + SIZE_BLOCK * 1;
BYTE* g_dBmap = NULL; // disk + SIZE_BLOCK * 2;
BYTE* g_inodeBlock = NULL; // disk + SIZE_BLOCK * 3;
BYTE* g_rootBlock = NULL; // disk + SIZE_BLOCK * 8;


// Find - FREE i-bmap Number
int find_imap(){
	int imap = -1;

	BYTE* target = g_iBmap;
	for(int i=0; i<10; ++i){ // 80node / 8bit == 10
		if((*target) == 0xff){ // full
			target += sizeof(BYTE);
			continue;
		}

		for(int j=7; j>=0; --j){ // Search 0~7bit
			if( (((*target) >> j) & 0x1) == 0x0 ){
				imap = i*8 + (7-j);
				break;
			}
		}
		break;
	}

	return imap;
}

// Find - FREE d-bmap Number
int find_dmap(){
	int dmap = -1;

	BYTE* target = g_dBmap;
	for(int i=0; i<7; ++i){ // (64-8)block / 8bit == 7
		if((*target) == 0xff){ // full
			target += sizeof(BYTE);
			continue;
		}

		for(int j=7; j>=0; --j){ // Search 0~7bit
			if( (((*target) >> j) & 0x1) == 0x0 ){
				dmap = i*8 + (7-j);
				break;
			}
		}
		break;
	}

	return dmap;
}



// Write
// Success -> Print Nothing
// Fail -> Print "No space\n" or "Already exists\n"
int W(char name[3], int size){
	// 1-1. Too Big File Size
	// Direct Pointer in One inode is 12.
	// That mean file's MAX size is 4K(block) x 12
	if(size > SIZE_BLOCK * 12){
		printf("No space\n");
		return 1;
	}

	// 1-2. Already Exists?
	BYTE* target = g_rootBlock;
	char tmp_name[3];

	for(int i=0; i<80; ++i){ // 80node
		if((*target) == 0){ // No File
			target += (sizeof(BYTE) * 4);
			continue;
		}

		target += sizeof(BYTE);
		memcpy(tmp_name, target, sizeof(BYTE) * 3);

		if(strcmp(tmp_name, name) == 0){ // Name is Same
			printf("Alreay exists\n");
			return 1;
		}
		
		target += (sizeof(BYTE) * 3);
	}

	// 1-3. Check No-space(i-bmap)
	int imap = find_imap();
	if(imap == -1){
		printf("No space\n");
		return 1;
	}

	// 1-4. Check No-space(d-bmap)
	int dmap = find_dmap();
	if(dmap == -1){
		printf("No space\n");
		return 1;
	}

// [DBG] 
// printf("IMAP %d / DMAP %d\n", imap, dmap);


	// 2. Write Operation
	// GC - Release Data Block When Allocate Failed (Only at Multi Direct Pointer)
	int buffer_dmap_len = 1;
	int buffer_dmap[12];
	memset(buffer_dmap, 0, sizeof(buffer_dmap));
	buffer_dmap[0] = dmap;

	// 2-0. update d-bmap(First Block)
	target = g_dBmap;
	target += (sizeof(BYTE) * (dmap/8));
	(*target) = ((*target) | (0x80 >> (dmap%8)));
	
	// 2-1. Data block Control
	// Write Data at DATA Block
	// Write Info at Inode Block
	struct inode *node = malloc(sizeof(struct inode));
	memset(node, 0, sizeof(struct inode));
	node->fsize = size;
	if(size <= 4096){ // Single Direct Pointer
		node->blocks = 1;
		node->pointer[0] = dmap;

		target = g_rootBlock + SIZE_BLOCK*dmap;
		memset(target, name[0], size); // Write Data
	}
	else{ // Multi Direct Pointer
		// First Direct Pointer
		node->pointer[0] = dmap;

		target = g_rootBlock + SIZE_BLOCK*dmap;
		memset(target, name[0], 4096); // Write Data

		// N Direct Pointer
		int len = (size-1) / 4096 + 1;
		node->blocks = len;

		for(int i=1; i<len; ++i){
			// Get Extra Data Block
			int dmap = find_dmap();
			
// [DBG]
// printf("Extra DMAP %d\n", dmap);
			if(dmap == -1){ // Failed to Allocate Extra Block
				printf("No space\n");
				free(node);
				
				// Release Data Block (at d-bmap)
				for(int i=0; i<buffer_dmap_len; ++i){
					dmap = buffer_dmap[i];
// [DBG]
// printf("DELETE dmap %d\n", dmap);
		
					target = g_dBmap;
					target += (sizeof(BYTE) * (dmap/8));
					(*target) = ((*target) & ( 0xff ^ (0x80 >> (dmap%8)) )); // ERASE MASK(0xFF XOR (Position bit))
				}

				return 1;
			}

			// Write Data
			node->pointer[i] = dmap;
			target = g_rootBlock + SIZE_BLOCK*dmap;
			if((i == len-1) && ((size % 4096) != 0)){
				memset(target, name[0], size % 4096);
			}
			else{
				memset(target, name[0], 4096);
			}

			// d-bmap update buffer
			buffer_dmap[buffer_dmap_len++] = dmap;
			
			// update d-bmap
			target = g_dBmap;
			target += (sizeof(BYTE) * (dmap/8));
			(*target) = ((*target) | (0x80 >> (dmap%8)));
		}
	}

	memcpy(g_inodeBlock + SIZE_INODE*imap, node, SIZE_INODE);
	free(node);
	
	// 2-2. update rootBlock
	target = g_rootBlock;
	for(int i=0; i<80; ++i){ // 80node
		if((*target) == 0){ // No File
			(*target) = imap; // Write inum at RootBlock
			
			target += (sizeof(BYTE));
			memcpy(target, name, sizeof(BYTE) * 3); // Write name at RootBlock
			break;
		}
		
		target += (sizeof(BYTE) * 4);
	}

	// 2-3. update i-bmap
	target = g_iBmap;
	target += (sizeof(BYTE) * (imap/8));
	(*target) = ((*target) | (0x80 >> (imap%8)));

	// 2-4. update d-bmap
	// d-bmap Sync doesn't Required (Already Done)
	// target = g_dBmap;
	// target += (sizeof(BYTE) * (dmap/8));
	// (*target) = ((*target) | (0x80 >> (dmap%8)));
	//
	// for(int i=0; i<buffer_dmap_len; ++i){
	//	dmap = buffer_dmap[i];
	//	target = g_dBmap;
	//	target += (sizeof(BYTE) * (dmap/8));
	//	(*target) = ((*target) | (0x80 >> (dmap%8)));
	//}

	return 0;
}

// Read
// Success -> Print Content (len=min(fsize,size), end='\n')
// Fail -> Print "No such file\n"
int R(char name[3], int size){
	// 1. File Exists?
	BYTE* target = g_rootBlock;
	char tmp_name[3];
	int inode = -1;

	for(int i=0; i<80; ++i){ // 80node
		if((*target) == 0){ // No File
			target += (sizeof(BYTE) * 4);
			continue;
		}

		target += sizeof(BYTE);
		memcpy(tmp_name, target, sizeof(BYTE) * 3);

		if(strcmp(tmp_name, name) == 0){ // Name is Same (Found)
			target -= sizeof(BYTE);
			inode = (*target);

			break;
		}
		
		target += (sizeof(BYTE) * 3);
	}

	if(inode == -1){
		printf("No such file\n");
		return 1;
	}

// [DBG] 
// printf("READ INODE %d\n", inode);


	// 2. READ Operation
	// read inode
	struct inode *node = malloc(sizeof(struct inode));
	target = g_inodeBlock + SIZE_INODE*inode;
	memcpy(node, target, sizeof(struct inode));

	// set read size
	int len;
	if(size > node->fsize){
		len = node->fsize;
	}
	else{
		len = size;
	}

	// read
	char buffer[BUFFER_LEN];
	if(len <= 4096){ // Single Direct Pointer
		memset(buffer, 0, sizeof(buffer));

		strncpy(buffer, g_rootBlock + SIZE_BLOCK*node->pointer[0], len);
		printf("%s\n", buffer);
	}
	else{ // Multi Direct Pointer
		for(int i=0; i< node->blocks; ++i){
			memset(buffer, 0, sizeof(buffer));

			if(len <= 4096){
				strncpy(buffer, g_rootBlock + SIZE_BLOCK*node->pointer[i], len);
			}
			else{
				strncpy(buffer, g_rootBlock + SIZE_BLOCK*node->pointer[i], 4096);
			}
			len -= 4096;

			printf("%s", buffer);
			if(len <= 0) break;
		}
		printf("\n");
	}

	free(node);

	return 0;
}

// Delete
// Success -> Print Nothing
// Fail -> Print "No such file\n"
int D(char name[3]){
	// 1. File Exists?
	BYTE* target = g_rootBlock;
	char tmp_name[3];
	int inode = -1;

	for(int i=0; i<80; ++i){ // 80node
		if((*target) == 0){ // No File
			target += (sizeof(BYTE) * 4);
			continue;
		}

		target += sizeof(BYTE);
		memcpy(tmp_name, target, sizeof(BYTE) * 3);

		if(strcmp(tmp_name, name) == 0){ // Name is Same (Found)
			target -= sizeof(BYTE);
			inode = (*target);

			break;
		}
		
		target += (sizeof(BYTE) * 3);
	}

	if(inode == -1){
		printf("No such file\n");
		return 1;
	}

// [DBG]
// printf("DELETE imap %d\n", inode);


	// 2. Delete Operation
	// Delete inum at RootBlock
	(*target) = 0;

	// Get inode
	struct inode *node = malloc(sizeof(struct inode));
	target = g_inodeBlock + SIZE_INODE*inode;
	memcpy(node, target, sizeof(struct inode));

	// Update i-bmap
	int imap = inode;

	target = g_iBmap;
	target += (sizeof(BYTE) * (imap/8));
	(*target) = ((*target) & ( 0xff ^ (0x80 >> (imap%8)) )); // ERASE MASK(0xFF XOR (Position bit))

	// Update d-bmap
	for(int i=0; i<node->blocks; ++i){
		int dmap = node->pointer[i];
// [DBG]
// printf("DELETE dmap %d\n", dmap);
		
		target = g_dBmap;
		target += (sizeof(BYTE) * (dmap/8));
		(*target) = ((*target) & ( 0xff ^ (0x80 >> (dmap%8)) )); // ERASE MASK(0xFF XOR (Position bit))
	}

	free(node);

	return 0;
}


int main(int argc, char *argv[]){
	FILE *fp;
	char buffer[BUFFER_LEN];

	// 1. Init
	// Verify Parameter
	if(argc != 2){
		printf("ku_fs: Wrong Parameter\n");
		return 1;
	}

	fp = fopen(argv[1], "r");
	if(!fp){
		printf("ku_fs: Failed to READ Input Files\n");
		return 1;
	}

	// Init Memory
	disk = (BYTE *)malloc(sizeof(BYTE) * SIZE_PARTITION);
	memset(disk, 0, sizeof(BYTE) * SIZE_PARTITION);
	//for(int i=0; i<SIZE_PARTITION; ++i){
	//	disk[i] = 0;
	//}

	// Set I-bmap
	disk[SIZE_BLOCK * 1] = 224; // 1111 0000

	// Set d-bmap
	disk[SIZE_BLOCK * 2] = 128; // 1000 0000

	// Set iblock0 - inode2 (Root Path)
	struct inode *root = malloc(sizeof(struct inode));
	memset(root, 0, sizeof(struct inode));
	root->fsize = 8*40; // 320
	root->blocks = 1;

	memcpy(disk + SIZE_BLOCK*3 + SIZE_INODE*2, root, SIZE_INODE);
	free(root);

	// Set Pointer (for EASY-coding)
	g_iBmap = disk + SIZE_BLOCK * 1;
	g_dBmap = disk + SIZE_BLOCK * 2;
	g_inodeBlock = disk + SIZE_BLOCK * 3;
	g_rootBlock = disk + SIZE_BLOCK * 8;


	// 2. Read From Input File
	char name[3];
	char mode;
	char size_str[BUFFER_LEN];
	int size;

	int str_len;

	while(fgets(buffer, sizeof(buffer), fp)){
		// Verify File Input
		str_len = strlen(buffer);
		if(str_len <= 4){
			printf("ku_fs: Wrong Input Format (in TXT). Exit Program.\n");
			break;
		}
		buffer[str_len-1]='\0'; // Remove \n
		
		// READ File
		strncpy(name, buffer, 2);
		mode = buffer[3];
		memset(size_str, 0, sizeof(size_str));
		size = -1;

		if(mode == 'w' || mode == 'r'){
			strncpy(size_str, buffer, sizeof(buffer));
			size = atoi(&size_str[5]);

			if(size <= -1){
				printf("ku_fs: Wrong Input Format (in TXT). Exit Program.\n");
				break;
			}
		}
		
// [DBG] Check Input Variable
// printf("%s\n", buffer);
// printf("NAME[%s]\n", name);
// printf("MODE[%c]\n", mode);
// printf("SIZE[%d]\n", size);
		
		switch(mode){
		case 'w':
			W(name, size);
			break;
		case 'r':
			R(name, size);
			break;
		case 'd':
			D(name);
			break;
		}
	}


	// 3. Close Program
	fclose(fp);

	BYTE *iter = disk;
	for(int i=0; i<SIZE_PARTITION; ++i, ++iter){
		printf("%.2x ", *iter);

// [DBG] Print Line by I-Node
// if(i % SIZE_INODE == SIZE_INODE-1){
// 	printf("\n");
// }
	}

	free(disk);

	return 0;
}
