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

#include <stdexcept>
#include <stdlib.h>
#include "rapidjson/document.h"

using namespace rapidjson;

class json_parse_error : public std::runtime_error
{
public:
	json_parse_error() : std::runtime_error("JSON parsing error") {}
	json_parse_error(const std::string& error) : std::runtime_error(error) {}
	json_parse_error(const char* error) : std::runtime_error(error) {}
};

typedef const Value* lpcJsVal;

inline uint64_t json_strtoull(const char* str)
{
	char* endPtr = nullptr;
	uint64_t ret = strtoull(str, &endPtr, 10);
	if(endPtr == nullptr)
		throw json_parse_error(std::string("Counld not convert to integer: ") + str);
	return ret;
}

inline uint32_t json_strtoul(const char* str)
{
	char* endPtr = nullptr;
	uint32_t ret = strtoull(str, &endPtr, 10);
	if(endPtr == nullptr)
		throw json_parse_error(std::string("Counld not convert to integer: ") + str);
	return ret;
}

/* This macro brings rapidjson more in line with other libs */
inline lpcJsVal GetObjectMember(const Value& obj, const char* key)
{
	Value::ConstMemberIterator itr = obj.FindMember(key);
	if (itr != obj.MemberEnd())
		return &itr->value;
	else
		return nullptr;
}

inline const Value& GetObjectMemberT(const Value& obj, const char* key)
{
	Value::ConstMemberIterator itr = obj.FindMember(key);
	if (itr != obj.MemberEnd())
		return itr->value;
	else
		throw json_parse_error(std::string("Key ") + key + " does not exist");
}

inline rapidjson::GenericValue<UTF8<>>::ConstArray GetArray(const Value& obj, ssize_t exp_len = -1)
{
	if(!obj.IsArray())
		throw json_parse_error("Expected array type");
	rapidjson::GenericValue<UTF8<>>::ConstArray ret = obj.GetArray();

	if(exp_len >= 0 && ret.Size() != unsigned(exp_len))
	{
		throw json_parse_error(std::string("Expected array size ") + 
			std::to_string(exp_len) + std::string(" got ") + std::to_string(ret.Size()));
	}

	return ret;
}

inline const char* GetString(const Value& obj)
{
	if(!obj.IsString())
		throw json_parse_error("Expected string type");
	return obj.GetString();
}

inline uint64_t GetUint64(const Value& obj)
{
	if(!obj.IsUint64())
		throw json_parse_error("Expected integer type");
	return obj.GetUint64();
}

inline uint32_t GetUint(const Value& obj)
{
	if(!obj.IsUint())
		throw json_parse_error("Expected integer type");
	return obj.GetUint();
}

inline int64_t GetJsonCallId(const Value& obj)
{
	lpcJsVal v = GetObjectMember(obj, "id");

	if(v == nullptr)
		throw json_parse_error(std::string("Key id does not exist"));

	if(v->IsString())
	{
		if(strcmp(v->GetString(), "Stratum") == 0)
			return -1;
		else
			return json_strtoul(v->GetString());
	}

	if(v->IsUint())
		return v->GetUint();

	throw json_parse_error(std::string("Key id has wrong type, expected a number"));
}

inline uint64_t GetJsonUInt64(const Value& obj, const char* key)
{
	lpcJsVal v = GetObjectMember(obj, key);

	if(v == nullptr)
		throw json_parse_error(std::string("Key ") + key + " does not exist");

	if(v->IsUint64())
		return v->GetUint64();

	if(v->IsString())
		return json_strtoull(v->GetString());

	throw json_parse_error(std::string("Key ") + key + " has wrong type, expected a number");
}

inline uint32_t GetJsonUInt(const Value& obj, const char* key)
{
	lpcJsVal v = GetObjectMember(obj, key);
	
	if(v == nullptr)
		throw json_parse_error(std::string("Key ") + key + " does not exist");
	
	if(v->IsUint())
		return v->GetUint();
	
	if(v->IsString())
		return json_strtoul(v->GetString());
	
	throw json_parse_error(std::string("Key ") + key + " has wrong type, expected a number");
}

inline const char* GetJsonString(const Value& obj, const char* key, unsigned& len)
{
	lpcJsVal v = GetObjectMember(obj, key);
	
	if(v == nullptr)
		throw json_parse_error(std::string("Key ") + key + " does not exist");
	
	if(!v->IsString())
		throw json_parse_error(std::string("Key ") + key + " has wrong type, expected a string");

	len = v->GetStringLength();
	return v->GetString();
}

inline const char* GetJsonString(const Value& obj, const char* key)
{
	unsigned len;
	return GetJsonString(obj, key, len);
}

typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>> MemDocument;

