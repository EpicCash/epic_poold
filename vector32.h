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

#include <functional>
#include <stdint.h>
#pragma GCC target("sse4.1")
#include <x86intrin.h>

struct v32
{
	v32() {}

	v32(const uint8_t* src) 
	{ 
		__m128i r1 = _mm_loadu_si128((const __m128i*)src);
		__m128i r2 = _mm_loadu_si128((const __m128i*)(src + 16));
		_mm_store_si128((__m128i*)data, r1);
		_mm_store_si128((__m128i*)(data + 16), r2);
	}

	v32(const v32& o)
	{
		__m128i r1 = _mm_load_si128((const __m128i*)o.data);
		__m128i r2 = _mm_load_si128((const __m128i*)(o.data + 16));
		_mm_store_si128((__m128i*)data, r1);
		_mm_store_si128((__m128i*)(data + 16), r2);
	}

	uint32_t get_work32() const
	{
		return __builtin_bswap32(*reinterpret_cast<const uint32_t*>(data));
	}

	uint64_t get_work64() const
	{
		return __builtin_bswap64(*reinterpret_cast<const uint64_t*>(data));
	}

	// 63 bit id of a vector used for atomics. Top bit is always zero, so -1 is never a valid id
	uint64_t get_id() const
	{
		return reinterpret_cast<const uint64_t*>(data)[0] & 0x7fffffffffffffffull;
	}

	v32& operator=(const v32& o)
	{
		__m128i r1 = _mm_load_si128((const __m128i*)o.data);
		__m128i r2 = _mm_load_si128((const __m128i*)(o.data + 16));
		_mm_store_si128((__m128i*)data, r1);
		_mm_store_si128((__m128i*)(data + 16), r2);
		return *this;
	}

	v32& operator=(const uint8_t* src)
	{
		__m128i r1 = _mm_loadu_si128((const __m128i*)src);
		__m128i r2 = _mm_loadu_si128((const __m128i*)(src + 16));
		_mm_store_si128((__m128i*)data, r1);
		_mm_store_si128((__m128i*)(data + 16), r2);
		return *this;
	}

	bool operator==(const v32& o) const
	{
		__m128i r;
		r = _mm_xor_si128(_mm_load_si128((const __m128i*)data),
						  _mm_load_si128((const __m128i*)o.data));

		if(_mm_testz_si128(r, r) == 0)
			return false;

		r = _mm_xor_si128(_mm_load_si128((const __m128i*)(data + 16)),
						  _mm_load_si128((const __m128i*)(o.data + 16)));

		return _mm_testz_si128(r, r) == 1;
	}

	void set_all_ones()
	{
		__m128i v = _mm_set1_epi64x(-1);
		_mm_store_si128((__m128i*)data, v);
		_mm_store_si128((__m128i*)(data + 16), v);
	}

	void set_all_zero()
	{
		__m128i v = _mm_setzero_si128();
		_mm_store_si128((__m128i*)data, v);
		_mm_store_si128((__m128i*)(data + 16), v);
	}

	bool operator!=(const v32& o) const { return !(*this == o); }

	operator const uint8_t*() const { return data; }
	operator uint8_t*() { return data; }

	static constexpr size_t size = 32;
	alignas(16) uint8_t data[32];
};

namespace std
{
	template<> struct hash<v32>
	{
		std::size_t operator()(v32 const& v) const noexcept
		{
			const uint64_t* data = reinterpret_cast<const uint64_t*>(v.data);
			return data[0] ^ data[1] ^ data[2] ^ data[3];
		}
	};
}
