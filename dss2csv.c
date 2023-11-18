#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "hecdss/hecdssInternal.h"
#include "hecdss/heclib.h"
 

int main(int argc, char* argv[])
{
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

	
	zStructTimeSeries *tss1;
	char cdate[13], ctime[10];
	char outfilename[_MAX_PATH];

	if (argc < 2)
	{
		printf("\nUsage: CatalogTest file.dss\n");
		return -1;
	}
		exists = zfileName(filename, sizeof(filename), argv[1], &idummy);
		if (!exists) {
			printf("*** Error - DSS file must exist for this sample.  File: %s\n", filename);
			return -1;
		}
		status = hec_dss_zopen(ifltab, filename);
		if (status != STATUS_OKAY) return status;

		//  Get a regular full sorted catalog and print first 5 pathnames
		catStruct = zstructCatalogNew();
		//int zcatalog(long long *ifltab, char *pathWithWild, zStructCatalog *catStruct, int boolSorted);
		numberPaths = zcatalog(ifltab, "", catStruct, 1);
		if (numberPaths < 0) return numberPaths;
		printf("%d total pathnames found\n", catStruct->numberPathnames);

		for (i = 0; i < numberPaths; i++){
		  printf("%s\n", catStruct->pathnameList[i]);
		  /* +1 here to skip the first '/' */
		  strcpy(outfilename, catStruct->pathnameList[i]+1);
		  int ind = 0;
		  while(outfilename[ind] != 0){
		    switch (outfilename[ind]) {
		    case '/':
		      outfilename[ind] = '_';
		    default:
		      break;
			}
		    ind += 1;
		  }
		  /* -1 here to change the last '/' (now '_') to '.' */
		  outfilename[ind-1] = '.';
		  strcat(outfilename, "csv");
		  
		  tss1 = zstructTsNew(catStruct->pathnameList[i]);
		  status = ztsRetrieve(ifltab, tss1, -1, 1, 1);
		  if (status != STATUS_OKAY) return status;
		  FILE *fp = fopen(outfilename, "w");
		  int t;
		  
		  fprintf(fp, "index,date,time,value\n");
                  for (t = 0; t < tss1->numberValues; t++) {
                    getDateAndTime(tss1->times[t], tss1->timeGranularitySeconds,
                                   tss1->julianBaseDate, cdate, sizeof(cdate),
                                   ctime, sizeof(ctime));
                    if (zisMissingFloat(tss1->floatValues[t])) {
                      fprintf(fp, "%d,%s,%s,\n", t, cdate, ctime);

                    } else {
                      fprintf(fp, "%d,%s,%s,%.2f\n", t, cdate, ctime,
                              tss1->floatValues[t]);
                    }
                  }
		  fclose(fp);
                }
		zstructFree(catStruct);
		zclose(ifltab);
	return 0;
}



/* IDK why it was not loaded into the .so file or .a file, so copying it here. */
int getDateAndTime(int time, int timeGranularitySeconds, int julianBaseDate,
				   char *dateString, int sizeOfDateString, char* hoursMins, int sizeofHoursMins)
{
	int numberInDay;
	int days;
	int timeOfDay;
	int julian;
	long long granularity;


	//  Convert minutes or seconds to days and seconds
	granularity = (long long)timeGranularitySeconds;
	if (granularity < 1) granularity = MINUTE_GRANULARITY;
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
	}
	else if (timeGranularitySeconds == MINUTE_GRANULARITY) {
		minutesToHourMin(timeOfDay, hoursMins, sizeofHoursMins);
	}
	else {
		//  Get minutes
		timeOfDay *= (timeGranularitySeconds / SECS_IN_1_MINUTE);
		minutesToHourMin(timeOfDay, hoursMins, sizeofHoursMins);
	}
	julianToDate(julian, 4, dateString, sizeOfDateString);

	return 0;
}

