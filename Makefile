build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out

MLFQ: 
	clear
	make all 
	./test_generator.out
	clear
	./process_generator.out processes.txt -sch 4 -q 10

RR: 
	clear
	make all 
	./test_generator.out
	clear
	./process_generator.out processes.txt -sch 3 -q 10