/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "rutil/Logger.hxx"
#include "resip/stack/ConnectionBase.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/WinLeakCheck.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#include "resip/stack/ssl/TlsConnection.hxx"
#endif

#ifdef USE_SIGCOMP
#include <osc/Stack.h>
#include <osc/TcpStream.h>
#include <osc/SigcompMessage.h>
#include <osc/StateChanges.h>
#endif

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

char 
ConnectionBase::connectionStates[ConnectionBase::MAX][32] = { "NewMessage", "ReadingHeaders", "PartialBody" };


ConnectionBase::ConnectionBase(Transport* transport, 
                                 const Tuple& peer, 
                                 Socket socket,
                                 Compression &compression)
   : mSendPos(0),
     mTransport(transport),
     mFailureReason(TransportFailure::None),
     mCompression(compression),
#ifdef USE_SIGCOMP
     mSigcompStack(0),
     mSigcompFramer(0),
#endif
     mSendingTransmissionFormat(Unknown),
     mReceivingTransmissionFormat(Unknown),
     mMessage(0),
     mBuffer(0),
     mBufferPos(0),
     mBufferSize(0),
     mLastUsed(Timer::getTimeMs()),
     mConnState(NewMessage)
{
   Tuple mWho(peer);
   mWho.mFlowKey=socket;
   // deprecated; stop doing this eventually
   mWho.transport=mTransport;
   mWho.transportKey=mTransport ? mTransport->getKey() : 0;
   setPeerAddr(mWho);

   DebugLog (<< "ConnectionBase::ConnectionBase, who: " << mWho << " " << this);
#ifdef USE_SIGCOMP
   if (mCompression.isEnabled())
   {
      DebugLog (<< "Compression enabled for connection: " << this);
      mSigcompStack = new osc::Stack(mCompression.getStateHandler());
      mCompression.addCompressorsToStack(mSigcompStack);
   }
   else
   {
      DebugLog (<< "Compression disabled for connection: " << this);
   }
#else
   DebugLog (<< "No compression library available: " << this);
#endif

}

ConnectionBase::~ConnectionBase()
{
   if(mTransport)
   {
      mTransport->flowTerminated(who());
   }

   while (!mOutstandingSends.empty())
   {
      SendData* sendData = mOutstandingSends.front();
      mTransport->fail(sendData->transactionId, mFailureReason);
      
      delete sendData;
      mOutstandingSends.pop_front();
   }
   delete [] mBuffer;
   delete mMessage;
#ifdef USE_SIGCOMP
   delete mSigcompStack;
#endif

   DebugLog (<< "ConnectionBase::~ConnectionBase " << this);
}

FlowKey
ConnectionBase::getFlowKey() const
{
   return who().mFlowKey;
}

bool
ConnectionBase::preparseNewBytes(int bytesRead, 
                                    std::deque<TransactionMessage*>& fifo,
                                    CongestionManager::RejectionBehavior b,
                                    time_t expectedWait)
{

   DebugLog(<< "In State: " << connectionStates[mConnState]);
   //getConnectionManager().touch(this); -- !dcm!
   
  start:   // If there is an overhang come back here, effectively recursing
   
   switch(mConnState)
   {
      case NewMessage:
      {
         if (strncmp(mBuffer + mBufferPos, Symbols::CRLFCRLF.data(), 4) == 0)
         {
            StackLog(<<"Throwing away incoming firewall keep-alive");
            DebugLog(<< "Got incoming double-CRLF keepalive (aka ping).");
            mBufferPos += 4;
            bytesRead -= 4;
            onDoubleCRLF();
            if (bytesRead)
            {
               goto start;
            }
            else
            {
               delete [] mBuffer;
               mBuffer = 0;
               return true;
            }
         }
         assert(mTransport);
         mMessage = new SipMessage(mTransport);
         
         DebugLog(<< "ConnectionBase::process setting source " << who());
         mMessage->setSource(who());
         mMessage->setTlsDomain(mTransport->tlsDomain());

#ifdef USE_SSL
         // Set TlsPeerName if message is from TlsConnection
         TlsConnection *tlsConnection = dynamic_cast<TlsConnection *>(this);
         if(tlsConnection)
         {
            std::list<Data> peerNameList;
            tlsConnection->getPeerNames(peerNameList);
            mMessage->setTlsPeerNames(peerNameList);
         }
#endif
         mMsgHeaderScanner.prepareForMessage(mMessage);
         // Fall through to the next case.
      }
      case ReadingHeaders:
      {
         unsigned int chunkLength = (unsigned int)mBufferPos + bytesRead;
         char *unprocessedCharPtr;
         MsgHeaderScanner::ScanChunkResult scanChunkResult =
            mMsgHeaderScanner.scanChunk(mBuffer,
                                        chunkLength,
                                        &unprocessedCharPtr);
         if (scanChunkResult == MsgHeaderScanner::scrError)
         {
            //.jacob. Not a terribly informative warning.
            WarningLog(<< "Discarding preparse!");
            delete [] mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            mConnState=NewMessage;
            return false;
         }

         if (mMsgHeaderScanner.getHeaderCount() > 256)
         {
            WarningLog(<< "Discarding preparse; too many headers");
            delete [] mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            mConnState=NewMessage;
            return false;
         }

         unsigned int numUnprocessedChars = 
            (unsigned int)((mBuffer + chunkLength) - unprocessedCharPtr);

         if(numUnprocessedChars > 2048 &&
            scanChunkResult == MsgHeaderScanner::scrNextChunk)
         {
            WarningLog(<< "Discarding preparse; header-field-value (or "
                        "header name) too long");
            delete [] mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            mConnState=NewMessage;
            return false;
         }

         if(numUnprocessedChars==chunkLength)
         {
            // .bwc. MsgHeaderScanner wasn't able to parse anything useful;
            // don't bother mMessage yet, but make more room in mBuffer.
            size_t size = numUnprocessedChars*3/2;
            if (size < ConnectionBase::ChunkSize)
            {
               size = ConnectionBase::ChunkSize;
            }
            char* newBuffer = 0;
            try
            {
               newBuffer=MsgHeaderScanner::allocateBuffer((int)size);
            }
            catch(std::bad_alloc&)
            {
               ErrLog(<<"Failed to alloc a buffer during preparse!");
               return false;
            }
            memcpy(newBuffer, unprocessedCharPtr, numUnprocessedChars);
            delete [] mBuffer;
            mBuffer = newBuffer;
            mBufferPos = numUnprocessedChars;
            mBufferSize = size;
            mConnState = ReadingHeaders;
            return true;
         }

         mMessage->addBuffer(mBuffer);
         mBuffer=0;

         if (scanChunkResult == MsgHeaderScanner::scrNextChunk)
         {
            // Message header is incomplete...
            if (numUnprocessedChars == 0)
            {
               // ...but the chunk is completely processed.
               //.jacob. I've discarded the "assigned" concept.
               //DebugLog(<< "Data assigned, not fragmented, not complete");
               try
               {
                  mBuffer = MsgHeaderScanner::allocateBuffer(ChunkSize);
               }
               catch(std::bad_alloc&)
               {
                  ErrLog(<<"Failed to alloc a buffer during preparse!");
                  return false;
               }
               mBufferPos = 0;
               mBufferSize = ChunkSize;
            }
            else
            {
               // ...but some of the chunk must be shifted into the next one.
               size_t size = numUnprocessedChars*3/2;
               if (size < ConnectionBase::ChunkSize)
               {
                  size = ConnectionBase::ChunkSize;
               }
               char* newBuffer = 0;
               try
               {
                  newBuffer = MsgHeaderScanner::allocateBuffer((int)size);
               }
               catch(std::bad_alloc&)
               {
                  ErrLog(<<"Failed to alloc a buffer during preparse!");
                  return false;
               }
               memcpy(newBuffer, unprocessedCharPtr, numUnprocessedChars);
               mBuffer = newBuffer;
               mBufferPos = numUnprocessedChars;
               mBufferSize = size;
            }
            mConnState = ReadingHeaders;
         }
         else
         {
            size_t contentLength = 0;
            
            try
            {
               // The message header is complete.
               contentLength=mMessage->const_header(h_ContentLength).value();
            }
            catch(resip::ParseException& e)
            {
               WarningLog(<<"Malformed Content-Length in connection-based transport"
                           ". Not much we can do to fix this.  " << e);
               // .bwc. Bad Content-Length. We are hosed.
               delete mMessage;
               mMessage = 0;
               mBuffer = 0;
               // .bwc. mMessage just took ownership of mBuffer, so we don't
               // delete it here. We do zero it though, for completeness.
               //.jacob. Shouldn't the state also be set here?
               return false;
            }
            
            if(contentLength > 10485760)
            {
               // !bwc! No more than 10M, thanks. We should make this
               // configurable.
               WarningLog(<<"Absurdly large Content-Length in connection-based "
                           "transport.");
               delete mMessage;
               mMessage = 0;
               mBuffer = 0;
               // .bwc. mMessage just took ownership of mBuffer, so we don't
               // delete it here. We do zero it though, for completeness.
               //.jacob. Shouldn't the state also be set here?
               return false;
            }

            if (numUnprocessedChars < contentLength)
            {
               // The message body is incomplete.
               DebugLog(<< "partial body received");
               size_t newSize=resipMin(resipMax((size_t)numUnprocessedChars*3/2,
                                             (size_t)ConnectionBase::ChunkSize),
                                    contentLength);
               char* newBuffer = MsgHeaderScanner::allocateBuffer((int)newSize);
               memcpy(newBuffer, unprocessedCharPtr, numUnprocessedChars);
               mBufferPos = numUnprocessedChars;
               mBufferSize = newSize;
               mBuffer = newBuffer;
               
               mConnState = PartialBody;
            }
            else
            {
               // Do this stuff BEFORE we kick the message out the door.
               // Remember, deleting or passing mMessage on invalidates our
               // buffer!
               int overHang = numUnprocessedChars - (int)contentLength;

               mConnState = NewMessage;
               mBuffer = 0;
               if (overHang > 0) 
               {
                  // The next message has been partially read.
                  size_t size = overHang*3/2;
                  if (size < ConnectionBase::ChunkSize)
                  {
                     size = ConnectionBase::ChunkSize;
                  }
                  char* newBuffer = MsgHeaderScanner::allocateBuffer((int)size);
                  memcpy(newBuffer,
                         unprocessedCharPtr + contentLength,
                         overHang);
                  mBuffer = newBuffer;
                  mBufferPos = 0;
                  mBufferSize = size;
                  
                  DebugLog (<< "Extra bytes after message: " << overHang);
                  DebugLog (<< Data(mBuffer, overHang));
                  
                  bytesRead = overHang;
               }

               // The message body is complete.
               mMessage->setBody(unprocessedCharPtr, (UInt32)contentLength);
               if (b==CongestionManager::REJECTING_NON_ESSENTIAL
                     || (b==CongestionManager::REJECTING_NEW_WORK
                        && mMessage->isRequest()))
               {
                  // .bwc. If this fifo is REJECTING_NEW_WORK, we will drop
                  // requests but not responses ( ?bwc? is this right for ACK?). 
                  // If we are REJECTING_NON_ESSENTIAL, 
                  // we reject all incoming work, since losing something from the 
                  // wire will not cause instability or leaks (see 
                  // CongestionManager.hxx)
                  
                  // .bwc. This handles all appropriate checking for whether
                  // this is a response or an ACK.
                  std::auto_ptr<SendData> tryLater(transport()->make503(*mMessage, expectedWait/1000));
                  if(tryLater.get())
                  {
                     transport()->send(tryLater);
                  }
                  delete mMessage; // dropping message due to congestion
                  mMessage = 0;
               }
               else if (!transport()->basicCheck(*mMessage))
               {
                  delete mMessage;
                  mMessage = 0;
               }
               else
               {
                  Transport::stampReceived(mMessage);
                  DebugLog(<< "##Connection: " << *this << " received: " << *mMessage);
                  fifo.push_back(mMessage);
                  mMessage = 0;
               }

               if (overHang > 0) 
               {
                  goto start;
               }
            }
         }
         break;
      }
      case PartialBody:
      {
         size_t contentLength = 0;

         try
         {
             contentLength = mMessage->const_header(h_ContentLength).value();
         }
         catch(resip::ParseException& e)
         {
            WarningLog(<<"Malformed Content-Length in connection-based transport"
                        ". Not much we can do to fix this. " << e);
            // .bwc. Bad Content-Length. We are hosed.
            delete [] mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            //.jacob. Shouldn't the state also be set here?
            return false;
         }

         mBufferPos += bytesRead;
         if (mBufferPos == contentLength)
         {
            mMessage->addBuffer(mBuffer);
            mMessage->setBody(mBuffer, (UInt32)contentLength);
            mBuffer=0;
            // .bwc. basicCheck takes up substantial CPU. Don't bother doing it
            // if we're overloaded.
            if (b==CongestionManager::REJECTING_NON_ESSENTIAL
                  || (b==CongestionManager::REJECTING_NEW_WORK
                     && mMessage->isRequest()))
            {
               // .bwc. If this fifo is REJECTING_NEW_WORK, we will drop
               // requests but not responses ( ?bwc? is this right for ACK?). 
               // If we are REJECTING_NON_ESSENTIAL, 
               // we reject all incoming work, since losing something from the 
               // wire will not cause instability or leaks (see 
               // CongestionManager.hxx)
               
               // .bwc. This handles all appropriate checking for whether
               // this is a response or an ACK.
               std::auto_ptr<SendData> tryLater = transport()->make503(*mMessage, expectedWait/1000);
               if(tryLater.get())
               {
                  transport()->send(tryLater);
               }
               delete mMessage; // dropping message due to congestion
               mMessage = 0;
            }
            else if (!transport()->basicCheck(*mMessage))
            {
               delete mMessage;
               mMessage = 0;
            }
            else
            {
               DebugLog(<< "##ConnectionBase: " << *this << " received: " << *mMessage);

               Transport::stampReceived(mMessage);
               fifo.push_back(mMessage);
               mMessage = 0;
            }
            mConnState = NewMessage;
         }
         else if (mBufferPos == mBufferSize)
         {
            // .bwc. We've filled our buffer; go ahead and make more room.
            size_t newSize = resipMin(mBufferSize*3/2, contentLength);
            char* newBuffer = 0;
            try
            {
               newBuffer=new char[newSize];
            }
            catch(std::bad_alloc&)
            {
               ErrLog(<<"Failed to alloc a buffer while receiving body!");
               return false;
            }
            memcpy(newBuffer, mBuffer, mBufferSize);
            mBufferSize=newSize;
            delete [] mBuffer;
            mBuffer = newBuffer;
         }
         break;
      }
      default:
         assert(0);
   }
   return true;
}

#ifdef USE_SIGCOMP
void
ConnectionBase::decompressNewBytes(int bytesRead,
                                   std::deque<TransactionMessage*>& fifo)
{
  mConnState = SigComp;

  if (!mSigcompFramer)
  {
    mSigcompFramer = new osc::TcpStream();
  }

  mSigcompFramer->addData(mBuffer, bytesRead);
  size_t bytesUncompressed;
  osc::StateChanges *sc = 0;
  char *uncompressed = new char[65536];
  while ((bytesUncompressed = mSigcompStack->uncompressMessage(
                *mSigcompFramer, uncompressed, 65536, sc)) > 0)
  {
    DebugLog (<< "Uncompressed Connection-oriented message");
    mMessage = new SipMessage(who().transport);

    mMessage->setSource(who());
    mMessage->setTlsDomain(who().transport->tlsDomain());

    char *sipBuffer = new char[bytesUncompressed];
    memmove(sipBuffer, uncompressed, bytesUncompressed);
    mMessage->addBuffer(sipBuffer);
    mMsgHeaderScanner.prepareForMessage(mMessage);
    char *unprocessedCharPtr;
    if (mMsgHeaderScanner.scanChunk(sipBuffer,
                                    bytesUncompressed,
                                    &unprocessedCharPtr) !=
        MsgHeaderScanner::scrEnd)
    {
       StackLog(<<"Scanner rejecting compressed message as unparsable");
       StackLog(<< Data(sipBuffer, bytesUncompressed));
       delete mMessage;
       mMessage=0;
    }
  
    unsigned int used = unprocessedCharPtr - sipBuffer;
    if (mMessage && (used < bytesUncompressed))
    {
      mMessage->setBody(sipBuffer+used, bytesUncompressed-used);
    }

    if (mMessage && !transport()->basicCheck(*mMessage))
    {
      delete mMessage;
      mMessage = 0;
    }

    if (mMessage)
    {
      Transport::stampReceived(mMessage);
      // If the message made it this far, we should let it store
      // SigComp state: extract the compartment ID.
      const Via &via = mMessage->const_header(h_Vias).front();
      if (mMessage->isRequest())
      {
        // For requests, the compartment ID is read out of the
        // top via header field; if not present, we use the
        // TCP connection for identification purposes.
        if (via.exists(p_sigcompId))
        {
            Data compId = via.param(p_sigcompId);
            if(!compId.empty())
            {
                mSigcompStack->provideCompartmentId(sc, compId.data(), compId.size());
            }
        }
        else
        {
          mSigcompStack->provideCompartmentId(sc, this, sizeof(this));
        }
      }
      else
      {
        // For responses, the compartment ID is supposed to be
        // the same as the compartment ID of the request. We
        // *could* dig down into the transaction layer to try to
        // figure this out, but that's a royal pain, and a rather
        // severe layer violation. In practice, we're going to ferret
        // the ID out of the the Via header field, which is where we
        // squirreled it away when we sent this request in the first place.
        Data compId = via.param(p_branch).getSigcompCompartment();
        if(!compId.empty())
        {
           mSigcompStack->provideCompartmentId(sc, compId.data(), compId.size());
        }
      }
      fifo.add(mMessage);
      mMessage = 0;
      sc = 0;
    }
    else
    {
      delete sc;
      sc = 0;
    }
  }
  delete uncompressed;

  // If there was a decompression failure, let the other side know.
  osc::SigcompMessage *nack = mSigcompStack->getNack();
  if (nack)
  {
    if (mSendingTransmissionFormat == Compressed)
    {
      mOutstandingSends.push_back(new SendData(
                   who(),
                   Data(nack->getStreamMessage(), nack->getStreamLength()),
                   Data::Empty,
                   Data::Empty,
                   true));
    }
    else
    {
      delete nack;
    }
  }
}
#endif
            
std::pair<char*, size_t> 
ConnectionBase::getWriteBuffer()
{
   if (mConnState == NewMessage)
   {
      if (!mBuffer)
      {
         DebugLog (<< "Creating buffer for " << *this);

         mBuffer = MsgHeaderScanner::allocateBuffer(ConnectionBase::ChunkSize);
         mBufferSize = ConnectionBase::ChunkSize;
      }
      mBufferPos = 0;
   }
   return getCurrentWriteBuffer();
}

std::pair<char*, size_t> 
ConnectionBase::getCurrentWriteBuffer()
{
   return std::make_pair(mBuffer + mBufferPos, mBufferSize - mBufferPos);
}

char*
ConnectionBase::getWriteBufferForExtraBytes(int extraBytes)
{
   if (extraBytes > 0)
   {
      char* buffer = MsgHeaderScanner::allocateBuffer((int)mBufferSize + extraBytes);
      memcpy(buffer, mBuffer, mBufferSize);
      delete [] mBuffer;
      mBuffer = buffer;
      buffer += mBufferSize;
      mBufferSize += extraBytes;
      return buffer;
   }
   else
   {
      assert(0);
      return mBuffer;
   }
}
            
void 
ConnectionBase::setBuffer(char* bytes, int count)
{
   mBuffer = bytes;
   mBufferPos = 0;
   mBufferSize = count;
}

Transport* 
ConnectionBase::transport() const
{
   assert(this);
   return mTransport;
}

std::ostream& 
resip::operator<<(std::ostream& strm, 
                  const resip::ConnectionBase& c)

{
   strm << "CONN_BASE: " << &c << " " << c.who();
   return strm;
}

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

