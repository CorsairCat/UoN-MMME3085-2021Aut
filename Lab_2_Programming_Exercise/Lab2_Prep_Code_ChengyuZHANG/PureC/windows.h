#include <assert.h>
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

typedef int BOOL;
uint32_t GetTickCount(void);
uint64_t GetTickCount64(void);

uint32_t GetTickCount(void)
{
    uint64_t        elapsedMills;

    elapsedMills =  clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) / 1E6; 

    return (uint32_t) elapsedMills;
}

uint64_t GetTickCount64(void)
{
    uint64_t        elapsedMills;

    elapsedMills = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) / 1E6; 

    return elapsedMills;
}