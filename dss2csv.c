#include <stdio.h>
#include <stdlib.h>
#include "hecdss/heclib7.h"


int main(int argc, char *argv[])
{
  if (argc < 2) {
      printf("Provide dss file path\n");
      exit(1);
    }
  printf("DSS Version: %d\n", zgetFileVersion(argv[1]));

  long long ifltab;
  int file = hec_dss_zopen(&ifltab, argv[1]);
  printf("%d\n", file);
  zclose(&ifltab);
  return 0;
}
