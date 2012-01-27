/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_TRANSACTIONTERMINATED_HXX)
#define RESIP_TRANSACTIONTERMINATED_HXX 

#include "TransactionMessage.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{
class TransactionUser;

/**
   @ingroup message_passing_tu

   @brief Indicates that the transaction is finished from the
   perspective of the SipStack protocol processing.

   @see TransactionTerminatedFactory
*/
class TransactionTerminated : public TransactionMessage
{
   public:
      RESIP_HeapCount(TransactionTerminated);

      /**
        @brief Transaction that has terminated
		
        @param tid  The id of the terminated transaction.
        @param isClient  used to distinguish server transactions from client
                         transactions.
        @param ptu  The transaction user that is the recipient of this message.
		*/
      TransactionTerminated(const Data& tid, bool isClient, TransactionUser* ptu) : 
         mTransactionId(tid), 
         mIsClient(isClient)
      {
         setTransactionUser(ptu);         
      }
      virtual const Data& getTransactionId() const { return mTransactionId; }
      virtual bool isClientTransaction() const { return mIsClient; }
      virtual std::ostream& encode(std::ostream& strm) const { return encodeBrief(strm); }
      virtual std::ostream& encodeBrief(std::ostream& str) const 
      {
         return str << (mIsClient ? "ClientTransactionTerminated " : "ServerTransactionTerminated ") << mTransactionId;
      }
      
      Data mTransactionId;
      bool mIsClient;
};
 
}

#endif
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
