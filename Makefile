all: 
	gcc libhecdss.so dss2csv.c -o dss2csv

clean:
	rm dss2csv
