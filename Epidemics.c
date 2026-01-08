#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_X_COORD 100
#define MAX_Y_COORD 100
#define INFECTED_DURATION 5
#define IMMUNE_DURATION 6

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

FILE *open_file(char *file_name, const char *permission)
{
    FILE *f = NULL;
    if ((f = fopen(file_name, permission)) == NULL)
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
        (*people)[i].infection_count = (status_val == INFECTED) ? 1 : 0;
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

void update_health(Person *people, int N, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        people[i].future_status = people[i].current_status;
        if (people[i].current_status == INFECTED)
        {
            people[i].timer++;
            if (people[i].timer >= INFECTED_DURATION)
            {
                people[i].future_status = IMMUNE;
                people[i].timer = 0;
            }
        }
        else if (people[i].current_status == IMMUNE)
        {
            people[i].timer++;
            if (people[i].timer >= IMMUNE_DURATION)
            {
                people[i].future_status = SUSCEPTIBLE;
                people[i].timer = 0;
            }
        }
        else if (people[i].current_status == SUSCEPTIBLE)
        {
            for (int j = 0; j < N; j++)
            {
                if ((people[j].current_status == INFECTED) && (people[i].x == people[j].x) && (people[i].y == people[j].y))
                {
                    people[i].future_status = INFECTED;
                    people[i].timer = 0;
                    people[i].infection_count++;
                    break;
                }
            }
        }
    }
}

void run_serial(Person *people, int N, int w, int h)
{
    for (int t = 0; t < TOTAL_SIMULATION_TIME; t++)
    {
        for (int i = 0; i < N; i++)
        {
            update_location(&people[i], w, h);
        }
        update_health(people, N, 0, N);
        for (int i = 0; i < N; i++)
        {
            people[i].current_status = people[i].future_status;
        }
    }
}

void save_final_state(char *file_name, Person *people, int N)
{
    FILE *f = NULL;
    f = open_file(file_name, "w");
    for (int i = 0; i < N; i++)
    {
        fprintf(f, "%d %d %d %d\n", people[i].id, people[i].x, people[i].y, people[i].infection_count);
    }
    fclose(f);
}

int main(int argc, char *argv[])
{
    int N;
    int rank, size;
    int grid_width, grid_height;
    double start_p, end_p, start_s, end_s;
    FILE *debug_log = NULL;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <total_simulation_time> <input_file_name>\n", argv[0]);
        return 1;
    }
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    Person *people = NULL;
    TOTAL_SIMULATION_TIME = atoi(argv[1]);
    if (rank == 0)
    {
        FILE *data = NULL;
        data = open_file(argv[2], "r");
        read_data(data, &N, &grid_width, &grid_height, &people);
        fclose(data);
        printf("Simulation started with %d persons on a %dx%d grid\n", N, grid_width, grid_height);
#ifdef DEBUG
        debug_log = open_file("debug_log.txt", "w");
#endif
        Person *copy_serial = init_persons(N);
        memcpy(copy_serial, people, N * sizeof(Person));
        start_s = MPI_Wtime();
        run_serial(copy_serial, N, grid_width, grid_height);
        end_s = MPI_Wtime();
        save_final_state("f_serial_out.txt", copy_serial, N);
        printf("Serial time: %f s\n", end_s - start_s);
        free(copy_serial);
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
    int *recv_counts = NULL;
    int *position = NULL;
    if ((recv_counts = malloc(size * sizeof(int))) == NULL)
    {
        fprintf(stderr, "Error - allocate memory\n");
        exit(EXIT_FAILURE);
    }
    if ((position = malloc(size * sizeof(int))) == NULL)
    {
        fprintf(stderr, "Error - allocate memory\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++)
    {
        int r_start = i * chunk;
        int r_end = (i == size - 1) ? N : (i + 1) * chunk;
        recv_counts[i] = (r_end - r_start) * sizeof(Person);
        position[i] = r_start * sizeof(Person);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    start_p = MPI_Wtime();
    for (int t = 0; t < TOTAL_SIMULATION_TIME; t++)
    {
        for (int i = start; i < end; i++)
        {
            update_location(&people[i], grid_width, grid_height);
        }
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, people, recv_counts, position, MPI_BYTE, MPI_COMM_WORLD);
        update_health(people, N, start, end);
        for (int i = start; i < end; i++)
        {
            people[i].current_status = people[i].future_status;
        }
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, people, recv_counts, position, MPI_BYTE, MPI_COMM_WORLD);
#ifdef DEBUG
        if (rank == 0 && debug_log)
        {
            fprintf(debug_log, "\nDEBUG - step %d\n", t + 1);
            for (int i = 0; i < N; i++)
            {
                fprintf(debug_log, "id %d: (%d,%d) status: %d\n", people[i].id, people[i].x, people[i].y, people[i].current_status);
            }
            for (int i = 0; i < N; i++)
            {
                if (people[i].current_status == INFECTED && people[i].timer == 0 && t > 0)
                {
                    fprintf(debug_log, "person %d got infected at (%d,%d) in step %d\n", people[i].id, people[i].x, people[i].y, t + 1);
                }
            }
        }
#endif
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
    {
        end_p = MPI_Wtime();
        printf("Parallel time: %f s\n", end_p - start_p);
        printf("Speedup: %f\n", (end_s - start_s) / (end_p - start_p));
        save_final_state("f_parallel_out.txt", people, N);
        printf("Simulation completed\n");
        if (debug_log)
            fclose(debug_log);
    }
    free(people);
    free(recv_counts);
    free(position);
    MPI_Finalize();
    return 0;
}