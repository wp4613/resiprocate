/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_EMBEDDED_HXX)
#define RESIP_EMBEDDED_HXX 

namespace resip
{

class Data;

/**
    @class Embedded
    @brief Performs %-encoding and -decoding of an input string.
    @note    
    Any characters not allowable in a SIP URI will be output as %HL, where 
    H and L are the high and low hex digits representing the character.
**/
class Embedded
{
   public:
      static char* decode(const Data& input, unsigned int& decodedLength);
      /**
        @brief This method encodes the hname and hvalue portion of the SIP URI
        @note relevant BNF from 3261
		<pre>
         SIP-URI          =  "sip:" [ userinfo ] hostport
                             uri-parameters [ headers ]
         headers         =  "?" header *( "&" header )
         header          =  hname "=" hvalue
         hname           =  1*( hnv-unreserved / unreserved / escaped )
         hvalue          =  *( hnv-unreserved / unreserved / escaped )
         hnv-unreserved  =  "[" / "]" / "/" / "?" / ":" / "+" / "$"
         unreserved  =  alphanum / mark
         mark        =  "-" / "_" / "." / "!" / "~" / "*" / "'"
                        / "(" / ")"
         escaped     =  "%" HEXDIG HEXDIG
         alphanum  =  ALPHA / DIGIT
	</pre>
       @note
       It is both unsafe and unwise to express what needs to be escaped
       and simply not escape everything else, because the omission of an item
       in need of escaping will cause mis-parsing on the remote end.
       Because escaping unnecessarily causes absolutely no harm, the omission
       of a symbol from the list of items positively allowed causes no
       damage whatsoever.
      */
      static Data encode(const Data& input);

   private:
      Embedded();
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
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
