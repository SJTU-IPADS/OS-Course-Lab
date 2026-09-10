/***********************************************************************
 * Sequential Y86-64 Simulators
 *
 * Copyright (c) 2002, 2010, 2013, 2015  R. Bryant and D. O'Hallaron,
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 ***********************************************************************/ 

This directory contains the code to construct simulators for SEQ,
SEQ+, and the variants of it described in the homework exercises.

**************************
1. Building the simulators
**************************

Different versions of the SEQ and SEQ+ simulators can be constructed
to use different HCL files when working on the different homework
problems.

Binary	VERSION	HCL File	Description
ssim	std	seq-std.hcl	Standard SEQ simulator described in textbook.
ssim	full	seq-full.hcl	For adding iaddq to SEQ.
ssim+	std	seq+-std.hcl	Standard SEQ+ simulator described in textbook.

The simulator running in TTY mode prints all information about its
runtime behavior on the terminal.  It's hard to understand what's
going on, but useful for automated testing, and doesn't require any
special installation features.

Once you've configured the Makefile, you can build the different
simulators with commands of the form

	unix> make clean; make ssim VERSION=xxx

where "xxx" is one of the versions listed above.  For example, to build
the version of SEQ described in the CS:APP text based on the control
logic in seq-std.hcl, type

	unix> make clean; make ssim VERSION=std

To save typing, you can also set the Makefile's VERSION variable.

***********************
1. Using the simulators
***********************

The simulators take identical command line arguments:

Usage: ssim [-htg] [-l m] [-v n] file.yo

   -h     Print this message
   -l m   Set instruction limit to m [TTY mode only] (default 10000)
   -v n   Set verbosity level to 0 <= n <= 2 [TTY mode only] (default 2)
   -t     Test result against the ISA simulator (yis) [TTY model only]

********
3. Files
********

Makefile		Builds the SEQ and SEQ+ simulators
Makefile-sim		Makefile for student distribution
README			This file

ssim.c			Base sequential simulator code and header file
sim.h

seq-std.hcl		Standard SEQ control logic
seq+-std.hcl		Standard SEQ+ control logic	
seq-full.hcl		Template for the iaddq problem (4.34-35)

seq-full-ans.hcl	Solution for the iaddq problems (4.34-35)
			(Instructor distribution only)
