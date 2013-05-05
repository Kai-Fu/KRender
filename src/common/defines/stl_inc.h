#pragma once
#ifdef __GNUC__
	#include <ext/hash_map>
	#include <ext/hash_set>
	#define std_hash_map __gnu_cxx::hash_map
	#define std_hash_set __gnu_cxx::hash_set
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
	#define std_hash_map stdext::hash_map
	#define std_hash_set stdext::hash_set
	
#endif
