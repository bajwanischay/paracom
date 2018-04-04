#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <set>
#include <iostream>
#include <string>
#include <omp.h>
#include "traversedir.h"
#include <math.h>

using namespace std;

struct table* T;
struct stack* top=NULL;
char* ROOT;
double IOT,COT;
omp_lock_t* lock;
void push(char* path )
{

	struct stack* newNode=(struct stack*)malloc(sizeof(struct stack));
	newNode->path=(char*)malloc((strlen(path)+1)*sizeof(char));
	strcpy(newNode->path,path);
	newNode->next=top;
	top=newNode;
}

struct stack* pop()
{
	if(top==NULL)
		return NULL;
	struct stack* temp=top;
	top=temp->next;
	temp->next=NULL;
	return temp;
}
int hash(char* a){
	int i=0,sum=0;
	while(a[i])
	{
		sum+=a[i];
		i++;
	}
	return (sum%((int)pow(2,16)))%(T->size);
}
void insertKM(struct record* r)
{
	int i;
	for(i=r->rank+1;i<T->k && T->max[i]->count<r->count;i++)
	{
		T->max[i-1]=T->max[i];
		T->max[i-1]->rank=i-1;
	}
	T->max[i-1]=r;
	r->rank=i-1;
}
void add(int i, char* word)
{
	struct record* r, *curr, *prev;
	r=(struct record*)malloc(sizeof(struct record));
	r->word=(char*)malloc((strlen(word)+1)*sizeof(char));
	strcpy(r->word,word);
	r->count=1;
	r->rank=0;
	r->next=NULL;
	omp_set_lock(&(lock[i]));
	if(T->a[i]==NULL)
	{
		//#pragma omp critical
		T->a[i]=r;//Lock
		//printf("Inserted0 %s %d\n",word,i);
		omp_unset_lock(&(lock[i]));
		return;
	}
		curr=T->a[i];
	omp_unset_lock(&(lock[i]));
	//printf("Linked List %d:",i);
	do
	{
		prev=curr;
		//printf("%s ",curr->word);
		if(!strcmp(curr->word,word))
		{
			//#pragma omp critical
			{
				curr->count+=1;//Lock
				//printf("Inserted1 %s\n",word);
				if(curr->count>T->max[0]->count)
				{
					
					insertKM(curr);//Lock
				}
			}
			free(r);
			return;
		}
		curr=curr->next;
	}while(curr!=NULL);
	//printf("\n");
	prev->next=r;//Lock
	//printf("Inserted2 %s %d\n",word,i);
	return;
}
void readFile(char* filePath)
{
	printf("Thread No %d Reading File:%s\n",omp_get_thread_num(),filePath);
	set <string> words;
	set<string>::iterator it;
	double t0=omp_get_wtime();
	FILE* fp=fopen(filePath,"r");
	IOT+=omp_get_wtime()-t0;
	while(!feof(fp))
	{
		char readWord[25];
		double t1=omp_get_wtime();
		fscanf(fp,"%s",readWord);
		IOT+=omp_get_wtime()-t1;
		//printf("Read %s ",readWord);
		double t2=omp_get_wtime();
		if(words.find(readWord)!=words.end())
			continue;
		else
		{
			words.insert(readWord);
			add(hash(readWord),readWord);
		}
		COT+=omp_get_wtime()-t2;
	}
	fclose(fp);
}

void walk(const char *pathName) {
   DIR *dir;
   struct dirent *entry;

   if (!(dir = opendir(pathName))) {
      fprintf(stderr, "Could not open directory\n");
      return;
   }

   if (!(entry = readdir(dir))) {
      fprintf(stderr, "Could not read directory\n");
      return;
   }
   char path[1024];
   do {
      
      sprintf(path, "%s/%s", pathName, entry->d_name);
      if (entry->d_type == DT_DIR && entry->d_name[0]!='.') {
        //printf("Entering Directory %s\n", path);
        walk(path);
      }
      else if (entry->d_type== DT_REG) {
        //printf("\tPushing File %s\n", path);
        push(path);
      }
   	} while ((entry = readdir(dir)));
   	if(!strcmp(pathName,ROOT))
   	ROOT=NULL;
   closedir(dir);
}

int main(int argc, char const *argv[])
{
	IOT=0;
	COT=0;
	T=(struct table*)malloc(sizeof(struct table));
	T->size=atoi(argv[3]);//arguement later
	T->a=(struct record**)malloc(T->size*sizeof(struct record*));
	T->k=atoi(argv[2]);
	T->max=(struct record**)malloc(T->k*sizeof(struct record));
	lock=(omp_lock_t*)malloc(T->size*sizeof(omp_lock_t));
	for(int i=0;i<T->size;i++)
	{
		omp_init_lock(&(lock[i]));
	}
	for(int i=0;i<T->k;i++)
	{
		struct record* temp=(struct record*)malloc(sizeof(struct record));
		temp->word=(char*)malloc(sizeof(char));
		strcpy(temp->word,"random");
		temp->count=1;
		T->max[i]=temp;
	}
	ROOT=(char*)malloc((strlen(argv[1])+1)*sizeof(char));
	strcpy(ROOT,argv[1]);
	double time1=omp_get_wtime();
	walk(ROOT);
	double time2=omp_get_wtime();
	printf("Walking Time %f\n",time2-time1);
	#pragma omp parallel shared(top, ROOT)
	while(top!=NULL || ROOT!=NULL)
	{
		struct stack* file;
		#pragma omp critical
		{ 
			file=pop();
		}

		if(file==NULL)
			continue;
		//printf("Popped %s\n",file->path);
		readFile(file->path);
	}
	double time3=omp_get_wtime();
	printf("Reading Time %f\n",time3-time2);
	int k=0;
	for(int i=0;i<T->k;i++)
	{
		printf("%s %d\n",T->max[i]->word,T->max[i]->count);
	}
	printf("%d\n",T->k);
	printf("IOT=%f COT=%f\n",IOT,COT);
	return 0;
}