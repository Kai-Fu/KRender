
#ifdef _WIN32
	// Windows platform
	#ifdef BUILD_KRT_CORE_DLL
		#define KRT_API _declspec( dllexport )
	#else
		#define KRT_API _declspec( dllimport )
	#endif

	#ifdef BUILD_KRT_IMAGE_LIB_DLL
		#define KRT_IMAGE_API _declspec( dllexport )
	#else
		#define KRT_IMAGE_API _declspec( dllimport )
	#endif

	typedef unsigned __int64 UINT64;
	#pragma warning( disable: 4251 )
#else
	// Non-windows platform, e.g. Linux
	# define DLL_PUBLIC	__attribute__ ((visibility("default")))
	# define DLL_PRIVATE __attribute__ ((visibility("hidden")))
	typedef unsigned long long UINT64;
	#ifdef BUILD_KRT_CORE_DLL
		#define KRT_API DLL_PUBLIC
	#else
		#define KRT_API DLL_PUBLIC
	#endif

	#ifdef BUILD_KRT_IMAGE_LIB_DLL
		#define KRT_IMAGE_API DLL_PUBLIC
	#else
		#define KRT_IMAGE_API DLL_PUBLIC
	#endif

#endif // _WIN32