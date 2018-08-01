// Copyright 2018 Minim Inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// unum system tests common code

#include "unum.h"

// Compile only in debug version
#ifdef DEBUG

// Keeps the running test numbrer (or 0 if not running a test)
static int running_test_num = 0;

// Returns the running test number (0 if no test running)
int get_test_num(void)
{
    return running_test_num;
}

// Tests that can run on the platform
static void print_tests_info()
{
    printf("The test option has to be: 't[1|2|3...] [arg1] [arg2] ...'\n");
    printf(UTIL_STR(U_TEST_CFG_TO_FILE)
           " - test dumping cfg to file\n");
    printf(UTIL_STR(U_TEST_FILE_TO_CFG)
           " - test restoring cfg from file\n");
    printf(UTIL_STR(U_TEST_SPEEDTEST)
           " - test speedtest\n");

    printf(UTIL_STR(U_TEST_TPCAP_BASIC)
           " - test basic packet capturing\n");
    printf(UTIL_STR(U_TEST_TPCAP_HOOKS)
           " - test packet capturing filters and callback hooks\n");
    printf(UTIL_STR(U_TEST_TPCAP_DNS)
           " - test DNS info collector\n");
    printf(UTIL_STR(U_TEST_TPCAP_STATS)
           " - test packet capturing and interface stats\n");
    printf(UTIL_STR(U_TEST_TPCAP_DT)
           " - test device and connection info collector\n");
    printf(UTIL_STR(U_TEST_TPCAP_DTJSON)
           " - test dev telemetry JSON generation\n");
    printf(UTIL_STR(U_TEST_TPCAP_FPRINT)
           "- test fingerprinting info collection\n");

    printf(UTIL_STR(U_TEST_TIMERS)
           "- test timers subsystem\n");

    printf(UTIL_STR(U_TEST_WL_RT)
           "- test radio telemetry info collection\n");
    printf(UTIL_STR(U_TEST_WL_SCAN)
           "- test wireless neighborhood scan info collection\n");
    printf(UTIL_STR(U_TEST_PORT_SCAN)
           "- test port scan\n");
    printf(UTIL_STR(U_TEST_CRASH)
           "- test crash handler (requires /dev/console)\n");
    printf(UTIL_STR(U_TEST_CONNCHECK)
           "- test cloud connectivity troubleshooter\n");
    printf(UTIL_STR(U_TEST_PINGFLOOD)
           "- test pingflood fetch URL handler\n");

    printf(UTIL_STR(U_TEST_UNUSED)
           "- unused\n");
    printf("...\n");
    return;
}

// Run the platform tests
// test_num - the test # as integer
// test_num_str - full test num string "# arg1 arg2 ..."
static int run_platform_tests(int test_num, char *test_num_str)
{
    int ret = 1;

    switch(test_num) {
        case U_TEST_CFG_TO_FILE:
            return test_loadCfg();
        case U_TEST_FILE_TO_CFG:
            return test_saveCfg();
        case U_TEST_SPEEDTEST:
            speedtest_perform();
            return 0;

        // TPCAP tests were written before shared global test IDs became
        // available. Do not copycat what was done here, it is obsolete.
        case U_TEST_TPCAP_BASIC:
            return TPCAP_RUN_TEST(TPCAP_TEST_BASIC);
        case U_TEST_TPCAP_HOOKS:
            return tpcap_test_filters("/var/tpcap_filters.txt");
        case U_TEST_TPCAP_DNS:
            return TPCAP_RUN_TEST(TPCAP_TEST_DNS);
        case U_TEST_TPCAP_STATS:
            return TPCAP_RUN_TEST(TPCAP_TEST_IFSTATS);
        case U_TEST_TPCAP_DT:
            return TPCAP_RUN_TEST(TPCAP_TEST_DT);
        case U_TEST_TPCAP_DTJSON:
            return TPCAP_RUN_TEST(TPCAP_TEST_DT_JSON);
        case U_TEST_TPCAP_FPRINT:
            return TPCAP_RUN_TEST(TPCAP_TEST_FP_JSON);

        case U_TEST_TIMERS:
            test_timers();
            return 0;

        // Wireless telemetry tests
        case U_TEST_WL_RT:
        case U_TEST_WL_SCAN:
            test_wireless();
            return 0;

        case U_TEST_PORT_SCAN:
            test_port_scan();
            return 0;

        case U_TEST_CRASH:
            test_crash_handling();
            return 0;

        case U_TEST_CONNCHECK:
            test_conncheck();
            return 0;

        case U_TEST_PINGFLOOD:
            test_pingflood();
            return 0;

        default:
            printf("There is no test %d\n", test_num);
            break;
    }

    return ret;
}

// Test mode main entry point
int run_tests(char *test_num_str)
{
    int level, ii, err;
    INIT_FUNC_t init_fptr;
    int test_num;

    // Redirect all output to console and disable all other logs
    // besides stdout.
    unsigned long mask = ~(
      (1 << LOG_DST_CONSOLE) |
      (1 << LOG_DST_STDOUT)
    );
    set_disabled_log_dst_mask(mask);
    set_proc_log_dst(LOG_DST_CONSOLE);

    if(!test_num_str || (test_num = atoi(test_num_str)) == 0) {
        print_tests_info();
        return 0;
    }
    running_test_num = test_num;

    // Make sure we are the only instance and create the PID file
    err = unum_handle_start("test");
    if(err != 0) {
        log("%s: startup failure, terminating.\n", __func__);
        return -1;
    }

    // Go through the init levels up to MAX_INIT_LEVEL_TESTS
    for(level = 1; level <= MAX_INIT_LEVEL_TESTS; level++)
    {
        printf("%s: Init level %d...\n", __func__, level);
        for(ii = 0; init_list[ii] != NULL; ii++) {
            init_fptr = init_list[ii];
            err = init_fptr(level);
            if(err != 0) {
            	printf("%s: Error at init level %d in %s(), terminating.\n",
                       __func__, level, init_str_list[ii]);
                util_restart();
                // should never reach this point
            }
        }
    }
    printf("%s: Init done\n", __func__);

    err = run_platform_tests(test_num, test_num_str);

    // In case the test allows graceful exit
    unum_handle_stop();
    return err;
}

#endif // DEBUG
