#include <mpi.h>
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

void update_location(Person *p, int width, int height)
{
    if (p->dir == NORTH)
    {
        p->y += p->amplitude;
        if (p->y >= height)
        {
            p->y = height - 1;
            p->dir = SOUTH;
        }
    }
    else if (p->dir == SOUTH)
    {
        p->y -= p->amplitude;
        if (p->y < 0)
        {
            p->y = 0;
            p->dir = NORTH;
        }
    }
    else if (p->dir == EAST)
    {
        p->x += p->amplitude;
        if (p->x >= width)
        {
            p->x = width - 1;
            p->dir = WEST;
        }
    }
    else if (p->dir == WEST)
    {
        p->x -= p->amplitude;
        if (p->x < 0)
        {
            p->x = 0;
            p->dir = EAST;
        }
    }
}

int main(int argc, char *argv[])
{
    int rank, size;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <total_simulation_time> <input_file_name>\n", argv[0]);
        return 1;
    }
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int N;
    int grid_width, grid_height;
    Person *people = NULL;
    TOTAL_SIMULATION_TIME = atoi(argv[1]);
    if (rank == 0)
    {
        FILE *data = NULL;
        data = open_file(argv[2]);
        read_data(data, &N, &grid_width, &grid_height, &people);
        fclose(data);
        printf("Simulation started with %d persons on a %dx%d grid\n", N, grid_width, grid_height);
    }
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&grid_width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&grid_height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0)
    {
        people = init_persons(N);
    }
    MPI_Bcast(people, N * sizeof(Person), MPI_BYTE, 0, MPI_COMM_WORLD);
    int chunk = N / size;
    int start = rank * chunk;
    int end = (rank == (size - 1)) ? N : start + chunk;
    for (int t = 0; t < TOTAL_SIMULATION_TIME; t++)
    {
        for (int i = start; i < end; i++)
        {
            update_location(&people[i], grid_width, grid_height);
        }
        for (int i = start; i < end; i++)
        {
            people[i].current_status = people[i].future_status;
        }
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, people, chunk * sizeof(Person), MPI_BYTE, MPI_COMM_WORLD);
#ifdef DEBUG
        if (rank == 0)
        {
            printf("\nDEBUG - step %d\n", t + 1);
            for (int i = 0; i < N; i++)
            {
                printf("id %d: (%d,%d) status: %d\n",
                       people[i].id, people[i].x, people[i].y, people[i].current_status);
            }
        }
#endif
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        printf("Simulation completed\n");
    }
    free(people);
    MPI_Finalize();
    return 0;
}