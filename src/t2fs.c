#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "t2fs.h"
#include "apidisk.h"

#define FSYSTEM_VERSION 0x7E12
#define FSYSTEM_ID "T2FS"

int InitializedDisk = 0;
unsigned char buffer[SECTOR_SIZE];
int openFiles[10] = {-1};
DWORD currentDir;


typedef struct t2fs_superbloco sBlock;
typedef struct t2fs_record Record;

sBlock *superblock;

int t2fsInit(){
    const char *temp;
    //Record parent, self, empty, block[4];
    superblock = malloc(sizeof(sBlock));
    if((read_sector(0, buffer)) != 0){
        return -1;
    }
    temp = (char *) buffer;
    strncpy(superblock->id, temp, 4);
    if(strncmp(superblock->id, FSYSTEM_ID, 4) != 0){
        return -1;
    }
    superblock->version = *((WORD *)(buffer + 4));
    if(superblock->version != FSYSTEM_VERSION){
        return -1;
    }

    superblock->SuperBlockSize = *((WORD *)(buffer + 6));
    superblock->DiskSize = *((DWORD *)(buffer + 8));
    superblock->NofSectors = *((DWORD *)(buffer + 12));
    superblock->SectorsPerCluster = *((DWORD *)(buffer + 16));
    superblock->pFATSectorStart = *((DWORD *)(buffer + 20));
    superblock->RootDirCluster = *((DWORD *)(buffer + 24));
    superblock->DataSectorStart = *((DWORD *)(buffer + 28));

    printf("id: %s\n",superblock->id);
    printf("versao: 0x%X\n",superblock->version);
    printf("superBloco: %hu\n",superblock->SuperBlockSize);
    printf("diskSize: %hu\n",superblock->DiskSize);
    printf("nOfSectors: %hu\n",superblock->NofSectors);
    printf("SectorsPerCluster: %hu\n",superblock->SectorsPerCluster);
    printf("pFATSectorStart: %hu\n",superblock->pFATSectorStart);
    printf("RootDirCluster: %hu\n",superblock->RootDirCluster);
    printf("DataSectorStart: %hu\n",superblock->DataSectorStart);

    currentDir = superblock->DataSectorStart + superblock->RootDirCluster*superblock->SectorsPerCluster;

    /*self.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(self.name, ".");
    self.bytesFileSize = superblock->SectorsPerCluster*SECTOR_SIZE;
    self.firstCluster = currentDir;

    parent.TypeVal = TYPEVAL_DIRETORIO;
    strcpy(parent.name, "..");
    parent.bytesFileSize = superblock->SectorsPerCluster*SECTOR_SIZE;
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
    return 0;
}

/* This funtion check is a path is valid.
    fileType is 'd' if looking for a dir, any other char if looking for a file.
    Return 1 if the path is not valid, the cluster number if it is */
WORD validPath(char *filename, char fileType){
    int lenght = strlen(filename);
    int normalFile = 0, numOfSectors = 0;
    DWORD entry = 0;
    char name[MAX_FILE_NAME_SIZE], auxName[lenght + 1], truncated[MAX_FILE_NAME_SIZE], previous[MAX_FILE_NAME_SIZE];
    char *safeCopy = filename;
    DWORD cluster = (currentDir - superblock->DataSectorStart)/superblock->SectorsPerCluster;
    strcpy(auxName, filename);
    if(strcmp(auxName, "/") == 0){
        if(read_sector((superblock->RootDirCluster*superblock->SectorsPerCluster + superblock->DataSectorStart), buffer) != 0){
            return 1;
        } else {
            if(fileType == 'd'){
                return superblock->RootDirCluster; //Valid path: root
            } else {
                return 1;
            }
        }
    }
    if(*((BYTE*)filename) == '/'){
        cluster = superblock->RootDirCluster;
    }
    strcpy(truncated,(strtok(auxName,"/")));
    if(read_sector((cluster*superblock->SectorsPerCluster + superblock->DataSectorStart), buffer) != 0){
        return 1;
    }
    entry = 0;
    while(strcmp(truncated,"") != 0){
        strncpy(name,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
        numOfSectors = 0;
        while(numOfSectors < superblock->SectorsPerCluster){
            while(strcmp(truncated,name) != 0 && entry <= SECTOR_SIZE - sizeof(Record)){
                entry = entry + sizeof(Record);
                strncpy(name,(const char*)(buffer + entry + 1), MAX_FILE_NAME_SIZE);
                if(name == NULL){
                    return 1;
                }
            }
            if(entry > SECTOR_SIZE - sizeof(Record)){
                numOfSectors++;
                if(numOfSectors < superblock->SectorsPerCluster){
                    if(read_sector((cluster*superblock->SectorsPerCluster + superblock->DataSectorStart + numOfSectors), buffer) != 0){
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
            if(read_sector((cluster*superblock->SectorsPerCluster + superblock->DataSectorStart), buffer) != 0){
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


int firstFitFat(){
    int sector = superblock->pFATSectorStart + 1;
    while(sector < superblock->DataSectorStart){
        if(read_sector(sector, buffer) != 0){
            return -1;
        }
        if(*((DWORD *)buffer) == 0x00000000){
            return sector;
        }
        sector++;
    }
    return -1;
}

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
            printf("%s\n", path);
            strcat(path,"/");
            strcat(path,safeCopy);
            strcpy(name,safeCopy);
        } else {
            endOfPath = 1;
        }
    }
    strcpy(path,previous);
}

FILE2 create2 (char *filename){
    WORD father;
    char path[strlen(filename) + 1], name[MAX_FILE_NAME_SIZE];
    if(!InitializedDisk){
        t2fsInit();
    }
    extractPath(filename,path,name);
    printf("filename: %s | path: %s | name: %s\n", filename, path, name);
    father = validPath(path,'d');
    printf("%hu\n", father);
    if(father == 1){
        return -1;
    }
    printf("first free cluster: %d\n", firstFitFat());
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
