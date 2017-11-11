#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "t2fs.h"
#include "apidisk.h"

#define FSYSTEM_VERSION 0x7E1
#define FSYSTEM_ID "T2FS"

int InitializedDisk = 0;
unsigned char buffer[SECTOR_SIZE];
int openFiles[10] = {-1};

typedef struct t2fs_superbloco sBlock;

sBlock *superblock;

int t2fsInit(){
    const char *temp;
    int erro;
    superblock = malloc(sizeof(sBlock));
    if((erro = read_sector(0, buffer)) != 0){
        printf("problema no read_sector: %s | %d\n", buffer, erro);
       return -1;
    }
    temp = (char *) buffer;
    strncpy(superblock->id, temp, 4);
    if(strncmp(superblock->id, FSYSTEM_ID, 4) != 0){
        printf("id com problema: %s\n", superblock->id);
        return -1;
    }
    superblock->version = *((WORD *)(buffer + 4));
    if(superblock->version != FSYSTEM_VERSION){
        printf("version com problema: 0x%X\n",superblock->version);
        return -1;
    }
    /*strncpy(&(superblock->SuperBlockSize), (buffer + 6), 2);
    strncpy(&(superblock->DiskSize), (buffer + 8), 4);
    strncpy(&(superblock->NofSectors), (buffer + 12), 4);
    strncpy(&(superblock->SectorsPerCluster), (buffer + 16), 4);
    strncpy(&(superblock->pFATSectorStart), (buffer + 20), 4);
    strncpy(&(superblock->RootDirCluster), (buffer + 24), 4);
    strncpy(&(superblock->DataSectorStart), (buffer + 28), 4);*/

    superblock->SuperBlockSize = *((WORD *)(buffer + 6));
    superblock->DiskSize = *((DWORD *)(buffer + 8));
    superblock->NofSectors = *((DWORD *)(buffer + 12));
    superblock->SectorsPerCluster = *((DWORD *)(buffer + 16));
    superblock->pFATSectorStart = *((DWORD *)(buffer + 20));
    superblock->RootDirCluster = *((DWORD *)(buffer + 24));
    superblock->DataSectorStart = *((DWORD *)(buffer + 28));

    printf("id: %s/n",superblock->id);
    printf("versao: 0x%X\n",superblock->version);
    printf("superBloco: %hu\n",superblock->SuperBlockSize);
    printf("diskSize: %hu\n",superblock->DiskSize);
    printf("nOfSectors: %hu\n",superblock->NofSectors);
    printf("SectorsPerCluster: %hu\n",superblock->SectorsPerCluster);
    printf("pFATSectorStart: %hu\n",superblock->pFATSectorStart);
    printf("RootDirCluster: %hu\n",superblock->RootDirCluster);
    printf("DataSectorStart: %hu\n",superblock->DataSectorStart);

    return 0;
}

FILE2 create2 (char *filename){
    t2fsInit();
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
