/*
 * G.711 assembler prototypes
 * Copyright (C) ARM Limited 1998-1999. All rights reserved.
 */

#ifndef _G711_S_
#define	_G711_S_

typedef  struct   {
    char  *input_buf  ;
    unsigned int in_file_size;
    char  *output_buf  ;
    unsigned int out_file_size;
   } INPARAM ;

#define LINEAR2ALAW     1
#define ALAW2LINEAR     2
#define LINEAR2ULAW     3
#define ULAW2LINEAR     4
#define ALAW2ULAW       5
#define ULAW2ALAW       6

int G711_linear2alaw( int pcmSample ) ;
int G711_alaw2linear( int aLawSample );

int G711_linear2ulaw( int pcmSample ) ;
int G711_ulaw2linear( int uLawSample ) ;

int G711_alaw2ulaw( int aLawSample ) ;
int G711_ulaw2alaw( int uLawSample ) ;
 
int G711_linear2linear( int pcmSample ) ;

char *perform_conversion( char inputs[ ], unsigned int numberBytes, unsigned int inBytes, unsigned int outBytes, unsigned int *numberBytesOut, int ( *ConvRoutine )( int ) );
void do_convertion(INPARAM *inparam, unsigned int option );

#endif	 /* _G711_S_ */
