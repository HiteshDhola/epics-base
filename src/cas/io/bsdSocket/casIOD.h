//
// $Id$
//
// casIOD.h - Channel Access Server IO Dependent for BSD sockets 
// 
//
// Some BSD calls have crept in here
//
// $Log$
// Revision 1.7  1997/01/10 21:18:55  jhill
// code around gnu g++ inline bug when -O isnt used
//
// Revision 1.6  1996/11/02 00:54:45  jhill
// many improvements
//
// Revision 1.5  1996/09/16 18:25:15  jhill
// vxWorks port changes
//
// Revision 1.4  1996/09/04 20:29:08  jhill
// removed os depen stuff
//
// Revision 1.3  1996/08/13 23:00:29  jhill
// removed include of netdb.h
//
// Revision 1.2  1996/07/24 22:03:36  jhill
// fixed net proto for gnu compiler
//
// Revision 1.1.1.1  1996/06/20 00:28:18  jhill
// ca server installation
//
//

#ifndef includeCASIODH
#define includeCASIODH 

#include "envDefs.h"

void hostNameFromIPAddr (const caNetAddr *pAddr, 
			char *pBuf, unsigned bufSize);

class casDGClient;

//
// casDGIntfIO
//
class casDGIntfIO {
public:
	casDGIntfIO (casDGClient &clientIn);
	caStatus init(const caNetAddr &addr, unsigned connectWithThisPort,
		int autoBeaconAddr=TRUE, int addConfigBeaconAddr=FALSE, 
		int useBroadcastAddr=FALSE, casDGIntfIO *pAltOutIn=NULL);
	virtual ~casDGIntfIO();

	int getFD() const;
	void xSetNonBlocking();
	void sendBeacon(char &msg, bufSizeT length, aitUint32 &m_ipa, aitUint16 &m_port);
	casIOState state() const;
	void clientHostName (char *pBuf, unsigned bufSize) const;
	
	xSendStatus osdSend (const char *pBuf, bufSizeT nBytesReq, 
		bufSizeT &nBytesActual, const caNetAddr &addr);
	xRecvStatus osdRecv (char *pBuf, bufSizeT nBytesReq, 
		bufSizeT &nBytesActual, caNetAddr &addr);
	void osdShow (unsigned level) const;

	static bufSizeT optimumBufferSize ();

	//
	// The server's port number
	// (to be used for connection requests)
	//
	unsigned serverPortNumber();

	virtual caStatus start()=0; // OS specific

	void processDG();

private:
        ELLLIST        		beaconAddrList;
	casDGIntfIO		*pAltOutIO;
	casDGClient		&client;
        SOCKET         		sock;
        casIOState     		sockState;
	aitInt16		connectWithThisPort;
	aitInt16		dgPort;
};

struct ioArgsToNewStreamIO {
	caNetAddr addr;
	SOCKET sock;
};

//
// casDGIO 
//
class casDGIO : public casDGClient {
public:
	casDGIO(caServerI &cas) :
		casDGClient(cas) {}

	//
	// clientHostName()
	//
	void clientHostName (char *pBufIn, unsigned bufSizeIn) const;
};

//
// casStreamIO
//
class casStreamIO : public casStrmClient {
public:
	casStreamIO(caServerI &cas, const ioArgsToNewStreamIO &args);
	caStatus init();
	~casStreamIO();

	int getFD() const;
	void xSetNonBlocking();

	casIOState state() const;
	void clientHostName (char *pBuf, unsigned bufSize) const;
	
	xSendStatus osdSend (const char *pBuf, bufSizeT nBytesReq, 
		bufSizeT &nBytesActual);
	xRecvStatus osdRecv (char *pBuf, bufSizeT nBytesReq, 
		bufSizeT &nBytesActual);

	xBlockingStatus blockingState() const;

	bufSizeT incommingBytesPresent() const;

	static bufSizeT optimumBufferSize ();

	void osdShow (unsigned level) const;

	//
	// The server's port number
	// (to be used for connection requests)
	//
	unsigned serverPortNumber();

	const caNetAddr getAddr()
	{
		return caNetAddr(this->addr);
	}
private:
        casIOState              sockState;
        SOCKET                  sock;
        struct sockaddr_in	addr;
	xBlockingStatus		blockingFlag;
};

class caServerIO;
class casDGClient;

class casStreamOS;

//
// casIntfIO
//
class casIntfIO {
public:
	casIntfIO();
	//constructor does not return status
	caStatus init(const caNetAddr &addr, casDGClient &dgClientIn,
		int autoBeaconAddr, int addConfigBeaconAddr); 
	virtual ~casIntfIO();
	void show(unsigned level) const;

        unsigned portNumber() const;

        int getFD() const;

        void setNonBlocking();

       	// 
        // called when we expect that a virtual circuit for a
        // client can be created
       	// 
       	casStreamOS *newStreamClient(caServerI &cas) const;
	
	virtual casDGIntfIO *newDGIntfIO (casDGClient &dgClientIn) const = 0;

	void requestBeacon();
private:
	casDGIntfIO *pNormalUDP;	// attached to this intf's addr
	casDGIntfIO *pBCastUDP;	// attached to this intf's broadcast addr
	SOCKET sock;
        struct sockaddr_in addr;
};

//
// caServerIO
//
class caServerIO {
public:
        caStatus init(caServerI &cas); //constructor does not return status
	~caServerIO();
 
        //
        // show status of IO subsystem
        //
        void show (unsigned level) const;
 
private:

        //
        // static member data
        //
        static int staticInitialized;
        //
        // static member func
        //
        static inline void staticInit();
};


// no additions below this line
#endif // includeCASIODH

