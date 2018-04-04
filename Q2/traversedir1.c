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
char * ROOT;
struct table* T;
struct stack* top=NULL;
char DONE;
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
	if(T->a[i]==NULL)
	{
		
		T->a[i]=r;//Lock
		//printf("Inserted0 %s %d\n",word,i);
		return;
	}
	curr=T->a[i];
	//printf("Linked List %d:",i);
	do
	{
		prev=curr;
		//printf("%s ",curr->word);
		if(!strcmp(curr->word,word))
		{
			curr->count+=1;//Lock
			//printf("Inserted1 %s\n",word);
			if(curr->count>=T->max[0]->count)
			{
				insertKM(curr);//Lock
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
int hash(char* a){
	int i=0,sum=0;
	while(a[i])
	{
		sum+=a[i];
		i++;
	}
	return (sum%((int)pow(2,16)))%(T->size);
}
void readFile(char* filePath)
{
	printf("Thread No %d Reading File:%s\n",omp_get_thread_num(),filePath);
	FILE* fp=fopen(filePath,"r");
	set <string> words;
	set<string>::iterator it;
	while(!feof(fp))
	{
		char readWord[25];
		fscanf(fp,"%s",readWord);
		//printf("Read %s\n",readWord);
		if(words.find(readWord)!=words.end())
			continue;
		else
		{
			words.insert(readWord);
			add(hash(readWord),readWord);
		}
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
   
   #pragma omp parallel
   do {
      sprintf(path, "%s/%s", pathName, entry->d_name);
      if (entry->d_type == DT_DIR && entry->d_name[0]!='.') {
        //printf("Entering Directory %s\n", path);
        walk(path);
      }
      else if (entry->d_type== DT_REG) {
        //printf("\tPushing File %s\n", path);
        readFile(path);
      }
   	} while ((entry = readdir(dir)));
   closedir(dir);
}

int main(int argc, char const *argv[])
{
	omp_set_dynamic(atoi(argv[4])+1);
	ROOT=(char*)malloc((strlen(argv[1])+1)*sizeof(char));
	strcpy(ROOT,argv[1]);
	T=(struct table*)malloc(sizeof(struct table));
	T->size=1000;//arguement later
	T->a=(struct record**)malloc(T->size*sizeof(struct record*));
	T->k=atoi(argv[2]);
	T->max=(struct record**)malloc(T->k*sizeof(struct record));
	for(int i=0;i<T->k;i++)
	{
		struct record* temp=(struct record*)malloc(sizeof(struct record));
		temp->word=(char*)malloc(sizeof(char));
		strcpy(temp->word,"random");
		temp->count=1;
		T->max[i]=temp;
	}
	double t1=omp_get_wtime();
	walk(argv[1]);
	printf("Total Time=%f\n",omp_get_wtime()-t1);
	int k=0;
	for(int i=0;i<T->k;i++)
	{
		printf("%s %d\n",T->max[i]->word,T->max[i]->count);
	}
	printf("%d\n",T->k);
	return 0;
}