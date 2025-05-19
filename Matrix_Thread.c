#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct {
    int row;           // Row of matrix A to compute
    int m, n, p;       // Dimensions: A -> m*n, B -> n*p
    int **A, **B, **C; // Matrices A, B, and C
} ThreadData;

void printMatrix(int rows, int cols, int** matrix) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int** create_matrix(int rows, int cols) {
    int **matrix = malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        matrix[i] = malloc(cols * sizeof(int));
    }
    return matrix;
}

void free_matrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void* compute_row(void* arg){
    ThreadData* data = (ThreadData*) arg;
    int row = data->row;
    int n = data->n;
    int p = data->p;
    int** A = data->A;
    int** B = data->B;
    int** C = data->C;

    for (int i = 0; i < p; i++){
        C[row][i] = 0;
        for (int j = 0; j < n; j++){
            C[row][i] += A[row][j] * B[j][i];
        }
    }
    pthread_exit(NULL);
}

int matrix_multiplication(int rowA, int mid, int colB, int** A, int** B, int** C){
    pthread_t* threads = malloc(rowA * sizeof(pthread_t));
    ThreadData* thread_data = malloc(rowA * sizeof(ThreadData));
    
    if (!threads || !thread_data) {
        fprintf(stderr, "Memory allocation failed\n");
        free(threads);
        free(thread_data);
        return -1;
    }

    for (int i = 0; i < rowA; i++){
        thread_data[i].row = i;
        thread_data[i].m = rowA;
        thread_data[i].n = mid;
        thread_data[i].p = colB;
        thread_data[i].A = A;
        thread_data[i].B = B;
        thread_data[i].C = C;
        
        int ret = pthread_create(&threads[i], NULL, compute_row, &thread_data[i]);
        if (ret != 0) {
            fprintf(stderr, "Error creating thread %d: %s\n", i, strerror(ret));
            // Clean up already created threads
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(thread_data);
            return -1;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < rowA; i++) {
        int ret = pthread_join(threads[i], NULL);
        if (ret != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i, strerror(ret));
        }
    }

    free(threads);
    free(thread_data);
    return 0;
}

int main(){
    int rowA = 0, colA = 0, rowB = 0, colB = 0;

    while(1) {
        printf("Enter row and columns for Matrix A: ");
        if (scanf("%d %d", &rowA, &colA) != 2 || rowA <= 0 || colA <= 0) {
            printf("Invalid input for Matrix A dimensions. Please enter positive integers.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }

        printf("Enter row and columns for Matrix B: ");
        if (scanf("%d %d", &rowB, &colB) != 2 || rowB <= 0 || colB <= 0) {
            printf("Invalid input for Matrix B dimensions. Please enter positive integers.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }

        if(colA != rowB){
            printf("A and B cannot be multiplied (columns of A must match rows of B), please try again.\n");
        } else {
            break;
        }
    }

    int** matrixA = create_matrix(rowA, colA);
    printf("Enter matrix A (%d x %d):\n", rowA, colA);
    for (int i = 0; i < rowA; i++){
        for (int j = 0; j < colA; j++){
            scanf("%d", &matrixA[i][j]);
        }
    }

    int** matrixB = create_matrix(rowB, colB);
    printf("Enter matrix B (%d x %d):\n", rowB, colB);
    for (int i = 0; i < rowB; i++) {
        for (int j = 0; j < colB; j++) {
            scanf("%d", &matrixB[i][j]);
        }
    }

    int **matrixC = create_matrix(rowA, colB);
    if (matrix_multiplication(rowA, colA, colB, matrixA, matrixB, matrixC) != 0) {
        printf("Error during matrix multiplication\n");
        free_matrix(matrixA, rowA);
        free_matrix(matrixB, rowB);
        free_matrix(matrixC, rowA);
        return 1;
    }
    printf("Result of Matrix A * Matrix B:\n");
    printMatrix(rowA, colB, matrixC);

    free_matrix(matrixA, rowA);
    free_matrix(matrixB, rowB);
    free_matrix(matrixC, rowA);

    return 0;
}