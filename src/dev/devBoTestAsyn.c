/* devBoTestAsyn.c */
/* share/src/dev $Id$ */

/* devBoTestAsyn.c - Device Support Routines for testing asynchronous processing*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
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
 * .02  mm-dd-yy        iii     Comment
 *      ...
 */



#include	<vxWorks.h>
#include	<types.h>
#include	<stdioLib.h>
#include	<wdLib.h>

#include	<alarm.h>
#include	<cvtTable.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<recSup.h>
#include	<devSup.h>
#include	<link.h>
#include	<boRecord.h>

/* Create the dset for devBoTestAsyn */
long init_record();
long write_bo();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_bo;
	DEVSUPFUN	special_linconv;
}devBoTestAsyn={
	6,
	NULL,
	NULL,
	init_record,
	NULL,
	write_bo,
	NULL};

/* control block for callback*/
struct callback {
	void (*callback)();
	int priority;
	struct dbAddr dbAddr;
	WDOG_ID wd_id;
	void (*process)();
};
void callbackRequest();

static void myCallback(pcallback)
    struct callback *pcallback;
{
    struct boRecord *pbo=(struct boRecord *)(pcallback->dbAddr.precord);

    dbScanLock(pbo);
    (pcallback->process)(&pcallback->dbAddr);
    dbScanUnlock(pbo);
}
    
    

static long init_record(pbo,process)
    struct boRecord	*pbo;
    void (*process)();
{
    char message[100];
    struct callback *pcallback;

    /* bo.out must be a CONSTANT*/
    switch (pbo->out.type) {
    case (CONSTANT) :
	pcallback = (struct callback *)(calloc(1,sizeof(struct callback)));
	pbo->dpvt = (caddr_t)pcallback;
	pcallback->callback = myCallback;
	pcallback->priority = priorityLow;
	if(dbNameToAddr(pbo->name,&(pcallback->dbAddr))) {
		logMsg("dbNameToAddr failed in init_record for devBoTestAsyn\n");
		exit(1);
	}
	pcallback->wd_id = wdCreate();
	pcallback->process = process;
	break;
    default :
	strcpy(message,pbo->name);
	strcat(message,": devBoTestAsyn (init_record) Illegal OUT field");
	errMessage(S_db_badField,message);
	return(S_db_badField);
    }
    return(2);
}

static long write_bo(pbo)
    struct boRecord	*pbo;
{
    char message[100];
    long status,options,nRequest;
    struct callback *pcallback=(struct callback *)(pbo->dpvt);
    int		wait_time;

    /* bo.out must be a CONSTANT*/
    switch (pbo->out.type) {
    case (CONSTANT) :
	if(pbo->pact) {
		printf("%s Completed\n",pbo->name);
		return(0); /* don`t convert*/
	} else {
		wait_time = (int)(pbo->disv * vxTicksPerSecond);
		if(wait_time<=0) return(0);
		printf("%s Starting asynchronous processing\n",pbo->name);
		wdStart(pcallback->wd_id,wait_time,callbackRequest,pcallback);
		return(1);
	}
    default :
	if(pbo->nsev<VALID_ALARM) {
		pbo->nsev = VALID_ALARM;
		pbo->nsta = SOFT_ALARM;
		if(pbo->stat!=SOFT_ALARM) {
			strcpy(message,pbo->name);
			strcat(message,": devBoTestAsyn (read_bo) Illegal OUT field");
			errMessage(S_db_badField,message);
		}
	}
    }
    return(0);
}
