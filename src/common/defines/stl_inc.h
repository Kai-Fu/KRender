#pragma once
#ifdef __GNUC__
	#include <ext/hash_map>
	#include <ext/hash_set>
	#define STDEXT __gnu_cxx
	namespace __gnu_cxx
	{
		template<> struct hash< std::string >
		{
			size_t operator()( const std::string& x ) const
			{
				return hash< const char* >()( x.c_str() );
			}
		};
	}
#else
	
	#include <hash_set>
	#include <hash_map>
	#define STDEXT stdext
	
#endif
