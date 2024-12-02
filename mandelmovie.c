// Name: Connor Albrecht
// File name: mandelmovie.c
//CPE 2600 121

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "jpegrw.h"

#define TOTAL_FRAMES 50

void generate_frame(int frame_number, double xcenter, double ycenter, double scale) {
    char command[256];
    char output_file[64];

    // Generate output file name
    snprintf(output_file, sizeof(output_file), "mandel%d.jpg", frame_number);

    // Construct the command for `mandel`
    snprintf(command, sizeof(command),
             "./mandel -x %lf -y %lf -s %lf -o %s",
             xcenter, ycenter, scale, output_file);

    // Execute the command
    system(command);
}

int main(int argc, char *argv[]) {
    int num_processes = 1;
    int opt;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
        case 'p':
            num_processes = atoi(optarg);
            if (num_processes < 1) {
                fprintf(stderr, "Number of processes must be at least 1.\n");
                return 1;
            }
            break;
        case 'h':
        default:
            printf("Usage: %s -p <number_of_processes>\n", argv[0]);
            return 0;
        }
    }

    double xcenter = -0.5, ycenter = 0.0, scale = 4.0;

    for (int frame = 0; frame < TOTAL_FRAMES; frame++) {
        // Adjust parameters for zooming
        scale *= 0.95;

        if (fork() == 0) {
            // Child process generates a frame
            generate_frame(frame, xcenter, ycenter, scale);
            exit(0);
        }

        // Parent process waits if the maximum number of processes is reached
        if ((frame + 1) % num_processes == 0) {
            for (int i = 0; i < num_processes; i++) {
                wait(NULL);
            }
        }
    }

    // Wait for any remaining child processes
    while (wait(NULL) > 0);

    printf("All frames generated. Use 'ffmpeg -i mandel%%d.jpg mandel.mpg' to create the movie.\n");

    return 0;
}
