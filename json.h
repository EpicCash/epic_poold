#pragma once

#include "rapidjson/document.h"

using namespace rapidjson;

typedef const Value* lpcJsVal;

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
		throw std::runtime_error(std::string("Key ") + key + " does not exist");
}

inline uint64_t GetJsonUInt64(const Value& obj, const char* key)
{
	lpcJsVal v = GetObjectMember(obj, key);

	if(v == nullptr)
		throw std::runtime_error(std::string("Key ") + key + " does not exist");

	if(v->IsUint64())
		return v->GetUint64();

	if(v->IsString())
		return std::stoull(v->GetString());

	throw std::runtime_error(std::string("Key ") + key + " has wrong type, expected a number");
}

inline const char* GetJsonString(const Value& obj, const char* key)
{
	lpcJsVal v = GetObjectMember(obj, key);
	
	if(v == nullptr)
		throw std::runtime_error(std::string("Key ") + key + " does not exist");
	
	if(!v->IsString())
		throw std::runtime_error(std::string("Key ") + key + " has wrong type, expected a string");

	return v->GetString();
}

typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>> MemDocument;

