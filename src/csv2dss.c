#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "hecdss/hecdss7.h"
#include "hecdss/hecdssInternal.h"
#include "hecdss/heclib.h"
#include "hecdss/heclibDate.h"
#include "hecdss/zStructAllocation.h"
#include "hecdss/zStructCatalog.h"
#include "hecdss/zStructSpatialGrid.h"
#include "hecdss/zdssKeys.h"
#include "hecdss/zdssMessages.h"

void filename2dsspath(char *infilename, char *path) {
  int offset = -1;
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

int next(FILE* fp, char * buffer, char delim) {
  int i = 0;
  char c;
  while ((c=getc(fp)) != delim){
    buffer[i++] = c;
  }
  buffer[i] = 0;
  return i;
}

int read_csv(char *filename, float **values, char *start_day, int *num_records) {
  char buffer[1000];
  size_t len;
  size_t read;
  int header = 0;
  int start_day_jul;
  int hour,min;
  FILE * fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Err Can't open: %s\n", filename);
    return 0;
  }

  /* Count the number of lines for malloc */
  int num_lines = 0;
  char c;
  while ((c=getc(fp)) != EOF){
    if (c == '\n') num_lines++;
  }

  /* *time = malloc(sizeof(int) * (num_lines)); */
  *values = malloc(sizeof(float) * (num_lines));
  
  if (/* *time == NULL ||  */*values == NULL){
    printf("Can't malloc.\n");
    exit(1);
  }

  rewind(fp);
  c = getc(fp);
  if (c < '0' || c > '9'){
    /* if it has text as the first character assume header line*/
    header = 1;
    num_lines--;
  }
  *num_records = num_lines;
  /* Detect the first date, in string and julian */
  
  rewind(fp);
  if (header){
    while (getc(fp) != '\n');
  }
  /* date */
  read = next(fp, buffer, ',');
  strcpy(start_day, buffer);
  start_day_jul = dateToJulian(buffer);
  /* time in HHMM format*/
  /* read = getdelim(&lineptr, &len, ',', fp); */
  /* *(buffer + read - 1) = 0; */
  /* min = atoi(buffer); */
  /* hour = min / 100; */
  /* min = min % 100; */
  
  /* start_day_jul += hour * 60 + min; */
  
  /* Now the values */
  rewind(fp);
  if (header){
    while (getc(fp) != '\n');
  }
  int i;
  for (i=0; i < num_lines; i++) {
    /* read date */
    read = next(fp, buffer, ',');
    /* *(*time + i) = (dateToJulian(buffer) - start_day_jul) * 24 * 60; */
    /* read time in HHMM format
       uncomment if needed.
     */
    /* read = getdelim(&buffer, &len, ',', fp); */
    /* *(buffer + read - 1) = 0; */
    /* min = atoi(buffer); */
    /* hour = min / 100; */
    /* min = min % 100; */
    /* *(*time + i) += hour * 60 + min; */
    read = next(fp, buffer, '\n');
    if (read > 0) {
      *(*values + i) = atof(buffer);
    } else {
      /* empty (only '\n') means no data, but NAN from math.h doesn't seem compatible with DSS*/
      /* *(*values + i) = NAN; */
      zsetMissingFloat(*values + i);
    }
  }
  fclose(fp);
  return 1;
}

int save_timeseries(long long *ifltab, char **input_files, int num_files) {
  zStructTimeSeries *tss1;
  char start_day[100];
  int num_records;
  float *fvalues;
  int *itimes;
  int status;
  int idx, i;
  char *infilename;
  char path[_MAX_PATH];

  for (i = 0; i < num_files; i++) {
    infilename = *(input_files + i);
    filename2dsspath(infilename, path);

    if (!read_csv(infilename, &fvalues, start_day, &num_records)) continue;
    printf("%5d: %s\n", i + 1, path);
    /* this fails if the path is just "/A/B/C/D/E/F/" but E as 1Day works, weird */
    tss1 = zstructTsNewRegFloats(path, fvalues, num_records, start_day, "1200", "CFS","INST-VAL");
    /* tss1->timeIntervalSeconds = 24 * 60 * 60; */
    /* tss1 = zstructTsNewIrregFloats(path, fvalues, num_records, itimes, MINUTE_GRANULARITY, start_day, "cfs", "Inst-Val"); */
    status = ztsStore(ifltab, tss1, 1);
    if (status != STATUS_OKAY){
      printf("Err Timeseries Store: %d\n", status);
      return status;
    }
    free(fvalues);
    zstructFree(tss1);
  }
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
    /* TODO: should I calculate it? */
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

    fscanf(fp, "ncols %d\n", &grid->_numberOfCellsX);
    fscanf(fp, "nrows %d\n", &grid->_numberOfCellsY);
    fscanf(fp, "xllcorner %f\n", &tempx);
    fscanf(fp, "yllcorner %f\n", &tempy);
    fscanf(fp, "cellsize %f\n", &grid->_cellSize);
    grid->_lowerLeftCellX = (int)(tempx / grid->_cellSize + 0.5);
    grid->_lowerLeftCellY = (int)(tempy / grid->_cellSize + 0.5);
    fscanf(fp, "NODATA_value %f\n", &grid->_nullValue);
    
    int x, y;

    /* these values coz I'm using them for precipitation weights */
    float min=100000000.0, max=-10000000.0, sum=0.0;
    int count=0;
    float *dat;
    data = (float*)calloc(grid->_numberOfCellsX * grid->_numberOfCellsY,
			  sizeof(float));
    for (y = 0; y < grid->_numberOfCellsY; y++) {
      for (x = 0; x < grid->_numberOfCellsX; x++) {
        dat = data + (grid->_numberOfCellsY - y - 1) * grid->_numberOfCellsX + x;
        fscanf(fp, "%f", dat);
	if (*dat != grid->_nullValue){
	  if (*dat > max) max = *dat;
	  if (*dat < min) min = *dat;
	  count += 1;
	  sum += *dat;
	}
      }
    }
    fclose(fp);
    grid->_data = data;

    float *maxp = malloc(sizeof(float));
    *maxp = max;
    float *minp = malloc(sizeof(float));
    *minp = min;
    float *mean = malloc(sizeof(float));
    *mean = sum / count;
    grid->_maxDataValue = maxp;
    grid->_minDataValue = minp;
    grid->_meanDataValue = mean;

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
      "\n"
      "    using grid command on timeseries file or vice versa will fail.\n"
      "Arguments:\n"
      "    dss_file.dss  : dss file to operate on.\n"
      "    input_files...: Input files to convert to dss format\n"
      "                  : filename should have fields A-F separated by _\n"
      "                    e.g. A_B_C_D_E_F.csv will become /A/B/C/D/E/F/\n");
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
  case 't':
    status = save_timeseries(ifltab, argv + 3, argc - 3);
    if (status != STATUS_OKAY) {
      printf("Error [code: %d]\n", status);
      return status;
    }
    break;
  default:
    printf("Unknown Command.\n\n");
    print_help(argv[0]);
    return 1;
  }
  zclose(ifltab);
  return 0;
}
