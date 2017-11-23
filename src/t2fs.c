#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "t2fs.h"
#include "apidisk.h"

#define FSYSTEM_VERSION 0x7E12
#define FSYSTEM_ID "T2FS"

int InitializedDisk = 0;
unsigned char buffer[SECTOR_SIZE];
int openedFiles[10] = {-1};
DWORD currentDir;

typedef struct t2fs_superbloco sBlock;
typedef struct t2fs_record Record;
int *currentPointer;

sBlock superblock;

/* Initialize the disk, copying the values of superblock, checking if the data is consistent.
Futhermore, allocate memory for currentPointer and set the value for currentDir for root (in sectors)*/
int t2fsInit(){
    const char *temp;
    //Record parent, self, empty, block[4];
    if((read_sector(0, buffer)) != 0){
        return -1;
    }
    temp = (char *) buffer;
    strncpy(superblock.id, temp, 4);
    if(strncmp(superblock.id, FSYSTEM_ID, 4) != 0){
        return -1;
    }
    superblock.version = *((WORD *)(buffer + 4));
    if(superblock.version != FSYSTEM_VERSION){
        return -1;
    }

    superblock.SuperBlockSize = *((WORD *)(buffer + 6));
    superblock.DiskSize = *((DWORD *)(buffer + 8));
    superblock.NofSectors = *((DWORD *)(buffer + 12));
    superblock.SectorsPerCluster = *((DWORD *)(buffer + 16));
    superblock.pFATSectorStart = *((DWORD *)(buffer + 20));
    superblock.RootDirCluster = *((DWORD *)(buffer + 24));
    superblock.DataSectorStart = *((DWORD *)(buffer + 28));

    printf("id: %s\n",superblock.id);
    printf("versao: 0x%X\n",superblock.version);
    printf("superBloco: %hu\n",superblock.SuperBlockSize);
    printf("diskSize: %hu\n",superblock.DiskSize);
    printf("nOfSectors: %hu\n",superblock.NofSectors);
    printf("SectorsPerCluster: %hu\n",superblock.SectorsPerCluster);
    printf("pFATSectorStart: %hu\n",superblock.pFATSectorStart);
    printf("RootDirCluster: %hu\n",superblock.RootDirCluster);
    printf("DataSectorStart: %hu\n",superblock.DataSectorStart);

    currentDir = superblock.DataSectorStart + superblock.RootDirCluster*superblock.SectorsPerCluster;

    currentPointer = malloc((int)sizeof((int)(superblock.NofSectors - superblock.DataSectorStart)/superblock.SectorsPerCluster));

    /*self.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(self.name, ".");
    self.bytesFileSize = superblock.SectorsPerCluster*SECTOR_SIZE;
    self.firstCluster = currentDir;

    parent.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(parent.name, "..");
    parent.bytesFileSize = superblock.SectorsPerCluster*SECTOR_SIZE;
    parent.firstCluster = currentDir;

    empty.TypeVal = TYPEVAL_INVALIDO;

    block[0] = self;
    block[1] = parent;
    block[2] = empty;
    block[3] = empty;
    memcpy(buffer,block,SECTOR_SIZE);
    if(write_sector(currentDir,buffer) != 0){
        return -1;
    }*/
    InitializedDisk = 1;
    return 0;
}

/* This funtion check is a path is valid.
    Param 1: the path to be validated
    Param 2: the type (dir or regular file) of the last token, being equal to 'd' if looking for a dir, any other char if looking for a file.
    Return: 1 if the path is not valid, the cluster number if it is */
DWORD validPath(char *filename, char fileType){
    int lenght = strlen(filename);
    int normalFile = 0, numOfSectors = 0;
    DWORD entry = 0;
    char name[MAX_FILE_NAME_SIZE], auxName[lenght + 1], truncated[MAX_FILE_NAME_SIZE], previous[MAX_FILE_NAME_SIZE];
    char *safeCopy = filename;
    DWORD cluster = (currentDir - superblock.DataSectorStart)/superblock.SectorsPerCluster;
    strcpy(auxName, filename);
    if(strcmp(auxName, "/") == 0){
        if(read_sector((superblock.RootDirCluster*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
            return 1;
        } else {
            if(fileType == 'f'){
                return superblock.RootDirCluster; //Valid path: root
            } else {
                return 1;
            }
        }
    }
    if(*((BYTE*)filename) == '/'){
        cluster = superblock.RootDirCluster;
    }
    strcpy(truncated,(strtok(auxName,"/")));
    if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
        return 1;
    }
    entry = 0;
    while(strcmp(truncated,"") != 0){
        strncpy(name,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
        //printf("name: %s | truncated: %s\n", name, truncated);
        numOfSectors = 0;
        while(numOfSectors < superblock.SectorsPerCluster){
            while(strcmp(truncated,name) != 0 && entry <= SECTOR_SIZE - sizeof(Record)){
                entry = entry + sizeof(Record);
                strncpy(name,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
                if(name == NULL){
                    return 1;
                }
            }
            if(entry > SECTOR_SIZE - sizeof(Record)){
                numOfSectors++;
                if(numOfSectors < superblock.SectorsPerCluster){
                    if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart + numOfSectors), buffer) != 0){
                        return 1;
                    }
                    entry = 0;
                    strncpy(name,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
                }
            } else {
                break;
            }
        }
        if(strcmp(truncated,name) == 0){
            if(*((BYTE *)(buffer + entry)) == TYPEVAL_REGULAR){
                normalFile++;
                if(normalFile > 1){
                    return 1;//Invalid path, only one normal type file per path (none if fileType = 'd'
                }
            }
            cluster = *((DWORD *)(buffer + entry + 1 + MAX_FILE_NAME_SIZE + 4));
            if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
                return 1;
            }
            entry = 0;
        } else {
            return 1;
        }
        strcpy(previous,truncated);
        safeCopy = (strtok(NULL,"/"));
        if(safeCopy != NULL){
            strcpy(truncated,safeCopy);
        } else {
            strcpy(truncated,"");
        }
    }
    if(fileType == 'd' && normalFile > 0){
        return 1; //Invalid, cannot have any normal type file if it's looking for a dir
    }
    return cluster; //Valid Path, return cluster number
}

/* This funtion finds the first entry at fat with free flag.
    Return: index of the entry, counting since the first entry of the first sector of the fat. -1 if no free entry found*/
DWORD firstFitFat(){
    int sector = superblock.pFATSectorStart, entry = 0;;
    while(sector < superblock.DataSectorStart){
        if(read_sector(sector, buffer) != 0){
            return -1;
        }
        while(entry < SECTOR_SIZE/4){
            if(*((DWORD *)(buffer + entry)) == 0x00000000){
                return (sector-1)*SECTOR_SIZE + (entry/4) + 1;
            }
            entry = entry + 4;
        }
        sector++;
        entry = 0;
    }
    return -1;
}

/* This function finds the first unused entry at a dir.
    Param 1: the cluster of the dir
    Return: index of the first unused entry of the dir, counting since the first entry of the first sector of the cluster. -1 if no unused entry found*/
int unusedEntryDir(DWORD cluster){
    DWORD entry = 0;
    char candidate[MAX_FILE_NAME_SIZE];
    int numOfSectors = 0;
    while(numOfSectors < superblock.SectorsPerCluster){
        if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart + numOfSectors), buffer) != 0){
            return -1;
        }
        while(entry <= SECTOR_SIZE - sizeof(Record)){
            strncpy(candidate,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
            entry = entry + sizeof(Record);
            if(strcmp(candidate,"") == 0){
                if((*(BYTE *)(buffer + entry - sizeof(Record))) ==  TYPEVAL_INVALIDO){
                    printf("unused is %d\n", (numOfSectors*SECTOR_SIZE + entry - sizeof(Record))/sizeof(Record));
                    return (numOfSectors*SECTOR_SIZE + entry - sizeof(Record))/sizeof(Record);
                }
            }
        }
        numOfSectors++;
        entry = 0;
    }
    return -1;
}

/* Split filename into a path and a name (of the file)
    Param 1: absolute or relative path to be splited
    Param 2: char array which will receive the first n-1 tokens
    Param 3: char array which will receive the last token (aka name of the file or dir)*/
void extractPath(char *filename, char path[], char name[MAX_FILE_NAME_SIZE]){
    char auxName[strlen(filename) + 1], *safeCopy = filename, previous[strlen(filename) + 1];
    int endOfPath = 0;
    if(*((BYTE*)filename) == '/'){
        strcpy(path,"/");
    } else {
        strcpy(path,"");
    }
    strcpy(auxName, filename);
    safeCopy = strtok(auxName,"/");
    if(safeCopy != NULL){
        strcat(path,safeCopy);
        strcpy(previous,path);
        strcpy(name,safeCopy);
    } else {
        endOfPath = 1;
    }
    while(!endOfPath){
        safeCopy = strtok(NULL,"/");
        if(safeCopy != NULL){
            strcpy(previous,path);
            //printf("%s\n", path);
            strcat(path,"/");
            strcat(path,safeCopy);
            strcpy(name,safeCopy);
        } else {
            endOfPath = 1;
        }
    }
    strcpy(path,previous);
}

/* Check if there's no other file/dir with the same name
    Param 1: cluster of the dir to be checked
    Param 2: name of the possible doppelganger (filw with the same name as an already existing one
    Return: -1 if error, 0 if no doppelganger and, if doppelganger found, the index of the entry (useful for seraching for a file)*/
int doppelganger(DWORD cluster, char name[MAX_FILE_NAME_SIZE]){
    DWORD entry = 0;
    char candidate[MAX_FILE_NAME_SIZE];
    int numOfSectors = 0;
    while(numOfSectors < superblock.SectorsPerCluster){
        if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart + numOfSectors), buffer) != 0){
            return -1; //error
        }
        while(entry <= SECTOR_SIZE - sizeof(Record)){
            strncpy(candidate,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
            entry = entry + sizeof(Record);
            if(strcmp(candidate,name) == 0){
                return (numOfSectors*SECTOR_SIZE + entry - sizeof(Record))/sizeof(Record); //doppelganger detected
            }
        }
        numOfSectors++;
        entry = 0;
    }
    return 0; // neither file nor dir with this name;
}

FILE2 create2 (char *filename){
    DWORD parent, freeCluster, clusterValue = 0xFFFFFFFF;
    Record newFile;
    int freeEntry;
    char path[strlen(filename) + 1], name[MAX_FILE_NAME_SIZE];
    if(!InitializedDisk){
        t2fsInit();
    }
    extractPath(filename,path,name);
    printf("filename: %s | path: %s | name: %s\n", filename, path, name);
    parent = validPath(path,'d');
    printf("parent: %hu\n", parent);
    if(parent == 1){
        if(strcmp(name,path) != 0){
            return -1;
        }
        parent = (currentDir - superblock.DataSectorStart)/superblock.SectorsPerCluster;
    }
    if(doppelganger(parent,name) != 0){
        return -1;
    }
    freeCluster = firstFitFat();
    printf("first free cluster: %d\n", freeCluster);
    if(freeCluster == -1){
        return -1;
    }
    if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
        return -1;
    }
    if(read_sector((superblock.pFATSectorStart + (int)floor((double)freeCluster/SECTOR_SIZE)), buffer) != 0){
        return -1;
    }

    memcpy((buffer + (freeCluster-1)*4), &clusterValue,4);
    write_sector((superblock.pFATSectorStart + (int)floor((double)freeCluster/SECTOR_SIZE)), buffer);

    newFile.TypeVal = TYPEVAL_REGULAR;
    strcpy(newFile.name,name);
    newFile.bytesFileSize = 0;
    newFile.firstCluster = freeCluster;

    freeEntry = unusedEntryDir(parent);

    read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor((freeEntry)/(SECTOR_SIZE/sizeof(Record)))), buffer);
    memcpy((buffer + (freeEntry - superblock.SectorsPerCluster*((int)floor((freeEntry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)), &newFile, sizeof(Record));
    write_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(freeEntry/(SECTOR_SIZE/sizeof(Record)))), buffer);

    currentPointer[freeCluster] = 0; //set currentPointer of the file at handler index to 0;
    return freeCluster;
}

int delete2 (char *filename){
    DWORD parent, clusterValue = 0x00000000;
    Record deletedFile;
    int entry, sectorFat, nextCluster;
    char path[strlen(filename) + 1], name[MAX_FILE_NAME_SIZE];
    if(!InitializedDisk){
        t2fsInit();
    }
    extractPath(filename,path,name);
    printf("filename: %s | path: %s | name: %s\n", filename, path, name);
    parent = validPath(path,'d');
    printf("self: %hu\n", parent);
    if(parent == 1){
            return -1;
    }
    entry = doppelganger(parent,name);
    printf("entry: %d\n", entry);
    if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer) != 0){
        return -1;
    }
    deletedFile.TypeVal = TYPEVAL_INVALIDO;
    strcpy(deletedFile.name,"");
    deletedFile.bytesFileSize = 0;
    deletedFile.firstCluster = 0;

    //sectorFat = *(DWORD *)(buffer + ((int)floor(entry/(SECTOR_SIZE/sizeof(Record)))*sizeof(Record)) +  1 + MAX_FILE_NAME_SIZE + sizeof(DWORD));
    sectorFat = *(DWORD *)((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)) +  1 + MAX_FILE_NAME_SIZE + sizeof(DWORD));
    printf("sectorFat: %hu\n", sectorFat);
    memcpy((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)), &deletedFile, sizeof(Record));
    printf("where: %d\n", (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record))))));
    write_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer);
    currentPointer[sectorFat] = -1; //set currentPointer of the file at handler index to -1;
    do{
        if(read_sector((superblock.pFATSectorStart + (int)floor((double)sectorFat/SECTOR_SIZE)), buffer) != 0){
            return -1;
        }
        nextCluster = *(DWORD *)(buffer + (sectorFat-1)*4);
        memcpy((buffer + (sectorFat-1)*4), &clusterValue, 4);
        write_sector((superblock.pFATSectorStart + (int)floor((double)sectorFat/SECTOR_SIZE)), buffer);
        sectorFat = nextCluster;
    }while(nextCluster != 0xFFFFFFFF);
    return 0;
}


int identify2 (char *name, int size){
    int printSize, strSize;
    char names[] = "Felipe de Almeida Graeff - 00261606\nJulia Eidelwein - 00274700\n";
    printSize = snprintf(name,size,"Group:\n%s",names);
    strSize = strlen(names) + strlen("Group:\n");
    if(printSize == strSize || printSize == size){
        return 0;
    }
    return -1;
}
