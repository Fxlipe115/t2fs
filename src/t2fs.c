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

    currentDir = superblock->DataSectorStart + superblock->RootDirCluster;

    return 0;
}


WORD validPath(char *filename){
    int entry = 0, lenght = strlen(filename);
    BYTE type;
    const char *temp;
    char name[MAX_FILE_NAME_SIZE], auxName[lenght + 1], truncated[MAX_FILE_NAME_SIZE], backPath[lenght + 1];
    WORD father;
    int back = 0;
    strcpy(auxName, filename);
    if(strcmp(auxName, "/") == 0){
        if(read_sector((superblock->RootDirCluster + superblock->DataSectorStart), buffer) != 0){
            return -1;
        } else {
            return superblock->RootDirCluster;
        }
    }
    printf("um - %s|%s\n",filename,auxName);
    printf("cinco\n");
    strcpy(truncated,(strtok(auxName,"/")));
    strcpy(backPath,truncated);
    while(strcmp(truncated, "..") == 0){
        back++;
        printf("seis - %s | %s | %s | %s\n", filename,auxName,truncated,backPath);
        if(strcmp(backPath, filename) != 0 && strcmp(strcat(backPath,"/"), filename) != 0){
            printf("seis e meio\n");
            strcpy(truncated,(strtok(NULL,"/")));
            strcat(backPath,truncated);
        } else {
            break;
        }
        if(back > 10){
            break;
        }
    }
    /*while(strcmp(strtok(auxName,"/"), "..") == 0){
        back++;
        filename = auxName;
        printf("seis - %s\n", auxName);
        if(back > 10){
            break;
        }
    }*/
    if(read_sector(currentDir, buffer) != 0){
        printf("sete\n");
        return 1;
    }
    father = currentDir;
    while(back > 0){
        type = *((BYTE *)(buffer + sizeof(Record)));
        if(type != TYPEVAL_DIRETORIO){
            printf("oito\n");
            return 1;
        }
        if(read_sector(*((DWORD *)(buffer + sizeof(Record) + 6 + MAX_FILE_NAME_SIZE)), buffer) != 0){
            printf("nove\n");
            return 1;
        }
        printf("dez\n");
   }
   read_sector(currentDir, buffer);

    temp = (char *) buffer + sizeof(BYTE);
    strcpy(auxName,filename);
    printf("dez e meio\n");
    strcpy(truncated,(strtok(auxName,"/")));
    printf("onze\n");
    while(entry < superblock->SectorsPerCluster*SECTOR_SIZE){
        type = *((BYTE *)(buffer + entry));
        strncpy(name, (temp + entry), MAX_FILE_NAME_SIZE);
        if(name == truncated){
            printf("doze\n");
            strcpy(truncated,(strtok(auxName,"/")));
            if(type == TYPEVAL_REGULAR){
                printf("treze\n");
                if(truncated == NULL){
                    printf("catorze\n");
                    return father;
                } else {
                    printf("quinze\n");
                    return 1; //an archive does not have dirs on it;
                }
            } else {
                if(type == TYPEVAL_DIRETORIO){
                    printf("dezesseis\n");
                    father = read_sector(*((DWORD *)(buffer + entry + 6 + MAX_FILE_NAME_SIZE)), buffer);
                    if(father != 0){
                        printf("dezessete\n");
                        return 1;
                    }
                }
            }
        }
        entry = entry + sizeof(Record);
        printf("dezoito\n");
    }
    printf("dezenove\n");
    return 1;
}

int firstFitFat(){
    int sector = superblock->pFATSectorStart;
    while(sector < superblock->DataSectorStart){
        if(sector == 0x00000000){
            return sector;
        }
        sector = sector + SECTOR_SIZE;
    }
    return -1;
}

FILE2 create2 (char *filename){
    WORD father;
    if(!InitializedDisk){
        t2fsInit();
    }
    father = validPath(filename);
    printf("%hu\n", father);
    if(father == 1){
        return -1;
    }
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
