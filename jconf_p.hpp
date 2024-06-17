// Copyright (c) 2014-2023, Epic Cash and fireice-uk
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "json.h"

/*
 * This enum needs to match index in oConfigValues, otherwise we will get a runtime error
 */

enum configEnum
{
	sPidFile,
	sLogFile,
	sLogLevel,
	sDbHostname,
	sDbPort,
	sDbName,
	sDbUsername,
	sDbPassword,
	sNodeHostname,
	sNodePort,
	sNodeUsername,
	sNodePassword,
	bDaemonize,
	iPlainPort,
	iTlsPort,
	sTlsCert,
	sTlsCipers,
	iTemplateTimeout,
	iFatalNodeTimeout
};

struct configVal
{
	configEnum iName;
	const char* sName;
	Type iType;
	size_t flagConstraints;
};

constexpr size_t flag_none = 0;
constexpr size_t flag_unsigned = 1 << 0;
constexpr size_t flag_u16bit = 1 << 1;

inline bool check_constraint_unsigned(lpcJsVal val)
{
	return val->IsUint();
}

inline bool check_constraint_u16bit(lpcJsVal val)
{
	return val->IsUint() && val->GetUint() <= 0xffff;
}

// Same order as in configEnum, as per comment above
// kNullType means any type
configVal oConfigValues[] = {
	{sPidFile, "pid_file", kStringType, flag_none},
	{sLogFile, "log_file", kStringType, flag_none},
	{sLogLevel, "log_level", kStringType, flag_none},
	{sDbHostname, "db_hostname", kStringType, flag_none},
	{sDbPort, "db_port", kStringType, flag_none},
	{sDbName, "db_name", kStringType, flag_none},
	{sDbUsername, "db_username", kStringType, flag_none},
	{sDbPassword, "db_password", kStringType, flag_none},
	{sNodeHostname, "node_hostname", kStringType, flag_none},
	{sNodePort, "node_port", kStringType, flag_none},
	{sNodeUsername, "node_username", kStringType, flag_none},
	{sNodePassword, "node_password", kStringType, flag_none},
	{bDaemonize, "daemonize", kTrueType, flag_none},
	{iPlainPort, "plain_port", kNumberType, flag_u16bit},
	{iTlsPort, "tls_port", kNumberType, flag_u16bit},
	{sTlsCert, "tls_certificate", kStringType, flag_none},
	{sTlsCipers, "tls_ciper_list", kStringType, flag_none},
	{iTemplateTimeout, "template_timeout", kNumberType, flag_unsigned},
	{iFatalNodeTimeout, "fatal_node_timeout", kNumberType, flag_unsigned}
};

constexpr size_t iConfigCnt = (sizeof(oConfigValues) / sizeof(oConfigValues[0]));

inline bool checkType(Type have, Type want)
{
	if(want == have)
		return true;
	else if(want == kNullType)
		return true;
	else if(want == kTrueType && have == kFalseType)
		return true;
	else if(want == kFalseType && have == kTrueType)
		return true;
	else
		return false;
}

class jconfPrivate
{
  private:
	friend class jconf;
	jconfPrivate(jconf* q);

	Document jsonDoc;
	lpcJsVal configValues[iConfigCnt];

	class jconf* const q;
};
