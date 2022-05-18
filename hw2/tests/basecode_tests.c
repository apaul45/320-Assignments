#include <stdio.h>

#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "test_common.h"

#define STANDARD_LIMITS "ulimit -t 10; ulimit -f 2000"

/*
 * Start the program and then trigger EOF on input.
 * The program should exit with EXIT_SUCCESS.
 */
//This will cause this case to be ran: 
//     bin/par < EOF.in > EOF.out > EOF.err
Test(base_suite, EOF_test) {
    char *name = "EOF";
    sprintf(program_options, "%s", "");
    int err = run_using_system(name, "", "", STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, err);
    assert_outfile_matches(name, NULL);
}

/*
 * Run the program with default options on a non-empty input file
 * and check the results.
 */
//Case: bin/par < basic.in > basic.out > basic.err
Test(base_suite, basic_test) {
    char *name = "basic";
    sprintf(program_options, "%s", "");
    int err = run_using_system(name, "", "", STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, err);
    assert_outfile_matches(name, NULL);
}

/*
 * Run the program with default options on an input file with
 * prefixes and suffixes and check the results.
 */
Test(base_suite, prefix_suffix_test) {
    char *name = "prefix_suffix";
    //sprintf will pass in command line arguments: in this case w80
    sprintf(program_options, "%s%s", "-w", "80");
    int err = run_using_system(name, "", "", STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, err);
    assert_outfile_matches(name, NULL);
}

/*
 * Run the program with default options on a non-empty input file
 * and use valgrind to check for leaks.
 */
Test(base_suite, valgrind_leak_test) {
    char *name = "valgrind_leak";
    sprintf(program_options, "%s", "");
    int err = run_using_system(name, "", "valgrind --leak-check=full --undef-value-errors=no --error-exitcode=37", STANDARD_LIMITS);
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
    assert_outfile_matches(name, NULL);
}

/*
 * Run the program with default options on a non-empty input file
 * and use valgrind to check for uninitialized values.
 */
//Case: bin/par p10 s10 < valgrind_uninitialized.in > valgrind_uninitialized.out > valgrind_uninitialized.err
Test(base_suite, valgrind_uninitialized_test) {
    char *name = "valgrind_uninitialized";
    //sprintf will pass in command line arguments: in this case p10 s10
    sprintf(program_options, "%s%s%s%s", "-p", "10", "-s", "10");
    int err = run_using_system(name, "", "valgrind --leak-check=no --undef-value-errors=yes --error-exitcode=37", STANDARD_LIMITS);
    assert_no_valgrind_errors(err);
    assert_expected_status(0x1, err);
    assert_outfile_matches(name, NULL);
}

