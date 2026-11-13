#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>

#define MAX_INODES 1024
#define NAME_LEN 32
typedef struct {
	uint32_t inode;
	uint32_t parentInode;
	char type;
	char name[NAME_LEN];
} Inode;

Inode inodeList[MAX_INODES];
size_t inodeCount = 0;
uint32_t currentInode = 0;

void loadInodeList(const char *path);
void saveInodeList(const char *path);
void changeDirectory(const char *name);
void listContents();
void createDirectory(const char *name);
void createFile(const char *name);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Incorrect number of arguments given.\n");
		return 1;
	}

	if (chdir(argv[1]) != 0) {
		printf("Could not open file system directory.\n");
		return 1;
	}

	loadInodeList("inodes_list");
	if (inodeList[0].type != 'd') {
		printf("First file is not a directory.\n");
		return 1;
	}

	char line[256];
	while (1) {
		printf("> ");
		fflush(stdout);
		if (fgets(line, sizeof(line), stdin) == NULL) {
			break;
		}
		char cmd[256], arg[256];
		int n = sscanf(line, "%s %s", cmd, arg);
		if (n == 1 && strcmp(cmd, "exit") == 0) {
			break;
		} else if (n == 1 && strcmp(cmd, "ls") == 0) {
			listContents();
		} else if (n == 2 && strcmp(cmd, "cd") == 0) {
		       changeDirectory(arg);	
		} else {
			printf("Unknown Command.\n");
		}
	}

	return 0;
}

void loadInodeList(const char *path) {
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) {
		perror("fopen");
		return;
	}

	Inode temp;
	while (fread(&temp, sizeof(Inode), 1, fp) == 1) {
		if (temp.inode >= MAX_INODES || (temp.type != 'd' && temp.type != 'f')) {
			printf("Invalid inode entry: %u\n", temp.inode);
			continue;
		} else {
	 		inodeList[temp.inode] = temp;
		}		
	}
	fclose(fp);
}

void changeDirectory(const char *name) {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inodeList[i].type == 'd' && inodeList[i].parentInode == currentInode && strncmp(inodeList[i].name, name, NAME_LEN) == 0) {
			currentInode = inodeList[i].inode;
			return;
		} else if (inodeList[i].type == 'f' && inodeList[i].parentInode == currentInode && strncmp(inodeList[i].name, name, NAME_LEN) == 0) {
			printf("%s is a regular file.\n", name);
			return;
		}
	}
	printf("%s doesn't exist.\n", name);
}

void listContents() {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inodeList[i].type != 0 && inodeList[i].parentInode == currentInode) {
			printf("inode: %u, type: %c, name: %.32s\n", inodeList[i].inode, inodeList[i].type, inodeList[i].name);
		}
	}
}

// void createDirectory(const char *name) {
// }
