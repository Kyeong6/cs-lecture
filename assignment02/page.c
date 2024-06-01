#include <stdio.h>
#include <stdlib.h>

#define LARGESIZE 100 * 1024 * 1024  // 100MB
#define TESTLOOP 1000000 // The number of trials
#define PAGELOOP 100     // The number of page entries

int main(int argc, char *argv[]){
  int *p = (int*)malloc(LARGESIZE); 

  //Find the pagesize of your computer by adjusting this value
  unsigned int pagesize = 100; // <- here!
  
  if(argc == 2) pagesize = atoi(argv[1]); 

  for(int i = 0; i < TESTLOOP; i++)
    for(int j = 0; j < PAGELOOP; j++)
    { 
      *(p + pagesize * j) = i;
    }
  return 0;
}

