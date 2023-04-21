#pragma once
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define DAP_END_OF_DAYS 4102444799
// Constant to convert seconds to nanoseconds
#define DAP_NSEC_PER_SEC 1000000000
// Constant to convert seconds to microseconds
#define DAP_USEC_PER_SEC 1000000
// Seconds per day
#define DAP_SEC_PER_DAY 86400

#ifdef __cplusplus
extern "C"{
#endif

// time in seconds
typedef uint64_t dap_time_t;
// time in nanoseconds
typedef uint64_t dap_nanotime_t;



// Create gdb time from second
dap_nanotime_t dap_nanotime_from_sec(uint32_t a_time);
// Get seconds from gdb time
long dap_gdb_time_to_sec(dap_nanotime_t a_time);

/**
 * @brief dap_chain_time_now Get current time in seconds since January 1, 1970 (UTC)
 * @return Returns current UTC time in seconds.
 */
dap_time_t dap_time_now(void);
/**
 * @brief dap_clock_gettime Get current time in nanoseconds since January 1, 1970 (UTC)
 * @return Returns current UTC time in nanoseconds.
 */
static inline dap_nanotime_t dap_nanotime_now()
{
    dap_nanotime_t l_time_nsec;
    struct timespec cur_time;
    clock_gettime(CLOCK_REALTIME, &cur_time);
    l_time_nsec = ((dap_nanotime_t)cur_time.tv_sec << 32) + cur_time.tv_nsec;
    return l_time_nsec;
}

// crossplatform usleep
void dap_usleep(dap_time_t a_microseconds);

/**
 * @brief dap_ctime_r This function does the same as ctime_r, but if it returns (null), a line break is added.
 * @param a_time
 * @param a_buf The minimum buffer size is 26 elements.
 * @return
 */
char* dap_ctime_r(dap_time_t *a_time, char* a_buf);
char* dap_nanotime_to_str(dap_nanotime_t *a_time, char* a_buf);


int dap_time_to_str_rfc822(char * out, size_t out_size_max, dap_time_t t);
dap_time_t dap_time_from_str_rfc822(const char *a_time_str);
int dap_gbd_time_to_str_rfc822(char *a_out, size_t a_out_size_max, dap_nanotime_t a_chain_time);
int timespec_diff(struct timespec *a_start, struct timespec *a_stop, struct timespec *a_result);

dap_time_t dap_time_from_str_simplified(const char *a_time_str);

#ifdef __cplusplus
}
#endif
