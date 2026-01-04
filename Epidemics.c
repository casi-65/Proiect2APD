//#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_X_COORD 100
#define MAX_Y_COORD 100
#define INFECTED_DURATION 100
#define IMMUNE_DURATION 100

int TOTAL_SIMULATION_TIME;

typedef enum
{
    INFECTED = 0,
    SUSCEPTIBLE = 1,
    IMMUNE = 2
} Status;

typedef enum
{
    NORTH = 0,
    SOUTH = 1,
    EAST = 2,
    WEST = 3
} Direction;

typedef struct
{
    int id;
    int x, y;
    Status current_status;
    Status future_status;
    Direction dir;
    int amplitude;
    int timer;
    int infection_count;
} Person;

Person *init_persons(int N)
{
    Person *people;
    if ((people = malloc(N * sizeof(Person))) == NULL)
    {
        fprintf(stderr, "Error - allocate memory\n");
        exit(EXIT_FAILURE);
    }
    return people;
}

FILE *open_file(char *file_name)
{
    FILE *f = NULL;
    if ((f = fopen(file_name, "r")) == NULL)
    {
        perror("Error - opening file");
        exit(EXIT_FAILURE);
    }
    return f;
}

void read_data(FILE *data, int *N, int *x, int *y, Person **people)
{
    if (fscanf(data, "%d %d", x, y) != 2)
    {
        fprintf(stderr, "Error - reading grid dimensions\n");
        exit(EXIT_FAILURE);
    }
    if (fscanf(data, "%d", N) != 1)
    {
        fprintf(stderr, "Error - reading the number of persons\n");
        exit(EXIT_FAILURE);
    }
    *people = init_persons(*N);
    for (int i = 0; i < *N; i++)
    {
        int status_val, dir_val;
        if (fscanf(data, "%d %d %d %d %d %d",
                   &(*people)[i].id,
                   &(*people)[i].x,
                   &(*people)[i].y,
                   &status_val,
                   &dir_val,
                   &(*people)[i].amplitude) != 6)
        {
            fprintf(stderr, "Error - reading person %d\n", i);
            exit(EXIT_FAILURE);
        }
        (*people)[i].current_status = (Status)status_val;
        (*people)[i].future_status = (Status)status_val;
        (*people)[i].dir = (Direction)dir_val;
        (*people)[i].timer = 0;
        (*people)[i].infection_count = 0;
    }
}

int main(int argc, char *argv[])
{
    int N;
    int grid_width, grid_height;
    FILE *data = NULL;
    Person *people = NULL;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <total_simulation_time> <input_file_name>\n", argv[0]);
        return 1;
    }
    TOTAL_SIMULATION_TIME = atoi(argv[1]);
    data = open_file(argv[2]);
    read_data(data, &N, &grid_width, &grid_height, &people);
    fclose(data);
    free(people);
    return 0;
}