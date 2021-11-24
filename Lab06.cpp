#include "mpi.h" 
#include <stdio.h> 
#include <stdlib.h>
#define MASTER 0
#define FROM_MASTER 1 
#define FROM_WORKER 2

double** alloc(int M, int col) {
    double** A = (double**)malloc(sizeof(double) * M * col);
    *A = (double*)malloc(sizeof(double) * M * col);
    for (int r = 0; r < M; r++)
    {
        A[r] = (*A + col * r);
    }
    return A;
}

double** fill_matrix(double** matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = rand() % 7;
        }
    }
    return matrix;
}

void print_matrix(double** matrix, int n) {

    printf("\n-----------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("\n");
        for (int j = 0; j < n; j++) {
            printf("%5.0f ", matrix[i][j]);
        }
    }

}

void dealloc(double** arr) {
    free(*arr);
    free(arr);
}

int main(int argc, char* argv[]) {
    int n = 1000;
    double start, end, time;
    int numtasks,
        taskid,
        numworkers,
        source,
        dest,
        rows,
        averow, extra, offset,
        i, j, k, rc = 0;
    double** A;
    double** B;
    double** C;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    if (numtasks < 2) {
        printf("Need at least two MPI tasks. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }
    A = alloc(n, n);
    B = alloc(n, n);
    C = alloc(n, n);
    numworkers = numtasks - 1;
    start = MPI_Wtime();
    if (taskid == MASTER) {
        // printf("mpi_mm has started with %d tasks.\n", numtasks);

        A = fill_matrix(A, n);
        B = fill_matrix(B, n);

        averow = n / numworkers;
        extra = n % numworkers;
        offset = 0;
        for (dest = 1; dest <= numworkers; dest++) {
            rows = (dest <= extra) ? averow + 1 : averow;
            // printf("Sending %d rows to task %d offset=% d\n",rows,dest,offset);
            MPI_Request master_send_req1;
            MPI_Request master_send_req2;
            MPI_Request master_send_req3;
            MPI_Request master_send_req4;
            MPI_Isend(&offset, 1, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD, &master_send_req1);
            MPI_Isend(&rows, 1, MPI_INT, dest, FROM_MASTER, MPI_COMM_WORLD, &master_send_req2);
            MPI_Isend(&A[offset][0], rows * n, MPI_DOUBLE, dest, FROM_MASTER, MPI_COMM_WORLD, &master_send_req3);
            MPI_Isend(*B, n * n, MPI_DOUBLE, dest, FROM_MASTER, MPI_COMM_WORLD, &master_send_req4);
            offset = offset + rows;
        }
        // Receive results from worker tasks 
        for (source = 1; source <= numworkers; source++) {
            MPI_Request master_recv_req1;
            MPI_Request master_recv_req2;
            MPI_Request master_recv_req3;
            MPI_Irecv(&offset, 1, MPI_INT, source, FROM_WORKER,
                MPI_COMM_WORLD, &master_recv_req1);
            MPI_Irecv(&rows, 1, MPI_INT, source, FROM_WORKER,
                MPI_COMM_WORLD, &master_recv_req2);
            MPI_Irecv(&C[offset][0], rows * n, MPI_DOUBLE, source,
                FROM_WORKER, MPI_COMM_WORLD, &master_recv_req3);
            MPI_Request master_recv_reqs[] = { master_recv_req1, master_recv_req2, master_recv_req3 };
            MPI_Waitall(3, master_recv_reqs, MPI_STATUSES_IGNORE);
            // printf("Received results from task %d\n", source);
        }
        end = MPI_Wtime();
        time = end - start;

        printf("\nTime = %.5f seconds\n", time);
        // Print results 
        // print_matrix(C, n);
    }
    //******** worker task *****************
    else { // if (taskid > MASTER) 
        MPI_Request worker_recv_req1;
        MPI_Request worker_recv_req2;
        MPI_Request worker_recv_req3;
        MPI_Request worker_recv_req4;
        MPI_Irecv(&offset, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD,
            &worker_recv_req1);
        MPI_Irecv(&rows, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &worker_recv_req2);
        MPI_Irecv(*A, -rows * n, MPI_DOUBLE, MASTER, FROM_MASTER, MPI_COMM_WORLD,
            &worker_recv_req3);
        MPI_Irecv(*B, n * n, MPI_DOUBLE, MASTER, FROM_MASTER, MPI_COMM_WORLD,
            &worker_recv_req4);
        MPI_Request worker_recv_reqs[] = { worker_recv_req1, worker_recv_req2, worker_recv_req3, worker_recv_req4 };
        MPI_Waitall(4, worker_recv_reqs, MPI_STATUSES_IGNORE);
        for (k = 0; k < n; k++)
            for (i = 0; i < rows; i++) {
                C[i][k] = 0.0;
                for (j = 0; j < n; j++)
                    C[i][k] = C[i][k] + A[i][j] * B[j][k];
            }
        MPI_Request worker_send_req1;
        MPI_Request worker_send_req2;
        MPI_Request worker_send_req3;
        MPI_Isend(&offset, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD, &worker_send_req1);
        MPI_Isend(&rows, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD, &worker_send_req2);
        MPI_Isend(*C, rows * n, MPI_DOUBLE, MASTER, FROM_WORKER, MPI_COMM_WORLD, &worker_send_req3);
    }
    MPI_Finalize();
}