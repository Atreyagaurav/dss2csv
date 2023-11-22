DBG=

all: dss2csv csv2dss

dss2csv: dss2csv.c libhecdss.so
	gcc $(DBG) libhecdss.so dss2csv.c -o dss2csv

csv2dss: csv2dss.c libhecdss.so
	gcc $(DBG) libhecdss.so csv2dss.c -o csv2dss

clean:
	rm dss2csv csv2dss
