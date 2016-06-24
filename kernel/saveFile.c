/*
 * The purpose of this module is to read RQ data from the PROC file system
 * and save it to memory.
 *
 */

	/* Needed by all modules*/ 
#include <linux/kernel.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef debug
# define debug 0
#endif

#define BUFSIZE (1 << 16)

int main(int argc, char **argv) {
  //Resets the experiement
  char *progName = argv[0];
  unsigned int sleepDelay = 1;
  FILE *reset;
  //Locks the CPU
  int command = system("echo \"0\" > /sys/devices/system/cpu/cpu1/online");
  //Resets the structs
  reset = fopen( "/proc/HARD_RESET", "r" ); 
  fclose(reset);
  //Removes old data file
  command = system("rm -rf sdcard/0/*.csv");
  int epochNum = 0;
  //Opens data file
  int ofd = creat("/sdcard/0/data.csv", O_APPEND);
  assert(ofd >= 0);
  
  while (1) {			/* epoch */
    int numReads = 0, numWrites = 0;
    int ifd, numLinesRead;
    char buf[BUFSIZE], *bufLimit = &buf[BUFSIZE], *bufInsertPtr = buf;
    ssize_t bytesRead;

    if (debug) printf("%s: epoch #%d, pausing for %ds, #lines=%d\n",
		      progName, epochNum++, sleepDelay, numLinesRead);
    sleep(sleepDelay);
    printf("done with sleep\n"); fflush(stdout);
    ifd = open("/proc/SEQ_RQ", O_RDONLY);
    assert(ifd >= 0);

    numLinesRead = 0;
    do {			/* repeat until a partial read */
      bufInsertPtr = buf;	/* insert at beginning of buffer */
      do {			/* read chunks up to BUFSIZE */
	bytesRead = read(ifd, bufInsertPtr, 4096); /* read up to 4k */
	if (bytesRead >= 0)			   /* update insertPtr */
	  bufInsertPtr += bytesRead;
	else 
	  perror("Error: ");
	numReads += 1;
	if (debug) printf(" read %d chars\n",  bytesRead);
      } while (bytesRead == 4096 && bufInsertPtr < bufLimit);
      if (bufInsertPtr != buf) { /* buffer not empty */
	{			 /* count newlines */
	  char *p;
	  for (p = buf; p < bufInsertPtr; p++) if (*p == '\n') numLinesRead++;
	}
	ssize_t bytesToWrite = bufInsertPtr - buf;
	assert(write(ofd, buf, bytesToWrite) == bytesToWrite);
	numWrites += 1;
	printf("."); fflush(stdout);
	if (debug) printf(" wrote %d bytes\n", bytesToWrite);
      }
    } while (bytesRead == 4096); /* repeat until non-partial read */
    printf("\n%d reads, %d writes, %d entries\n", numReads, numWrites, numLinesRead);
    if (numLinesRead > 6000)	 /* multiplicative decrease */
      sleepDelay=1; // >>= 1;		
    if (numLinesRead < 3000)	/* additive increase */
      sleepDelay=1; //++;
    if (sleepDelay < 1) sleepDelay = 1; /* never zero! */
  }
}
