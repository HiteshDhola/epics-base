/*
 *	$Id$	
 *      Author: Jeffrey O. Hill
 *              hill@luke.lanl.gov
 *              (505) 665 1831
 *      Date:  9-93
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
 *      Modification Log:
 *      -----------------
 * $Log$
 * Revision 1.15  1995/08/22  00:22:07  jhill
 * Dont recompute connection timers if the time stamp hasnt changed
 *
 *
 */

#include <unistd.h>
#include <pwd.h>
#include <sys/param.h>

#include "iocinf.h"


/*
 * cac_gettimeval
 */
void cac_gettimeval(struct timeval  *pt)
{
        struct timezone tz;
	int		status;

	/*
	 * Not POSIX but available on most of the systems that we use
	 */
        status = gettimeofday(pt, &tz);
	assert(status == 0);
}


/*
 * cac_block_for_io_completion()
 */
void cac_block_for_io_completion(struct timeval *pTV)
{
	cac_mux_io (pTV);
}

/*
 * cac_block_for_sg_completion()
 */
void cac_block_for_sg_completion(CASG *pcasg, struct timeval *pTV)
{
	cac_mux_io (pTV);
}


/*
 * os_specific_sg_io_complete()
 */
void os_specific_sg_io_complete(CASG *pcasg)
{
}


/*
 * does nothing but satisfy undefined
 */
void os_specific_sg_create(CASG *pcasg)
{
}
void os_specific_sg_delete(CASG *pcasg)
{
}


/*
 * CAC_ADD_TASK_VARIABLE()
 */
int cac_add_task_variable(struct ca_static *ca_temp)
{
	ca_static = ca_temp;
	return ECA_NORMAL;
}


/*
 * cac_os_depen_init()
 */
int cac_os_depen_init(struct ca_static *pcas)
{
        int 			status;
	struct sigaction	sa;

	ca_static = pcas;

	/*
	 * dont allow disconnect to terminate process
	 * when running in UNIX environment
	 *
	 * allow error to be returned to sendto()
	 * instead of handling disconnect at interrupt
	 */
	status = sigaction(SIGPIPE, NULL, &sa);
	if (status==0) {
		if (sa.sa_handler == SIG_DFL) {
			sa.sa_handler = SIG_IGN;
			status = sigaction(SIGPIPE, &sa, NULL);
			if (status) {
				ca_printf(
				"%s: Error from signal replace was \"%s\"\n",
				__FILE__,
				strerror(MYERRNO));
			}
		}
	}
	else {
		ca_printf(
		"%s: Error from signal query was \"%s\"\n",
		__FILE__,
		strerror(MYERRNO));
	}

	status = ca_os_independent_init ();

        return status;
}


/*
 * cac_os_depen_exit ()
 */
void cac_os_depen_exit (struct ca_static *pcas)
{
	ca_static = pcas;
	ca_process_exit();
	ca_static = NULL;

	free ((char *)pcas);
}


/*
 *
 * This should work on any POSIX compliant OS
 *
 * o Indicates failure by setting ptr to nill
 */
char *localUserName()
{
	int	length;
	char	*pName;
	char	*pTmp;

	pName = getlogin();
	if(!pName){
		pName = getpwuid(getuid())->pw_name;
		if(!pName){
			pName = "";
		}
	}

	length = strlen(pName)+1;
	pTmp = malloc(length);
	if(!pTmp){
		return pTmp;
	}
	strncpy(pTmp, pName, length-1);
	pTmp[length-1] = '\0';

	return pTmp;
}



/*
 * ca_spawn_repeater()
 */
void ca_spawn_repeater()
{
	int     status;
	char	*pImageName;

	/*
	 * create a duplicate process
	 */
	status = fork();
	if (status < 0){
		SEVCHK(ECA_NOREPEATER, NULL);
		return;
	}

	/*
 	 * return to the caller
	 * if its in the initiating process
	 */
	if (status){
		return;
	}

	/*
 	 * running in the repeater process
	 * if here
	 */
	pImageName = "caRepeater";
	status = execlp(pImageName, NULL);
	if(status<0){	
		ca_printf("!!WARNING!!\n");
		ca_printf("The executable \"%s\" couldnt be located\n", pImageName);
		ca_printf("because - %s\n", strerror(MYERRNO));
		ca_printf("You may need to modify your PATH environment variable.\n");
		ca_printf("Creating CA repeater with fork() system call.\n");
		ca_printf("Repeater will inherit parents process name and resources.\n");
		ca_printf("Duplicate resource consumption may occur.\n");
		ca_repeater();
		assert(0);
	}
	exit(0);
}


/*
 * caSetDefaultPrintfHandler ()
 * use the normal default here
 * ( see access.c )
 */
void caSetDefaultPrintfHandler ()
{
        ca_static->ca_printf_func = epicsVprintf;
}

