/* share/src/libCom $Id$ 
 * memDebugLib.c
 *      Author:          Jeff Hill 
 *      Date:            03-29-93
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .01  mm-dd-yy        iii     Comment
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <ellLib.h>

#ifdef vxWorks
#define LOCKS_REQUIRED
#include <tickLib.h>
#endif /*vxWorks*/

#ifdef LOCKS_REQUIRED
#include <fast_lock.h>
#else /*LOCKS_REQUIRED*/
#define FASTLOCK(A) 
#define FASTUNLOCK(A)
#define FASTLOCKINIT(A)
#endif /*LOCKS_REQUIRED*/

unsigned memDebugLevel = 1;

#define debugMallocMagic  0xaaaaaaaa

typedef struct debugMallocFooter{
        unsigned long   magic;
}DMF;

typedef struct debugMallocHeader{
        ELLNODE         node;
        char            *pFile;
        char            *pUser;
        DMF             *pFoot;
        unsigned long   line;
        unsigned long   size;
        unsigned long   tick;
        unsigned long   magic;
}DMH;

#include <memDebugLib.h>
#undef free
#undef malloc
#undef calloc

#define LOCAL static

#ifdef LOCKS_REQUIRED 
LOCAL int memDebugInit;
LOCAL FAST_LOCK	memDebugLock;
#endif /*LOCKS_REQUIRED*/ 

#ifdef __STDC__
LOCAL int memDebugVerify(DMH *pHdr);
#else /*__STDC__*/
LOCAL int memDebugVerify();
#endif /*__STDC__*/

LOCAL ELLLIST memDebugList;


/*
 * memDebugMalloc()
 */
#ifdef __STDC__ 
char *memDebugMalloc(char *pFile, unsigned long line, unsigned long size)
#else /*__STDC__*/
char *memDebugMalloc(pFile, line, size)
char 		*pFile;
unsigned long 	line;
unsigned long 	size;
#endif /*__STDC__*/
{
	DMH	*pHdr;

	pHdr = (DMH *)calloc(1,sizeof(DMH)+sizeof(DMF)+size);
	if(!pHdr){
		return NULL;
	}
	pHdr->pUser = (char *) (pHdr+1);
	pHdr->pFoot = (DMF *) (pHdr->pUser+size);
	pHdr->pFile = pFile;
	pHdr->line = line;
	pHdr->size = size;
	pHdr->magic = debugMallocMagic;
#ifdef vxWorks
	pHdr->tick = tickGet();
#else /*vxWorks*/
	pHdr->tick = 0;
#endif /*vxWorks*/
	pHdr->pFoot->magic = debugMallocMagic;

#ifdef LOCKS_REQUIRED
	if(!memDebugInit){
		memDebugInit = 1;
		FASTLOCKINIT(&memDebugLock);
	}
#endif /*LOCKS_REQUIRED*/

	FASTLOCK(&memDebugLock);
	ellAdd(&memDebugList, &pHdr->node);
	FASTUNLOCK(&memDebugLock);

	if(memDebugLevel>2){
		fprintf(stderr, "%08x=malloc(%d) %s.%d\n", 
			pHdr->pUser, size, pFile, line);
	}

	return pHdr->pUser;
}



/*
 * memDebugCalloc()
 */
#ifdef __STDC__
char *memDebugCalloc(char *pFile, unsigned long line, 
	unsigned long nelem, unsigned long elsize)
#else /*__STDC__*/
char *memDebugCalloc(pFile, line, nelem, elsize)
char *pFile;
unsigned long line;
unsigned long nelem;
unsigned long elsize;
#endif /*__STDC__*/
{
	return memDebugMalloc(pFile, line, nelem*elsize);
}


/*
 * memDebugFree()
 */
#ifdef __STDC__
void memDebugFree(char *pFile, unsigned long line, void *ptr)
#else /*__STDC__*/
void memDebugFree(pFile, line, ptr)
char *pFile;
unsigned long line;
void *ptr;
#endif /*__STDC__*/
{
	DMH	*pHdr;
	int	status;

	pHdr = -1 + (DMH *) ptr;
	FASTLOCK(&memDebugLock);
	status = ellFind(&memDebugList, &pHdr->node);

	if(status<0 || (pHdr->pUser != ptr)){
		FASTUNLOCK(&memDebugLock);
		fprintf(stderr, "%s.%d free(%08x) failed\n", pFile, line, ptr);
		fprintf(stderr, "malloc occured at %s.%d\n", pHdr->pFile, pHdr->line);
		assert(0);
	}
	ellDelete(&memDebugList, &pHdr->node);
	FASTUNLOCK(&memDebugLock);

	memDebugVerify(pHdr);	

	if(memDebugLevel>2){
		fprintf(stderr, "free(%08x) %s.%d\n", ptr, pFile, line);
		fprintf(stderr, 
			"\tmalloc(%d) at %s.%d\n", 
			pHdr->size,
			pHdr->pFile,
			pHdr->line);
	}

	free(pHdr);
}


/*
 * memDebugVerify()
 */
#ifdef __STDC__
LOCAL int memDebugVerify(DMH *pHdr)
#else /*__STDC__*/
LOCAL int memDebugVerify(pHdr)
DMH *pHdr;
#endif /*__STDC__*/
{
	if(memDebugLevel==0){
		return 1;
	}

	if(pHdr->magic != debugMallocMagic || pHdr->pFoot->magic != debugMallocMagic){
		fprintf(stderr, "block overwritten %x\n", pHdr->pUser);
		fprintf(stderr, "malloc occured at %s.%d\n", pHdr->pFile, pHdr->line);
		return 1;
	}

	return 0;
}


/*
 * memDebugVerifyAll()
 */
int memDebugVerifyAll()
{
	int	status;
	DMH	*pHdr;

	FASTLOCK(&memDebugLock);
	pHdr = (DMH *) ellFirst(&memDebugList);
	while(pHdr = (DMH *) ellNext(pHdr)){
		status = memDebugVerify(pHdr);
	}
	FASTUNLOCK(&memDebugLock);
	return 0;
}


/*
 * memPrintAll()
 */
#ifdef __STDC__
int memDebugPrintAll(unsigned long ignoreBeforeThisTick)
#else /*__STDC__*/
int memDebugPrintAll(ignoreBeforeThisTick)
unsigned long ignoreBeforeThisTick;
#endif /*__STDC__*/
{
	int	status;
	DMH	*pHdr;

	FASTLOCK(&memDebugLock);
	pHdr = (DMH *) ellFirst(&memDebugList);
	while(pHdr = (DMH *) ellNext(pHdr)){
		if(pHdr->tick<ignoreBeforeThisTick){
			continue;
		}
		fprintf(stderr, 
			"%8x = malloc(%d) at %s.%d tick=%d\n", 
			pHdr->pUser,
			pHdr->size,
			pHdr->pFile,
			pHdr->line,
			pHdr->tick);
	}
	FASTUNLOCK(&memDebugLock);
	return 0;
}
