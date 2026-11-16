/*
 * fs_emulator.c - emulates a simple file system using real files.
 * Usage: ./fs_emulator <file system directory>
 * Commands: ls, cd <name>, mkdir <name>, touch <name>, exit
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>

#define MAX_INODES 1024
#define NAME_LEN 32
#define LINE_LEN 1024

/* One entry of inodes_list: its own number, the directory it is in,
   type of file ('d' for directory, 'f' for file, 0 if the slot is unused),
   and its name. The name might not end in '\0' if it uses all 32 characters. */
typedef struct {
	uint32_t inode;
	uint32_t parentInode;
	char type;
	char name[NAME_LEN];
} Inode;

/* All inodes, indexed by inode number. */
Inode inodeList[MAX_INODES];

/* Inode number of the directory the user is currently in. */
uint32_t currentInode = 0;

void loadInodeList(const char *path);
void saveInodeList(const char *path);
void changeDirectory(const char *name);
void listContents(void);
void createDirectory(const char *name);
void createFile(const char *name);
int findEntry(const char *name);
int findFreeInode(void);
void createInode(const char *name, char type);
int inCurrentDirectory(int i);

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

	char line[LINE_LEN];
	while (1) {
		printf("> ");
		fflush(stdout);
		if (fgets(line, sizeof(line), stdin) == NULL) {
			printf("\n");
			break;
		}
		char cmd[LINE_LEN], arg[LINE_LEN], extra[LINE_LEN];
		int n = sscanf(line, "%s %s %s", cmd, arg, extra);
		if (n < 1) {
			continue;
		}
		if (n == 1 && strcmp(cmd, "exit") == 0) {
			break;
		} else if (n == 1 && strcmp(cmd, "ls") == 0) {
			listContents();
		} else if (n == 2 && strcmp(cmd, "cd") == 0) {
			changeDirectory(arg);
		} else if (n == 2 && strcmp(cmd, "touch") == 0) {
			createFile(arg);
		} else if (n == 2 && strcmp(cmd, "mkdir") == 0) {
			createDirectory(arg);
		} else {
			printf("Unknown command or wrong number of arguments.\n");
		}
	}

	saveInodeList("inodes_list");
	return 0;
}

/* Reads inodes_list and fills inodeList, using each entry's inode number
   as its slot. Entries with a bad number or type are reported and skipped. */
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
		}
		inodeList[temp.inode] = temp;
	}
	fclose(fp);
}

/* Writes every in-use inode back to inodes_list. Called on exit. */
void saveInodeList(const char *path) {
	FILE *fp = fopen(path, "wb");
	if (fp == NULL) {
		perror("fopen");
		return;
	}

	for (int i = 0; i < MAX_INODES; i++) {
		if (inodeList[i].type != 0) {
			fwrite(&inodeList[i], sizeof(Inode), 1, fp);
		}
	}
	fclose(fp);
}

/* Moves into the directory called name inside the current directory, or to
   the parent for "..". Prints an error if name isn't a directory. */
void changeDirectory(const char *name) {
	if (strcmp(name, "..") == 0) {
		currentInode = inodeList[currentInode].parentInode;
		return;
	}
	int entry = findEntry(name);
	if (entry != -1 && inodeList[entry].type == 'd') {
		currentInode = inodeList[entry].inode;
	} else if (entry != -1) {
		printf("%s is a regular file.\n", name);
	} else {
		printf("%s doesn't exist.\n", name);
	}
}

/* Lists the contents, name and inodes, of the current working directory. */
void listContents(void) {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inCurrentDirectory(i)) {
			printf("inode: %u, type: %c, name: %.*s\n",
			       inodeList[i].inode, inodeList[i].type, NAME_LEN, inodeList[i].name);
		}
	}
}

/* Creates a directory called name in the current directory. Prints an error
   if the name already exists or no inode is free. */
void createDirectory(const char *name) {
	if (findEntry(name) != -1) {
		printf("%.*s already exists.\n", NAME_LEN, name);
		return;
	}
	createInode(name, 'd');
}

/* Creates a regular file called name in the current directory. Does nothing
   if the name already exists or no inode is free. */
void createFile(const char *name) {
	if (findEntry(name) != -1) {
		return;
	}
	createInode(name, 'f');
}

/* Finds the inode that matches the name that was given, otherwise returns -1. */
int findEntry(const char *name) {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inCurrentDirectory(i) && strncmp(inodeList[i].name, name, NAME_LEN) == 0) {
			return i;
		}
	}
	return -1;
}

/* Finds the first available inode, otherwise returns -1 */
int findFreeInode(void) {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inodeList[i].type == 0) {
			return i;
		}
	}
	return -1;
}

/* Fills the first free slot with a new entry in the current directory and
   creates its real file on disk (the name for a file, "." and ".." lines
   for a directory). Prints an error and changes nothing if it can't. */
void createInode(const char *name, char type) {
	int slot = findFreeInode();
	if (slot == -1) {
		printf("No open inodes found.\n");
		return;
	}
	inodeList[slot].inode = slot;
	inodeList[slot].parentInode = currentInode;
	inodeList[slot].type = type;
	strncpy(inodeList[slot].name, name, NAME_LEN);
	
	char filename[16];
	snprintf(filename, sizeof(filename), "%u", inodeList[slot].inode);
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		perror("fopen");
		inodeList[slot].type = 0;
		return;
	}
	if (type == 'f') {
		fprintf(fp, "%.*s", NAME_LEN, inodeList[slot].name);
	} else {
		fprintf(fp, "%u .\n%u ..\n", inodeList[slot].inode, inodeList[slot].parentInode);
	}
	fclose(fp);
}

/* Checks if the inode is a used inode and if it is in the current directory. */
int inCurrentDirectory(int i) {
	return inodeList[i].type != 0 && inodeList[i].parentInode == currentInode;
}
