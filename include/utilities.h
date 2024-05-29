/* 
 * Copyright 2005 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*****************************************************************************************\
*                                                                                         *
*  Utilities for allocating memory, opening files, and processing command line arguments  *
*                                                                                         *
*  Author:  Gene Myers                                                                    *
*  Date  :  October 2005                                                                  *
*                                                                                         *
*                                                                                         *
*  Copyright (c) Oct. 1, '05 Dr. Gene Myers and Howard Hughes Medical Institute ("HHMI")  *
*                                                                                         *
*  This software is provided and licensed on the following terms.  By downloading,        *
*  using and/or copying the software, you agree that you have read, understood, and       *
*  will comply with these terms.                                                          *
*                                                                                         *
*  Redistribution and use in source and binary forms, with or without modification,       *
*  are permitted provided that the following conditions are met:                          *
*     1. Redistributions of source code must retain the above copyright notice, this      *
*          list of conditions and the following disclaimer.                               *
*     2. Redistributions in binary form must reproduce the above copyright notice, this   *
*          list of conditions and the following disclaimer in the documentation and/or    *
*          other materials provided with the distribution.                                *
*     3. Neither the name of the Howard Hughes Medical Institute nor the names of its     *
*          contributors may be used to endorse or promote products derived from this      *
*          software without specific prior written permission.                            *
*                                                                                         *
*  For use of this software and documentation for purposes other than those listed above, *
*  contact Dr. Gene Myers at:                                                             *
*                                                                                         *
*    Janelia Farms Research Campus                                                        *
*    19700 Helix Drive                                                                    *
*    Ashburn, VA  20147-2408                                                              *
*    Tel:   571.209.4153                                                                  *
*    Fax:   571.209.4083                                                                  *
*    Email: myersg@janelia.hhmi.org                                                       *
*                                                                                         *
*  For any issues regarding HHMI or use of its name, contact:                             *
*                                                                                         *
*    Howard Hughes Medical Institute                                                      *
*    4000 Jones Bridge Road                                                               *
*    Chevy Chase, MD 20815-6789                                                           *
*    (301) 215-8500                                                                       *
*    Email: webmaster@hhmi.org                                                            *
*                                                                                         *
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY    *
*  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES   *
*  OF MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE ARE          *
*  DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY   *
*  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, *
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR *
*  PROFITS; REASONABLE ROYALTIES; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         *
*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF   *
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                             *
*                                                                                         *
*  The names of the authors and copyright holders may not be used in advertising or       *
*  publicity pertaining to the software without specific, written prior permission.       *
*  Title to copyright in this software will remain with copyright holders.                *
*                                                                                         *
\*****************************************************************************************/

#ifndef SR_UTILITIES
#define SR_UTILITIES

#include "compat.h"
#include <stdio.h>
#include <stdlib.h>

// Macro to check memory allocation
#define CHECK_ALLOC(ptr) if ((ptr) == NULL) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }

#define ASCII 128

/* The usual protected allocation and file opening routines. */

/**
 * @brief Allocates memory with error checking.
 * 
 * @param size The size of the memory to allocate.
 * @param routine The name of the routine requesting memory.
 * @return Pointer to the allocated memory.
 */
void *Guarded_Malloc(size_t size, const char *routine);

/**
 * @brief Reallocates memory with error checking.
 * 
 * @param array The original memory block to reallocate.
 * @param size The new size of the memory block.
 * @param routine The name of the routine requesting memory.
 * @return Pointer to the reallocated memory.
 */
void *Guarded_Realloc(void *array, size_t size, const char *routine);

/**
 * @brief Duplicates a string with error checking.
 * 
 * @param string The original string to duplicate.
 * @param routine The name of the routine requesting the string duplication.
 * @return Pointer to the duplicated string.
 */
char *Guarded_Strdup(const char *string, const char *routine);

/**
 * @brief Opens a file with error checking.
 * 
 * @param name The name of the file to open.
 * @param options The mode in which to open the file.
 * @param routine The name of the routine requesting the file open.
 * @return FILE pointer to the opened file.
 */
FILE *Guarded_Fopen(const char *name, const char *options, const char *routine);

/* Process the command line according to 'spec' (See Appendix A for details). Any
   failure results in an error message and the end of execution. One tricky thing
   here is that 'spec' itself is interpreted, so an invalid spec will also produce
   an error message. Be sure to debug the spec! If no_escapes is non-zero then any
   escaping quotes in the specification will not be seen in a usage statement, should
   one be printed out with an error. */

/**
 * @brief Processes command line arguments according to a specification.
 * 
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @param spec The specification for parsing arguments.
 * @param no_escapes If non-zero, escaping quotes will not be shown in usage statement.
 */
void Process_Arguments(int argc, char *argv[], char *spec[], int no_escapes);

/* Once the command line has been parsed you can get the value of any argument by calling
   the appropriately typed 'Get' routine with the name of the argument. You presumably
   know the type since you wrote the spec. If an argument is an iterator then you must
   further supply the 'Get' call with the index of the instance you want, where the numbering
   starts at 1. To know how many instances there are call Get_Repeat_Count. If an argument
   is a multi-value option then you must also specify which value you want. Is_Arg_Matched
   will tell you if any particular argument has been matched on the command line or not.
   Finally, you can get the program name with Program_Name. */

/**
 * @brief Gets the program name.
 * 
 * @return The program name.
 */
char *Program_Name();

/**
 * @brief Gets the repeat count of an argument.
 * 
 * @param name The name of the argument.
 * @return The repeat count of the argument.
 */
int Get_Repeat_Count(char *name);

/**
 * @brief Checks if an argument has been matched on the command line.
 * 
 * @param name The name of the argument.
 * @param ... Additional arguments.
 * @return Non-zero if the argument is matched, zero otherwise.
 */
int Is_Arg_Matched(char *name, ...);

/**
 * @brief Gets the integer value of an argument.
 * 
 * @param name The name of the argument.
 * @param ... Additional arguments.
 * @return The integer value of the argument.
 */
int Get_Int_Arg(char *name, ...);

/**
 * @brief Gets the double value of an argument.
 * 
 * @param name The name of the argument.
 * @param ... Additional arguments.
 * @return The double value of the argument.
 */
double Get_Double_Arg(char *name, ...);

/**
 * @brief Gets the string value of an argument.
 * 
 * @param name The name of the argument.
 * @param ... Additional arguments.
 * @return The string value of the argument.
 */
char *Get_String_Arg(char *name, ...);

/* There may be constraints among the arguments that are not captured by the spec that
   you explicitly check after the call to Process_Args. If you detect an error and wish
   to print out a usage message, a call to Print_Argument_Usage will do so on the file
   'file'. As for Process_Arguments, passing in a non-zero no_escapes value suppresses
   the printing of escape chars in the statement. */

/**
 * @brief Prints the argument usage message to a file.
 * 
 * @param file The file to print the usage message to.
 * @param no_escapes If non-zero, escaping quotes will not be shown in usage statement.
 */
void Print_Argument_Usage(FILE *file, int no_escapes);

#endif
