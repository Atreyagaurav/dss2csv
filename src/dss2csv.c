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

void read_range(char *rng, int *start, int *end) {
  int flag_start = 0;
  int flag_end = 0;
  int val_start = 0;
  int val_end = 0;

  int *val = &val_start;
  int *flag = &flag_start;

  int ind = 0;
  while (rng[ind] != 0) {
    switch (rng[ind]) {
    case '-':
      val = &val_end;
      flag = &flag_end;
      break;
    case '0' ... '9':
      *flag = 1;
      *val = *val * 10 + rng[ind] - '0';
      break;
    default:
      printf("Invalid character '%c' in %s", rng[ind], rng);
    }
    ind += 1;
  }
  if (flag_start)
    *start = val_start - 1;

  if (flag_end)
    *end = val_end;
  else if (rng[ind - 1] != '-') {
    /* there is no '-' character means it's a single number and not a range */
    *end = val_start;
  }
}

void list_paths(zStructCatalog *catalogue, int start, int end) {
  int i;
  for (i = start; i < end; i++) {
    printf("%5d: %s\n", i + 1, catalogue->pathnameList[i]);
  }
}

void dsspath2filename(char *outfilename, char *path, char *ext) {
  /* +1 here to skip the first '/' */
  strcpy(outfilename, path + 1);
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
  strcat(outfilename, ext);
}

int extract_timeseries(long long *ifltab, zStructCatalog *catalogue, int start,
               int end) {
  int i;
  zStructTimeSeries *tss1;
  char cdate[13], ctime[10];
  char outfilename[_MAX_PATH];
  int status;

  for (i = start; i < end; i++) {
    dsspath2filename(outfilename, catalogue->pathnameList[i], "csv");
    printf("%5d: %s\n", i + 1, outfilename);

    tss1 = zstructTsNew(catalogue->pathnameList[i]);
    status = ztsRetrieve(ifltab, tss1, 0, 1, 0);
    if (status != STATUS_OKAY)
      return status;
    FILE *fp = fopen(outfilename, "w");
    int t;

    fprintf(fp, "date,value\n");
    for (t = 0; t < tss1->numberValues; t++) {
      getDateAndTime(tss1->times[t], tss1->timeGranularitySeconds,
                     tss1->julianBaseDate, cdate, sizeof(cdate), ctime,
                     sizeof(ctime));
      if (zisMissingFloat(tss1->floatValues[t])) {
        fprintf(fp, "%s,\n", cdate);
      } else {
        fprintf(fp, "%s,%.2f\n", cdate, tss1->floatValues[t]);
      }
    }
    fclose(fp);
  }
  return STATUS_OKAY;
}

int extract_grid(long long *ifltab, zStructCatalog *catalogue, int start,
              int end) {
  zStructSpatialGrid *grid;
  float *data;
  int idx, i;
  char outfilename[_MAX_PATH];
  char outfilename_prj[_MAX_PATH];
  char outfilename_info[_MAX_PATH];
  int status = 0;

  for (i = start; i < end; i++) {
    dsspath2filename(outfilename, catalogue->pathnameList[i], "asc");

    printf("%5d: %s\n", i + 1, outfilename);

    FILE *fp = fopen(outfilename, "w");
    int t;

    grid = zstructSpatialGridNew(catalogue->pathnameList[i]);
    status = zspatialGridRetrieve(ifltab, grid, 1);

    if (status != STATUS_OKAY) {
      printf("Error retrieving grid: %d", status);
      return status;
    }
    
    data = (float *)grid->_data;
    fprintf(fp, "ncols %d\n", grid->_numberOfCellsX);
    fprintf(fp, "nrows %d\n", grid->_numberOfCellsY);
    fprintf(fp, "xllcorner %f\n",
	    grid->_lowerLeftCellX * grid->_cellSize);
    fprintf(fp, "yllcorner %f\n",
	    grid->_lowerLeftCellY * grid->_cellSize);
    fprintf(fp, "cellsize %f\n", grid->_cellSize);
    fprintf(fp, "NODATA_value %f\n", grid->_nullValue);
    int x, y;
    for (y = 0; y < grid->_numberOfCellsY; y++) {
      for (x = 0; x < grid->_numberOfCellsX; x++) {
        idx = (grid->_numberOfCellsY - y) * grid->_numberOfCellsX + x;
        fprintf(fp, "%f ", data[idx]);
	/* TODO: the dss made from csv2dss tool have weird numbers
	   printing here, although I can't see those in original asc
	   or HEC-DSSVue */
	
	/* if (*(data + idx) > 1.0 || *(data + idx) < 0.0){ */
	/*   printf("%f ", *(data + idx)); */
	/* } */
      }
      fprintf(fp, "\n");
    }
    fclose(fp);

    /* just a workaround to maintain the metadata for now, need to write into better format later */
    dsspath2filename(outfilename_info, catalogue->pathnameList[i], "dssinfo");
    fp = fopen(outfilename_info, "w");
    fprintf(fp, "pathname:%s\n", grid->pathname);
    fprintf(fp, "structVersion:%d\n", grid->_structVersion);
    fprintf(fp, "type:%d\n", grid->_type);
    fprintf(fp, "version:%d\n", grid->_version);
    fprintf(fp, "dataUnits:%s\n", grid->_dataUnits);
    fprintf(fp, "dataType:%d\n", grid->_dataType);
    fprintf(fp, "dataSource:%s\n", grid->_dataSource);
    fprintf(fp, "compressionMethod:%d\n", grid->_compressionMethod);
    fprintf(fp, "timeZoneID:%s\n", grid->_timeZoneID);
    fprintf(fp, "timeZoneRawOffset:%d\n", grid->_timeZoneRawOffset);
    fprintf(fp, "isInterval:%d\n", grid->_isInterval);
    fprintf(fp, "isTimeStamped:%d\n", grid->_isTimeStamped);
    fprintf(fp, "numberOfRanges:%d\n", grid->_numberOfRanges);
    fprintf(fp, "storageDataType:%d\n", grid->_storageDataType);
    fclose(fp);

    /* int xi; */
    /* for (xi=0; xi < grid->_numberOfRanges; xi++){ */
    /*   printf("%f, %d \n", ((float*)grid->_rangeLimitTable)[xi], grid->_numberEqualOrExceedingRangeLimit[xi]); */
    /* } */

    /* Warning: The given WKT format when inserted to prj file doesn't
       always work */
    if (strcmp(grid->_srsName, "WKT") == 0) {
      dsspath2filename(outfilename_prj, catalogue->pathnameList[i], "prj");
      fp = fopen(outfilename_prj, "w");
      fprintf(fp, "%s\n", grid->_srsDefinition);
      fclose(fp);
    }

    zstructFree(grid);
  }
  return status;
}

void print_help(char *name) {
  printf("\nUsage: %s command dss_file.dss [rng]\n", name);
  printf(
      "\nCommands:\n"
      "    help[h]       : print this help menu.\n"
      "    list[l]       : list the available paths.\n"
      "    timeseries[t] : extract the timeseries for paths.\n"
      "    grid[g]       : extract the grid for paths.\n"
      "\n"
      "    using grid command on timeseries file or vice versa will fail.\n"
      "Arguments:\n"
      "    dss_file.dss  : dss file to operate on.\n"
      "    rng           : Range of the chosen timeseries use in format M-N\n"
      "                    where M is start and N is the end number "
      "(inclusive)\n"
      "                    Omitting the start or end will default in "
      "available\n"
      "                    start or the end. (e.g. 1-5 or -5 or 5-)\n"
      "                    [Optional: Defaults to all available]\n");
}

int main(int argc, char *argv[]) {
  long long ifltab[250];
  char filename[_MAX_PATH];
  char pathname[393];
  char *tempname;
  zStructCatalog *catalogue;
  int exists;
  int idummy;
  int loop;
  int numberPaths;
  int numberBytes;
  int ihandle;
  int status, i, j;
  long lbyte;

  if (argc < 3) {
    if (argc < 2 || argv[1][0] != 'h') {
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

  catalogue = zstructCatalogNew();
  numberPaths = zcatalog(ifltab, "", catalogue, 1);
  if (numberPaths < 0)
    return numberPaths;
  printf("%d total pathnames found\n", catalogue->numberPathnames);

  int list = 0;
  int start = 0;
  int end = catalogue->numberPathnames;
  if (argc == 4) {
    read_range(argv[3], &start, &end);
  } else {
    end = catalogue->numberPathnames;
  }

  switch (argv[1][0]) {
  case 'h':
    print_help(argv[0]);
    return 0;
  case 'l':
    list_paths(catalogue, start, end);
    break;
  case 'g':
    status = extract_grid(ifltab, catalogue, start, end);
    if (status != STATUS_OKAY) {
      printf("Error [code: %d]\n", status);
      return status;
    }
    break;
  case 't':
    status = extract_timeseries(ifltab, catalogue, start, end);
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
  zstructFree(catalogue);
  zclose(ifltab);
  return 0;
}
