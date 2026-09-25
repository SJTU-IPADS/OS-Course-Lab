// yat.c - Utility for testing y64 assembler.
// author: Lewis Cheng
// date: 2012/4/21
// update: 2012/4/22

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int make_y64asm()
{   
    return system("make > /dev/null");
}

#define COMMAND_BUFFER_SIZE 1024
static char cmdbuf[COMMAND_BUFFER_SIZE];

static int make_app_stu(const char *name)
{
    sprintf(cmdbuf, "cd y64-app; make %s.yo > /dev/null", name);
    
    return system(cmdbuf);
}

static int make_app_base(const char *name)
{
    sprintf(cmdbuf, "cd y64-base; make %s.yo > /dev/null", name);
    
    return system(cmdbuf);
}

static int diff_app(const char *name)
{
    sprintf(cmdbuf, "diff y64-app/%s.yo y64-base/%s.yo; diff y64-app/%s.bin y64-base/%s.bin", name, name, name, name);
    
    return system(cmdbuf);
}

static int make_err_stu(const char *name)
{
    sprintf(cmdbuf, "./y64asm y64-err/%s.ys 2> %s.err", name, name);
    
    return !system(cmdbuf);
}

static int make_err_base(const char *name)
{
    sprintf(cmdbuf, "./y64-base/y64asm-base y64-err/%s.ys 2> %s.err.base", name, name);
    
    return !system(cmdbuf);
}

static int diff_err(const char *name)
{
    sprintf(cmdbuf, "diff %s.err %s.err.base", name, name);
    
    return system(cmdbuf);
}

static int make_ins_stu(const char *name)
{
    sprintf(cmdbuf, "cd y64-ins; make %s.yo > /dev/null", name);
   
    return system(cmdbuf);
}

static int make_ins_base(const char *name)
{
    sprintf(cmdbuf, "cd y64-ins; ../y64-base/y64asm-base -v %s.ys > %s.yo", name, name);
    
    if (system(cmdbuf))
        return 1;
    
    sprintf(cmdbuf, "cd y64-ins; mv %s.yo %s.yo.base; mv %s.bin %s.bin.base", name, name, name, name);
    system(cmdbuf);
    
    return 0;
}

static int diff_ins(const char *name)
{
    sprintf(cmdbuf, "cd y64-ins; diff %s.yo.base %s.yo; diff %s.bin.base %s.bin", name, name, name, name);
    
    return system(cmdbuf);
}

static int ins_pass_count;
static int err_pass_count;
static int ins_test_count;
static int err_test_count;
static int app_test_count;
static int app_pass_count;

// test a uniterm, either an instruction or an error-handling case.
static void test_uni(const char *name)
{
    // if name contains 'error', it's an error-handling case.
    char *occurrence = strstr(name, "error");
    
    if (occurrence) {
        // test an error-handling case
        err_test_count++;
        printf("[ Testing error-handling case: %s ]\n", name);

        if (!make_err_base(name) && !make_err_stu(name) && !diff_err(name)) {
            err_pass_count++;
            printf("[ Result: Pass ]\n");
        } else {
            printf("[ Result: Fail ]\n");
        }
    } else {
        // test an instruction
        ins_test_count++;
        printf("[ Testing instruction: %s ]\n", name);
        
        if (!make_ins_base(name) && !make_ins_stu(name) && !diff_ins(name)) {
            ins_pass_count++;
            printf("[ Result: Pass ]\n");
        } else {
            printf("[ Result: Fail ]\n");
        }
    }
}

static char *uni_list[] = {
    "halt", 
    "nop", 
    "rrmovq", 
    "cmovle", 
    "cmovl", 
    "cmove",
    "cmovne",
    "cmovge",
    "cmovg",
    "irmovq", 
    "rmmovq", 
    "mrmovq", 
    "addq", 
    "subq", 
    "andq", 
    "xorq", 
    "jmp",
    "jle",
    "jl",
    "je",
    "jne",
    "jge",
    "jg",
    "call",
    "ret", 
    "pushq",
    "popq",
    "byte",
    "word",
    "long",
    "quad",
    "pos",
    "align",
    "dup-symbol-error",
    "invalid-reg-error",
    "delim-missing-error",
    "invalid-imm-error",
    "invalid-mem-error",
    "invalid-dest-error",
    "unknown-symbol-error",
    "invalid-directive-error",
    NULL
};

static void test_all_uni()
{
    char **p = uni_list;
    while (*p)
        test_uni(*p++);
}

static void test_app(const char *name)
{
    ++app_test_count;
    printf("[ Testing application: %s ]\n", name);
    
    if (!make_app_base(name) && !make_app_stu(name) && !diff_app(name)) {
        ++app_pass_count;
        printf("[ Result: Pass ]\n");
    } else {
        printf("[ Result: Fail ]\n");
    }
}

static char *app_list[] = {
    "abs-asum-cmov",
    "abs-asum-jmp",
    "asumr",
    "asum",
    "cjr",
    "j-cc",
    "poptest",
    "prog10",
    "prog1",
    "prog2",
    "prog3",
    "prog4",
    "prog5",
    "prog6",
    "prog7",
    "prog8",
    "prog9",
    "pushquestion",
    "pushtest",
    "ret-hazard",
    NULL
};

static void test_all_app()
{
    // compare all .bin and .yo files
    char **p = app_list;
    while (*p)
        test_app(*p++);
}

#define SCORE_PER_INS 1.0
#define SCORE_PER_ERR 1.0
#define SCORE_PER_APP 2.0

static void print_result()
{
    double ins_gained_score = ins_pass_count * SCORE_PER_INS;
    double ins_full_score = ins_test_count * SCORE_PER_INS;
    if (ins_test_count)
        printf("Score for instructions:\t\t%5.2f/%5.2f\n", ins_gained_score, ins_full_score);
    
    double err_gained_score = err_pass_count * SCORE_PER_ERR;
    double err_full_score = err_test_count * SCORE_PER_ERR;
    if (err_test_count)
        printf("Score for error-handling cases:\t%5.2f/%5.2f\n", err_gained_score, err_full_score);
    
    double app_gained_score = app_pass_count * SCORE_PER_APP;
    double app_full_score = app_test_count * SCORE_PER_APP;
    if (app_test_count)
        printf("Score for applications:\t\t%5.2f/%5.2f\n", app_gained_score, app_full_score);
    
    double total_gained_score = ins_gained_score + err_gained_score + app_gained_score;
    double total_full_score = ins_full_score + err_full_score + app_full_score;
    if (ins_test_count && err_test_count && app_test_count)
        printf("Total score:\t\t\t%5.2f/%5.2f\n", total_gained_score, total_full_score);
}

static void print_usage()
{
    printf("Usage: yat -s <name>\n"
           "   Or: yat -S\n"
           "   Or: yat -a <name>\n"
           "   Or: yat -A\n"
           "   Or: yat -F\n\n"
           "Option specification:\n"
           "  -s         test single instruction ./y64-ins/<name>.ys,\n"
           "             or error-handling case in ./y64-err/<name>.ys\n"
           "             e.g. yat -s rrmovl, yat -s symbol-error\n"
           "  -S         test both instructions and error-handling\n"
           "  -a         test single application ./y64-app/<name>.ys\n"
           "  -A         test the application codes in ./y64-app\n"
           "  -F         test instructions, error-handling and application codes,\n"
           "             and you will get a total score\n"
           "  -h         print this message\n");
}

static void clean_up()
{
    // system("make clean > /dev/null");
    system("cd y64-ins; make clean > /dev/null");
    system("cd y64-err; make clean > /dev/null");
    system("cd y64-app; make clean > /dev/null");
    system("cd y64-base; make clean > /dev/null");
    system("rm -f *.err *.err.base");
}

int main(int argc, char *argv[])
{
    int stuff = 0;
  
    if (argc < 2)
        stuff = 1;
    else if (!strcmp(argv[1], "-h"))
        stuff = 1;
    else if (!strcmp(argv[1], "-s"))
        stuff = 2;
    else if (!strcmp(argv[1], "-S"))
        stuff = 3;
    else if (!strcmp(argv[1], "-a"))
        stuff = 4;
    else if (!strcmp(argv[1], "-A"))
        stuff = 5;
    else if (!strcmp(argv[1], "-F"))
        stuff = 6;
    
    if (stuff == 0) {
        fprintf(stderr, "yat: Invalid option %s\n", argv[1]);
        return 1;
    }
    
    if (stuff == 1) {
        print_usage();
        return 0;
    }
    
    if (make_y64asm()) {
        fprintf(stderr, "yat: Cannot make y64asm, go check y64asm.c or y64asm.h\n");
        return 1;
    }
    
    if (stuff == 2 || stuff == 4) {
        if (argc != 3) {
            fprintf(stderr, "yat: You have to specify exactly one file\n");
            return 1;
        }
        
        if (stuff == 2)
            test_uni(argv[2]);
        else
            test_app(argv[2]);
    } else if (stuff == 3) {
        test_all_uni();
    } else if (stuff == 5) {
        test_all_app();
    } else if (stuff == 6) {
        test_all_uni();
        test_all_app();
    }
        
    clean_up();
    
    print_result();
    
	return 0;
}

