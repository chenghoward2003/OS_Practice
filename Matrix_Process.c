#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include<sys/mman.h>

#define MAP_ANONYMOUS 0x20

int main() {
	int size[3];
	printf("Please enter the size of matrices A:mn, B:np\n");
	for(int i=0; i<3; i++) {
		if (scanf("%d", &size[i]) != 1 || size[i] <= 0) {
			fprintf(stderr, "Invalid input. Please enter positive integers.\n");
			exit(EXIT_FAILURE);
		}
	}

	// Allocate matrices
	int A[size[0]][size[1]];
	int B[size[1]][size[2]];
	
	// Allocate shared memory for result matrix
	int *C = mmap(NULL, sizeof(int) * size[0] * size[2], 
				  PROT_READ | PROT_WRITE, 
				  MAP_SHARED | MAP_ANONYMOUS, 
				  -1, 0);

	if (C == MAP_FAILED) {
		perror("mmap failed");
		return 1;
	}

	printf("Memory attached at %p\n", (void*)C);

	// Initialize result matrix to zero
	for(int i = 0; i < size[0] * size[2]; i++) {
		C[i] = 0;
	}

	printf("Enter elements of matrix A:\n");
	for (int i = 0; i < size[0]; i++) {
		for (int j = 0; j < size[1]; j++) {
			if (scanf("%d", &A[i][j]) != 1) {
				printf("Invalid input for matrix A\n");
				munmap(C, sizeof(int) * size[0] * size[2]);
				return 1;
			}
		}
	}

	printf("Enter elements of matrix B:\n");
	for (int i = 0; i < size[1]; i++) {
		for (int j = 0; j < size[2]; j++) {
			if (scanf("%d", &B[i][j]) != 1) {
				printf("Invalid input for matrix B\n");
				munmap(C, sizeof(int) * size[0] * size[2]);
				return 1;
			}
		}
	}

	// Create child processes for each row
	for (int i = 0; i < size[0]; i++) {
		pid_t pid = fork();

		if (pid < 0) {
			perror("fork failed");
			munmap(C, sizeof(int) * size[0] * size[2]);
			return 1;
		} else if (pid == 0) { // Child process
			printf("Child process (PID: %d) working on row %d\n", getpid(), i);
			
			// Calculate row i of the result matrix
			for (int j = 0; j < size[2]; j++) {
				for (int k = 0; k < size[1]; k++) {
					C[i * size[2] + j] += A[i][k] * B[k][j];
				}
			}
			exit(0);
		}
	}

	// Wait for all children to complete
	for (int i = 0; i < size[0]; i++) {
		wait(NULL);
	}

	// Print the result matrix
	printf("\nResult of Matrix_A * Matrix_B:\n");
	for (int i = 0; i < size[0]; i++) {
		for (int j = 0; j < size[2]; j++) {
			printf("%d  ", C[i * size[2] + j]);
		}
		printf("\n");
	}

	// Clean up
	munmap(C, sizeof(int) * size[0] * size[2]);
	return 0;
}