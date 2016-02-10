#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include "simVM.h"
#include <math.h>

int userthreads;
sem_t queuelock, pagetablelock, mainmemorylock, garbagelock, threadcountlock;

/*Array for the addresses in main mem */
int* mainmemory;
/* Array used to keep track of the accesses of a certain frame */
int* mainmemoryuse;
/*Which user thread the address belongs to */
int* mainmemorythreadnum;
/* Used by mainmemoryuse to keep track of the LRU frame */
int mainmemorycounter;


int** pagetables;

/* Valid bit */
int** pagetableshasframe;


int threadsleft;


/*Used to terminate the memmanager & faultHandler threads */
int oktoexitmemcount;
int oktoexitmem;
int oktoexitfault;
int oktoprintdone;
int terminatefaulthandler;


/* Queue for the memory manager & fault handler & garbage queue */
struct addresses *head , *tail, *garbagehead, *garbagetail, *faulthead, *faulttail;

/* Used to store the number of bits for the offset */
int offset;

/*Used to keep track of the number of addresses that have been created */
int addressescount;


int page_size;
int num_pages;
int num_mainmem_frames;
int num_threads;



main(int argc, char** argv)
{
    setbuf(stdout,NULL);
	int i,j;
	
	addressescount = 0;
	
	terminatefaulthandler = 0;
	
	oktoexitmemcount = 0;
	
	oktoexitmem = 0;
	
	oktoexitfault = 0;
	
	oktoprintdone = 0;
	
	/* USED TO KEEP TRACK OF THE USER THREADS, USED INSIDE USERTHREAD FUNCTION */
	userthreads = 1;
	
	sem_init(&queuelock,0,1);
	sem_init(&pagetablelock,0,1);
	sem_init(&mainmemorylock,0,1);
	sem_init(&garbagelock,0,1);
	sem_init(&threadcountlock,0,1);
	
	/* FOR MISSING VARIABLES */
	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] == NULL)
	{
		printf("MISSING SOME COMMAND LINE ARGUMENTS!\n");
		return 0;
	}
	
	
	/* VARIABLES ARE INITIALIZED */
	page_size = atoi(argv[1]);
	num_pages = atoi(argv[2]);
	num_mainmem_frames = atoi(argv[3]);
	num_threads = atoi(argv[4]);

	
	char* temporary = (char*) malloc( 100* sizeof(char));
	strcpy(temporary, convertToBinary(page_size));
	
	offset = strlen(temporary) - 1;
	
	/*
	printf("Offset in binary is %s, Number of bits to offset is %d\n", temporary, offset);
	*/
	free(temporary);
	
	head = NULL;
	tail = NULL;
	garbagehead = NULL;
	garbagetail = NULL;
	faulthead = NULL;
	faulttail = NULL;
	
	/* NUMBER OF RUNNING THREADS*/
	threadsleft = 0;
	
	
	/* INITIALIZE MAIN MEMORY STUFF */
	mainmemory = (int*) malloc(num_mainmem_frames * sizeof(int));
	mainmemoryuse = (int*) calloc (num_mainmem_frames, num_mainmem_frames * sizeof(int));
	mainmemorythreadnum = (int*) calloc (num_mainmem_frames, num_mainmem_frames * sizeof(int));
	mainmemorycounter = 0;

	
	
	/* MALLOC PAGE TABLES ; I'm using N+1 so that I can use 1 to N instead of 0 to N-1 (0 is not used at all)*/
	pagetables = malloc ((num_threads+1) * sizeof(int*));
	pagetableshasframe = malloc ((num_threads+1) * sizeof(int*));
	for(j = 1; j < (num_threads+1); j++)
	{
		/*
		printf("Malloc loop number %d\n", j);
		*/
		pagetables[j] = malloc (num_pages * sizeof(int));
		pagetableshasframe[j] = malloc (num_pages * sizeof(int));
	}
	
	int k;
	for(j = 1; j < (num_threads+1); j++)
	{
		for(k = 0; k < num_pages; k++)
		{
			pagetableshasframe[j][k] = 0;
			pagetables[j][k] = 999999;
		}
	}
	
	void *child_stack1;
	child_stack1=(void *)malloc(16384); child_stack1+=16383;
	clone((int (*)(void*))memManager, child_stack1, CLONE_VM|CLONE_FILES, NULL);
	
	void *child_stack2;
	child_stack2=(void *)malloc(16384); child_stack2+=16383;
	clone((int (*)(void*))faultHandler, child_stack2, CLONE_VM|CLONE_FILES, NULL);
	
	/* Creates n number of user threads */
	for(i = 1; i < num_threads+1; i++)
	{
		
		char* filetoopen ;
		filetoopen = (char*) malloc (100* sizeof(char));
		/* Initialize the blank string */
		strcpy(filetoopen, "");
		strcat(filetoopen,"address_");
		char* one = malloc(100* sizeof(char));
		sprintf(one, "%d", i);
		strcat(filetoopen,one);
		strcat(filetoopen,".txt");
		
		
		FILE* to_open;
		to_open = fopen(filetoopen,"r");
		if( to_open == 0)
		{
			printf("File %s not found!\n", filetoopen);
			
			oktoexitmemcount++;
			if(oktoexitmemcount == num_threads)
			{
				oktoexitmem = 1;
				/*
				printf("OK TO EXIT MEMORY MANAGER!\n");
				*/
			}
			
			free(filetoopen);
			free(one);
		}
		
		else
		{
			fclose(to_open);
			free(filetoopen);
			free(one);
			threadsleft++;
			void *child_stack;
			child_stack=(void *)malloc(16384); child_stack+=16383;
			clone((int (*)(void*))userthread, child_stack, CLONE_VM|CLONE_FILES, NULL);
		}
	}

	/* Waits for the faulthandler and memorymanager threads to quit before quitting */
	while(1)
	{
		if(oktoexitmem == 1 && oktoexitfault == 1)
		{
			
			/* Frees Memory Before Quitting */
			free(mainmemory);
			free(mainmemoryuse);
			free(mainmemorythreadnum);
			
			for(j = 1; j < (num_threads+1); j++)
			{
				free(pagetables[j]);
				free(pagetableshasframe[j]);
			}
			
			free(pagetables);
			free(pagetableshasframe);
			
			struct addresses *toclean;
			
			/* Frees garbage queue */
			while(garbagehead != NULL)
			{
				toclean = garbagehead;
				
				garbagehead = garbagehead->next;
				
				free(toclean);
			}
			
			/*
			printf("Exiting main!\n");
			*/
			exit(0);
		}
	}
}

void userthread()
{
	int num;
	
	/*Num is the number assigned to the current user thread eg. the first user thread created is 1 (value was initialized to 1*/
	sem_wait(&threadcountlock);
	num = userthreads;
	userthreads++;
	sem_post(&threadcountlock);
	
	/*printf("[Process %d] starts\n", num);*/
	
	/* Concatenates the file name */
	char* filetoopen ;
	filetoopen = (char*) malloc (100* sizeof(char));
	/* Initialize the blank string */
	strcpy(filetoopen, "");
	strcat(filetoopen,"address_");
	char* one = malloc(100* sizeof(char));
	sprintf(one, "%d", num);
	strcat(filetoopen,one);
	strcat(filetoopen,".txt");
	
	/*
	printf("File to open (USER THREAD) = %s\n", filetoopen);
	*/
	
	FILE* to_open;
	to_open = fopen(filetoopen,"r");
	
	if( to_open == 0)
	{
		printf("(File not found! \n");
		free(filetoopen);
		free(one);
		threadsleft--;
		
		oktoexitmemcount++;
		if(oktoexitmemcount == num_threads)
		{
			oktoexitmem = 1;
			
			printf("OK TO EXIT MEMORY MANAGER (IN USER THREAD!\n");
			
		}
		
		exit(0);
	}
	
	
	char* buf;
	buf = (char*) malloc(1000 *sizeof(char));
	
	
	/*Pushes all the addresses into the queue */
	while (fgets(buf,1000, to_open)!=NULL)
	{
		struct addresses *topush;
		topush = (struct addresses*)malloc(sizeof(struct addresses));
		topush->next = NULL;
		topush->addresstodecipher  = atoi(buf);
		topush->processnumber = num;
		
		
		sem_wait(&queuelock);
		
		/*Push it into the memory manager queue */
		if(head == NULL)
		{
			/*
			printf("Pushing %d for process %d into the head\n", topush->addresstodecipher , num);
			*/
			head = topush;
			tail = topush;
			topush->next = NULL;
			addressescount++;
		}
		
		else
		{
			/*
			printf("Pushing %d for process %d to the tail\n", topush->addresstodecipher , num);
			*/
			tail->next = topush;
			topush->next = NULL;
			tail = topush;
			addressescount++;
		}
		
		sem_post(&queuelock);
		/*printf("Addresses after pushing: %d\n", addressescount);*/
		
	}
	
	struct addresses *temp;
	temp = head;
	
	/*
	while(temp != NULL)
	{
		printf("%d from process %d\n", temp->addresstodecipher, temp->processnumber);
		temp = temp->next;
	}
	*/
	
	
	/* FREE MEMORY */
	fclose(to_open);
	free(buf);
	free(filetoopen);
	free(one);
	
	
	printf("[Process %d] ends\n", num); 
	num++;
	
	threadsleft--;
	oktoexitmemcount++;
	
	/*Once all the addresses have been pushed into the memory manager queue, it is safe to terminate the memmanager when the queue head is null */
	if(oktoexitmemcount == num_threads)
	{
		oktoexitmem = 1;
		/*
		printf("OK TO EXIT MEMORY MANAGER (IN USER THREAD!\n");
		*/
	}
	
	exit(0);
}

void memManager()
{
	/* Keeps running while there is still stuff to do */
	while(1)
	{
		
		if(head == NULL)
		{
			/* The way I implemented it, head only becomes NULL and oktomemexit== 1 if all the addresses have been processed and translated */
			if(oktoexitmem == 1)
			{
				
				
				/*
				printf("OK TO EXIT FAULT HANDLER!\n");
				printf("Exiting memmanager!\n");
				*/
				oktoexitfault = 1;
				exit(0);
			}
		}
		
		if(head != NULL)
		{
			
			sem_wait(&queuelock);
			/*Converts the head's address to binary */
			int i;
			int n = head->addresstodecipher;
			char* stringnum = malloc(20* sizeof(char));
			char* temp = malloc(10*sizeof(char));
			char* temp2 = malloc(20* sizeof(char));
			strcpy(stringnum, "");
			strcpy(temp, "");
			strcpy(temp2, "");
			while(n > 0)
			{
				int a = n%2;
				sprintf(temp,"%d", a);
				strcpy(temp2, stringnum);
				strcpy(stringnum, temp);
				strcat(stringnum, temp2);
				
				n = n/2;
			}
			 
			int lengthofaddress = strlen(stringnum);
			
			/*Number of bits in the page number */
			int pagenumberbits = lengthofaddress-offset;
			
			/*Used to store the page number that is converted to decimal form */
			int decimalpagenumber;
			
			/*Used to store the page number bits (in binary) */
			char* binarypagenum = malloc(100* sizeof(char));

			/*Used to store the offset */
			char* offsetinstring;
			
			
			/*If the the number of bits in the address < the offset bits it means that it is in page 0*/
			if(pagenumberbits <= 0)
			{
				pagenumberbits = 0;
				decimalpagenumber = 0;
				offsetinstring = malloc(100* sizeof(char));
				strcpy(offsetinstring, stringnum);
			/*	printf("Page number has 0 bits therefore page number is 0\n");
				printf("Offset is %s\n", offsetinstring); */
			}
			
			else
			{
				/*Copies the first x bits (page number) into binarypagenum */
				strncpy(binarypagenum,stringnum,pagenumberbits);
				binarypagenum[pagenumberbits] = '\0';
				
				/*Converts it to decimal representation*/
				long longpagenumber = atol(binarypagenum);
				decimalpagenumber = convertToDecimal(longpagenumber);
				
				/*printf("Page number in binary is %s, Converted to decimal: %d\n",binarypagenum, decimalpagenumber);*/
				
				/*Gets the offset of the address */
				offsetinstring = lastx(lengthofaddress - pagenumberbits, stringnum);
				/*printf("Offset is %s \n",offsetinstring);*/
				
			}
			
			head->pagenumber = decimalpagenumber;
			
			/*
			printf("Address has %d bits, offset is %d bits, therefore page number is %d bits\n", lengthofaddress, offset, pagenumberbits);
			
			printf("Address %d in binary is %s\n", head->addresstodecipher, stringnum);
			
	 
			for(i = 0; i <  num_mainmem_frames; i++)
			{
				printf("Frame %d in main memory has address %d of process %d\n", i, mainmemory[i], mainmemorythreadnum[i]);
			}
			*/
			 
			 
			/*Checks if it is in main memory */
			sem_wait(&mainmemorylock);
			int isinmainmemory = 0;
			for(i = 0; i <  num_mainmem_frames; i++)
			{
				if(mainmemory[i] == head->addresstodecipher && mainmemorythreadnum[i] == head->processnumber)
				{
					/*
					printf("FOUND: Frame %d in main memory has address %d of process %d\n", i, mainmemory[i], mainmemorythreadnum[i]);
					*/
					isinmainmemory = 1;
					break;
				}
			}
			sem_post(&mainmemorylock);
			
			/*If it is in the main memory then only the "recently used" list is updated*/
			if(isinmainmemory == 1)
			{
			
				printf("[Process %d] accesses address %d (page number = %d, page offset = %s) in main memory (frame number = %d)\n", head->processnumber, head->addresstodecipher, decimalpagenumber, offsetinstring, pagetables[head->processnumber][decimalpagenumber] );
				
				
				printf("\n");
				
				
				/*Updates the "recently used" list */
				sem_wait(&mainmemorylock);
				mainmemoryuse[pagetables[head->processnumber][decimalpagenumber]] = mainmemorycounter++;
				sem_post(&mainmemorylock);
				
				struct addresses* topush = head;
				
				/* Push into garbage queue */
				sem_wait(&garbagelock);
				if(garbagehead == NULL)
				{
					garbagehead = topush;
					garbagetail = topush;
				}
				
				else
				{
					garbagetail->next = topush;
					garbagetail = topush;
				}
				
				
				 
				addressescount--;
				head = head->next;
				
				if(garbagetail != NULL)
				{
					garbagetail->next = NULL;
				}
				sem_post(&garbagelock);
				sem_post(&queuelock);
			}
			
			else
			{
				printf("[Process %d] accesses address %d (page number = %d, page offset = %s) not in main memory\n", head->processnumber, head->addresstodecipher, decimalpagenumber, offsetinstring);
				
				/*TRIGGER PAGE FAULT */
				struct addresses* topush = head;
				
				/*
				printf("Thread %d has triggered a page fault \n",head->processnumber);
				*/
				
				sem_wait(&mainmemorylock);
				if(faulthead == NULL)
				{
					faulthead = topush;
					faulttail = topush;
					
				}
				
				else
				{
					faulttail->next = topush;
					faulttail = topush;
				}
				
				/*
				printf("[Process %d] accesses address %d (page number = %d, page offset = %d) in main memory (frame number = %d)");
				*/
				
				struct addresses* towait = head;
				
				addressescount--;
				head = head->next;
				
				if(faulttail != NULL)
				{
					faulttail->next = NULL;
				}
				
				sem_post(&mainmemorylock);
				
				/*
				printf("Waits while the fault handler handles Process %d's Page %d\n", towait->processnumber,towait->pagenumber);
				*/
				while(oktoprintdone == 0)
				{
					
				}
				/*
				printf("Done waiting\n");
				*/
				printf("[Process %d] accesses address %d (page number = %d, page offset = %s) in main memory (frame number = %d)\n", towait->processnumber, towait->addresstodecipher, decimalpagenumber, offsetinstring, pagetables[towait->processnumber][decimalpagenumber] );
				
				
				oktoprintdone = 0;
				
				
				printf("\n");
				
				
				sem_post(&queuelock);
				
			}
			
			
			/* Frees memory */
			free(stringnum);
			free(temp);
			free(temp2);
			free(binarypagenum);
			free(offsetinstring);
		}

	}
}

void faultHandler()
{
	while(1)
	{
		if(faulthead == NULL)
		{
			if(oktoexitfault == 1)
			{
				/*
				printf("Exiting faultHandler!\n");
				*/
				exit(0);
			}
		}
		
		if(faulthead != NULL)
		{
			sem_wait(&mainmemorylock);
			struct addresses* temp = faulthead;
			
			/*Look for a free frame */
			int freeframefound = 0;
			int i;
			for(i = 0; i < num_mainmem_frames; i++)
			{
				if(mainmemorythreadnum[i] == 0)
				{
					freeframefound = 1;
					printf("[Process %d] finds a free frame in main memory (frame number = %d)\n",faulthead->processnumber, i);
					break;
				}
			}
			
			/* Looks for the least recently used frame */
			if(freeframefound == 0)
			{
				int smallest = 9999999;
				int smallestposition = 0;
				for( i = 0; i < num_mainmem_frames; i++)
				{
					if(mainmemoryuse[i] < smallest)
					{
						smallest = mainmemoryuse[i];
						smallestposition = i;
					}
				}
				
				i = smallestposition;
				
				printf("[Process %d] replaces a frame (frame number = %d) from the main memory\n", faulthead->processnumber,i);
			}
			
			printf("[Process %d] issues an I/O operation to swap in demanded page (page number = %d)\n",faulthead->processnumber, faulthead->pagenumber);
			
			/*Updates the page table of the page that was previously occupying the frame */
			if (mainmemorythreadnum[i] != 0)
			{
				int position = 0;
				for(position = 0; position < num_pages; position++)
				{
					if(pagetables[mainmemorythreadnum[i]][position] == i)
					{
						pagetables[mainmemorythreadnum[i]][position] = 0;
						pagetableshasframe[mainmemorythreadnum[i]][position] = 0;
						
						/*
						printf("Page %d of the table of Process %d has %d in it's valid bit\n", position, mainmemorythreadnum[i], pagetableshasframe[mainmemorythreadnum[i]][position]);
						*/
						break;
					}
				}
			}
			
			/*Updates main memory */
			mainmemory[i] = faulthead->addresstodecipher;
			mainmemoryuse[i] = mainmemorycounter++;
			mainmemorythreadnum[i] = faulthead->processnumber;
			
			sleep(1);
			
			
			
			/* If the pagetable entry of the process has already been occupied by another address with the same page number, remove its entries from the main memory to reflect the page being replaced by a new address */
			if(pagetableshasframe[faulthead->processnumber][faulthead->pagenumber] == 1)
			{
				mainmemory[pagetables[faulthead->processnumber][faulthead->pagenumber]] = 0;
				mainmemorythreadnum[pagetables[faulthead->processnumber][faulthead->pagenumber]] = 0;
			}
		
			/*Updates the corresponding page table */
			pagetables[faulthead->processnumber][faulthead->pagenumber] = i;
			pagetableshasframe[faulthead->processnumber][faulthead->pagenumber] = 1;
			
			/*
			printf("Page %d of process %d now holds frame %d\n", faulthead->pagenumber, faulthead->processnumber, pagetables[faulthead->processnumber][faulthead->pagenumber]);
			*/
			
			printf("[Process %d] demanded page (page number = %d) has been swapped in main memory (frame number = %d)\n", mainmemorythreadnum[i], faulthead->pagenumber, i);

			/* Signals the memmanager to print */
			oktoprintdone = 1;
			
			struct addresses* topush = faulthead;
			
			/* printf("Thread %d's page fault has been handled\n",faulthead->processnumber); */

			
			sem_wait(&garbagelock);
			
			/*Push used address into the garbage queue */
			if(garbagehead == NULL)
			{
				garbagehead = topush;
				garbagetail = topush;
			}
			else
			{
				garbagetail->next = topush;
				garbagetail = topush;
			}
			
			faulthead = faulthead->next;
			garbagetail->next = NULL;
			
			sem_post(&garbagelock);
			sem_post(&mainmemorylock);
			
		}
	}
}


/* HELPER METHOD */
char* convertToBinary(int toconvert)
{
	int n = toconvert;
	char* stringnum = malloc(20* sizeof(char));
	char* temp = malloc(10*sizeof(char));
	char* temp2 = malloc(20* sizeof(char));
	strcpy(stringnum, "");
	strcpy(temp, "");
	strcpy(temp2, "");
	while(n > 0)
	{
		int a = n%2;
		sprintf(temp,"%d", a);
		strcpy(temp2, stringnum);
		strcpy(stringnum, temp);
		strcat(stringnum, temp2);
		
		n = n/2;
	}
	
	free(temp);
	free(temp2);
	/*
    printf("%d in binary is %s\n", toconvert, stringnum);
	*/
	return stringnum;
}

int convertToDecimal(long toconvert)
{
	long n = toconvert;

	long temp = n;
		  
	int decimal=0, i=0, rem;
	while (n!=0)
	{
		rem = n%10;
		n/=10;
		decimal += rem*pow(2,i);
		++i;
	}
	
	return decimal;
}

char* lastx(int s, char *input)
{
    char *ret = malloc (100* sizeof(char));
	int i;
    for (i = 0; i <= s; i++)
    {
        ret[i] = input[strlen(input) - (s - i)];
    }
    return ret;
}
