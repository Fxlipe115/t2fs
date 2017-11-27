#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "t2fs.h"
#include "apidisk.h"

#define FSYSTEM_VERSION 0x7E12
#define FSYSTEM_ID "T2FS"
#define MAX_OPENED_FILES 10

/*===== Type definitions =====*/
typedef struct t2fs_superbloco sBlock;
typedef struct t2fs_record Record;
typedef enum {FILE_TYPE_DIRECTORY, FILE_TYPE_FILE} file_type_t;

/*===== Global variables =====*/
int InitializedDisk = 0;
BYTE buffer[SECTOR_SIZE];
FILE2 openedFiles[10] = {0};
DWORD currentDir;
DWORD openedDir;
char *nominalPath;
//char nominalPath[100*MAX_FILE_NAME_SIZE];
int *currentPointer;
int currentEntry;
sBlock superblock;

/*===== Function prototypes =====*/
/* Initialize the disk, copying the values of superblock, checking if the data is consistent.
Futhermore, allocate memory for currentPointer and set the value for currentDir for root (in sectors)*/
int t2fsInit();

/* This funtion check is a path is valid.
Param 1: the path to be validated
Param 2: the type (dir or regular file) of the last token, being equal to FILE_TYPE_DIRECTORY if looking for a dir or FILE_TYPE_FILE if looking for a file.
Return: 1 if the path is not valid, the cluster number if it is */
DWORD validPath(char *filename, file_type_t fileType);

/* This funtion finds the first entry at fat with free flag.
Return: index of the entry, counting since the first entry of the first sector of the fat. -1 if no free entry found*/
DWORD firstFitFat();

/* This function finds the first unused entry at a dir.
Param 1: the cluster of the dir
Return: index of the first unused entry of the dir, counting since the first entry of the first sector of the cluster. -1 if no unused entry found*/
int unusedEntryDir(DWORD cluster);

/* Split filename into a path and a name (of the file)
Param 1: absolute or relative path to be splited
Param 2: char array which will receive the first n-1 tokens
Param 3: char array which will receive the last token (aka name of the file or dir)*/
void extractPath(char *filename, char path[], char name[MAX_FILE_NAME_SIZE]);

/* Check if there's no other file/dir with the same name
Param 1: cluster of the dir to be checked
Param 2: name of the possible doppelganger (filw with the same name as an already existing one
Return: -1 if error, 0 if no doppelganger and, if doppelganger found, the index of the entry (useful for seraching for a file)*/
int doppelganger(DWORD cluster, char name[MAX_FILE_NAME_SIZE]);

/* This function looks for the index of a file in the openedFiles array
Param 1: first cluster of the file
Return: index of the file in the array or -1 if absent */
int getFileIndex(FILE2 firstCluster);

/* This function checks if a dir has any file on its entries
Param 1: cluster of the dir
Return: 1 if empty, 0 if has at least one file and -1 if an error occured */
int isEmptyDir(DWORD cluster);

/* This function copy newPath to nominalPath if newPath is a absolute path,
or truncate nominalPath and remove the "../ ./" part of newPath to concatenate them if newPath is relative
Param 1: new path (relative)*/
void truncateAndConcat(char newPath[]);

<<<<<<< Updated upstream
/*
Param 2: old path (absolute)
Param 3: 'g' for ignoring the second parameter and use the global nominalPath, any other thing if you want o use param 2
void truncateAndConcat(char newPath[], char *pathname, char purpose);
*/
=======

>>>>>>> Stashed changes
/*===== Function implementations =====*/
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


FILE2 create2 (char *filename){
  DWORD parent, freeCluster, clusterValue = 0xFFFFFFFF;
  Record newFile;
  BYTE flush[64] = {0};
  int freeEntry;
  char path[strlen(filename) + 1], name[MAX_FILE_NAME_SIZE];
  if(!InitializedDisk){
    t2fsInit();
  }
  extractPath(filename,path,name);
  parent = validPath(path,FILE_TYPE_DIRECTORY);
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
  if(freeCluster == -1){
    return -1;
  }
  if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
    return -1;
  }
  if(read_sector((superblock.pFATSectorStart + (int)floor((double)freeCluster/SECTOR_SIZE)), buffer) != 0){
    return -1;
  }

  memcpy((buffer + (freeCluster)*4), &clusterValue,4);
  write_sector((superblock.pFATSectorStart + (int)floor((double)freeCluster/SECTOR_SIZE)), buffer);

  memcpy(&newFile, &flush, sizeof(Record));

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
  DWORD parent, clusterValue = 0x00000000, self;
  BYTE flush[64] = {0};
  int entry, sectorFat, nextCluster, i, j;
  char path[strlen(filename) + 1], name[MAX_FILE_NAME_SIZE];
  if(!InitializedDisk){
    t2fsInit();
  }
  if((self = validPath(filename,FILE_TYPE_FILE)) == 1){
    return -1;
  }
  if(getFileIndex(self) != -1){
    fprintf(stderr, "Cannot delete a opened file!\n");
    return -1;
  }
  extractPath(filename,path,name);
  parent = validPath(path,FILE_TYPE_DIRECTORY);
  if(parent == 1){
    return -1;
  }
  if(strcmp(path, filename) == 0){
    parent =  (currentDir - superblock.DataSectorStart)/superblock.SectorsPerCluster;
  }
  entry = doppelganger(parent,name);
  if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer) != 0){
    return -1;
  }

  sectorFat = *(DWORD *)((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)) +  1 + MAX_FILE_NAME_SIZE + sizeof(DWORD));
  memcpy((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)), &flush, sizeof(Record));
  write_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer);
  do{
    for(j = 0; j < 4; j++){
        if(read_sector((sectorFat*superblock.SectorsPerCluster + superblock.DataSectorStart + j), buffer) != 0){
            return -1;
        }
        for(i = 0; i < superblock.SectorsPerCluster; i++){
            memcpy(buffer + i*sizeof(Record), &flush, sizeof(Record));
        }
        write_sector((sectorFat*superblock.SectorsPerCluster + superblock.DataSectorStart + j), buffer);
    }
    if(read_sector((superblock.pFATSectorStart + (int)floor((double)sectorFat/SECTOR_SIZE)), buffer) != 0){
      return -1;
    }
    nextCluster = *(DWORD *)(buffer + (sectorFat)*4);
    memcpy((buffer + (sectorFat)*4), &clusterValue, 4);
    write_sector((superblock.pFATSectorStart + (int)floor((double)sectorFat/SECTOR_SIZE)), buffer);
    sectorFat = nextCluster;
  }while(nextCluster != 0xFFFFFFFF);
  return 0;
}


FILE2 open2 (char *filename){
  DWORD fileCluster;
  int index;
  if(!InitializedDisk){
    t2fsInit();
  }
  fileCluster = validPath(filename, FILE_TYPE_FILE);
  if(fileCluster == 1){
      return -1;
  }
  index = getFileIndex(0);//looks for free space in openedFile array
  if(index == -1){
      return -1;//already reached 10 opened files at once
  }
  openedFiles[index] = fileCluster;
  currentPointer[fileCluster] = 0;
  return fileCluster;
}


int close2 (FILE2 handle){
  int index;
  if(!InitializedDisk){
    t2fsInit();
  }
  index = getFileIndex(handle);
  if(index == -1){
    return -1;
  }
  openedFiles[index] = 0;
  return 0;
}


int read2 (FILE2 handle, char *buffer, int size){
  if(!InitializedDisk){
    t2fsInit();
  }

  int index = getFileIndex(handle);
  if(index == -1){
    return -1;
  }
  FILE2 file = openedFiles[index];

  return 0;
}


int write2 (FILE2 handle, char *buffer, int size){
  //TODO
  return 0;
}


int truncate2 (FILE2 handle){
  //TODO
  return 0;
}


int seek2 (FILE2 handle, unsigned int offset){
  //TODO
  return 0;
}


int mkdir2 (char *pathname){
  DWORD parent, freeCluster, clusterValue = 0xFFFFFFFF;
  Record father, self;
  Record newFile;
  BYTE flush[64] = {0};
  int freeEntry, i, j;
  char path[strlen(pathname) + 1], name[MAX_FILE_NAME_SIZE];
  if(!InitializedDisk){
    t2fsInit();
  }
  extractPath(pathname,path,name);
  parent = validPath(path,FILE_TYPE_DIRECTORY);
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
  if(freeCluster == -1){
    return -1;
  }
  if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
    return -1;
  }
  if(read_sector((superblock.pFATSectorStart + (int)floor((double)freeCluster/SECTOR_SIZE)), buffer) != 0){
    return -1;
  }

  memcpy((buffer + (freeCluster)*4), &clusterValue,4);
  write_sector((superblock.pFATSectorStart + (int)floor((double)freeCluster/SECTOR_SIZE)), buffer);

  memcpy(&newFile, &flush, sizeof(Record));

  newFile.TypeVal = TYPEVAL_DIRETORIO;
  strcpy(newFile.name,name);
  newFile.bytesFileSize = SECTOR_SIZE*superblock.SectorsPerCluster;
  newFile.firstCluster = freeCluster;

  freeEntry = unusedEntryDir(parent);

  read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor((freeEntry)/(SECTOR_SIZE/sizeof(Record)))), buffer);
  memcpy((buffer + (freeEntry - superblock.SectorsPerCluster*((int)floor((freeEntry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)), &newFile, sizeof(Record));
  write_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(freeEntry/(SECTOR_SIZE/sizeof(Record)))), buffer);

  for(j = 0; j < 4; j++){
    if(read_sector((freeCluster*superblock.SectorsPerCluster + superblock.DataSectorStart + j), buffer) != 0){
        return -1;
    }
    for(i = 0; i < superblock.SectorsPerCluster; i++){
        memcpy(buffer + i*sizeof(Record), &flush, sizeof(Record));
    }
    write_sector((freeCluster*superblock.SectorsPerCluster + superblock.DataSectorStart + j), buffer);
  }

  if(read_sector((freeCluster*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer) != 0){
    return -1;
  }
  memcpy(&self, &flush, sizeof(Record));
  self.TypeVal = TYPEVAL_DIRETORIO;
  strcpy(self.name, ".");
  self.bytesFileSize = superblock.SectorsPerCluster*SECTOR_SIZE;
  self.firstCluster = freeCluster;

  memcpy(&father, &flush, sizeof(Record));
  father.TypeVal = TYPEVAL_DIRETORIO;
  strcpy(father.name, "..");
  father.bytesFileSize = superblock.SectorsPerCluster*SECTOR_SIZE;
  father.firstCluster = parent;

  memcpy(buffer, &self, sizeof(Record));
  memcpy((buffer + sizeof(Record)), &father, sizeof(Record));

  write_sector((freeCluster*superblock.SectorsPerCluster + superblock.DataSectorStart), buffer);

  return 0;
}


int rmdir2 (char *pathname){
  DWORD parent, clusterValue = 0x00000000, self;
  BYTE flush[64] = {0}, typeVal;
  int entry, sectorFat, nextCluster, i, j;
  char path[strlen(pathname) + 1], name[MAX_FILE_NAME_SIZE];
  if(!InitializedDisk){
    t2fsInit();
  }
  if((self = validPath(pathname,FILE_TYPE_DIRECTORY)) == 1){
    return -1;
  }
  if(!isEmptyDir(self)){
    fprintf(stderr, "Cannot delete non empty dir!\n");
    return -1;
  }
  extractPath(pathname,path,name);
  parent = validPath(path,FILE_TYPE_DIRECTORY);
  if(parent == 1){
    return -1;
  }
  if(strcmp(path, pathname) == 0){
    parent =  (currentDir - superblock.DataSectorStart)/superblock.SectorsPerCluster;
  }
  entry = doppelganger(parent,name);
  if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer) != 0){
    return -1;
  }
  typeVal = *(DWORD *)((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)));
  if(typeVal != TYPEVAL_DIRETORIO){
    return -1;
  }

  sectorFat = *(DWORD *)((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)) +  1 + MAX_FILE_NAME_SIZE + sizeof(DWORD));
  if(currentDir == superblock.DataSectorStart + sectorFat*superblock.SectorsPerCluster){
    currentDir = superblock.DataSectorStart + superblock.RootDirCluster*superblock.SectorsPerCluster;
  }
  memcpy((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)), &flush, sizeof(Record));
  write_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer);
  do{
    for(j = 0; j < 4; j++){
        if(read_sector((sectorFat*superblock.SectorsPerCluster + superblock.DataSectorStart + j), buffer) != 0){
            return -1;
        }
        for(i = 0; i < superblock.SectorsPerCluster; i++){
            memcpy(buffer + i*sizeof(Record), &flush, sizeof(Record));
        }
        write_sector((sectorFat*superblock.SectorsPerCluster + superblock.DataSectorStart + j), buffer);
    }
    if(read_sector((superblock.pFATSectorStart + (int)floor((double)sectorFat/SECTOR_SIZE)), buffer) != 0){
      return -1;
    }
    nextCluster = *(DWORD *)(buffer + (sectorFat)*4);
    memcpy((buffer + (sectorFat)*4), &clusterValue, 4);
    write_sector((superblock.pFATSectorStart + (int)floor((double)sectorFat/SECTOR_SIZE)), buffer);
    sectorFat = nextCluster;
  }while(nextCluster != 0xFFFFFFFF);
  return 0;
}


int chdir2 (char *pathname){
  DWORD parent, self;
  BYTE typeVal;
  int entry, sectorFat;
  char path[strlen(pathname) + 1], name[MAX_FILE_NAME_SIZE];
  if(!InitializedDisk){
    t2fsInit();
  }
  //printf("started chdir: %s | %s\n", nominalPath, pathname);
  if((self = validPath(pathname,FILE_TYPE_DIRECTORY)) == 1){
    return -1;
  }
  extractPath(pathname,path,name);
  parent = validPath(path,FILE_TYPE_DIRECTORY);
  if(parent == 1){
    return -1;
  }
  if(strcmp(path, pathname) == 0){
    parent =  (currentDir - superblock.DataSectorStart)/superblock.SectorsPerCluster;
  }
  entry = doppelganger(parent,name);
  if(read_sector((parent*superblock.SectorsPerCluster + superblock.DataSectorStart + floor(entry/(SECTOR_SIZE/sizeof(Record)))), buffer) != 0){
    return -1;
  }
  typeVal = *(DWORD *)((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)));
  if(typeVal != TYPEVAL_DIRETORIO){
    fprintf(stderr, "The current dir cannot be a regular file!\n");
    return -1;
  }
  sectorFat = *(DWORD *)((buffer + (entry - superblock.SectorsPerCluster*((int)floor((entry)/(SECTOR_SIZE/sizeof(Record)))))*sizeof(Record)) +  1 + MAX_FILE_NAME_SIZE + sizeof(DWORD));
  currentDir = superblock.DataSectorStart + sectorFat*superblock.SectorsPerCluster;
  //printf("ready to truncate: %s | %s\n", nominalPath, pathname);
  free(nominalPath);
  nominalPath = malloc(sizeof(char)*strlen(pathname) + 5);
  //truncateAndConcat(pathname, "using nominalPath", 'g');
  truncateAndConcat(pathname);
  //printf("aqui\n");
  //free(nominalPath);
  //nominalPath = NULL;
  //nominalPath = (char *)malloc(sizeof(char)*(strlen(pathname) + 1));
  //nominalPath = realloc(nominalPath,(strlen(pathname)));
  //nominalPath = pathname;
  return 0;
}


int getcwd2 (char *pathname, int size){
  int nominalPathSize, strSize;
  nominalPathSize = strlen(pathname);
  if(nominalPathSize > size){
    return -1;
  }
  snprintf(pathname,size,"%s",nominalPath);
  strSize = strlen(pathname);
  if(strSize <= size){
    return 0;
  }
  return -1;
}


DIR2 opendir2 (char *pathname){
  DWORD parent, self;
  //char pathNameAux[(strlen(pathname))], *nominalPathAux;
  if(!InitializedDisk){
    t2fsInit();
  }
  //nominalPathAux = malloc(strlen(nominalPath) + strlen(pathname));
  //strcpy(pathNameAux, pathname);
  //strcpy(nominalPathAux,nominalPath);
  //printf("pathname antes: %s | %s\n", pathname, nominalPathAux);
  //truncateAndConcat(pathNameAux, pathNameAux, 'n');
  //if((self = validPath(nominalPathAux,FILE_TYPE_DIRECTORY)) == 1){
  if((self = validPath(pathname,FILE_TYPE_DIRECTORY)) == 1){
    return -1;
  }
  openedDir = self;
  currentEntry = 0;
  //printf("pathname: %s | cluster: %hu\n", nominalPathAux, openedDir);
  //printf("cluster: %hu\n",openedDir);
  return openedDir;
}


int readdir2 (DIR2 handle, DIRENT2 *dentry){
  //TODO
  return 0;
}


int closedir2 (DIR2 handle){
  openedDir = -1;
  return 0;
}


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

    currentPointer = malloc((int)sizeof(int)*((superblock.NofSectors - superblock.DataSectorStart)/superblock.SectorsPerCluster));

    //nominalPath = malloc(sizeof(char)*(strlen("/") + 1));
    //printf("ok\n");
    //strcpy(nominalPath,"/");
    //nominalPath[0] = '/';
    //nominalPath[1] = '\0';
    nominalPath = malloc(sizeof(char)*10);
    strcpy(nominalPath,"/");
    //printf("foi\n");
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


DWORD validPath(char *filename, file_type_t fileType){
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
            //if(fileType == FILE_TYPE_FILE){
            return superblock.RootDirCluster; //Valid path: root
            //} else {
              //  return 1;
           // }
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
                    return 1;//Invalid path, only one normal type file per path (none if fileType = FILE_TYPE_DIRECTORY)
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
    if(fileType == FILE_TYPE_DIRECTORY && normalFile > 0){
        return 1; //Invalid, cannot have any normal type file if it's looking for a dir
    }
    return cluster; //Valid Path, return cluster number
}


DWORD firstFitFat(){
    int sector = superblock.pFATSectorStart, entry = 0;;
    while(sector < superblock.DataSectorStart){
        if(read_sector(sector, buffer) != 0){
            return -1;
        }
        while(entry < SECTOR_SIZE/4){
            if(*((DWORD *)(buffer + entry)) == 0x00000000){
                return (sector-1)*SECTOR_SIZE + (entry/4);
            }
            entry = entry + 4;
        }
        sector++;
        entry = 0;
    }
    return -1;
}


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
                    return (numOfSectors*SECTOR_SIZE + entry - sizeof(Record))/sizeof(Record);
                }
            }
        }
        numOfSectors++;
        entry = 0;
    }
    return -1;
}


void extractPath(char *filename, char path[], char name[MAX_FILE_NAME_SIZE]){
    char auxName[strlen(filename) + 1], *safeCopy = filename, previous[strlen(filename) + 1], verify[strlen(filename) + 1];
    int endOfPath = 0;
    if(*((BYTE*)filename) == '/'){
        strcpy(path,"/");
    } else {
        strcpy(path,"");
    }
    strcpy(auxName, filename);
    safeCopy = strtok(auxName,"/");
    if(safeCopy != NULL){
        strcpy(verify, "/");
        strcat(verify,safeCopy);
        if(strcmp(filename,safeCopy) == 0 || strcmp(filename,verify) == 0){
            strcpy(previous,"./");
            strcpy(name,safeCopy);
            endOfPath = 1;
        } else {
            strcat(path,safeCopy);
            strcpy(previous,path);
            strcpy(name,safeCopy);
        }
    } else {
        endOfPath = 1;
    }
    while(!endOfPath){
        safeCopy = strtok(NULL,"/");
        if(safeCopy != NULL){
            strcpy(previous,path);
            strcat(path,"/");
            strcat(path,safeCopy);
            strcpy(name,safeCopy);
        } else {
            endOfPath = 1;
        }
    }
    strcpy(path,previous);
    //printf("name: %s | path: %s | filename: %s\n", name, path,filename);
}


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

int getFileIndex(FILE2 firstCluster){
    int i;
    for(i = 0; i < MAX_OPENED_FILES; i++){
        if(openedFiles[i] == firstCluster){
            return i;
        }
    }
    return -1;
}

int isEmptyDir(DWORD cluster){
    char name[MAX_FILE_NAME_SIZE];
    int i, j;
    for(i = 0; i < superblock.SectorsPerCluster; i++){
        if(read_sector((cluster*superblock.SectorsPerCluster + superblock.DataSectorStart + i), buffer) != 0){
            return -1; //error
        }
        for(j = 0; (j < SECTOR_SIZE/sizeof(Record)); j++){
            strncpy(name, (char *)(buffer + 1 + j*sizeof(Record)), MAX_FILE_NAME_SIZE);
            if(name != NULL){
                if(strcmp(name, "") != 0 && strcmp(name, ".") != 0 && strcmp(name, "..") != 0){
                    return 0;
                }
            }
        }
    }
   return 1;
}

//void truncateAndConcat(char newPath[], char *pathname, char purpose){
void truncateAndConcat(char newPath[]){
    char auxPath[strlen(nominalPath) + 1], auxNewPath[strlen(newPath) + 1], *safeCopy = newPath, *safeCopy2 = nominalPath, previous[strlen(newPath) + 1], previous2[strlen(nominalPath) + 1], *auxNew = newPath, intermed[strlen(newPath) + 1];
    char *endNew, *endPath;
    int truncatedBytes = 0;
    //if(purpose == 'g'){
    if(*((BYTE*)newPath) == '/'){
        strcpy(nominalPath,newPath);
        //printf("caso1\n");
    } else {
        //printf("caso2\n");
        strcpy(previous, newPath);
        //safeCopy = strtok(newPath,"/");
        strcpy(auxNewPath, newPath);
        safeCopy = strtok_r(auxNewPath, "/", &endNew);
        //printf("passou 1\n");
        while((safeCopy != NULL) && ((strcmp(safeCopy,".") == 0) || (strcmp(safeCopy,"..") == 0))){
            //strcat(previous,safeCopy);
            if((strcmp(safeCopy,"..") == 0)){
                truncatedBytes = truncatedBytes + 3;
                strcpy(auxPath, "");
                safeCopy2 = strtok_r(nominalPath, "/", &endPath);
                //printf("passou 4\n");
                while(safeCopy2 != NULL){
                    strcpy(previous2, auxPath);
                    strcat(auxPath,safeCopy2);
                    strcat(auxPath,"/");
                    safeCopy2 = strtok_r(NULL, "/", &endPath);
                }
                strcpy(nominalPath, previous2);
            } else {
                if((strcmp(safeCopy,".") == 0)){
                    //strcpy(newPath, (newPath+2));
                    truncatedBytes = truncatedBytes + 2;
                }
            }
            strcpy(previous, auxNew);
            safeCopy = strtok_r(NULL, "/", &endNew);
        }
        strncpy(intermed, (newPath + truncatedBytes), (MAX_FILE_NAME_SIZE - truncatedBytes));
        if(strcmp(nominalPath,"") == 0){
            strcpy(nominalPath,"/");
        }
        strcat(nominalPath,intermed);
    }
   /* } else {
        if(*((BYTE*)newPath) == '/'){
            strcpy(pathname,newPath);
            //printf("caso1\n");
        } else {
            //printf("caso2\n");
            strcpy(previous, newPath);
            //safeCopy = strtok(newPath,"/");
            strcpy(auxNewPath, newPath);
            safeCopy = strtok_r(auxNewPath, "/", &endNew);
            //printf("passou 1\n");
            while((safeCopy != NULL) && ((strcmp(safeCopy,".") == 0) || (strcmp(safeCopy,"..") == 0))){
                //strcat(previous,safeCopy);
                if((strcmp(safeCopy,"..") == 0)){
                    truncatedBytes = truncatedBytes + 3;
                    strcpy(auxPath, "");
                    safeCopy2 = strtok_r(pathname, "/", &endPath);
                    //printf("passou 4\n");
                    while(safeCopy2 != NULL){
                        strcpy(previous2, auxPath);
                        strcat(auxPath,safeCopy2);
                        strcat(auxPath,"/");
                        safeCopy2 = strtok_r(NULL, "/", &endPath);
                    }
                    strcpy(pathname, previous2);
                } else {
                    if((strcmp(safeCopy,".") == 0)){
                        //strcpy(newPath, (newPath+2));
                        truncatedBytes = truncatedBytes + 2;
                    }
                }
                strcpy(previous, auxNew);
                safeCopy = strtok_r(NULL, "/", &endNew);
            }
            strncpy(intermed, (newPath + truncatedBytes), (MAX_FILE_NAME_SIZE - truncatedBytes));
            if(strcmp(pathname,"") == 0){
                strcpy(pathname,"/");
            }
            strcat(pathname,intermed);
        }
    }*/
    //printf("nominalPath: %s\n", nominalPath);
}
