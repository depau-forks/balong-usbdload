#include <stdio.h>
#include <stdint.h>

#ifndef WIN32
//%%%%
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#else
//%%%%
#include <windows.h>
#include "getopt.h"
#include "printf.h"
#endif

#include "patcher.h"


//#######################################################################################################
void main(int argc, char* argv[]) {
  
FILE* in;
FILE* out;
uint8_t* buf;
uint32_t fsize;
int opt;
uint8_t outfilename[100];
int oflag=0,bflag=0;
uint32_t res;


// Command line parsing

while ((opt = getopt(argc, argv, "o:bh")) != -1) {
  switch (opt) {
   case 'h': 
     
printf("\n Program for automatic patching of Balong V7 platform loaders\n\n\
%s [keys] <usbloader file>\n\n\
 The following keys are valid:\n\n\
-o file  - output file name. By default, only a patch possibility check is performed\n\
-b       - add a patch that disables checking for bad blocks\n\
\n",argv[0]);
    return;

   case 'o':
    strcpy(outfilename,optarg);
    oflag=1;
    break;

   case 'b':
     bflag=1;
     break;
     
   case '?':
   case ':':  
     return;
    
  }
}  

printf("\n Program for automatic modification of Balong V7 loaders, (c) forth32");

 if (optind>=argc) {
    printf("\n - No file name specified for download\n - For a hint, specify the -h key\n");
    return;
}  
    
in=fopen(argv[optind],"rb");
if (in == 0) {
  printf("\n Error opening file %s",argv[optind]);
  return;
}

// determine the file size
fseek(in,0,SEEK_END);
fsize=ftell(in);
rewind(in);

// allocate a buffer and read the entire file into it
buf=malloc(fsize);
fread(buf,1,fsize,in);
fclose(in);

//==================================================================================

res=pv7r1(buf, fsize);
if (res != 0)  {
  printf("\n* V7R1 type signature found at offset %08x",res);
  goto endpatch;
}  

res=pv7r2(buf, fsize);
if (res != 0)  {
  printf("\n* V7R2 type signature found at offset %08x",res);
  goto endpatch;
}  

res=pv7r11(buf, fsize);
if (res != 0)  {
  printf("\n* V7R11 type signature found at offset %08x",res);
  goto endpatch;
}   

res=pv7r22(buf, fsize);
if (res != 0)  {
  printf("\n* V7R22 type signature found at offset %08x",res);
  goto endpatch;
}  

res=pv7r22_2(buf, fsize);
if (res != 0)  {
  printf("\n* V7R22_2 type signature found at offset %08x",res);
  goto endpatch;
}

res=pv7r22_3(buf, fsize);
if (res != 0)  {
  printf("\n* V7R22_3 type signature found at offset %08x",res);
  goto endpatch;
}

printf("\n! Eraseall-patch signature not found");

//==================================================================================
endpatch:

if (bflag) {
   res=perasebad(buf, fsize);
   if (res != 0) printf("\n* isbad signature found at offset %08x",res);  
   else  printf("\n! isbad signature not found");  
}

if (oflag) {
  out=fopen(outfilename,"wb");
  if (out != 0) {
    fwrite(buf,1,fsize,out);
    fclose(out);
  }
  else printf("\n Error opening output file %s",outfilename);
}
free(buf);
printf("\n");
}

   