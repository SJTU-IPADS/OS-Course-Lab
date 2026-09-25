// yat.c - Utility for testing y64 assembler.
// author: Lewis Cheng
// date: 2012/4/21
// update: 2012/4/22 by Lewis Cheng
// update: 2012/5/7 by Shida Min

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int make_y64sim()
{   
    return system("make > /dev/null");
}

#define COMMAND_BUFFER_SIZE 1024
static char cmdbuf[COMMAND_BUFFER_SIZE];

static int make_app_stu(const char *name,int steps)
{
	if(steps)
		sprintf(cmdbuf, "cd y64-app-bin; ../y64sim %s.bin %d > %s.sim", name,steps,name);
	else
		sprintf(cmdbuf, "cd y64-app-bin; ../y64sim %s.bin > %s.sim", name,name);
    
    return system(cmdbuf);
}

static int make_app_base(const char *name,int steps)
{
	if(steps)
		sprintf(cmdbuf, "cd y64-base; ./y64asm-base %s.ys; ./y64sim-base %s.bin %d > %s.sim.base", name,name,steps,name);
	else
		sprintf(cmdbuf, "cd y64-base; ./y64asm-base %s.ys; ./y64sim-base %s.bin > %s.sim.base",name,name,name);
    
    return system(cmdbuf);
}

static int diff_app(const char *name)
{
    sprintf(cmdbuf, " diff ./y64-base/%s.sim.base ./y64-app-bin/%s.sim", name, name);
    
    return system(cmdbuf);
}

static int make_ins_stu(const char *name,int steps)
{
	if(steps){
    		sprintf(cmdbuf, "cd y64-ins-bin; ../y64sim %s.bin %d > %s.sim", name,steps,name);
	}
	else
		sprintf(cmdbuf, "cd y64-ins-bin; make %s.sim > /dev/null", name);
   
    return system(cmdbuf);
}

static int make_ins_base(const char *name,int steps)
{
	if(steps)
  		sprintf(cmdbuf, "cd y64-base; ./y64asm-base %s.ys; ./y64sim-base %s.bin %d > %s.sim.base", name, name,steps,name);
	else
		sprintf(cmdbuf, "cd y64-base; ./y64asm-base %s.ys; ./y64sim-base %s.bin > %s.sim.base",name,name,name);

    return system(cmdbuf);
}

static int diff_ins(const char *name)
{
    sprintf(cmdbuf, " diff ./y64-base/%s.sim.base ./y64-ins-bin/%s.sim", name, name);
    
    return system(cmdbuf);
}

static int ins_pass_count;
static int ins_test_count;
static int app_test_count;
static int app_pass_count;

// test a uniterm, either an instruction or an error-handling case.
static void test_ins_bin(const char *name,int steps)
{
        // test an instruction
	ins_test_count++;
	printf("[ Testing instruction: %s ]\n", name);
        
	if (!make_ins_base(name,steps) && !make_ins_stu(name,steps) && !diff_ins(name)) {
		ins_pass_count++;
		printf("[ Result: Pass ]\n");
	} else {
		printf("[ Result: Fail ]\n");
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
    NULL
};

static void test_all_ins_bin()
{
    char **p = uni_list;
    while (*p)
        test_ins_bin(*p++,0);
}

static void test_app_bin(const char *name,int steps)
{
    ++app_test_count;
    printf("[ Testing application: %s ]\n", name);
    
    if (!make_app_base(name,steps) && !make_app_stu(name,steps) && !diff_app(name)) {
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

static void test_all_app_bin()
{
    // compare all .bin and .yo files
    char **p = app_list;
    while (*p)
        test_app_bin(*p++,0);
}

static int get_correct(const char*name,int steps)
{
	if(steps)
		sprintf(cmdbuf, "cd y64-base; ./y64asm-base %s.ys; ./y64sim-base %s.bin %d",name,name,steps);
	else
		sprintf(cmdbuf, "cd y64-base; make %s.sim; cat %s.sim",name,name);

	return system(cmdbuf);
}

#define SCORE_PER_INS 1.0
#define SCORE_PER_APP 2.0

static void print_result()
{
    double ins_gained_score = ins_pass_count * SCORE_PER_INS;
    double ins_full_score = ins_test_count * SCORE_PER_INS;
    if (ins_test_count)
        printf("Score for instructions:\t\t%5.2f/%5.2f\n", ins_gained_score, ins_full_score);
    
    double app_gained_score = app_pass_count * SCORE_PER_APP;
    double app_full_score = app_test_count * SCORE_PER_APP;
    if (app_test_count)
        printf("Score for applications:\t\t%5.2f/%5.2f\n", app_gained_score, app_full_score);
    
    double total_gained_score = ins_gained_score + app_gained_score;
    double total_full_score = ins_full_score + app_full_score;
    if (ins_test_count && app_test_count)
        printf("Total score:\t\t\t%5.2f/%5.2f\n", total_gained_score, total_full_score);
}

static void print_usage()
{
    printf("Usage: yat -c <name> [max_steps]\n"
		   "   Or: yat -s <name> [max_steps]\n"
           "   Or: yat -S\n"
		   "   Or: yat -a <name> [max_steps]\n"
           "   Or: yat -A\n"
           "   Or: yat -F\n\n"
           "Option specification:\n"
           "  [max_steps] limit the steps to observe the intermediate result\n"
		   "  -c          get the correct status of registers and memory\n"
		   "              (e.g. yat -c prog9 4)\n"
           "  -s          test single instruction in ./y64-ins-bin/<name>.bin,\n"
           "              (e.g. yat -s rrmovl)\n"
           "  -S          test all instructions\n"
           "  -a          test single application in ./y64-app-bin/<name>.bin\n"
           "              (e.g. yat -a asum)\n"
           "  -A          test all applications\n"
           "  -F          test instructions and applications, and get final score\n"
           "  -h          print this message\n");
}

static void clean_up()
{
    system("make clean > /dev/null");
    system("cd y64-ins-bin; make clean > /dev/null");
    system("cd y64-app-bin; make clean > /dev/null");
    system("cd y64-base; make clean > /dev/null");
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
    else if (!strcmp(argv[1], "-c"))
        stuff = 7;
    
    if (stuff == 0) {
        fprintf(stderr, "yat: Invalid option %s\n", argv[1]);
        return 1;
    }
    
    if (stuff == 1) {
        print_usage();
        return 0;
    }
    
    if (make_y64sim()) {
        fprintf(stderr, "yat: Cannot make y64sim, go check y64sim.c or y64sim.h\n");
        return 1;
    }
    
    if (stuff == 2) {
	   if (argc == 3) {
		test_ins_bin(argv[2],0);
	} else if (argc == 4){
		int step = atoi(argv[3]);
		test_ins_bin(argv[2],step);
	} else {
            fprintf(stderr, "yat: You have to specify the arguments\n");
            return 1;
        }
    

    } else if (stuff == 4) {
        if (argc == 3)
            test_app_bin(argv[2],0);
        else if(argc == 4){
		int step = atoi(argv[3]);
            test_app_bin(argv[2],step);
		}
        else{
            fprintf(stderr, "yat: You have to specify the arguments\n");
            return 1;
        }    
    
    } else if (stuff == 3) {
        test_all_ins_bin();
    } else if (stuff == 5) {
        test_all_app_bin();
    } else if (stuff == 6) {
        test_all_ins_bin();
        test_all_app_bin();
    } else if (stuff == 7) {
		if(argc == 3)
			get_correct(argv[2],0);
		else if (argc == 4){
			int step = atoi(argv[3]);
			get_correct(argv[2],step);
		}
	}
        
    clean_up();
    
    print_result();
    
	return 0;
}

