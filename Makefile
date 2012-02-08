all:
	gcc -Wall -o SampleD SampleD.c
	gcc -Wall -o report report.c

clean:
	rm -rf SampleD
	rm -rf report
