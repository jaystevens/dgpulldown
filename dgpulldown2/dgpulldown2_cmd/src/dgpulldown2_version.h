#ifndef DGPULLDOWN2_VERSION_H
#define DGPULLDOWN2_VERSION_H


// compiler detection
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#ifdef _MSC_VER
    #define COMPILER_NAME "Visual Studio"
    #if (_MSC_VER >= 1900 && _MSC_VER < 1910)
        #define COMPILER_VER "2015 - 14.0"
    #elif (_MSC_VER == 1910)
        #define COMPILER_VER "2017 update 0/1/2 - 15.0"
    #elif (_MSC_VER == 1911)
        #define COMPILER_VER "2017 update 3/4 - 15.3"
    #elif (_MSC_VER == 1912)
        #define COMPILER_VER "2017 update 5 - 15.5"
    #elif (_MSC_VER == 1913)
        #define COMPILER_VER "2017 update 6 - 15.6"
    #elif (_MSC_VER == 1914)
        #define COMPILER_VER "2017 update 7 - 15.7"
    #elif (_MSC_VER == 1915)
        #define COMPILER_VER "2017 update 8 - 15.8"
    #elif (_MSC_VER == 1916)
        #define COMPILER_VER "2017 update 9 - 15.9"
    #else
        #define COMPILER_VER TOSTRING(_MSC_FULL_VER)
    #endif
#elif __GNUC__
    #define COMPILER_NAME "GCC"
    #define COMPILER_VER __VERSION__
//#elif __INTEL_COMPILER
//    #define COMPILER_NAME "ICC"
//    #define COMPILER_VER TOSTRING(__INTEL_COMPILER)
#endif
// fail safe
#ifndef COMPILER_NAME
    #define COMPILER_NAME "???"
#endif
#ifndef COMPILER_VER
    #define COMPILER_VER " "
#endif


extern const int DGPULLDOWN2_VERSION_MAJOR;
extern const int DGPULLDOWN2_VERSION_MINOR;
extern const int DGPULLDOWN2_VERSION_PATCH;
extern const char* DGPULLDOWN2_VERSION;
extern const char* DGPULLDOWN2_VERSION_FULL;
extern const char* DGPULLDOWN2_BUILD_HOST;

#endif // DGPULLDOWN2_VERSION_H

