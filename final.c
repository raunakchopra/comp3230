/*
# Filename: psort.c
# Student name and No.: Chopra Raunak
# Development platform:  Ubutntu VS Code
# Remark: Completed all parts
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>

int checking(unsigned int *, long);
int compare(const void *, const void *);

// Global Variables

long size;            // size of the array
int total_threads;    // total number of worker threads
unsigned int *intarr; // main array

unsigned int *sample_arr;       // array to store samples from worker thread
long sample_arr_itr = 0;        // iterator variable to to iterate over sample_arr
unsigned int *pivot_values_arr; // array to store and handle the calculated pivot values
unsigned int *thread_partition; // array to store and handle partiton of each thread
unsigned int *final_arr;        // array to store final output
long final_arr_itr = 0;         // iterator variable to iterate over final_arr
unsigned int *thread_size;      // to store the elements size of each thread

// Synchronization Variables - Locks and associated conditional variables
pthread_mutex_t main_worker_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex lock to switch between threads
pthread_cond_t thread_scheduler = PTHREAD_COND_INITIALIZER;    // conditional variable to handle different phases of the program
int thread_id_check = 0;                                       // to handle the  worker threads and main thread
int thread_var = 0;                                            // to manage the program execution between phases

void *routine(void *id)
{
    unsigned int *thread_arr; // to store and manage thread subsequence

    int *thread_id = (int *)id;
    double range_thread_arr = (double)size / (double)total_threads;

    long subsequence_head = range_thread_arr * (*thread_id);
    long subsequence_tail = range_thread_arr * (*thread_id + 1) - 1;
    long size_thread_arr = subsequence_tail - subsequence_head + 1;

    thread_size[*thread_id] = size_thread_arr;

    thread_arr = (unsigned int *)malloc(size_thread_arr * sizeof(unsigned int));

    long itr = subsequence_head;

    pthread_mutex_lock(&main_worker_mutex);

    // Crerating and storing a subsequence from intarr

    long intarr_itr = 0;
    while (intarr_itr < size_thread_arr)
    {
        if (itr <= subsequence_tail)
        {
            thread_arr[intarr_itr++] = intarr[itr++];
        }
    }

    itr = subsequence_head;

    // Sorting the extracted subsequence

    qsort(thread_arr, size_thread_arr, sizeof(unsigned int), compare);

    // Rearranging intarr with sorted subsequences from each thread

    long thread_itr = 0;
    while (thread_itr < size_thread_arr)
    {
        if (itr <= subsequence_tail)
        {
            intarr[itr++] = thread_arr[thread_itr++];
        }
    }

    // Synchronization Processto put a thread on wait

    while (thread_id_check != *thread_id)
    {
        pthread_cond_wait(&thread_scheduler, &main_worker_mutex);
    }

    // Extracting p samples from each worker thread and stroing it in sample_arr

    for (int p = 0; p < total_threads; p++)
    {
        sample_arr[sample_arr_itr++] = thread_arr[p * size / (total_threads * total_threads)];
    }

    // End of Phase 1

    // Broadcasting signal
    pthread_cond_broadcast(&thread_scheduler);

    thread_id_check++;

    pthread_mutex_unlock(&main_worker_mutex);

    pthread_mutex_lock(&main_worker_mutex);

    // Start of Phase 3

    while (thread_id_check != total_threads + 1)
    {
        pthread_cond_wait(&thread_scheduler, &main_worker_mutex);
    }

    // Ensuring that Phase 1 is complete

    while (thread_var != *thread_id)
    {
        pthread_cond_wait(&thread_scheduler, &main_worker_mutex);
    }

    // Phase 3 variables

    int partition = 0;       // to store the number of partition
    long partition_size = 0; // to store the size of partion
    long partition_itr = 0;
    long level = 0;

    partition = *thread_id;

    // Identifying the size of each partition

    for (int i = 0; i < total_threads; i++)
    {
        long x;
        long y;

        if (i == 0)
        {
            x = 0;
            y = thread_size[i];
        }
        else if (i > 0)
        {
            x = level;
            y = level + thread_size[i];
        }

        for (long counter = x; counter < y; counter++)
        {
            if (partition == 0)
            {
                if (intarr[counter] <= pivot_values_arr[partition])
                {
                    partition_size++;
                }
            }
            else if (partition == total_threads - 1)
            {
                if (pivot_values_arr[partition - 1] < intarr[counter])
                {
                    partition_size++;
                }
            }
            else
            {
                if (intarr[counter] > pivot_values_arr[partition - 1] && intarr[counter] <= pivot_values_arr[partition])
                {
                    partition_size++;
                }
            }
        }
        level = level + thread_size[i];
    }

    // Creating a an array to store and handle partition

    thread_partition = (unsigned int *)malloc((partition_size) * sizeof(unsigned int));

    level = 0;

    // Adding Elements to the array created above

    for (int i = 0; i < total_threads; i++)
    {
        long x;
        long y;

        if (i == 0)
        {
            x = 0;
            y = thread_size[i];
        }
        else if (i > 0)
        {
            x = level;
            y = level + thread_size[i];
        }

        for (long counter = x; counter < y; counter++)
        {
            if (partition == 0)
            {
                if (intarr[counter] <= pivot_values_arr[partition])
                {
                    *(thread_partition + partition_itr++) = intarr[counter];
                }
            }
            else if (partition == total_threads - 1)
            {
                if (intarr[counter] > pivot_values_arr[partition - 1])
                {
                    *(thread_partition + partition_itr++) = intarr[counter];
                }
            }
            else
            {
                if (intarr[counter] > pivot_values_arr[partition - 1] && intarr[counter] <= pivot_values_arr[partition])
                {
                    *(thread_partition + partition_itr++) = intarr[counter];
                }
            }
        }
        level = level + thread_size[i];
    }

    // Sorting the partition
    qsort(thread_partition, partition_size, sizeof(unsigned int), compare);

    // Adding the partitions generated to a new array

    long i = 0;
    while (i < partition_size)
    {
        final_arr[final_arr_itr++] = thread_partition[i++];
    }

    pthread_cond_broadcast(&thread_scheduler);

    thread_var++;

    pthread_mutex_unlock(&main_worker_mutex);
}

int main(int argc, char **argv)
{
    long i, j;
    struct timeval start, end;

    if (argc < 2)
    {
        printf("Usage: seq_sort <number>\n");
        exit(0);
    }

    else if (argc == 3)
    {
        if (atol(argv[2]) <= 1)
        {
            printf("Number of threads can be lesser or equat to 1!");
            exit(0);
        }
        total_threads = atol(argv[2]);
    }

    else
    {
        total_threads = 4;
    }

    int thread_ids[total_threads];
    pthread_t threads[total_threads];

    int id = 0;
    while (id < total_threads)
    {
        thread_ids[id] = id;
        id++;
    }

    size = atol(argv[1]);
    intarr = (unsigned int *)malloc(size * sizeof(unsigned int));
    if (intarr == NULL)
    {
        perror("malloc");
        exit(0);
    }

    // set the random seed for generating a fixed random
    // sequence across different runs
    char *env = getenv("RANNUM"); // get the env variable
    if (!env)                     // if not exists
        srandom(3230);
    else
        srandom(atol(env));

    for (i = 0; i < size; i++)
    {
        intarr[i] = random();
    }

    // measure the start time
    gettimeofday(&start, NULL);

    // Allocating the memory for sample array, pivot_values_arr, and others

    sample_arr = (unsigned int *)malloc((total_threads * total_threads) * sizeof(unsigned int));
    final_arr = (unsigned int *)malloc(size * sizeof(unsigned int));
    thread_size = (unsigned int *)malloc(total_threads * sizeof(unsigned int));

    // Creating worker threads

    for (int thread_num = 0; thread_num < total_threads; thread_num++)
    {
        pthread_create(&threads[thread_num], NULL, routine, (void *)&thread_ids[thread_num]);
    }

    pthread_mutex_lock(&main_worker_mutex);

    while (thread_id_check != total_threads)
    {
        pthread_cond_wait(&thread_scheduler, &main_worker_mutex);
    }

    // Start of Phase 2

    qsort(sample_arr, total_threads * total_threads, sizeof(unsigned int), compare);

    // Identifying Pivot values and storing them in pivot_values_arr
    pivot_values_arr = (unsigned int *)malloc((total_threads - 1) * sizeof(unsigned int));
    for (int p = 0; p < total_threads - 1; p++)
    {
        pivot_values_arr[p] = sample_arr[(p + 1) * total_threads + (total_threads / 2) - 1];
    }

    // End of Phase 2

    thread_id_check++;

    pthread_cond_broadcast(&thread_scheduler);

    pthread_mutex_unlock(&main_worker_mutex);

    for (int i = 0; i < total_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // // measure the end time
    gettimeofday(&end, NULL);

    if (!checking(final_arr, size))
    {
        printf("The array is not in sorted order!!\n");
    }

    printf("Total elapsed time: %.4f s\n", (end.tv_sec - start.tv_sec) * 1.0 + (end.tv_usec - start.tv_usec) / 1000000.0);
    return 0;
}

int compare(const void *a, const void *b)
{
    return (*(unsigned int *)a > *(unsigned int *)b) ? 1 : ((*(unsigned int *)a == *(unsigned int *)b) ? 0 : -1);
}

int checking(unsigned int *list, long size)
{
    long i;
    printf("First : %d\n", list[0]);
    printf("At 25%%: %d\n", list[size / 4]);
    printf("At 50%%: %d\n", list[size / 2]);
    printf("At 75%%: %d\n", list[3 * size / 4]);
    printf("Last  : %d\n", list[size - 1]);
    for (i = 0; i < size - 1; i++)
    {
        if (list[i] > list[i + 1])
        {
            return 0;
        }
    }
    return 1;
}
