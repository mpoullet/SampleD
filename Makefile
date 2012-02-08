all:
	gcc -Wall -o build/SampleD src/SampleD.c
	gcc -Wall -o build/report  src/report.c

clean:
	rm -rf build
