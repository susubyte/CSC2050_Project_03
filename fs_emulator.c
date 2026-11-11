#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>

#define MAX_INODES 1024
#define NAME_LEN 32
typedef struct {
	uint32_t number;
	uint32_t parent;
	char name[NAME_LEN];
	char type;
	int in_use;
} inode_t;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Incorrect number of arguments given.\n");
		return 1;
	}
	if (chdir(argv[1]) != 0) {
		printf("Could not open file system directory.\n");
		return 1;
	}

	inode_t inodes[MAX_INODES];
	memset(inodes, 0, sizeof(inodes));

	return 0;
}

void load_inodes(inode_t inodes[]) {
	FILE *f = fopen("inodes_list", "rb");
}
