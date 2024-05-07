README
Project 1
Authors: Sihua Zhou (sz583), Yihang Sun (ys815)
-----------------------------------------------

Makefile
---------------------------------------------------------------------------------
Makefile is included
For command are down below:

For correctness testing:
make err 
./err #
Note: # means case number totally 3 cases, so it is from [1, 3] inclusive

For proformance testing
make memgrind
./memgrind
Note: the time of each cases running 50 times of itself is display in the terminal/console.

For unit testing (edge cases)
make test
./test
Note: every case along with itself result will be displayed in the terminal/console

For compile all together 
make all
Note: then run these ./memgrind /test ./err # as needed

make clean is avaliable for cleaning the source folder
Note: following file will be cleaned up (rm -f *.o err memgrind mymalloc test)


TEST PLAN and EXECUTION
---------------------------------------------------------------------------------
#1 Correctness Testing
File: err.c
Purpose: To check whether the program is functioning correctly
We have used err.c (the file given by the professor) to test the correctness 
of the mymalloc() and myfree() function.

#1 By given false ptr, it returns warning message (check)
#2 By doing pointer arithematic to intended to make flase pointer, it returns warning message (check)
#3 By trying double freeing, it returnings warning messages (check) 

Result: same correct result as the result in the demo in lecture.
---------------------------------------------------------------------------------
#2 Proformance Testing 
File: memgrind.c
Purpose: To check the performance of the code under different stress condition.
We do totally 5 memgrind tests to check its perfomance

memgrind test #1:
1. malloc() and immediately free() a 1-byte chunk, 120 times.
Time taken to count to 10^5 is : 90 micro seconds
Average time: 1.8 micro seconds

memgrind test #2:
2. Use malloc() to get 120 1-byte chunks, storing the pointers in an array, then use free() to deallocate the chunks.
Time taken to count to 10^5 is : 1614 micro seconds
Average: 32.28 micro seconds

memgrind test #3:
3. Randomly choose between
        •Allocating a 1-byte chunk and storing the pointer in an array
        •Deallocating one of the chunks in the array (if any)
    Repeat until you have called malloc() 120 times, then free all remaining allocated chunks.
Time taken to count to 10^5 is : 1643 micro seconds
Average: 32.86 micro seconds

memgrind test #4.1:
4.1 We allocated a 4000-byte chunk and freed it, then we use a ptr array to store 120 of allocations then free everything, 
after it the process is repeat again with store 120 of allocations and free them all.
Time taken to count to 10^5 is : 3043 micro seconds
Average: 60.86 micro seconds

memgrind test #4.2:
4.2 For this test we did allocating two big chunk with half the size of the total size of the memory (including the metadata size), 
then free both chunks and 
Time taken to count to 10^5 is : 1490 micro seconds
Average:  29.8 micro seconds
---------------------------------------------------------------------------------
#3 Unit Tests for Some Edge Cases
Purpose: To test some of the edge cases with the usage of the unit testing 
By giving all the result to determine the case is pass or not, with total 3 cases.

Sample Result After EXECUTION along with the result:

Unit Tests For Some Edge Cases
Test 1 Allocates 0 bytes
case passed

Test 2 Allocates 0 bytes
case passed

Test 3 Trying to free when no allocations/pointer not belong to the memory. With different pointers involved.
free: attempt to free non-block ptr (test.c:31)
free: attempt to free non-block ptr (test.c:32)
