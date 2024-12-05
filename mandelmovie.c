// Name: Connor Albrecht
// File name: mandelmovie.c
//CPE 2600 121

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include "jpegrw.h"

#define TOTAL_FRAMES 50

typedef struct {
    int start_frame;
    int end_frame;
    double xcenter;
    double ycenter;
    double scale;
} thread_data_t;

void generate_frame(int frame_number, double xcenter, double ycenter, double scale) {
    char command[256];
    char output_file[64];

    snprintf(output_file, sizeof(output_file), "mandel%d.jpg", frame_number);

    snprintf(command, sizeof(command),
             "./mandel -x %lf -y %lf -s %lf -o %s",
             xcenter, ycenter, scale, output_file);

    system(command);
}

void *thread_function(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    double scale = data->scale;

    for (int frame = data->start_frame; frame <= data->end_frame; frame++) {
        scale *= 0.95;  // Apply zooming logic
        generate_frame(frame, data->xcenter, data->ycenter, scale);
    }

    return NULL;
}

void process_function(int process_id, int num_processes, int num_threads, double xcenter, double ycenter, double scale) {
    int frames_per_process = TOTAL_FRAMES / num_processes;
    int start_frame = process_id * frames_per_process;
    int end_frame = (process_id == num_processes - 1) ? TOTAL_FRAMES - 1 : (process_id + 1) * frames_per_process - 1;

    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    int frames_per_thread = (end_frame - start_frame + 1) / num_threads;

    for (int t = 0; t < num_threads; t++) {
        thread_data[t].start_frame = start_frame + t * frames_per_thread;
        thread_data[t].end_frame = (t == num_threads - 1) ? end_frame : thread_data[t].start_frame + frames_per_thread - 1;
        thread_data[t].xcenter = xcenter;
        thread_data[t].ycenter = ycenter;
        thread_data[t].scale = scale;

        pthread_create(&threads[t], NULL, thread_function, &thread_data[t]);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}

int main(int argc, char *argv[]) {
    int num_processes = 1;
    int num_threads = 1;
    int opt;

    while ((opt = getopt(argc, argv, "p:t:h")) != -1) {
        switch (opt) {
        case 'p':
            num_processes = atoi(optarg);
            if (num_processes < 1) {
                fprintf(stderr, "Number of processes must be at least 1.\n");
                return 1;
            }
            break;
        case 't':
            num_threads = atoi(optarg);
            if (num_threads < 1) {
                fprintf(stderr, "Number of threads must be at least 1.\n");
                return 1;
            }
            break;
        case 'h':
        default:
            printf("Usage: %s -p <number_of_processes> -t <number_of_threads>\n", argv[0]);
            return 0;
        }
    }

    double xcenter = -0.5, ycenter = 0.0, scale = 4.0;

    for (int process_id = 0; process_id < num_processes; process_id++) {
        if (fork() == 0) {
            // Child process handles a subset of frames
            process_function(process_id, num_processes, num_threads, xcenter, ycenter, scale);
            exit(0);
        }
    }

    // Parent process waits for all child processes
    while (wait(NULL) > 0);

    printf("All frames generated. Use 'ffmpeg -i mandel%%d.jpg mandel.mpg' to create the movie.\n");

    return 0;
}
