/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_PARAMETERTYPES_HXX)
#define RESIP_PARAMETERTYPES_HXX 

#include "resip/stack/BranchParameter.hxx"
#include "resip/stack/DataParameter.hxx"
#include "resip/stack/ExistsOrDataParameter.hxx"
#include "resip/stack/QuotedDataParameter.hxx"
#include "resip/stack/IntegerParameter.hxx"
#include "resip/stack/UInt32Parameter.hxx"
#include "resip/stack/QValueParameter.hxx"
#include "resip/stack/ExistsParameter.hxx"
#include "resip/stack/ParameterTypeEnums.hxx"
#include "resip/stack/RportParameter.hxx"
#include "resip/stack/Symbols.hxx"

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)  \
   class _enum##_Param : public ParamBase                   \
   {                                                        \
     public:                                                \
      typedef _type Type;                                   \
      typedef _type::Type DType;                            \
      virtual ParameterTypes::Type getTypeNum() const;      \
      virtual const char* name() const { return _name; }    \
      _enum##_Param();                                      \
   };                                                       \
   extern _enum##_Param p_##_enum

namespace resip
{

/**
  @brief This is used in generating a string representation of a parameter 
  name from a type enum.
  
  Subclasses are created by using the defineParam macro.  Any additions 
  should have corresponding additions in several other files.  
  
  @see ParameterTypes
**/
class ParamBase
{
   public:
      virtual ~ParamBase() {}
      virtual ParameterTypes::Type getTypeNum() const = 0;
      virtual const char* name() const = 0;
};

defineParam(data, "data", ExistsParameter, "RFC 3840");
defineParam(control, "control", ExistsParameter, "RFC 3840");
defineParam(mobility, "mobility", QuotedDataParameter, "RFC 3840"); // mobile|fixed
defineParam(description, "description", QuotedDataParameter, "RFC 3840"); // <> quoted
defineParam(events, "events", QuotedDataParameter, "RFC 3840"); // list
defineParam(priority, "priority", QuotedDataParameter, "RFC 3840"); // non-urgent|normal|urgent|emergency
defineParam(methods, "methods", QuotedDataParameter, "RFC 3840"); // list
defineParam(schemes, "schemes", QuotedDataParameter, "RFC 3840"); // list
defineParam(application, "application", ExistsParameter, "RFC 3840");
defineParam(video, "video", ExistsParameter, "RFC 3840");
defineParam(language, "language", QuotedDataParameter, "RFC 3840"); // list
defineParam(type, "type", QuotedDataParameter, "RFC 3840"); // list
defineParam(isFocus, "isfocus", ExistsParameter, "RFC 3840");
defineParam(actor, "actor", QuotedDataParameter, "RFC 3840"); // principal|msg-taker|attendant|information
defineParam(text, "text", ExistsOrDataParameter, "RFC 3840"); // using ExistsOrDataParameter so this parameter is compatible with both RFC3840 and RFC3326
defineParam(extensions, "extensions", QuotedDataParameter, "RFC 3840"); //list
defineParam(Instance, "+sip.instance", QuotedDataParameter, "outbound");  // <> quoted
defineParam(regid, "reg-id", UInt32Parameter, "outbound");
defineParam(ob,"ob",ExistsParameter,"outbound-05");
defineParam(pubGruu, "pub-gruu", QuotedDataParameter, "gruu");
defineParam(tempGruu, "temp-gruu", QuotedDataParameter, "gruu");
defineParam(gr, "gr", ExistsOrDataParameter, "gruu");

defineParam(accessType, "access-type", DataParameter, "RFC 2046");
defineParam(algorithm, "algorithm", DataParameter, "RFC 2617");
defineParam(boundary, "boundary", DataParameter, "RFC 2046");
defineParam(branch, "branch", BranchParameter, "RFC 3261");
defineParam(charset, "charset", DataParameter, "RFC 2045");
defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC 2617");
defineParam(comp, "comp", DataParameter, "RFC 3486");
defineParam(dAlg, "d-alg", DataParameter, "RFC 3329");
defineParam(dQop, "d-qop", DataParameter, "RFC 3329");
defineParam(dVer, "d-ver", QuotedDataParameter, "RFC 3329");
defineParam(directory, "directory", DataParameter, "RFC 2046");
defineParam(domain, "domain", QuotedDataParameter, "RFC 3261");
defineParam(duration, "duration", UInt32Parameter, "RFC 4240");
defineParam(expiration, "expiration", QuotedDataParameter, "RFC 2046");
defineParam(expires, "expires", UInt32Parameter, "RFC 3261");
defineParam(filename, "filename", DataParameter, "RFC 2183");
defineParam(fromTag, "from-tag", DataParameter, "RFC 4235");
defineParam(handling, "handling", DataParameter, "RFC 3261");
defineParam(id, "id", DataParameter, "RFC 3265");
defineParam(lr, "lr", ExistsParameter, "RFC 3261");
defineParam(maddr, "maddr", DataParameter, "RFC 3261");
defineParam(max_interval, "max-interval", UInt32Parameter, "draft-ietf-sipcore-event-rate-control");
defineParam(method, "method", DataParameter, "RFC 3261");
defineParam(micalg, "micalg", DataParameter, "RFC 1847");
defineParam(min_interval, "min-interval", UInt32Parameter, "draft-ietf-sipcore-event-rate-control");
defineParam(mode, "mode", DataParameter, "RFC 2046");
defineParam(name, "name", DataParameter, "RFC 2046");
defineParam(nc, "nc", DataParameter, "RFC 2617");
defineParam(nonce, "nonce", QuotedDataParameter, "RFC 2617");
defineParam(opaque, "opaque", QuotedDataParameter, "RFC 2617");
defineParam(permission, "permission", DataParameter, "RFC 2046");
defineParam(protocol, "protocol", QuotedDataParameter, "RFC 1847");
defineParam(purpose, "purpose", DataParameter, "RFC 3261");
defineParam(q, "q", QValueParameter, "RFC 3261");
defineParam(realm, "realm", QuotedDataParameter, "RFC 2617");
defineParam(reason, "reason", DataParameter, "RFC 3265");
defineParam(received, "received", DataParameter, "RFC 3261");
defineParam(response, "response", QuotedDataParameter, "RFC 3261");
defineParam(retryAfter, "retry-after", UInt32Parameter, "RFC 3265");
defineParam(rinstance, "rinstance", DataParameter, "proprietary");
defineParam(rport, "rport", RportParameter, "RFC 3581");
defineParam(server, "server", DataParameter, "RFC 2046");
defineParam(site, "site", DataParameter, "RFC 2046");
defineParam(size, "size", DataParameter, "RFC 2046");
defineParam(smimeType, "smime-type", DataParameter, "RFC 2633");
defineParam(stale, "stale", DataParameter, "RFC 2617");
defineParam(tag, "tag", DataParameter, "RFC 3261");
defineParam(toTag, "to-tag", DataParameter, "RFC 4235");
defineParam(transport, "transport", DataParameter, "RFC 3261");
defineParam(ttl, "ttl", UInt32Parameter, "RFC 3261");
defineParam(uri, "uri", QuotedDataParameter, "RFC 3261");
defineParam(user, "user", DataParameter, "RFC 3261, 4967");
defineParam(extension, "ext", DataParameter, "RFC 3966");
defineParam(username, "username", QuotedDataParameter, "RFC 3261");
defineParam(earlyOnly, "early-only", ExistsParameter, "RFC 3891");
defineParam(refresher, "refresher", DataParameter, "RFC 4028");

defineParam(profileType, "profile-type", DataParameter, "RFC 6080");
defineParam(vendor, "vendor", QuotedDataParameter, "RFC 6080");
defineParam(model, "model", QuotedDataParameter, "RFC 6080");
defineParam(version, "version", QuotedDataParameter, "RFC 6080");
defineParam(effectiveBy, "effective-by", UInt32Parameter, "RFC 6080");
defineParam(document, "document", DataParameter, "draft-ietf-sipping-config-framework-07 (removed in 08)");
defineParam(appId, "app-id", DataParameter, "draft-ietf-sipping-config-framework-05 (renamed to auid in 06, which was then removed in 08)");
defineParam(networkUser, "network-user", DataParameter, "draft-ietf-sipping-config-framework-11 (removed in 12)");

defineParam(url, "url", QuotedDataParameter, "RFC 4483");

defineParam(sigcompId, "sigcomp-id", QuotedDataParameter, "RFC 5049");
defineParam(qop,"qop",DataParameter,"RFC 3261");

/// @internal
defineParam(qopOptions,"qop",DataParameter,"RFC 3261");
defineParam(addTransport, "addTransport", ExistsParameter, "RESIP INTERNAL");

}

#undef defineParam
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
