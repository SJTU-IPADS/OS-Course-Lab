/* 
 * CS:APP Data Lab 
 * 
 * btest.c - A test harness that checks a student's solution 
 *           in bits.c for correctness. 
 *
 * Copyright (c) 2001, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 *
 * Usage:
 *    -e <N>       Limit number of errors to report for single function to N
 *    -f <Name>    Check only the named function
 *    -g           Print compact grading summary (implies -v 0 and -e 0)
 *    -h           Print help message
 *    -a           Don't check team structure
 *    -r <N>       Give uniform weight of N for all problems
 *    -v <N>       Set verbosity level to N
 *                 N=0:  Only give final scores
 *                 N=1:  Also report individual correctness scores (default)
 * 
 * Each problem has a weight 1 to 4, which is defined in legallist.c.

 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "btest.h"

/* Globals defined in other modules */
extern team_struct team;    /* defined in bits.c */
extern test_rec test_set[]; /* defined in decl.c */
                            /* and generated from templates in ./puzzles */

/* Generate test values near "corner cases" */
#define TEST_RANGE 5
#define TEST_COUNT 33

/* Print only compact grading summary if set (-g) */
static int grade = 0;

/* Max errors reported per function (-e) */
static int error_limit = 1000;

/* If non-NULL, test only one function (-f) */
static char* test_fname = NULL;  

/* Should I used fixed weight for rating, and if so, what should it be? (-r)*/
static int global_rating = 0;

/* Return random value between min and max */
static int random_val(int min, int max)
{
    double weight = rand()/(double) RAND_MAX;
    int result = min * (1-weight) + max * weight;
    return result;
}

/* Generate the integer values we'll use to test a function */
static int gen_vals(int test_vals[], int min, int max)
{
    int i;
    int test_count = 0;

    /* If range small enough, then do exhaustively */
    if (max-32 <= min) {
	for (i = min; i <= max; i++)
	    test_vals[test_count++] = i;
	return test_count;
    }
    /* Otherwise, need to sample.
       Do so near the boundaries and for a few random cases */
    for (i = 0; i < TEST_RANGE; i++) {
	test_vals[test_count++] = min+i;
	test_vals[test_count++] = max-i;
	test_vals[test_count++] = (max+min-TEST_RANGE)/2+i;
	test_vals[test_count++] = random_val(min, max);
    }
    return test_count;
}

/* Test a function with zero arguments */
static int test_0_arg(funct_t f, funct_t ft, char *name, int report)
{
    int r = f();
    int rt = ft();
    int error =  (r != rt);

    if (error && report)
	printf("Test %s() failed.\n  Gives %d[0x%x].  Should be %d[0x%x]\n",
	       name, r, r, rt, rt);
    return error;
}

/* Test a function with one argument */
static int test_1_arg(funct_t f, funct_t ft, int arg1, char *name, int report)
{
    funct1_t f1 = (funct1_t) f;
    funct1_t f1t = (funct1_t) ft;
    int r, rt, error;

    r = f1(arg1);
    rt = f1t(arg1);
    error = (r != rt);
    if (error && report)
	printf("Test %s(%d[0x%x]) failed.\n  Gives %d[0x%x].  Should be %d[0x%x]\n",
	       name, arg1, arg1, r, r, rt, rt);
    return error;
}

/* Test a function with two arguments */
static int test_2_arg(funct_t f, funct_t ft, 
		      int arg1, int arg2, 
		      char *name, int report)
{
    funct2_t f2 = (funct2_t) f;
    funct2_t f2t = (funct2_t) ft;
    int r = f2(arg1, arg2);
    int rt = f2t(arg1, arg2);
    int error = (r != rt);

    if (error && report)
	printf(
	       "Test %s(%d[0x%x],%d[0x%x]) failed.\n  Gives %d[0x%x].  Should be %d[0x%x]\n",
	       name, arg1, arg1, arg2, arg2, r, r, rt, rt);
    return error;
}

/* Test a function with three arguments */
static int test_3_arg(funct_t f, funct_t ft,
	   int arg1, int arg2, int arg3,
	   char *name, int report)
{
    funct3_t f3 = (funct3_t) f;
    funct3_t f3t = (funct3_t) ft;
    int r = f3(arg1, arg2, arg3);
    int rt = f3t(arg1, arg2, arg3);
    int error = (r != rt);

    if (error && report)
	printf(
	       "Test %s(%d[0x%x],%d[0x%x],%d[0x%x]) failed.\n  Gives %d[0x%x].  Should be %d[0x%x]\n",
	       name, arg1, arg1, arg2, arg2, arg3, arg3, r, r, rt, rt);
    return error;
}

/* Test a function.  Return number of errors */
static int test_function(test_ptr t, int report) {
    int test_vals[3][TEST_COUNT];
    int test_counts[3];
    int errors = 0;
    int i;
    int a1, a2, a3;
    int args = t->args;

    /* Create test set */
    for (i = 0; i < 3; i++)
	test_counts[i] =
	    gen_vals(test_vals[i], t->arg_ranges[i][0], t->arg_ranges[i][1]);
    if (args == 0) {
	errors += test_0_arg(t->solution_funct, t->test_funct,
			     t->name, report && errors < error_limit);
    } else for (a1 = 0; a1 < test_counts[0]; a1++) {
	if (args == 1) {
	    errors += test_1_arg(t->solution_funct, t->test_funct,
				 test_vals[0][a1],
				 t->name, report && errors < error_limit);
	} else for (a2 = 0; a2 < test_counts[1]; a2++) {
	    if (args == 2) {
		errors += test_2_arg(t->solution_funct, t->test_funct,
				     test_vals[0][a1], test_vals[1][a2],
				     t->name, report && errors < error_limit);
	    } else for (a3 = 0; a3 < test_counts[2]; a3++) {
		errors += test_3_arg(t->solution_funct, t->test_funct,
				     test_vals[0][a1], test_vals[1][a2],
				     test_vals[2][a3],
				     t->name, report && errors < error_limit);
	    }
	}
    }

    if (!grade) {
	if (report && errors > error_limit)
	    printf("... %d total errors for function %s\n",
		   errors, t->name);
    }

    return errors;
}

/* Run series of tests.  Return number of errors */ 
static int run_tests(int report) {
    int i;
    int errors = 0;
    double points = 0.0;
    double max_points = 0.0;

    if (grade)
	printf("Score\tErrors\tFunction\n");

    for (i = 0; test_set[i].solution_funct; i++) {
	int terrors;
	double tscore;
	double tpoints;
	if (!test_fname || strcmp(test_set[i].name,test_fname) == 0) {
	    int rating = global_rating ? global_rating : test_set[i].rating;
	    terrors = test_function(&test_set[i], report);
	    errors += terrors;
	    if (test_set[i].args == 0)
		tscore = terrors == 0 ? 1.0 : 0.0;
	    else
		tscore = terrors == 0 ? 1.0 : terrors == 1 ? 0.5 : 0.0;
	    tpoints = rating * tscore;
	    points += tpoints;
	    max_points += rating;
	    if (grade)
		printf(" %.1f\t%d\t%s\n", tpoints, terrors, test_set[i].name);
	    if (report)
		printf("Test %s score: %.2f/%.2f\n",
		       test_set[i].name, tpoints, (double) rating);
	}
    }

    if (grade) 
	printf("Total points: %.2f/%.2f\n", points, max_points);
    else
	printf("Overall correctness score: %.2f/%.2f\n", points, max_points);

    return errors;
}

static void usage(char *cmd) {
    printf("Usage: %s [-v 0|1] [-hag] [-f <func name>] [-e <max errors>]\n", cmd);
    printf("  -e <n>    Limit number of errors to report for single function to n\n");
    printf("  -f <name> Check only the named function\n");
    printf("  -g        Print compact grading summary (implies -v 0 and -e 0)\n");
    printf("  -h        Print this message\n");
    printf("  -a        Omit check for valid team members\n");
    printf("  -r <n>    Give uniform weight of n for all problems\n");
    printf("  -v <n>    Set verbosity to level n\n");
    printf("               n=0: Only give final scores\n");
    printf("               n=1: Also report individual correctness scores (default)\n");
    exit(1);
}



/************** 
 * main routine 
 **************/

int main(int argc, char *argv[])
{
    int verbose_level = 1;
    int errors;
    int team_check = 1;
    char c;

    /* parse command line args */
    while ((c = getopt(argc, argv, "hagv:f:e:r:")) != -1)
        switch (c) {
        case 'h': /* help */
	    usage(argv[0]);
	    break;
        case 'a': /* Don't check team structure */
	    team_check = 0;
	    break;
        case 'g': /* grading summary */
	    grade = 1;
	    break;
        case 'v': /* set verbosity level */
	    verbose_level = atoi(optarg);
	    if (verbose_level < 0 || verbose_level > 1)
		usage(argv[0]);
	    break;
	case 'f': /* test only one function */
	    test_fname = strdup(optarg);
	    break;
	case 'e': /* set error limit */
	    error_limit = atoi(optarg);
	    if (error_limit < 0)
		usage(argv[0]);
	    break;
	case 'r': /* set global rating for each problem */
	    global_rating = atoi(optarg);
	    if (global_rating < 0)
		usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
    }

    if (grade) {
	error_limit = 0;
	verbose_level = 0;
    }

    if (team_check) {
	/* Students must fill in their team information */
	if (*team.teamname == '\0') {
	    printf("%s: ERROR.  Please enter your team name in the team struct in bits.c.\n", argv[0]);
	    exit(1);
	} else
	    printf("Team: %s\n", team.teamname);
	if ((*team.name1 == '\0') || (*team.id1 == '\0')) {
	    printf("%s: ERROR. Please complete all team member 1 fields in the team struct.\n", argv[0]);
	    exit(1);
	} 
	else
	    printf("Member 1:\t%s\t%s\n", team.name1, team.id1);

	if (((*team.name2 != '\0') && (*team.id2 == '\0')) ||
	    ((*team.name2 == '\0') && (*team.id2 != '\0'))) { 
	    printf("%s: ERROR.  You must fill in all or none of the team member 2 fields in the team struct.\n", argv[0]);
	    exit(1);
	}
	else if (*team.name2 != '\0')
	    printf("Member 2:\t%s\t%s\n", team.name2, team.id2);

	printf("\n");
    }

    /* test each function */
    errors = run_tests(verbose_level > 0);

    if (!grade) {
	if (errors > 0)
	    printf("%d errors encountered.\n", errors);
	else {
	    printf("All tests passed.\n");
	}
    }

    return 0;
}
