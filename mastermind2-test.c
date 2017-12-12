//***********************************************************
//File Name: mastermind-test.c
//Author Name: Dominic Rollo
//Assignment: project 1
//
//Description: This program test the reading and writing calls
//		the the /dev/mm and /dev/mm_ctl devices
//
//**********************************************************

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <string.h>

#define mm_path "/dev/mm"
#define mm_ctl_path "/dev/mm_ctl"

int main(void) {
	int test=11;
	int passed_test =0;

	int mm_fd = open(mm_path, O_RDWR);
	if(mm_fd==-1){
		fprintf(stderr, "Cannot open %s. Try again later.", mm_path);
		return 1;
	}
	int mm_ctl_fd= open(mm_ctl_path, O_WRONLY);
	if(mm_ctl_fd==-1){
		fprintf(stderr, "Cannot open %s. Try again later.", mm_ctl_path);
		return 1;
	}

	printf("Test 1: does /dev/mm gives return \"No game yet\"\n");
	char testbuff[80];
	char bad_buff[400];
	memset(testbuff, '\0', 80);
	if(read(mm_fd, testbuff, 11)==-1)
		goto read_err;
	printf("mm: %s\n", testbuff);
	if (strcmp("No game yet", testbuff)==0){
		passed_test++;
		printf("Test 1 passed.\n\n" );
	}
	else
		printf("Test 1 failed\n\n");

	printf("Test 2: User entered \"sTart\"\n");
	if(write(mm_ctl_fd, "sTart", 5)==-1){
		passed_test++;
		printf("Test 2 passed\n\n");
	}
	else
		printf("Test 2 failed\n\n");
	close(mm_fd);
	mm_fd = open(mm_path, O_RDWR);
	if(mm_fd==-1){
		fprintf(stderr, "Cannot open %s. Try again later.", mm_path);
		return 1;
	}

	printf("Test 3: User entered \"start\\n\"\n");
	if(write(mm_ctl_fd, "start\n", 6)==-1){
		passed_test++;
		printf("Test 3 passed\n\n");
	}
	else
		printf("Test 3 failed\n\n");

	printf("Test 4: User entered \"start\"\n");
	if(write(mm_ctl_fd, "start", 5)==-1)
		goto write_err;
	memset(testbuff, '\0', 80);
	if(read(mm_fd, testbuff, 13)==-1)
		goto read_err;
	printf("mm: %s\n", testbuff);
	if (strcmp("Starting game", testbuff)==0){
		passed_test++;
		printf("Test 4 passed.\n\n");
	}
	else{
		printf("Test 4 failed.\n\n" );	
	}
	close(mm_fd);
	mm_fd = open(mm_path, O_RDWR);
	if(mm_fd==-1){
		fprintf(stderr, "Cannot open %s. Try again later.", mm_path);
		return 1;
	}

	printf("Test 5: User enters 000012\n");
	if(write(mm_fd, "000012", 4)==-1)
		goto write_err;
	memset(testbuff, '\0', 80);
	if(read(mm_fd, testbuff, 39)==-1)
		goto read_err;
	printf("mm: %s\n", testbuff);
	if (strcmp("Guess 1: 2 black peg(s), 0 white peg(s)", testbuff)==0){
		passed_test++;
		printf("Test 5 passed.\n\n");
	}
	else{
		printf("Test 5 failed.\n\n" );	
	}
	close(mm_fd);
	mm_fd = open(mm_path, O_RDWR);
	if(mm_fd==-1){
		fprintf(stderr, "Cannot open %s. Try again later.", mm_path);
		return 1;
	}

	printf("Test 6: User enters \"I win\"\n");
	if(write(mm_fd, "I win", 5)==-1){
		passed_test++;
		printf("Test 6 passed\n\n");
	}
	else
		printf("Test 6 failed\n\n");

	printf("Test 7: requesting more than 80 bytes from /dev/mm\n");
	if(read(mm_fd, bad_buff, 400)==80){
		passed_test++;
		printf("Test 7 passed\n\n");
	}
	else
		printf("Test 7 fail\n\n");

	printf("Test 8: mmap for the guesses 0000: B2 W0, 1200: B0 W4, 0012: B4 W0\n");
	char * start = NULL;
    start = mmap(start, PAGE_SIZE, PROT_READ, MAP_PRIVATE, mm_fd, 0);
    if(strcmp("000020120004001240", start)){
    	passed_test++;
    	printf("Test 8 passed\n\n");
    }
    else
    	printf("Test 8 failed\n\n");

    printf("Test 9: User enters \"quit\"\n");
	if(write(mm_ctl_fd, "quit", 4)==-1)
		goto write_err;
	memset(testbuff, '\0', 80);
	if(read(mm_fd, testbuff, 29)==-1)
		goto read_err;
	printf("mm: %s\n", testbuff);
	if(strcmp("Game over. The code was 0012.", testbuff)){
		passed_test++;
		printf("Test 9 passed\n\n");
	}
	else
		printf("Test 9 fail\n\n");
	close(mm_fd);
	mm_fd = open(mm_path, O_RDWR);
	if(mm_fd==-1){
		fprintf(stderr, "Cannot open %s. Try again later.", mm_path);
		return 1;
	}

	printf("Test 10-11: User restarts game (10) check user_buff is empty and (11) the game over is displayes correctly\n");
	start = NULL;
    start = mmap(start, PAGE_SIZE, PROT_READ, MAP_PRIVATE, mm_fd, 0);
    if(strcmp("000020120004001240", start)!=0){
    	passed_test++;
    	printf("Test 10 passed\n\n");
    }
    else
    	printf("Test 10 failed\n\n");
    if(write(mm_ctl_fd, "start", 5)==-1)
		goto write_err;
    if(read(mm_fd, testbuff, 13)==-1)
		goto read_err;
	printf("mm: %s\n", testbuff);
	if(strcmp("Starting game", testbuff)==0){
		passed_test++;
		printf("Test 11 passed\n\n");
	}
	else
		printf("Test 11 fail\n\n");
	close(mm_fd);
	close(mm_ctl_fd);

	printf("Passed %i of %i tests\n", passed_test, test);

	return 0;

	write_err:
		printf("There was an error writing to device.\n");
		return 1;
	read_err:
		printf("There was an error reading from device.\n");
		return 1;
}
