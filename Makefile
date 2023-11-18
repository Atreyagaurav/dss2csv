all: 
	gcc dss2csv.c heclib.a -o dss2csv

clean:
	rm dss2csv
