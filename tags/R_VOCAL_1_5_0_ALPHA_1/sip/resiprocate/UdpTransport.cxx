
#include <util/Socket.hxx>

#ifndef WIN32
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <util/Data.hxx>
#include <sipstack/UdpTransport.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/Preparse.hxx>
#include <util/Logger.hxx>
#include <util/Socket.hxx>

#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace std;
using namespace Vocal2;

const size_t UdpTransport::MaxBufferSize = 8192;


UdpTransport::UdpTransport(int portNum, Fifo<Message>& fifo) : 
   Transport(portNum, fifo)
{
   mFd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if ( mFd == INVALID_SOCKET )
   {
      InfoLog (<< "Failed to open socket: " << portNum);
   }
   
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY); // !jf! 
   addr.sin_port = htons(portNum);
   
   if ( bind( mFd, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR )
   {
      int err = errno;
      if ( err == EADDRINUSE )
      {
         InfoLog (<< "Address already in use");
      }
      else
      {
         InfoLog (<< "Could not bind to port: " << portNum);
      }
      
      throw TransportException("Address already in use", __FILE__,__LINE__);
   }

   // make non blocking 
#if WIN32
   unsigned long block = 0;
   int errNoBlock = ioctlsocket( mFd, FIONBIO , &block );
   assert( errNoBlock == 0 );
#else
   int flags  = fcntl( mFd, F_GETFL, 0);
   int errNoBlock = fcntl(mFd,F_SETFL, flags| O_NONBLOCK );
   assert( errNoBlock == 0 );
#endif

}


UdpTransport::~UdpTransport()
{
}


void 
UdpTransport::send( const sockaddr_in& dest,
                    const  char* buffer,
                    const size_t length) //, TransactionId txId)
{
   SendData* data = new  SendData(dest);
   data->buffer = buffer;
   data->length = length;
   //data->tid = txId;

   DebugLog (<< "Adding message to tx buffer: " << string(buffer, length));
   
   mTxFifo.add(data); // !jf!
}


void 
UdpTransport::process()
{
   // pull buffers to send out of TxFifo
   // receive datagrams from fd
   // preparse and stuff into RxFifo

   
   // how do we know that buffer won't get deleted on us !jf!
   if (mTxFifo.messageAvailable())
   {
      SendData* data = mTxFifo.getNext();
      DebugLog (<< "Sending message on udp");
      int count = sendto(mFd, data->buffer, data->length, 0,
                                    (const sockaddr*)&data->destination, 
                                    int(sizeof(data->destination)) );
   
      if ( count == SOCKET_ERROR )
      {
         DebugLog (<< strerror(errno));
         // !jf! what to do if it fails
         assert(0);
      }

      assert ( (count == int(data->length) ) || (count == SOCKET_ERROR ) );
   }

// #define UDP_SHORT   

   struct sockaddr_in from;

#if !defined(UDP_SHORT)
   // !ah! debug is just to always return a sample message
   // !jf! this may have to change - when we read a message that is too big
   
   char* buffer = new char[MaxBufferSize];
   int fromLen = sizeof(from);
   
   // !jf! how do we tell if it discarded bytes 
   // !ah! we use the len-1 trick :-(

   int len = recvfrom( mFd,
                       buffer,
                       MaxBufferSize,
                       0 /*flags */,
                       (struct sockaddr*)&from,
                       (socklen_t*)&fromLen);

      
#else

#define CRLF "\r\n"
   
   char buffer[] = 
      "INVITE foo@bar.com SIP/2.0" CRLF
      "Via: SIP/2.0/UDP pc33.atlanta.com;branch=foo" CRLF
      "To: <sip:alan@ieee.org>" CRLF
      "From: <sip:cj@whistler.com>"CRLF
      "Route: <sip:1023@1.2.3.4>, <sip:1023@3.4.5.6>, <sip:1023@10.1.1.1>" CRLF
      "Subject: Good Morning!"CRLF
      "Call-Id: 123" CRLF
      CRLF
   ;
   int len = sizeof(buffer)/sizeof(*buffer);
   static int fired = 0;
   if (!fired)
   {
      fired++;
   }
   else
   {
      len = 0;
   }
     
#endif

   if ( len == SOCKET_ERROR )
   {
      int err = errno;
      ErrLog(<<"Error receiving, errno="<<err);
   }
   else if (len > 0)
   {
	   // TODO - the next line is really really gross
      unsigned long len = static_cast<unsigned long>(len);
      
      if (len == MaxBufferSize)
      {
         InfoLog(<<"Datagram exceeded max length "<<MaxBufferSize);
         InfoLog(<<"Possibly truncated");
      }
      DebugLog( << "UDP Rcv : " << len << " b" );
      
      SipMessage* message = new SipMessage;
      
      // set the received from information into the received= parameter in the
      // via

      // It is presumed that UDP Datagrams are arriving atomically and that
      // each one is a unique SIP message


      message->setSource(from);

      // Tell the SipMessage about this datagram buffer.
      message->addBuffer(buffer);

      Preparse preParser(*message, buffer, len);

      bool ppStatus = preParser.process();
      // this won't work if UDPs are fragd !ah!

      if (!ppStatus)
      {
         // ppStatus will be false ONLY when the DATAGRAM did not
         // contain a Preparsable byte-stream. In the UDP transport,
         // this is an error. 

         // OTHER TRANSPORTS will have to handle fragmentation when
         // this condition is set.
         // determine that there is a fragment that needs to be done
         // alloc buffer to hold remainder and next network fragment
         // as per !cj! ideas on anti-frag.
         // need interface to Preparser that can detect frags remaining. !ah!
         // ?? think about this design.

         InfoLog(<<"Rejecting datagram as unparsable.");
         delete message;
      }
      else
      {
      
         DebugLog (<< "adding new SipMessage to state machine's Fifo: "
                   << message);
         mStateMachineFifo.add(message);
      }
   }
}