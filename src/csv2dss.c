#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "hecdss/hecdss7.h"
#include "hecdss/hecdssInternal.h"
#include "hecdss/heclib.h"
#include "hecdss/zStructAllocation.h"
#include "hecdss/zStructCatalog.h"
#include "hecdss/zStructSpatialGrid.h"
#include "hecdss/zdssKeys.h"
#include "hecdss/zdssMessages.h"

void filename2dsspath(char *infilename, char *path) {
  int offset = 0;
  int ind = 0;
  /* To ignore the pathnames till the filename */
  while (infilename[ind] != 0){
    if (infilename[ind] == '/') offset = ind;
    ind += 1;
  }
  /* the first '/' */
  *path = '/';
  ind = offset + 1;
  while (infilename[ind] != '.') {
    switch (infilename[ind]) {
    case '_':
      path[ind - offset] = '/';
      break;
    default:
      path[ind - offset] = infilename[ind];
      break;
    }
    ind += 1;
  }
  ind -= offset;
  /* the last '/' */
  path[ind] = '/';
  path[ind + 1] = 0;
}

void with_extension(char *infilename, char *outfilename, char * ext) {
  int offset = 0;
  int ind = 0;
  /* To ignore the pathnames till the filename */
  while (infilename[ind] != 0){
    if (infilename[ind] == '.') offset = ind;
    ind += 1;
  }
  strcpy(outfilename + offset + 1, ext);
}

int save_timeseries(long long *ifltab, zStructCatalog *catStruct, int start,
               int end) {
  /* TODO */
  
  /* int i; */
  /* zStructTimeSeries *tss1; */
  /* char cdate[13], ctime[10]; */
  /* char outfilename[_MAX_PATH]; */
  /* int status; */

  /* for (i = start; i < end; i++) { */
  /*   dsspath2filename(outfilename, catStruct->pathnameList[i], "csv"); */
  /*   printf("%5d: %s\n", i + 1, outfilename); */

  /*   tss1 = zstructTsNew(catStruct->pathnameList[i]); */
  /*   status = ztsRetrieve(ifltab, tss1, -1, 1, 1); */
  /*   if (status != STATUS_OKAY) */
  /*     return status; */
  /*   FILE *fp = fopen(outfilename, "w"); */
  /*   int t; */

  /*   fprintf(fp, "index,date,time,value\n"); */
  /*   for (t = 0; t < tss1->numberValues; t++) { */
  /*     getDateAndTime(tss1->times[t], tss1->timeGranularitySeconds, */
  /*                    tss1->julianBaseDate, cdate, sizeof(cdate), ctime, */
  /*                    sizeof(ctime)); */
  /*     if (zisMissingFloat(tss1->floatValues[t])) { */
  /*       fprintf(fp, "%d,%s,%s,\n", t, cdate, ctime); */

  /*     } else { */
  /*       fprintf(fp, "%d,%s,%s,%.2f\n", t, cdate, ctime, tss1->floatValues[t]); */
  /*     } */
  /*   } */
  /*   fclose(fp); */
  /* } */
  return STATUS_OKAY;
}

int save_grid(long long *ifltab, char ** input_files, int num_files) {
  zStructSpatialGrid *grid;
  float *data;
  float tempx,tempy;
  int idx, i;
  char *infilename;
  char path[_MAX_PATH];
  char infilename_prj[_MAX_PATH];
  int status = 0;
  for (i = 0; i < num_files; i++) {
    infilename = *(input_files + i);
    filename2dsspath(infilename, path);

    printf("%5d: %s\n", i + 1, path);

    FILE *fp = fopen(infilename, "r");
    int t;

    grid = zstructSpatialGridNew(path);
    
    grid->_type = 420;
    grid->_dataSource = mallocAndCopy("INTERNAL");
    grid->_version = 1;
    grid->_dataUnits = mallocAndCopy("mm");

    /* temp */
    int range = 5;
	static float *rangelimit;
	static int *histo;

	rangelimit = calloc(range, sizeof(float));
	histo = calloc(range, sizeof(float));

	for (idx = 0; idx < range; idx++) {
		rangelimit[idx] = idx * 1.1;
		histo[idx] = idx * 2;
	}
	grid->_rangeLimitTable = &(rangelimit[0]);
	grid->_numberEqualOrExceedingRangeLimit = &(histo[0]);
	grid->_numberOfRanges = range;
	grid->_timeZoneID = mallocAndCopy("PST");
	grid->_compressionMethod = NO_COMPRESSION;

	/* TODO: should I calculate it? */
	float* max = malloc(sizeof(float));
	*max = 1.0;
	float* min = malloc(sizeof(float));
	 *min = 0.001;
	 float* mean = malloc(sizeof(float));
	 *mean = 0.2;
	grid->_maxDataValue =  max;
	grid->_minDataValue = min;
	grid->_meanDataValue = mean;

    fscanf(fp, "ncols %d\n", &grid->_numberOfCellsX);
    fscanf(fp, "nrows %d\n", &grid->_numberOfCellsY);
    fscanf(fp, "xllcorner %f\n", &tempx);
    fscanf(fp, "yllcorner %f\n", &tempy);
    fscanf(fp, "cellsize %f\n", &grid->_cellSize);
    grid->_lowerLeftCellX = (int)(tempx / grid->_cellSize + 0.5);
    grid->_lowerLeftCellY = (int)(tempy / grid->_cellSize + 0.5);
    fscanf(fp, "NODATA_value %f\n", &grid->_nullValue);
    
    int x, y;
    data = (float*)calloc(grid->_numberOfCellsX * grid->_numberOfCellsY,
			  sizeof(float));
    for (y = 0; y < grid->_numberOfCellsY; y++) {
      for (x = 0; x < grid->_numberOfCellsX; x++) {
        idx = (grid->_numberOfCellsY - y - 1) * grid->_numberOfCellsX + x;
        fscanf(fp, "%f", data + idx);
      }
    }
    fclose(fp);
    grid->_data = data;

    grid->_srsDefinitionType = 1;
    grid->_srsName = mallocAndCopy("WKT");
    /* char* buffer = NULL; */
    /* size_t len; */
    /* with_extension(infilename, infilename_prj, "prj"); */
    /* fp = fopen(infilename_prj, "r"); */
    /* if (fp != NULL) { */
    /* ssize_t bytes_read = getdelim(&buffer, &len, '\0', fp); */
    /* grid->_srsDefinition = mallocAndCopy(buffer); */
    /* } else { */
    grid->_srsDefinition = mallocAndCopy(SHG_SRC_DEFINITION);
    /* } */
    /* fclose(fp); */

    status = zspatialGridStore(ifltab, grid);
    
    if (status != STATUS_OKAY) {
      printf("Error storing grid: %d", status);
      return status;
    }
    zstructFree(grid);
    }
  return status;
}

void print_help(char *name) {
  printf("\nUsage: %s command dss_file.dss input_files...\n", name);
  printf(
      "\nCommands:\n"
      "    help[h]       : print this help menu.\n"
      "    timeseries[t] : extract the timeseries for paths.\n"
      "    grid[g]       : extract the grid for paths.\n"
      "Arguments:\n"
      "    dss_file.dss  : dss file to operate on.\n"
      "    input_files...: Input files to convert to dss format\n"
      "                  : filename should have fields A-F separated by _\n");
}

int main(int argc, char *argv[]) {
  long long ifltab[250];
  char filename[_MAX_PATH];
  int exists;
  int idummy;
  int status;

  if (argc < 4) {
    if (argv[1][0] != 'h') {
      printf("Not enough arguments.\n\n");
    }
    print_help(argv[0]);
    return -1;
  }
  zsetMessageLevel(0, 0);

  exists = zfileName(filename, sizeof(filename), argv[2], &idummy);
  if (exists) {
    printf("*** Error - DSS file %s already exists, replace? <Y/n>.\n",
           filename);
    char y;
    scanf("%c", &y);
    if (y == 'n' || y == 'N'){
      return -1;
    }
  }
  status = hec_dss_zopen(ifltab, filename);
  if (status != STATUS_OKAY)
    return status;
  switch (argv[1][0]) {
  case 'h':
    print_help(argv[0]);
    return 0;
  case 'g':
    status = save_grid(ifltab, argv + 3, argc - 3);
    if (status != STATUS_OKAY) {
      printf("Error [code: %d]\n", status);
      return status;
    }
    break;
  /* case 't': */
  /*   status = save_timeseries(ifltab, catStruct); */
  /*   if (status != STATUS_OKAY) { */
  /*     printf("Error [code: %d]\n", status); */
  /*     return status; */
  /*   } */
  /*   break; */
  default:
    printf("Unknown Command.\n\n");
    print_help(argv[0]);
    return 1;
  }
  zclose(ifltab);
  return 0;
}
