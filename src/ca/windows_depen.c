/*
 *	$Id$	
 *      Author: Jeffrey O. Hill, Chris Timossi
 *              hill@luke.lanl.gov
 *		CATimossi@lbl.gov
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
 *              Lawrence Berkley National Laboratory
 *
 *      Modification Log:
 *      -----------------
 *
 */

/*
 * Windows includes
 */
#include <process.h>

#include "iocinf.h"

#ifndef WIN32
#error This source is specific to WIN32 
#endif

static int get_subnet_mask ( char SubNetMaskStr[256]);
static int RegTcpParams (char IpAddr[256], char SubNetMask[256]);
static int RegKeyData (CHAR *RegPath, HANDLE hKeyRoot, LPSTR lpzValueName, 
                      LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData );


/*
 * cac_gettimeval
 */
void cac_gettimeval(struct timeval  *pt)
{
        SYSTEMTIME st;

	GetSystemTime(&st);
	pt->tv_sec = time(NULL);
	pt->tv_usec = st.wMilliseconds*1000;
}


/*
 * cac_block_for_io_completion()
 */
void cac_block_for_io_completion(struct timeval *pTV)
{
	cac_mux_io(pTV);
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


void cac_block_for_sg_completion(CASG *pcasg, struct timeval *pTV)
{
	cac_mux_io(pTV);
}


/*
 * cac_os_depen_init()
 */
int cac_os_depen_init(struct ca_static *pcas)
{
    	int status;
	WSADATA WsaData;

	ca_static = pcas;

	/*
	 * dont allow disconnect to terminate process
	 * when running in UNIX enviroment
	 *
	 * allow error to be returned to sendto()
	 * instead of handling disconnect at interrupt
	 */

	/* signal(SIGPIPE,SIG_IGN); */

#	ifdef _WINSOCKAPI_
		status = WSAStartup(MAKEWORD(1,1), &WsaData);
		assert (status==0);
#	endif

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
    	int     length;
    	char    *pName;
    	char    *pTmp;
	char    Uname[] = "";

    	pName = getenv("USERNAME");
	if (!pName) {
		pName = Uname;
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
 	 * running in the repeater process
	 * if here
	 */
	pImageName = "caRepeater.exe";
	//status = system(pImageName);
	//Need to check if repeater is already loaded
	//For now, start Repeater from a command line, not here
	status = 0;
	//status = _spawnlp(_P_DETACH,pImageName,"");

	if(status<0){	
		ca_printf("!!WARNING!!\n");
		ca_printf("Unable to locate the executable \"%s\".\n", 
			pImageName);
		ca_printf("You may need to modify your environment.\n");
	}
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



/*
 *
 * Network interface routines
 *
 */

/*
 * local_addr()
 *
 * return 127.0.0.1
 * (the loop back address)
 */
int local_addr (SOCKET s, struct sockaddr_in *plcladdr)
{
	ca_uint32_t	loopBackAddress = 0x7f000001;

	plcladdr->sin_family = AF_INET;
	plcladdr->sin_port = 0;
	plcladdr->sin_addr.s_addr = ntohl (loopBackAddress);
	return OK;
}


/*
 *  	caDiscoverInterfaces()
 *
 *	This routine is provided with the address of an ELLLIST a socket
 * 	and a destination port number. When the routine returns there
 *	will be one additional inet address (a caAddrNode) in the list 
 *	for each inet interface found that is up and isnt a loop back 
 *	interface. If the interface supports broadcast then I add its
 *	broadcast address to the list. If the interface is a point to 
 *	point link then I add the destination address of the point to
 *	point link to the list. In either case I set the port number
 *	in the address node to the port supplied in the argument
 *	list.
 *
 * 	LOCK should be applied here for (pList)
 * 	(this is also called from the server)
 */
void caDiscoverInterfaces(ELLLIST *pList, SOCKET socket, int port)
{
	struct in_addr bcast_addr;
	caAddrNode		*pNode;

	pNode = (caAddrNode *) calloc(1,sizeof(*pNode));
	if(!pNode){
		return;
	}
	broadcast_addr(&bcast_addr);
	pNode->destAddr.inetAddr.sin_addr.s_addr = bcast_addr.s_addr;	//broadcast addr
	pNode->destAddr.inetAddr.sin_port = htons(port);
	pNode->destAddr.inetAddr.sin_family = AF_INET;
	//pNode->srcAddr.inetAddr = 0 ;//localAddr;

	/*
	 * LOCK applied externally
	 */
	 ellAdd(pList, &pNode->node);
}

int
broadcast_addr( struct in_addr *pcastaddr )
{
	char netmask[256], lhostname[80];
	static struct in_addr castaddr;
	int status;
	static char     	init = FALSE;
	struct hostent *phostent;
	unsigned long laddr;

	if (init) {
		*pcastaddr = castaddr;
		return OK;
	}
   gethostname(lhostname,sizeof(lhostname));
   phostent = gethostbyname(lhostname);
   if (!phostent) {
   		return MYERRNO;
   }

   if (status = get_subnet_mask(netmask))
   		return ERROR;

   laddr = *( (unsigned long *) phostent->h_addr_list[0]);
   castaddr.s_addr = (laddr & inet_addr(netmask)) | ~inet_addr(netmask);

	if (!init){
		init = TRUE;
		*pcastaddr = castaddr;
	}
	return OK;
}

static int get_subnet_mask ( char SubNetMaskStr[256])
{
   char localadr[256];
   
   return RegTcpParams (localadr, SubNetMaskStr);
}

	   /* For NT 3.51, enumerates network interfaces returns the ip address */
	   /* and subnet mask for the LAST interface found. This needs to be changed */
	   /* to work in conjuction with caDiscoverInterfaces to add all the */
	   /* add all the interfaces to the elist. Also could be more efficient in
calling */
	   /* RegKeyOpen                                                            */

static int RegTcpParams (char IpAddrStr[256], char SubNetMaskStr[256])
{
#define MAX_VALUE_NAME              128
  static CHAR   ValueName[MAX_VALUE_NAME];
  static CHAR   RegPath[256];
  DWORD  cbDataLen;
  CHAR   cbData[256];
  DWORD  dwType;
  int status, i, card_cnt;
  char *pNetCard[16], *pData;

	static char IpAddr[256], SubNetMask[256];

	cbDataLen = sizeof(cbData);

	/****
	strcpy(RegPath,"SOFTWARE\\Microsoft\\Windows
NT\\CurrentVersion\\NetworkCards\\1");
	status = RegKeyData (RegPath, HKEY_LOCAL_MACHINE, "ServiceName", &dwType,
cbData, &cbDataLen);
	if (status) {
		strcpy(RegPath,"SOFTWARE\\Microsoft\\Windows
NT\\CurrentVersion\\NetworkCards\\01");
		status = RegKeyData (RegPath, HKEY_LOCAL_MACHINE, "ServiceName", &dwType,
cbData, &cbDataLen);
		if (status)
			return status;
	}
	****/

	strcpy(RegPath,"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage");
	status = RegKeyData (RegPath, HKEY_LOCAL_MACHINE, "Route", &dwType, cbData,
&cbDataLen);
	if (status) {
		return status;
	}

	i=0; card_cnt = 0; pData = cbData;	/* enumerate network interfaces */

	while( i < 16 && (pNetCard[i]=strtok(pData,"\""))  ) {
		strcpy(RegPath,"SYSTEM\\CurrentControlSet\\Services\\");
		strcat(RegPath,pNetCard[i]);
		strcat(RegPath,"\\Parameters\\Tcpip");

   		cbDataLen = sizeof(IpAddr);
   		status = RegKeyData (RegPath, HKEY_LOCAL_MACHINE, "IPAddress", &dwType,
IpAddr, &cbDataLen);
   		if (status == 0)  {
   			cbDataLen = sizeof(SubNetMask);
   			status = RegKeyData (RegPath, HKEY_LOCAL_MACHINE, "SubnetMask",
&dwType, SubNetMask, &cbDataLen);
   			if (status)
      			return status;
			card_cnt++;
		}
		pData += strlen(pNetCard[i])+3;
		i++;
   } 

   if (card_cnt == 0)
   		return 1;

   strcpy(IpAddrStr,IpAddr);
   
   strcpy(SubNetMaskStr,SubNetMask);

   return 0;
}


static int RegKeyData (CHAR *RegPath, HANDLE hKeyRoot, LPSTR lpzValueName, 
                      LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData )
  {
  HKEY   hKey;
   DWORD  retCode;

  DWORD  dwcClassLen = MAX_PATH;

  // OPEN THE KEY.

  retCode = RegOpenKeyEx (hKeyRoot,    // Key handle at root level.
                          RegPath,     // Path name of child key.
                          0,           // Reserved.
                          KEY_QUERY_VALUE, // Requesting read access.
                          &hKey);      // Address of key to be returned.

  if (retCode)
    {
    //wsprintf (Buf, "Error: RegOpenKeyEx = %d", retCode);
    return -1;
    }


  retCode = RegQueryValueEx (hKey,        // Key handle returned from
RegOpenKeyEx.
                          lpzValueName,   // Name of value.
                          NULL,        // Reserved, dword = NULL.
                          lpdwType,     // Type of data.
                          lpbData,       // Data buffer.
                          lpcbData);    // Size of data buffer.

   if (retCode)
    {
    //wsprintf (Buf, "Error: RegQIK = %d, %d", retCode, __LINE__);
    return -2;
    }

   return 0;

 }

BOOL epicsShareAPI DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	int status;
	WSADATA WsaData;

	switch (dwReason)  {

	case DLL_PROCESS_ATTACH:

		if ((status = WSAStartup(MAKEWORD(1,1), &WsaData)) != 0)
			return FALSE;
#if _DEBUG
		if (AllocConsole())	{
			SetConsoleTitle("Channel Access Status");
    		freopen( "CONOUT$", "a", stderr );
			fprintf(stderr, "Process attached to ca.dll R12\n");
		}
#endif
		break;

	case DLL_PROCESS_DETACH:
		
		if ((status = WSACleanup()) !=0)
			return FALSE;
		break;

	case DLL_THREAD_ATTACH:
#if _DEBUG
		fprintf(stderr, "Thread attached to ca.dll R12\n");
#endif
		break;

	case DLL_THREAD_DETACH:
#if _DEBUG
		fprintf(stderr, "Thread detached from ca.dll R12\n");
#endif
		break;

	default:
		break;
	}

return TRUE;

}


