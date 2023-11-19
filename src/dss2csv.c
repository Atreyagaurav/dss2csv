#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "hecdss/hecdss7.h"
#include "hecdss/hecdssInternal.h"
#include "hecdss/heclib.h"
#include "hecdss/zStructCatalog.h"
#include "hecdss/zdssMessages.h"


void read_range(char *rng, int *start, int *end) {
  int flag_start = 0;
  int flag_end = 0;
  int val_start = 0;
  int val_end = 0;

  int *val = &val_start;
  int *flag = &flag_start;

  int ind = 0;
  while (rng[ind] != 0) {
    switch (rng[ind]){
    case '-':
      val = &val_end;
      flag = &flag_end;
      break;
    case '0'...'9':
      *flag = 1;
      *val = *val * 10 + rng[ind] - '0';
      break;
    default:
      printf("Invalid character '%c' in %s", rng[ind], rng);
    }
    ind += 1;
  }
  if (flag_start) *start = val_start - 1 ;
  if (flag_end) *end = val_end;
  }

  void list_paths(zStructCatalog *catStruct, int start, int end) {
  int i;
  for (i = start; i < end; i++) {
    printf("%5d: %s\n", i + 1, catStruct->pathnameList[i]);
  }
  }
  
int save_paths(long long *ifltab, zStructCatalog *catStruct, int start, int end) {
  int i;
  zStructTimeSeries *tss1;
  char cdate[13], ctime[10];
  char outfilename[_MAX_PATH];
  int status;
  
  for (i = start; i < end; i++) {
      /* +1 here to skip the first '/' */
      strcpy(outfilename, catStruct->pathnameList[i] + 1);
      int ind = 0;
      while (outfilename[ind] != 0) {
        switch (outfilename[ind]) {
        case '/':
          outfilename[ind] = '_';
        default:
          break;
        }
        ind += 1;
      }
      /* -1 here to change the last '/' (now '_') to '.' */
      outfilename[ind - 1] = '.';
      strcat(outfilename, "csv");
      
      printf("%5d: %s\n", i + 1, outfilename);

      tss1 = zstructTsNew(catStruct->pathnameList[i]);
      status = ztsRetrieve(ifltab, tss1, -1, 1, 1);
      if (status != STATUS_OKAY)
        return status;
      FILE *fp = fopen(outfilename, "w");
      int t;

      fprintf(fp, "index,date,time,value\n");
      for (t = 0; t < tss1->numberValues; t++) {
        getDateAndTime(tss1->times[t], tss1->timeGranularitySeconds,
                       tss1->julianBaseDate, cdate, sizeof(cdate), ctime,
                       sizeof(ctime));
        if (zisMissingFloat(tss1->floatValues[t])) {
          fprintf(fp, "%d,%s,%s,\n", t, cdate, ctime);

        } else {
          fprintf(fp, "%d,%s,%s,%.2f\n", t, cdate, ctime, tss1->floatValues[t]);
        }
      }
      fclose(fp);
    }
  return STATUS_OKAY;
}

void print_help(char *name){
    printf("\nUsage: %s command dss_file.dss [rng]\n", name);
    printf("\nCommands:\n"
	   "    help[h]      : print this help menu.\n"
           "    list[l]      : list the available paths.\n"
	   "    extract[e]   : extract the timeseries for paths.\n"
	   "Arguments:\n"
	   "    dss_file.dss : dss file to operate on.\n"
	   "    rng          : Range of the chosen timeseries use in format M-N\n"
	   "                   where M is start and N is the end number (inclusive)\n"
	   "                   Omitting the start or end will default in available\n"
	   "                   start or the end. (e.g. 1-5 or -5 or 5-)\n"
	   "                   [Optional: Defaults to all available]\n");
}

int main(int argc, char *argv[]) {
  long long ifltab[250];
  char filename[_MAX_PATH];
  char pathname[393];
  char *tempname;
  zStructCatalog *catStruct;
  int exists;
  int idummy;
  int loop;
  int numberPaths;
  int numberBytes;
  int ihandle;
  int status, i, j;
  long lbyte;

  if (argc < 3) {
    if (argv[1][0] != 'h'){
      printf("Not enough arguments.\n\n");
    }
    print_help(argv[0]);
    return -1;
  }
  zsetMessageLevel(0, 0);

  exists = zfileName(filename, sizeof(filename), argv[2], &idummy);
  if (!exists) {
    printf("*** Error - DSS file must exist for this sample.  File: %s\n",
           filename);
    return -1;
  }
  status = hec_dss_zopen(ifltab, filename);
  if (status != STATUS_OKAY)
    return status;

  catStruct = zstructCatalogNew();
  numberPaths = zcatalog(ifltab, "", catStruct, 1);
  if (numberPaths < 0)
    return numberPaths;
  printf("%d total pathnames found\n", catStruct->numberPathnames);

  int list = 0;
  int start = 0;
  int end = catStruct->numberPathnames;
  if (argc == 4) {
      read_range(argv[3], &start, &end);
  } else {
    end = catStruct->numberPathnames;
  }

  switch (argv[1][0]) {
  case 'h':
    print_help(argv[0]);
    return 0;
  case 'l':
    list_paths(catStruct, start, end);
    break;
  case 'e':
    status = save_paths(ifltab, catStruct, start, end);
    if (status != STATUS_OKAY)
        return status;
    break;
  default:
    printf("Unknown Command.\n\n");
    print_help(argv[0]);
    return 1;
  }
  zstructFree(catStruct);
  zclose(ifltab);
  return 0;
}

/* IDK why it was not loaded into the .so file or .a file, so copying it here.
 */
int getDateAndTime(int time, int timeGranularitySeconds, int julianBaseDate,
                   char *dateString, int sizeOfDateString, char *hoursMins,
                   int sizeofHoursMins) {
  int numberInDay;
  int days;
  int timeOfDay;
  int julian;
  long long granularity;

  //  Convert minutes or seconds to days and seconds
  granularity = (long long)timeGranularitySeconds;
  if (granularity < 1)
    granularity = MINUTE_GRANULARITY;
  numberInDay = (int)(SECS_IN_1_DAY / granularity);
  days = time / numberInDay;
  julian = julianBaseDate + days;

  //  Now start day and seconds
  timeOfDay = time - (days * numberInDay);
  if (timeOfDay < 1) {
    julian--;
    timeOfDay += numberInDay;
  }

  if (timeGranularitySeconds == SECOND_GRANULARITY) {
    secondsToTimeString(timeOfDay, 0, 2, hoursMins, sizeofHoursMins);
  } else if (timeGranularitySeconds == MINUTE_GRANULARITY) {
    minutesToHourMin(timeOfDay, hoursMins, sizeofHoursMins);
  } else {
    //  Get minutes
    timeOfDay *= (timeGranularitySeconds / SECS_IN_1_MINUTE);
    minutesToHourMin(timeOfDay, hoursMins, sizeofHoursMins);
  }
  julianToDate(julian, -13, dateString, sizeOfDateString);

  return 0;
}
