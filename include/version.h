#define RADIANT_VERSION "0.9.11"

#define RADIANT_APPNAME "DarkRadiant"
#define RADIANT_BLANK " "

#if defined(_M_X64) || defined(__amd64__) || defined(_WIN64)
	#define RADIANT_PLATFORM "x64"
#else
	#define RADIANT_PLATFORM "x86"
#endif

#define RADIANT_APPNAME_FULL (RADIANT_APPNAME RADIANT_BLANK RADIANT_VERSION RADIANT_BLANK RADIANT_PLATFORM RADIANT_BLANK)
