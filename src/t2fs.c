/*	IMPLEMENTAÇÃO DO SISTEMA DE ARQUIVOS T2FS

	INSTITUTO DE INFORMÁTICA - UFRGS
	Implementação do Trabalho Prático 2
	da disciplina de Sistemas Operacionais IN

	Autores:	Átila R. Costa
				Guilherme Delazeri
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/t2fs.h"
#include "../include/bitmap2.h"
#include "../include/apidisk.h"

#define ERROR_SAME_NAME " SAME_NAME: This file already exists "
#define ERROR_FULL_INODE " FULL INODE "

#define RECORD_SIZE 64

typedef struct no{
  char content[32];
  struct no *next;
  }no;

  typedef struct openFile{
    struct t2fs_record *record;
    struct t2fs_record *record_father;
    char *path;
    int seekValue;
    int record_index;
  }openFile;



/***************  VARIAVEIS GLOBAIS  ***************/
struct t2fs_superbloco superB;
int fsInit = 0;
struct t2fs_record *ROOT;
int secInodeZero;
int secBlockZero;
openFile openFiles[20];

/***************  PROTOTYPES DAS FUNCOES AUXILIARES  ***************/
no* parse_pathname(char *entry,no *output, int debug);
char* getfatherpath(char* output,char* pathname);
void createRoot();
void getInfoSuperbloco(int debug);
int writeBackInode(struct t2fs_inode *inode, int inodeNumber);
void writeBackBlock(BYTE* block, int blockNumber);
struct t2fs_inode* initInode(struct t2fs_inode * inode);
struct t2fs_record* scanDirOnBlock(BYTE* block, struct t2fs_record *record, int record_index,int *aux_counter);
struct t2fs_record* getRecordDir(char* pathname, struct t2fs_record *record);
struct t2fs_record* searchEntryOnDir(char* dirEntryName, struct t2fs_inode *inode, int blocksFileSize, struct t2fs_record *record);
struct t2fs_record* searchEntryOnBlock(char* dirEntryName, BYTE* block, struct t2fs_record *record);
struct t2fs_record* setEntry(struct t2fs_record *new_record, struct t2fs_inode *inodePai, char* name, WORD typeVal);
int checkEntry(struct t2fs_record* currentEntry, char* name);
FILE2 open2 (char *filename);
void printInode(struct t2fs_inode *inode);
int freeInode(struct t2fs_inode *inode, int inodeNumber);
void printRecord(struct t2fs_record *record);
int createNewBlock(int numNewBlock, struct t2fs_inode *inodePai, int inodePaiNumber, char* name, WORD typeVal);
int allocateBlockSingleInd(int numIndexS, int index);
int getNumberBlockSingleInd(int numIndexS, int index);
int write2 (FILE2 handle, char *buffer, int size);
int allocateBlockDoubleInd(int numIndexD,int numIndexS, int index);
int getNumberBlockDoubleInd(int numIndexD,int numIndexS, int index);
struct t2fs_record* getEntry(char* filename);
BYTE* getBlock(BYTE* block, int blockNumber);
struct t2fs_inode* getInode(int inodeNumber, struct t2fs_inode* inode);
int seek2 (FILE2 handle, DWORD offset);
int setEntryOnDir(struct t2fs_record *recordPai, BYTE typeVal, char* name, int update,struct t2fs_record *update_record);
void error(char* message);
/***************  PRIMITIVAS DA BIBLIOTECA t2fs.h  ***************/
int identify2 (char *names, int size){
    getInfoSuperbloco(0);

    char alumni[] = "Atila_Costa_261574\nGuilherme_Delazeri_261584\n";

    if(size <= sizeof(names))
        return -1;
    strncpy(names,alumni,size);

    return 0;
}
int seek2 (FILE2 handle, DWORD offset){
  openFiles[handle].seekValue = offset;
  return 0;
}
FILE2 create2 (char *filename){
    printf(">CREATE2()\n");
    //printf("\npathname = %s\n",filename);
    getInfoSuperbloco(0);
    struct t2fs_record *record;
    char path[50], pathhhh[50];
    int hand, isRoot=0 , entryStatus;
    strcpy(path,filename);
    strcpy(pathhhh,filename);
    record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));

    printf("\npathname in create = %s\n",filename);
    record = getRecordDir(filename,record);
    printf("PAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAI\n");
    printRecord(record);
    no *no_aux;
    no *output = (no*) malloc(sizeof(no));

    output = parse_pathname(path,output ,0);
    no_aux = output;
    //printf("\npathname = %s\n",pathname);
    if (no_aux == NULL)
        return -1;
    if(no_aux != NULL)
      do{
        output = no_aux;
        printf("------%s\n",no_aux->content );
        no_aux = no_aux->next;
      }while(no_aux != NULL);

      if(record != NULL){
          printf("Diretorio pai = %s\n", record->name);
          if (record->inodeNumber == 0)
              isRoot = 1;
      }
      else
          printf("Esse caminho nao existe\n");

    printf("no_aux->content = %s\n", output->content);
    //if(output->next == NULL)
    //    printf("\tprox eh null\n");
    entryStatus = setEntryOnDir(record, TYPEVAL_REGULAR, output->content,0,NULL);
    if(entryStatus == 1){
        record->blocksFileSize += 1;
        printf("atualiza blocksFileSize +++++\n");
    }
    else if (entryStatus == -1){
            error(ERROR_SAME_NAME);
            return -1;
        }

    char* pathFather = (char *) malloc(SECTOR_SIZE);
    if(isRoot == 0){
        printf("RecordFather Updated:\n");
        printRecord(record);
        ////getchar();
        pathFather = getfatherpath(pathFather,pathhhh);
        hand = opendir2(pathFather);
        openFiles[hand].record = record;
        closedir2(hand);
    }
    else
        ROOT->bytesFileSize += 64;

    return 0;    //Error
}

int write2 (FILE2 handle, char *buffer, int size){
    getInfoSuperbloco(0);
    printf("write2()\n");

    int seek = openFiles[handle].seekValue;
    int maxBlocks = openFiles[handle].record->blocksFileSize;
    int bytesOnFile = openFiles[handle].record->bytesFileSize;
    int writtenBytes=0, iBlock, seekOnBlock, numBlock, seekStarter = seek;

    BYTE* block = (BYTE *)malloc(16*SECTOR_SIZE);
    struct t2fs_inode* inode = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode));
    inode = getInode(openFiles[handle].record->inodeNumber, inode);

    iBlock = seek / 4096;
    int i;
    while (writtenBytes < size)
    {
        if (iBlock < 2) {
            if (inode->dataPtr[i] == INVALID_PTR){
                printf("Precisou alocar mais um bloco em dataPtr:\n");
                numBlock = createNewBlock(iBlock, inode, openFiles[handle].record->inodeNumber, "", 0);
                openFiles[handle].record->blocksFileSize += 1;
                block = getBlock(block, numBlock);
                printInode(inode);
            }
            else
                block = getBlock(block, inode->dataPtr[i]);

            seekOnBlock = seek % 4096;
            while (writtenBytes < size && (seek / 4096) <= iBlock) {
                memcpy(&block[seekOnBlock], &buffer[writtenBytes], 1);
                writtenBytes++;
                seek++;
                seekOnBlock = seek % 4096;
            }
        }
        else
        if (iBlock < 1026) {
            if ((numBlock = getNumberBlockSingleInd(inode->singleIndPtr, iBlock-2)) == INVALID_PTR){
                printf("Precisou alocar mais um bloco em singleIndPtr:\n");
                numBlock = createNewBlock(iBlock, inode, openFiles[handle].record->inodeNumber, "", 0);
                openFiles[handle].record->blocksFileSize += 1;
                block = getBlock(block, numBlock);
                printInode(inode);
            }
            else{
                numBlock = getNumberBlockSingleInd(inode->singleIndPtr, iBlock-2);
                block = getBlock(block, numBlock);
            }

            seekOnBlock = seek % 4096;
            while (writtenBytes < size && (seek / 4096) <= iBlock) {
                memcpy(&block[seekOnBlock], &buffer[writtenBytes], 1);
                writtenBytes++;
                seek++;
                seekOnBlock = seek % 4096;
            }
        }
        else{
            if ((numBlock = getNumberBlockDoubleInd(inode->doubleIndPtr, (iBlock-1026)/1024, (iBlock-1026)%1024)) == INVALID_PTR){
                printf("Precisou alocar mais um bloco em doubleIndPtr:\n");
                numBlock = createNewBlock(iBlock, inode, openFiles[handle].record->inodeNumber, "", 0);
                openFiles[handle].record->blocksFileSize += 1;
                block = getBlock(block, numBlock);
                printInode(inode);
            }
            else{
                numBlock = getNumberBlockDoubleInd(inode->doubleIndPtr, (iBlock-1026)/1024, (iBlock-1026)%1024);
                block = getBlock(block, numBlock);
            }

            seekOnBlock = seek % 4096;
            while (writtenBytes < size && (seek / 4096) <= iBlock) {
                memcpy(&block[seekOnBlock], &buffer[writtenBytes], 1);
                writtenBytes++;
                seek++;
                seekOnBlock = seek % 4096;
            }
        }
    }

    printf("\nWRITTEN: writtenBytes= %d/%d | seek = %d | bytesFileSize = %d\n",writtenBytes,size,seek,bytesOnFile);

    if (seekStarter == bytesOnFile)
        openFiles[handle].record->bytesFileSize += size;
    else
    if (seekStarter < bytesOnFile && (seekStarter+size) > bytesOnFile)
        openFiles[handle].record->bytesFileSize += (seekStarter+size) - bytesOnFile;

    openFiles[handle].seekValue = seek;
    printf("WRITTEN(upd): writtenBytes= %d/%d | seek = %d | bytesFileSize = %d\n",writtenBytes,size,seek,bytesOnFile);

    return 0;
}

int delete2 (char *filename)
{
  getInfoSuperbloco(0);
  char* name = (char*) malloc (32);
  char* name_aux = (char*) malloc (32);
  strcpy(name,filename);
  char* name_forget = (char*) malloc (32);
  strcpy(name_forget,filename);
  strcpy(name_aux,filename);
  char* name_aux2 = (char*) malloc (32);
  char* name_aux3 = (char*) malloc (32);
  name_aux2 = getfatherpath(name_aux2,filename);
  name_aux3 = getfatherpath(name_aux3,filename);
  struct t2fs_record *record_father_father;
  struct t2fs_record *record_father;

  record_father = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record_father = getRecordDir(name_aux,record_father);

  no *no_aux;

  no *output = (no*) malloc(sizeof(no));

  no *grand_father = (no*) malloc(sizeof(no));

  output = parse_pathname(name,output ,0);
  no_aux = output;
  //printf("\npathname = %s\n",pathname);
  if (no_aux == NULL)
      return -1;
  if(no_aux != NULL)
    do{
      output = no_aux;
      printf("------%s\n",no_aux->content );
      no_aux = no_aux->next;
    }while(no_aux != NULL);

  record_father_father = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record_father_father = getRecordDir(name_aux2,record_father_father);
  printf("\nprintando_pai\n");
  printRecord(record_father);
  printf("\nprintando_pai_do_pai\n");
  printRecord(record_father_father);

  grand_father = parse_pathname(name_aux3,grand_father ,0);
  no_aux = grand_father;
  //printf("\npathname = %s\n",pathname);
  if (no_aux == NULL)
      return -1;
  if(no_aux != NULL)
    do{
      grand_father = no_aux;
      printf("------%s\n",no_aux->content );
      no_aux = no_aux->next;
    }while(no_aux != NULL);

  struct t2fs_record *record;
  record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record = getEntry(name_forget);
  //printRecord(record);
  if(record->TypeVal != TYPEVAL_REGULAR){
    printf("\n%s is not a valid file path",name);
    return -1;
  }
  //printf("\nprintando_filho\n");
  //printRecord(record);
  int i;
  struct t2fs_record *record_file = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  memcpy(record_file,record,sizeof(struct t2fs_record));
  struct t2fs_inode* inode = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode));
  freeInode(getInode(record->inodeNumber,inode),record->inodeNumber);
  record_file->TypeVal = TYPEVAL_INVALIDO;
  if(setEntryOnDir(record_father,TYPEVAL_REGULAR, output->content, 1,record_file) != 0)
    return -1;

  memcpy(record_file,record_father,sizeof(struct t2fs_record));
  record_file->bytesFileSize = record_file->bytesFileSize - 64;
  //printf("ALTERANDO O PAI\n" );
  if(setEntryOnDir(record_father_father,TYPEVAL_DIRETORIO, grand_father->content, 1,record_file) != 0)
    return -1;

  return 0;
}

FILE2 open2 (char *filename)
{
    getInfoSuperbloco(0);
    printf(">open2()\n");
    char* name = (char*) malloc (32);
    char* name_aux = (char*) malloc (32);
    strcpy(name,filename);
    strcpy(name_aux,filename);
    struct t2fs_record *record_father;
    record_father = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    record_father = getRecordDir(name_aux,record_father);


    struct t2fs_record *record;
    record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    record = getEntry(filename);
    if(record == NULL){
      printf("arquivo não existe\n");
      return -1;
    }
    if(record->TypeVal != TYPEVAL_REGULAR){
        printf("\n%s is not a valid file path",name);
        return -1;
    }
    int i;
    struct t2fs_record *record_file = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    memcpy(record_file,record,sizeof(struct t2fs_record));
    printf ("ABRI\n");
    printRecord(record_file);

    for(i = 0;i<20;i++){
        if(openFiles[i].record == NULL){
            openFiles[i].record = record_file;
            openFiles[i].record_father = record_father;
            openFiles[i].path = name;
            openFiles[i].seekValue = 0;
            openFiles[i].record_index = 0;
            printf("---------------------%s-------------------\n", openFiles[i].path);
            //printRecord(record_file);
            //printf("\n-----------------------------------------\n");
            //printRecord(openFiles[0].record);
            return i;
        }
    }

  return -1;
}


int close2 (FILE2 handle){
    getInfoSuperbloco(0);
    char* path = (char *) malloc (32);
    strcpy(path,openFiles[handle].path);
    no *no_aux;
    no *output = (no*) malloc(sizeof(no));

    output = parse_pathname(path,output ,0);
    no_aux = output;
    //printf("\npathname = %s\n",pathname);
    if (no_aux == NULL)
        return -1;
    if(no_aux != NULL)
      do{
        output = no_aux;
        printf("------%s\n",no_aux->content );
        no_aux = no_aux->next;
      }while(no_aux != NULL);

    struct t2fs_record *closed = (struct t2fs_record *) malloc (sizeof(struct t2fs_record));
    memcpy (closed,openFiles[handle].record,sizeof(struct t2fs_record));
    //closed->bytesFileSize=666;
    if(setEntryOnDir(openFiles[handle].record_father,TYPEVAL_REGULAR, output->content, 1,closed) != 0)
      return -1;
    openFiles[handle].record_father = NULL;
    openFiles[handle].record = NULL;
    openFiles[handle].path = NULL;
    openFiles[handle].seekValue = 0;
    return 0;
    return -1;    //Error
}

int read2 (FILE2 handle, char *buffer, int size){
    getInfoSuperbloco(0);
    printf(">read2()\n");
    int seek = openFiles[handle].seekValue, i, readBytes=0, numBlock;
    int maxBlocks = openFiles[handle].record->blocksFileSize;
    int maxBytes = openFiles[handle].record->bytesFileSize;
    struct t2fs_inode* inode = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode));
    BYTE* block = (BYTE *) malloc(16*SECTOR_SIZE);

    inode = getInode(openFiles[handle].record->inodeNumber,inode);
    //printf("oi\n");
    i = seek / (16*SECTOR_SIZE);
    while (i < maxBlocks && seek < maxBytes && readBytes < size) {
        if(i < 2)   //Ponteiros diretos
        {
            //printf("i = %d | readBytes = %d | seek = %d\n",i,readBytes,seek);
            block = getBlock(block,inode->dataPtr[i]);
            memcpy(&buffer[readBytes],(BYTE *) &block[seek-(i*4096)], sizeof(BYTE));
            readBytes++;
            seek++;
            openFiles[handle].seekValue = seek;
        }
        else
        if(i < 1026)    //Ponteiros indiretos simples
        {
            //printf("i = %d | readBytes = %d | seek = %d\n",i,readBytes,seek);
            numBlock = getNumberBlockSingleInd(inode->singleIndPtr,i-2);
            block = getBlock(block, numBlock);
            memcpy(&buffer[readBytes],(BYTE *) &block[seek-(i*4096)], sizeof(BYTE));
            readBytes++;
            seek++;
            openFiles[handle].seekValue = seek;
        }
        else
        {
            //printf("i = %d | readBytes = %d | seek = %d\n",i,readBytes,seek);
            numBlock = getNumberBlockDoubleInd(inode->doubleIndPtr, (i-1026)/1024, (i-1026)%1024);
            block = getBlock(block, numBlock);
            memcpy(&buffer[readBytes],(BYTE *) &block[seek-(i*4096)], sizeof(BYTE));
            readBytes++;
            seek++;
            openFiles[handle].seekValue = seek;
        }
        if (seek-(i*4096) == 4096)
            i++;
    }

    //printf("read2: size=%d | readBytes=%d | seek=%d\n", size,readBytes,seek);
    //printf("tam = %d\n", maxBytes);
    return 0;    //Error
}



int truncate2 (FILE2 handle){
    getInfoSuperbloco(0);

    return -1;    //Error
}

int mkdir2 (char *pathname){
    //printf("\npathname = %s\n",pathname);
    getInfoSuperbloco(0);
    struct t2fs_record *record;
    char path[50], pathhhh[50];
    int hand, isRoot=0, entryStatus;
    strcpy(path,pathname);
    strcpy(pathhhh,pathname);
    record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));

    printf("\npathname in mkdir = %s\n",pathname);
    record = getRecordDir(pathname,record);

    no *no_aux;
    no *output = (no*) malloc(sizeof(no));

    output = parse_pathname(path,output ,0);
    no_aux = output;
    //printf("\npathname = %s\n",pathname);
    if (no_aux == NULL)
        return -1;
    if(no_aux != NULL)
      do{
        output = no_aux;
        printf("------%s\n",no_aux->content );
        no_aux = no_aux->next;
      }while(no_aux != NULL);

    if(record != NULL){
        printf("Diretorio pai = %s\n", record->name);
        if (record->inodeNumber == 0)
            isRoot = 1;
    }
    else
        printf("Esse caminho nao existe\n");

    printf("no_aux->content = %s\n", output->content);
    //if(output->next == NULL)
    //    printf("\tprox eh null\n");
    entryStatus = setEntryOnDir(record, TYPEVAL_DIRETORIO, output->content,0,NULL);
    if(entryStatus == 1){
        record->blocksFileSize += 1;
        printf("atualiza blocksFileSize +++++\n");
        //
    }
    else if(entryStatus == -1){
            error(ERROR_SAME_NAME);
            return -1;
        }
    char* pathFather = (char *) malloc(SECTOR_SIZE);
    if(isRoot == 0){
        printf("RecordFather Updated:\n");
        printRecord(record);
        ////getchar();
        pathFather = getfatherpath(pathFather,pathhhh);
        hand = opendir2(pathFather);
        openFiles[hand].record = record;
        closedir2(hand);
    }
    else
        ROOT->bytesFileSize += 64;

    return 0;    //Error
}

int rmdir2 (char *pathname){
  getInfoSuperbloco(0);
  printf("remdir============================\n");
  char* name = (char*) malloc (32);
  char* name_aux = (char*) malloc (32);
  strcpy(name,pathname);
  char* name_forget = (char*) malloc (32);
  strcpy(name_forget,pathname);
  strcpy(name_aux,pathname);
  char* name_aux2 = (char*) malloc (32);
  char* name_aux3 = (char*) malloc (32);
  name_aux2 = getfatherpath(name_aux2,pathname);
  name_aux3 = getfatherpath(name_aux3,pathname);
  struct t2fs_record *record_father_father;
  struct t2fs_record *record_father;

  record_father = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record_father = getRecordDir(name_aux,record_father);

  no *no_aux;

  no *output = (no*) malloc(sizeof(no));

  no *grand_father = (no*) malloc(sizeof(no));

  output = parse_pathname(name,output ,0);
  no_aux = output;
  //printf("\npathname = %s\n",pathname);
  if (no_aux == NULL)
      return -1;
  if(no_aux != NULL)
    do{
      output = no_aux;
      printf("------%s\n",no_aux->content );
      no_aux = no_aux->next;
    }while(no_aux != NULL);

  record_father_father = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record_father_father = getRecordDir(name_aux2,record_father_father);
  //printf("\nprintando_pai\n");
  //printRecord(record_father);
  //printf("\nprintando_pai_do_pai\n");
  //printRecord(record_father_father);

  grand_father = parse_pathname(name_aux3,grand_father ,0);
  no_aux = grand_father;
  //printf("\npathname = %s\n",pathname);
  printf("---e---%s\n",no_aux->content );
  if (no_aux == NULL)
      return -1;
  if(no_aux != NULL)
    do{
      grand_father = no_aux;
      //printf("------%s\n",no_aux->content );
      no_aux = no_aux->next;
    }while(no_aux != NULL);
    printf("---s---%s\n",no_aux->content );
  struct t2fs_record *record;
  record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record = getEntry(name_forget);
  if(record->TypeVal != TYPEVAL_DIRETORIO){
    printf("\n%s is not a valid file path",name);
    return -1;
  }
  printf("------%s\n",no_aux->content );
  if(record->bytesFileSize != 0)
    return -1;
  int i;
  printf("vaialterar\n" );
  struct t2fs_record *record_file = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  memcpy(record_file,record,sizeof(struct t2fs_record));
  struct t2fs_inode* inode = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode));
  freeInode(getInode(record->inodeNumber,inode),record->inodeNumber);
  printf("------%s\n",no_aux->content );
  record_file->TypeVal = TYPEVAL_INVALIDO;
  if(setEntryOnDir(record_father,TYPEVAL_DIRETORIO, output->content, 1,record_file) != 0)
    return -1;

  memcpy(record_file,record_father,sizeof(struct t2fs_record));
  record_file->bytesFileSize = record_file->bytesFileSize - 64;
  printf("ALTERANDO O PAI\n" );
  printRecord(record_file);
  if(setEntryOnDir(record_father_father,TYPEVAL_DIRETORIO, grand_father->content, 1,record_file) != 0)
    return -1;

  return 0;
}

DIR2 opendir2 (char *pathname){
    getInfoSuperbloco(0);
    printf(">opendir2()\n");
    //char* filename = pathname;
    char* name = (char*) malloc (32);
    char* name_aux = (char*) malloc (32);
    strcpy(name,pathname);
    strcpy(name_aux,pathname);
    struct t2fs_record *record_father;
    //printf("pathname = %s\n", pathname);
    record_father = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    record_father = getRecordDir(name_aux,record_father);
    struct t2fs_record *record;
    record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    record = getEntry(pathname);

    if(record->TypeVal != TYPEVAL_DIRETORIO){
      printf("\n%s is not a valid directory path",name);
      return -1;
    }
    int i;
    struct t2fs_record *record_file = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    memcpy(record_file,record,sizeof(struct t2fs_record));

    printRecord(record_file);
    //printRecord(record_file);
    for(i = 0;i<20;i++){
      if(openFiles[i].record == NULL){
        openFiles[i].record = record_file;
        openFiles[i].record_father = record_father;
        openFiles[i].path = name;
        openFiles[i].seekValue = 0;
        printf("---------------------%s-------------------\n", openFiles[i].path);
        //printRecord(record_file);
        //printf("\n-----------------------------------------\n");
        //printRecord(openFiles[0].record);
        return i;
      }
    }
    return -1;    //Error
}




int readdir2 (DIR2 handle, DIRENT2 *dentry){
    getInfoSuperbloco(0);
    int j = 0;
    int *aux_counter;
    aux_counter = &j;
    DIRENT2 *dentry_aux;
    dentry_aux = dentry;
    //printf("\n =======%s\n",openFiles[handle].path);

    int record_index = openFiles[handle].record_index;
    int blocksFileSize = openFiles[handle].record->blocksFileSize;
    struct t2fs_record *record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
    struct t2fs_inode *inode = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode *));
    inode = getInode(openFiles[handle].record->inodeNumber, inode);
        int i=0, numBlock;
        BYTE* block = (BYTE *) malloc(superB.blockSize*SECTOR_SIZE);
        //printf(">searchEntryOnDir1()\n");
        //printf("%d\n",blocksFileSize );
        //Percorre todos os blocos do arquivo procurando por dirEntryName
        while (i < blocksFileSize)
        {
            if (i<2) {  //Blocos diretos
                //printInode(inode);
                block = getBlock(block, inode->dataPtr[i]);
                //printf("%d\n",*aux_counter );
                //printf("%d\n",record_index );
                record = scanDirOnBlock(block, record,record_index, aux_counter);
                //printf("saiu\n");
                if(record != NULL){
                    strcpy(dentry_aux->name,record->name);
                    dentry_aux->fileType = record->TypeVal;
                    printf("%d\n", dentry_aux->fileType);
                    dentry_aux->fileSize = record->bytesFileSize;
                    openFiles[handle].record_index = openFiles[handle].record_index + 1;
                    printf("---DO READDIR2\n");
                    printRecord(record);
                    return 0;}
            }
            else if (i < 1026) {    //Blocos simples indiretos
                numBlock = getNumberBlockSingleInd(inode->singleIndPtr, i-2);
                block = getBlock(block, numBlock);
                record = scanDirOnBlock(block, record,record_index, aux_counter);
                if(record != NULL){
                    strcpy(dentry_aux->name,record->name);
                    dentry_aux->fileType = record->TypeVal;
                    dentry_aux->fileSize = record->bytesFileSize;
                    openFiles[handle].record_index = openFiles[handle].record_index + 1;
                    printRecord(record);
                    return 0;
                  }
                }

                else{   //Blocos duplo indiretos
                    numBlock = getNumberBlockDoubleInd(inode->doubleIndPtr, (i-1026)/1024, (i-1026)%1024);
                    block = getBlock(block, numBlock);
                    record = scanDirOnBlock(block, record,record_index, aux_counter);
                    if(record != NULL){
                        strcpy(dentry_aux->name,record->name);
                        dentry_aux->fileType = record->TypeVal;
                        dentry_aux->fileSize = record->bytesFileSize;
                        openFiles[handle].record_index = openFiles[handle].record_index + 1;
                        printRecord(record);
                        return 0;
                      }
                }
            i++;
        }
        openFiles[handle].record_index = 0;
        return -1;
    }

int closedir2 (DIR2 handle){
  getInfoSuperbloco(0);
  printf("closedir2()\n");
  char* path = (char *) malloc (32);
  strcpy(path,openFiles[handle].path);
  no *no_aux;
  no *output = (no*) malloc(sizeof(no));

  output = parse_pathname(path,output ,0);

  no_aux = output;
  //printf("\npathname = %s\n",pathname);
  if (no_aux == NULL)
      return -1;
  if(no_aux != NULL)
    do{
      output = no_aux;
      printf("------%s\n",no_aux->content );
      no_aux = no_aux->next;
    }while(no_aux != NULL);

  struct t2fs_record *closed = (struct t2fs_record *) malloc (sizeof(struct t2fs_record));
  memcpy (closed,openFiles[handle].record,sizeof(struct t2fs_record));
  //closed->bytesFileSize=666;

  if(setEntryOnDir(openFiles[handle].record_father,TYPEVAL_DIRETORIO, output->content, 1,closed) != 0)
    return -1;
  openFiles[handle].record_father = NULL;
  openFiles[handle].record = NULL;
  openFiles[handle].path = NULL;
  openFiles[handle].seekValue = 0;
  return 0;

    return -1;    //Error
}




/********************  FUNÇÕES AUXILIARES  ********************/
void getInfoSuperbloco(int debug) {
    int n;
    if(!fsInit){
        for (n = 0; n < 20; n++) {
          openFiles[n].record = NULL;
          openFiles[n].path = NULL;
          openFiles[n].seekValue = 0;
        }

        BYTE superblock[SECTOR_SIZE];
        WORD *pointer2bytes;
        DWORD *pointer4bytes;

        read_sector (0,superblock);

        superB.id[0]=superblock[0];
        superB.id[1]=superblock[1];
        superB.id[2]=superblock[2];
        superB.id[3]=superblock[3];

        pointer2bytes=(WORD *)(&(superblock[4]));
        superB.version=*pointer2bytes;

        pointer2bytes=(WORD *)(&(superblock[6]));
        superB.superblockSize=*pointer2bytes;

        pointer2bytes=(WORD *)(&(superblock[8]));
        superB.freeBlocksBitmapSize=*pointer2bytes;

        pointer2bytes=(WORD *)(&(superblock[10]));
        superB.freeInodeBitmapSize=*pointer2bytes;

        pointer2bytes=(WORD *)(&(superblock[12]));
        superB.inodeAreaSize=*pointer2bytes;

        pointer2bytes=(WORD *)(&(superblock[14]));
        superB.blockSize=*pointer2bytes;

        pointer4bytes=(DWORD *)(&(superblock[16]));
        superB.diskSize=*pointer4bytes;

        secInodeZero = superB.superblockSize + superB.freeBlocksBitmapSize + superB.freeInodeBitmapSize;
        secBlockZero = superB.superblockSize + superB.freeBlocksBitmapSize + superB.freeInodeBitmapSize + superB.inodeAreaSize;
        createRoot();
    }

    if(debug && !fsInit){
        printf("SUPERBLOCK DESCRIPTION:\n");
        printf("id: %c%c%c%c\n", superB.id[0],superB.id[1],superB.id[2],superB.id[3]);
        printf("version: %d\n", superB.version);
        printf("superblockSize: %d\n", superB.superblockSize);
        printf("freeBlocksBitmapSize: %d\n", superB.freeBlocksBitmapSize);
        printf("freeInodeBitmapSize: %d\n", superB.freeInodeBitmapSize);
        printf("inodeAreaSize: %d\n", superB.inodeAreaSize);
        printf("blockSize: %d\n", superB.blockSize);
        printf("diskSize: %d\n\n", superB.diskSize);
    }

    fsInit = 1;     //Seta para 1 para nao coletar novamente os dados
}

int writeBackInode(struct t2fs_inode *inode, int inodeNumber){ //ok
    printf(">writeBackInode()\n");
    //printf("inodeNumber = %d\n", inodeNumber);
    //printf("inodeNumber mod 16 = %d\n", (inodeNumber % 16));
    int inodeNumSector;
    BYTE* inodeSector = (BYTE *) malloc(SECTOR_SIZE);

    inodeNumSector = secInodeZero + (inodeNumber/16);
    //printf("inodeNumSector = %d\n", inodeNumSector);
    read_sector(inodeNumSector, inodeSector);
    //printf("index of inodeSector = %d\n", ((inodeNumber % 16) * 16));
    memcpy(&inodeSector[(inodeNumber % 16) * 16], (BYTE *)inode, sizeof(struct t2fs_inode));
    write_sector(inodeNumSector, inodeSector);

    //struct t2fs_inode* i = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode));
    /*getInode(inodeNumber,i);
    printInode(i);*/
    //printf("fimDaWriteBackI\n");

    return 0;
}

struct t2fs_record* getRecordDir(char* pathname, struct t2fs_record *record)
{
    printf(">getRecordDir()\n");
    struct t2fs_inode *inode, *i;
    int nextInode;

    nextInode = 0;

    //Transforma o pathname em uma LSE
    no *path = (no*) malloc(sizeof(no)), *first;
    printf("\npathname in get = %s\n",pathname);
    path = parse_pathname(pathname,path,0);
    printf("\npathname in get = %s\n",pathname);
    first = path;

    if (path == NULL)   //Se for NULL, não é um caminho válido
        return NULL;
    else
    if (path->next == NULL) //Diretório pai é a Root
        return ROOT;
    else
    if (path->next->next == NULL){   //Diretório pai está na root
        i = (struct t2fs_inode *) malloc(sizeof(struct t2fs_inode));
        i = getInode(nextInode, i);
        //printInode(i);
        //printRecord(record);
        record = searchEntryOnDir(path->content, i, ROOT->blocksFileSize, record);
        if (record == NULL)
            return NULL;
    }
    else
    {   //É preciso navegar nos diretórios para achar o dir pai
        //record = ROOT;
        while (path->next != NULL)
        {
            inode = getInode(nextInode, inode);
            record = searchEntryOnDir(path->content, inode, record->blocksFileSize, record);

            if (record == NULL)
                return NULL;
            nextInode = record->inodeNumber;
            path = path->next;
        }
    }

    //Desaloca estruturas:
    //free(inode);      //Se descomentar da pau
    while (first != NULL) {
        path = first;
        first = first->next;
        free(path);

    }
    return record;
}
struct t2fs_record* getEntry(char* filename){
  printf(">getEntry()\n");
  if(strlen(filename)==1)
    return ROOT;
  struct t2fs_record *record;
  char path[50],pathAux[50];
  strcpy(path,filename);
  strcpy(pathAux,filename);
  record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  record = getRecordDir(filename,record);
  no *no_aux;
  no *output = (no*) malloc(sizeof(no));
  output = parse_pathname(path,output ,0);
  no_aux = output;
  if (no_aux == NULL)
      return NULL;
  if(no_aux != NULL)
    do{
      output = no_aux;
      no_aux = no_aux->next;
    }while(no_aux != NULL);

  if(record != NULL)
      printf("Diretorio pai = %s\n", record->name);
  else
      printf("Esse caminho nao existe\n");
      //--------------------------------------------------------------//
  struct t2fs_record *record_file = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
  struct t2fs_inode *inode = (struct t2fs_inode *)malloc(sizeof(struct t2fs_inode *));
  //printf("\n%s\n%d\n",output->content,record->blocksFileSize);
  inode = getInode(record->inodeNumber, inode);
  record_file = searchEntryOnDir(output->content,inode, record->blocksFileSize, record_file);
  //printRecord(record_file);
  return record_file;
  }


struct t2fs_record* searchEntryOnDir(char* dirEntryName, struct t2fs_inode *inode, int blocksFileSize, struct t2fs_record *record)
{
    printf(">searchEntryOnDir()\n");
    int i=0, numBlock;
    BYTE* block = (BYTE *) malloc(superB.blockSize*SECTOR_SIZE);
    printf(">searchEntryOnDir1()\n");
    printf("%d\n",blocksFileSize );
    //Percorre todos os blocos do arquivo procurando por dirEntryName
    while (i < blocksFileSize)
    {
        if (i<2) {  //Blocos diretos
            //printInode(inode);
            block = getBlock(block, inode->dataPtr[i]);
            record = searchEntryOnBlock(dirEntryName, block, record);
            if(record != NULL){
                return record;}
        }
        else if (i < 1026) {    //Blocos simples indiretos
            numBlock = getNumberBlockSingleInd(inode->singleIndPtr, i-2);
            block = getBlock(block, numBlock);
            if((record = searchEntryOnBlock(dirEntryName, block, record)) != NULL)
                return record;
            }
            else{   //Blocos duplo indiretos
                numBlock = getNumberBlockDoubleInd(inode->doubleIndPtr, (i-1026)/1024, (i-1026)%1024);
                block = getBlock(block, numBlock);
                if((record = searchEntryOnBlock(dirEntryName, block, record)) != NULL)
                    return record;
            }
        i++;
    }
    return NULL;
}

struct t2fs_record* searchEntryOnBlock(char* dirEntryName, BYTE* block, struct t2fs_record *record)
{
    printf(">searchEntryOnBlock()\n");
    int i;

    for(i=0; i<64; i++)
    {
        record = (struct t2fs_record *) &(block[i*RECORD_SIZE]);
        printf("%d TypeVal: %s %d\n",i, record->name, record->TypeVal);
        //GARANTIR QUE É DIRETORIO ANTES // TO USANDO PRA ARQUIVOS
        if (record->TypeVal != TYPEVAL_INVALIDO){
            if(strcmp(record->name, dirEntryName) == 0)
            {
                printf("-name == dirEntryName == %s\n", record->name);
                return record;
            }
        }
    }

    //free(record);
    return NULL;
}

int setEntryOnDir(struct t2fs_record *recordPai, BYTE typeVal, char* name, int update, struct t2fs_record *update_record)
{
    printf(">setEntryOnDir()\n");
    BYTE *block;
    struct t2fs_inode *inodePai;
    int i=0, j, infoEntry, indexBlockNewRec, indexNewRecord, notInserted=1, numBlock;
    struct t2fs_record *new_record;

    block = (BYTE *) malloc(sizeof(superB.blockSize*SECTOR_SIZE));
    inodePai = (struct t2fs_inode *) malloc(sizeof(struct t2fs_inode));
    new_record = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));

    inodePai = getInode(recordPai->inodeNumber, inodePai);

    printf("recordPai:\n");
    //printRecord(recordPai);
    while (i < recordPai->blocksFileSize){
        if(i < 2){
            block = getBlock(block, inodePai->dataPtr[i]);
            inodePai = getInode(recordPai->inodeNumber, inodePai);
            printf("\ni = %d\n", i);

            for(j=0; j<64; j++){
                //printf("\nrecord = %d\n", j);
                //printf("\nblockfilesize = %d\n", recordPai->blocksFileSize);
                //getchar();
                new_record = (struct t2fs_record *) &block[j*RECORD_SIZE];
                //printf("tentando inserir '%s' na entrada do diretorio = '%s'\n",name, new_record->name);
                infoEntry = checkEntry(new_record, name);

                //printf("infoEntry %d\t(0=entrada valida; 1=pode inserir; -1=nomes iguais) \n", infoEntry);
                if((infoEntry == 1 && notInserted)||(infoEntry == -1 && update)){
                      printf("PODE INSERIR AQUI!!!!!!\n");
                      indexBlockNewRec = inodePai->dataPtr[i];
                      indexNewRecord = j;
                      notInserted = 0;
                      if(update){
                        printf("UPDATE!!!!!!!!!\n");
                        block = getBlock(block, indexBlockNewRec);
                        new_record = update_record;
                        //printRecord(new_record);
                        memcpy(&block[indexNewRecord*RECORD_SIZE],(BYTE *) new_record, RECORD_SIZE);
                        writeBackBlock(block,indexBlockNewRec);
                        //
                        return 0;   //Indica que não foi adicionado mais um bloco
                      }
                    }
                else
                if(infoEntry == -1)   //Já tem um arquivo com esse nome
                    return -1;
            }
        }
        else
        if(i < 1026){
            numBlock = getNumberBlockSingleInd(inodePai->singleIndPtr, i-2);
            block = getBlock(block, numBlock);
            inodePai = getInode(recordPai->inodeNumber, inodePai);
            printf("\ni = %d  singleIndPtr\n", i);
            for (j = 0; j < 64; j++){
                //printf("\nrecord = %d\n", j);
                new_record = (struct t2fs_record *) &block[j*RECORD_SIZE];
                //printf("tentando inserir '%s' na entrada do diretorio = '%s'\n",name, new_record->name);
                infoEntry = checkEntry(new_record, name);

                //printf("infoEntry %d\t(0=entrada valida; 1=pode inserir; 2=nomes iguais) \n", infoEntry);
                if((infoEntry == 1 && notInserted)||(infoEntry == -1 && update)){
                      printf("PODE INSERIR AQUI!!!!!!\n");

                      indexBlockNewRec =getNumberBlockSingleInd(inodePai->singleIndPtr, i-2);
                      indexNewRecord = j;
                      notInserted = 0;
                      if(update){
                        block = getBlock(block, indexBlockNewRec);
                        new_record = update_record;
                        memcpy(&block[indexNewRecord*RECORD_SIZE],(BYTE *) new_record, RECORD_SIZE);
                        writeBackBlock(block,indexBlockNewRec);
                        //
                        return 0;   //Indica que não foi adicionado mais um bloco
                      }
                    }
                else
                if(infoEntry == -1)   //Já tem um arquivo com esse nome
                    return -1;
            }
        }
        else{
            numBlock = getNumberBlockDoubleInd(inodePai->doubleIndPtr, (i-1026)/1024, (i-1026)%1024);
            block = getBlock(block, numBlock);
            inodePai = getInode(recordPai->inodeNumber, inodePai);
            printf("\ni = %d  doubleIndPtr\n", i);
            for (j = 0; j < 64; j++){
                //printf("\nblockfilesize = %d\n", recordPai->blocksFileSize);
                new_record = (struct t2fs_record *) &block[j*RECORD_SIZE];
                //printf("tentando inserir '%s' na entrada do diretorio = '%s'\n",name, new_record->name);
                infoEntry = checkEntry(new_record, name);

                //printf("infoEntry %d\t(0=entrada valida; 1=pode inserir; 2=nomes iguais) \n", infoEntry);
                if((infoEntry == 1 && notInserted)||(infoEntry == -1 && update)){
                      printf("PODE INSERIR AQUI!!!!!!\n");
                      indexBlockNewRec = getNumberBlockDoubleInd(inodePai->doubleIndPtr, (i-1026)/1024, (i-1026)%1024);
                      indexNewRecord = j;
                      notInserted = 0;
                      if(update){
                          printf("UPDATE!!!!!!!\n");
                        block = getBlock(block, indexBlockNewRec);
                        new_record = update_record;
                        memcpy(&block[indexNewRecord*RECORD_SIZE],(BYTE *) new_record, RECORD_SIZE);
                        writeBackBlock(block,indexBlockNewRec);
                        //
                        return 0;   //Indica que não foi adicionado mais um bloco
                      }
                    }
                else
                if(infoEntry == -1)   //Já tem um arquivo com esse nome
                    return -1;
            }
        }
        i++;
    }

    if(notInserted){
        int n;
        printf("Precisa alocar novo bloco!!!!!!!!!!!\n");
        //
        //recordPai->blocksFileSize += 1;
        recordPai->bytesFileSize += RECORD_SIZE;
        n = createNewBlock(i, inodePai, recordPai->inodeNumber, name, typeVal);

        block = getBlock(block, n);
        struct t2fs_record *rec;
        rec = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
        rec = setEntry(rec, inodePai, name, typeVal);
        memcpy(&block[0],(BYTE *) rec, RECORD_SIZE);
        writeBackBlock(block,n);
        return 1;   //Indica que um adicionado mais um bloco
    }
    else{
        struct t2fs_record *rec;
        rec = (struct t2fs_record *) malloc(sizeof(struct t2fs_record));
        printf("INSERINDO NOVO ENTRY\n");
        recordPai->bytesFileSize += RECORD_SIZE;
        printf("indexBlockNewRec = %d  | indexNewRecord = %d\n", indexBlockNewRec, indexNewRecord);
        block = getBlock(block, indexBlockNewRec);
        rec = setEntry(rec, inodePai, name, typeVal);
        memcpy(&block[indexNewRecord*RECORD_SIZE],(BYTE *) rec, RECORD_SIZE);
        writeBackBlock(block,indexBlockNewRec);
        //
        return 0;   //Indica que não foi adicionado mais um bloco
    }
    return -1;
}

struct t2fs_record* scanDirOnBlock(BYTE* block, struct t2fs_record *record, int record_index,int *aux_counter)
{
    //printf("oi");
    int i;
    for(i=0; i<64; i++)
    {
        //printf("oi1");
        record = (struct t2fs_record *) &(block[i*RECORD_SIZE]);
        //printf("%d TypeVal: %s %d\n",i, record->name, record->TypeVal);
        //GARANTIR QUE É DIRETORIO ANTES // TO USANDO PRA ARQUIVOS
        if (record->TypeVal != TYPEVAL_INVALIDO){
            *aux_counter = *aux_counter + 1;
            //printf("oi");
            if(*aux_counter > record_index)
            {
                //printf("oi");
                //printf("-name == dirEntryName == %s\n", record->name);
                return record;
            }
        }
    }

    //free(record);
    return NULL;
}

int createNewBlock(int numNewBlock, struct t2fs_inode *inodePai, int inodePaiNumber, char* name, WORD typeVal)
{  /*new_record = record a ser inserida
    numNewBlock = cardinalidade com novo bloco*/
    printf(">createNewBlock()\n");
    /*Essa função aloca um novo bloco no inodePai*/

    //BYTE* block;
    int numAllocatedBlock;
    //printf("numNewBlock = %d\n", numNewBlock);
    //printf("inodePaiNumber = %d\n", inodePaiNumber);
    if(numNewBlock == 1){   //Alocar mais um bloco direto
        inodePai->dataPtr[1] = searchBitmap2(BITMAP_DADOS,0);
        setBitmap2(BITMAP_DADOS,inodePai->dataPtr[1],1);
        //printf("inode Atualizado:");
        //printInode(inodePai);
        writeBackInode(inodePai, inodePaiNumber);
        //
        return inodePai->dataPtr[1];
    }
    else
    if(numNewBlock < 1026){     //Alocar um bloco do singleIndPtr
        if (inodePai->singleIndPtr == INVALID_PTR){     //Alocar o primeiro bloco de singleIndPtr
            inodePai->singleIndPtr = searchBitmap2(BITMAP_DADOS,0);
            setBitmap2(BITMAP_DADOS,inodePai->singleIndPtr,1);
            writeBackInode(inodePai, inodePaiNumber);

            //Aloca novo bloco de dados:
            numAllocatedBlock = allocateBlockSingleInd(inodePai->singleIndPtr,numNewBlock-2);
            return numAllocatedBlock;
        }
        else{   //Alocar mais um bloco de singleIndPtr
            numAllocatedBlock = allocateBlockSingleInd(inodePai->singleIndPtr,numNewBlock-2);

            return numAllocatedBlock;
        }
    }
    else
    if (numNewBlock < 67108864) {   //Alocar um bloco do doubleIndPtr
        if (inodePai->doubleIndPtr == INVALID_PTR) {    //Alocar o primeiro bloco de doubleIndPtr
            inodePai->doubleIndPtr = searchBitmap2(BITMAP_DADOS,0);
            setBitmap2(BITMAP_DADOS,inodePai->doubleIndPtr,1);
            writeBackInode(inodePai, inodePaiNumber);

            //Aloca novo bloco de dados:
            numAllocatedBlock = allocateBlockDoubleInd(inodePai->doubleIndPtr,(numNewBlock-1026)/1024,(numNewBlock-1026)%1024);
            return numAllocatedBlock;
        }
        else{   //Alocar mais um bloco de doubleIndPtr
            numAllocatedBlock = allocateBlockDoubleInd(inodePai->doubleIndPtr,(numNewBlock-1026)/1024,(numNewBlock-1026)%1024);

            return numAllocatedBlock;
        }
    }
    else{
        error(ERROR_FULL_INODE);
        return -1;
    }
    return 0;
}


void printRecord(struct t2fs_record *record)
{
    printf("record->TypeVal = %d\n", record->TypeVal);
    printf("record->name = %s\n", record->name);
    printf("record->blocksFileSize = %d\n", record->blocksFileSize);
    printf("record->bytesFileSize = %d\n", record->bytesFileSize);
    printf("record->inodeNumber = %d\n", record->inodeNumber);
}

struct t2fs_record* setEntry(struct t2fs_record *new_record, struct t2fs_inode *inodePai, char* name, WORD typeVal)
{
    printf(">setEntry()\n");
    //printf("TypeVal = %d\nNome = %s", new_record->TypeVal,new_record->name);

    new_record->TypeVal = typeVal;
    strcpy(new_record->name,name);
    new_record->blocksFileSize = 1;
    new_record->bytesFileSize = 0;
    new_record->inodeNumber = searchBitmap2(BITMAP_INODE,0);
    setBitmap2(BITMAP_INODE,new_record->inodeNumber,1);
    struct t2fs_inode *inode =  (struct t2fs_inode *) malloc(sizeof(struct t2fs_inode));
    inode = initInode(inode);
    //printInode(inode);
    writeBackInode(inode, new_record->inodeNumber);

    return new_record;
}

int checkEntry(struct t2fs_record* currentEntry, char* name)
{
    if((strcmp(currentEntry->name,name)==0)&&(currentEntry->TypeVal != TYPEVAL_INVALIDO))
        return -1;  //Ja existe entrada com mesmo nome
    else
    if (currentEntry->TypeVal != TYPEVAL_REGULAR && currentEntry->TypeVal != TYPEVAL_DIRETORIO)
        return 1;
    return 0;
}

int getNumberBlockSingleInd(int numIndexS, int index)
{
    BYTE* block = (BYTE *) malloc(16*SECTOR_SIZE);
    int *num, numBlock;

    block = getBlock(block,numIndexS);
    num = (int *) &block[index*4];
    numBlock = *num;
    return numBlock;
}

int getNumberBlockDoubleInd(int numIndexD, int numIndexS, int index)
{
    BYTE* block = (BYTE *) malloc(16*SECTOR_SIZE);
    int *num, numBlockS;

    block = getBlock(block,numIndexD);
    num = (int *) &block[numIndexS*4];
    numBlockS = *num;

    return getNumberBlockSingleInd(numBlockS, index);
}

no* parse_pathname(char *entry,no *output ,int debug){
    char *entry_buffer;
    entry_buffer = entry;
    char *p = strtok (entry_buffer, "/");
    no* aux, *new_path;

    if(p != NULL){
        new_path = (no*) malloc(sizeof(no));
        strcpy(new_path->content,p);
        new_path->next = NULL;
        output = new_path;
        p = strtok (NULL, "/");
    }
    else
        return NULL;

    while (p != NULL) {
        new_path = (no*) malloc(sizeof(no));
        strcpy(new_path->content,p);
        new_path->next = NULL;

        aux = output;
        while (aux->next != NULL)
            aux = aux->next;

        aux->next = new_path;
        p = strtok (NULL, "/");
    }

    return output;
}

struct t2fs_inode* getInode(int inodeNumber, struct t2fs_inode* inode)
{
    printf(">getInode()\n");
    int inodeNumSector = secInodeZero + inodeNumber/16;
    BYTE* sector = malloc(SECTOR_SIZE);

    //printf("inodeNumber = %d\n", inodeNumber);
    //Recupera inode do disco
    if(read_sector(inodeNumSector, sector))
        return NULL;

    //printf("index of sector = %d\n", ((inodeNumber % 16) * 16));
    inode = (struct t2fs_inode *) &sector[(inodeNumber % 16) * 16];
    //printInode(inode);
    //printf("fim getInode\n");
    return inode;
}

BYTE* getBlock(BYTE *block, int blockNumber)
{
    printf(">getBlock()\n");
    int blockNumSector,i;
    BYTE* buffer, *block_aux;
    buffer = (BYTE *) malloc(SECTOR_SIZE);
    block_aux = (BYTE *) malloc(16*SECTOR_SIZE);

    blockNumSector = secBlockZero; //+ blockNumber;

    for(i=0; i<superB.blockSize; i++){
        //printf("i = %d\n", i);
        if(read_sector(blockNumSector + (blockNumber*superB.blockSize) + i, buffer ) )
            printf("Erro na leitura do setor %d\n", blockNumSector + (blockNumber*superB.blockSize) + i);
        memcpy(&(block_aux[i*SECTOR_SIZE]), buffer, SECTOR_SIZE);
    }
    memcpy(block, block_aux, 16*SECTOR_SIZE);
    return block;
}
//INITIATES INODE WITH ONE BLOCK ALREADY ALLOCATED
struct t2fs_inode* initInode(struct t2fs_inode * inode)
{
    int bitnumber = searchBitmap2 (1,0);
    setBitmap2 (1,bitnumber,1);
    inode->dataPtr[0] = bitnumber;
    inode->dataPtr[1] = INVALID_PTR;
    inode->singleIndPtr = INVALID_PTR;
    inode->doubleIndPtr = INVALID_PTR;

    return inode;
}

void createRoot()
{
    ROOT = (struct t2fs_record*)malloc(sizeof(struct t2fs_record));
    ROOT->TypeVal = TYPEVAL_DIRETORIO;
    strcpy(ROOT->name,"root");
    ROOT->blocksFileSize = 1;
    ROOT->bytesFileSize = 0;
    ROOT->inodeNumber = 0;
}

void printInode(struct t2fs_inode *inode){
    printf("INODE:\n");
    printf("dataPtr[0] = %d\n", inode->dataPtr[0]);
    printf("dataPtr[1] = %d\n", inode->dataPtr[1]);
    printf("singleIndPtr = %d\n", inode->singleIndPtr);
    printf("doubleIndPtr = %d\n", inode->doubleIndPtr);
}

void writeBackBlock(BYTE* block, int blockNumber)
{
    printf("writeBackBlock()\n");
    //printf("blockNumber = %d\n", blockNumber);
    int blockNumSector = secBlockZero + blockNumber*16, i;
    BYTE* sector = (BYTE *) malloc(SECTOR_SIZE);

    for (i = 0; i < 16; i++){
        memcpy(sector, &block[i*SECTOR_SIZE], SECTOR_SIZE);
        write_sector(blockNumSector+i, sector);
    }
    return;
}

int allocateBlockSingleInd(int numIndexS, int index)
{
     printf(">allocateBlockSingleInd()\n");
     BYTE *block;
     int newBlock, inv = INVALID_PTR;
     block = (BYTE *)malloc(16*SECTOR_SIZE);

     if(index < 1024){
         block = getBlock(block, numIndexS); //Bloco de indices (simples indireto)
         newBlock = searchBitmap2(BITMAP_DADOS,0);
         setBitmap2(BITMAP_DADOS,newBlock,1);
         printf("newBlock allocated %d\n", newBlock);
        memcpy(&block[index*4],(BYTE*) newBlock, 4);    //Seta entrada do bloco de indice

        if(index < 1023){
            memcpy(&block[(index+1)*4],(int *) &inv, 4);    //Seta a proxima entrada para INVALID_PTR
            printf("Seta a proxima entrada para INVALID_PTR\n");
        }
        writeBackBlock(block, numIndexS);
        return newBlock;
     }
 }


int allocateBlockDoubleInd(int numIndexD,int numIndexS, int index)
{
    printf(">allocateBlockDoubleInd()\n");
    BYTE *block;
    int *entry, newBlockIndex;

    block = (BYTE *)malloc(16*SECTOR_SIZE);
    block = getBlock(block, numIndexD);

    entry = (int *) &block[numIndexS*4];
    printf("*entry = %d\n", *entry);

    if (*entry == INVALID_PTR){
        newBlockIndex = searchBitmap2(BITMAP_DADOS,0);
        setBitmap2(BITMAP_DADOS,newBlockIndex,1);
        printf("INVALID_PTR, newBlockIndex = %d\n", newBlockIndex);

        memcpy(&block[(numIndexS)*4], (BYTE *) newBlockIndex, 4);
        if(numIndexS < 1023)
            memcpy(&block[(numIndexS+1)*4], (BYTE *) INVALID_PTR, 4);
        return allocateBlockSingleInd(newBlockIndex, index);
    }
    else{
        printf("not INVALID_PTR, just allocateNewBlock\n");
        return allocateBlockSingleInd(*entry, index);
    }
    return -1;
}
int freeInode(struct t2fs_inode *inode, int inodeNumber){
    int i=0, numB;

    if (inode->dataPtr[0] != INVALID_PTR)
        setBitmap2(BITMAP_DADOS,inode->dataPtr[0],0);

    if (inode->dataPtr[1] != INVALID_PTR)
        setBitmap2(BITMAP_DADOS,inode->dataPtr[1],0);

    if (inode->singleIndPtr != INVALID_PTR) {
        i=0;
        while (i < 1024) {
            numB = getNumberBlockSingleInd(inode->singleIndPtr, i);
            if(numB != INVALID_PTR)
                setBitmap2(BITMAP_DADOS,numB,0);
            else
                break;
            i++;
        }
        setBitmap2(BITMAP_INODE,inode->singleIndPtr,0);
    }

    if(inode->doubleIndPtr != INVALID_PTR){
        i=0;
        while (i < 1048576) {
            numB = getNumberBlockDoubleInd(inode->doubleIndPtr, i/1024, i%1024);
            if(numB != INVALID_PTR)
                setBitmap2(BITMAP_DADOS,numB,0);
            else
                break;
            i++;
        }
        i=0;
        while (i < 1024) {
            numB = getNumberBlockSingleInd(inode->doubleIndPtr, i);
            if(numB != INVALID_PTR)
                setBitmap2(BITMAP_DADOS,numB,0);
            else
                break;
            i++;
        }
    }
    setBitmap2(BITMAP_INODE,inodeNumber,0);
    return 0;
}
char* getfatherpath(char* output,char* pathname){
    int i;
    for(i = strlen(pathname)-2;i >= 0; i--){
    if(pathname[i] == '/'){
      memcpy(output,pathname,i);
      return output;
        }
    }
    return output;
}

void error(char* message) {
    printf("\n<ERROR>%s<ERROR>\n\n", message);
}
