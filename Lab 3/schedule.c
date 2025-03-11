// 22CS10072
// Shivam Choudhury

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

int n; // Number of processes

// Structure for the Process
typedef struct
{
    int pid, arrival;
    int *bursts;
    int num_bursts;
    int current_index; // Tracks the current burst index
    int completion_time;
    int turnaround_time;
    int wait_time;
    int sum_bursts;
    int first_arrival;
    int *temp_bursts;
} Process;

Process *p; // Array of processes

// Function to create processes
Process *create_processes(int n)
{
    Process *p = (Process *)malloc(n * sizeof(Process));
    for (int i = 0; i < n; i++)
    {
        p[i].bursts = (int *)malloc(100 * sizeof(int));
        p[i].temp_bursts = (int *)malloc(100 * sizeof(int));
        p[i].num_bursts = 0;
        p[i].current_index = 0;
        p[i].completion_time = 0;
        p[i].turnaround_time = 0;
        p[i].wait_time = 0;
    }
    return p;
}

// Priority queue structure for the event queue
typedef struct
{
    int *data;
    int size;
    int capacity;
} priority_queue;

// Queue structure for the ready queue
typedef struct
{
    int *data;
    int front, rear, size, capacity;
} Queue;

// Function to create a new Min-Heap
priority_queue *createMinHeap(int capacity)
{
    priority_queue *heap = (priority_queue *)malloc(sizeof(priority_queue));
    heap->data = (int *)malloc(capacity * sizeof(int));
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

// Function to swap two integers
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Function to return the maximum of two integers
int max(int a, int b)
{
    return a > b ? a : b;
}

// Comparison function for the priority queue
bool compare(int a, int b)
{
    if (p[a].arrival != p[b].arrival)
        return p[a].arrival < p[b].arrival;
    return a < b;
}

// Function to heapify up
void heapifyUp(priority_queue *heap, int index)
{
    while (index != 0 && compare(heap->data[index], heap->data[(index - 1) / 2]))
    {
        swap(&heap->data[index], &heap->data[(index - 1) / 2]);
        index = (index - 1) / 2;
    }
}

// Function to heapify down
void heapifyDown(priority_queue *heap, int index)
{
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && compare(heap->data[left], heap->data[smallest]))
        smallest = left;
    if (right < heap->size && compare(heap->data[right], heap->data[smallest]))
        smallest = right;

    if (smallest != index)
    {
        swap(&heap->data[index], &heap->data[smallest]);
        heapifyDown(heap, smallest);
    }
}

// Function to insert a value into the Min-Heap
void insert(priority_queue *heap, int value)
{
    if (heap->size == heap->capacity)
    {
        heap->capacity *= 2;
        heap->data = (int *)realloc(heap->data, heap->capacity * sizeof(int));
    }
    heap->data[heap->size++] = value;
    heapifyUp(heap, heap->size - 1);
}

// Function to return the minimum value from the Min-Heap
int Min_element(priority_queue *heap)
{
    if (heap->size == 0)
        return INT_MAX;
    return heap->data[0];
}

// Function to extract the minimum value from the Min-Heap
int extractMin(priority_queue *heap)
{
    if (heap->size == 0)
        return INT_MAX;

    int root = heap->data[0];
    heap->data[0] = heap->data[--heap->size];
    heapifyDown(heap, 0);

    return root;
}

// Function to free the Min-Heap
void freeMinHeap(priority_queue *heap)
{
    free(heap->data);
    free(heap);
}

// Function to create a new Queue
Queue *createQueue(int capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->data = (int *)malloc(capacity * sizeof(int));
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->capacity = capacity;
    return queue;
}

// Function to enqueue an element
void enqueue(Queue *queue, int value)
{
    if (queue->size == queue->capacity)
    {
        queue->capacity *= 2;
        queue->data = (int *)realloc(queue->data, queue->capacity * sizeof(int));
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->data[queue->rear] = value;
    queue->size++;
}

// Function to dequeue an element from the Queue
int dequeue(Queue *queue)
{
    if (queue->size == 0)
        return INT_MIN;

    int value = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return value;
}

// Function to free the Queue
void freeQueue(Queue *queue)
{
    int top = dequeue(queue);
    if (top == INT_MIN)
    {
        return;
    }
    while (top != INT_MIN)
    {
        top = dequeue(queue);
    }
}

// Scheduling function
void scheduling(int q)
{
    int time = 0, count_remain = n;
    priority_queue *event_queue = createMinHeap(n);
    Queue *ready_queue = createQueue(n);

    if (q == INT_MAX)
        printf("**** FCFS Scheduling ****\n");
    else
        printf("**** RR Scheduling with q = %d ****\n", q);

    int idle_time = 0;

    // Initialize priority queue with process arrivals
    for (int i = 0; i < n; i++)
        insert(event_queue, i);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < p[i].num_bursts; j++)
        {
            p[i].temp_bursts[j] = p[i].bursts[j];
        }
    }

    while (count_remain)
    {
        // Extract the next event from the event queue
        if (ready_queue->size == 0)
        {
            if (event_queue->size > 0)
            {
#ifdef VERBOSE
                if (time == 0)
                {
                    printf("%-10d: Starting\n", time);
                }
                else
                {
                    printf("%-10d: CPU goes idle\n", time);
                }
#endif
                int next_process = extractMin(event_queue);
                if (time < p[next_process].arrival)
                {
                    idle_time += p[next_process].arrival - time;
                    time = p[next_process].arrival;
                }

#ifdef VERBOSE
                if (p[next_process].current_index == 0)
                    printf("%-10d: Process %d joins ready queue upon arrival\n", time, next_process + 1);
                else
                    printf("%-10d: Process %d joins ready queue after IO completion\n", time, next_process + 1);
#endif
                enqueue(ready_queue, next_process);
            }
        }

        if (ready_queue->size > 0)
        {
            int current = dequeue(ready_queue);
            int burst_time = p[current].temp_bursts[p[current].current_index];

            if (burst_time <= q)
            {
#ifdef VERBOSE
                printf("%-10d: Process %d is scheduled to run for time %d\n", time, current + 1, burst_time);
#endif
                while (event_queue->size > 0 && p[Min_element(event_queue)].arrival <= time + burst_time)
                {
                    int next_process = extractMin(event_queue);
#ifdef VERBOSE
                    if (p[next_process].current_index == 0)
                        printf("%-10d: Process %d joins ready queue upon arrival\n", p[next_process].arrival, next_process + 1);
                    else
                        printf("%-10d: Process %d joins ready queue after IO completion\n", p[next_process].arrival, next_process + 1);
#endif
                    enqueue(ready_queue, next_process);
                }
                time += burst_time;
                p[current].current_index++;
                if (p[current].current_index == p[current].num_bursts)
                {
                    // Process completed
                    p[current].completion_time = time;
                    p[current].turnaround_time = p[current].completion_time - p[current].first_arrival;
                    p[current].wait_time = p[current].turnaround_time - p[current].sum_bursts;
                    int temp = (p[current].turnaround_time * 100.0) / p[current].sum_bursts + 0.5;

#ifdef VERBOSE
                    printf("%-10d: Process%7d exits. Turnaround time =%5d (%d%%), Wait time = %d\n", time, current + 1, p[current].turnaround_time, temp, p[current].wait_time);
#else
                    printf("%-10d: Process%7d exits. Turnaround time =%5d (%d%%), Wait time = %d\n", time, current + 1, p[current].turnaround_time, temp, p[current].wait_time);
#endif
                    count_remain--;
                }
                else
                {
                    // Process transitions to I/O burst

                    p[current].arrival = time + p[current].temp_bursts[p[current].current_index++];
                    insert(event_queue, current);
                }
            }
            else
            {
                // Process is preempted
#ifdef VERBOSE
                printf("%-10d: Process %d is scheduled to run for time %d\n", time, current + 1, q);
#endif
                while (event_queue->size > 0 && p[Min_element(event_queue)].arrival <= time + q)
                {
                    int next_process = extractMin(event_queue);
#ifdef VERBOSE
                    if (p[next_process].current_index == 0)
                        printf("%-10d: Process %d joins ready queue upon arrival\n", p[next_process].arrival, next_process + 1);
                    else
                        printf("%-10d: Process %d joins ready queue after IO completion\n", p[next_process].arrival, next_process + 1);
#endif
                    enqueue(ready_queue, next_process);
                }
#ifdef VERBOSE
                printf("%-10d: Process %d joins ready queue after timeout\n", time + q, current + 1);
#endif
                time += q;
                p[current].temp_bursts[p[current].current_index] -= q;
                enqueue(ready_queue, current);
            }
        }
    }
#ifdef VERBOSE
    printf("%-10d: CPU goes idle\n", time);
#endif

    int average_wait_time = 0;
    int total_turnaround_time = 0;

    for (int i = 0; i < n; i++)
    {
        average_wait_time += p[i].wait_time;
        total_turnaround_time = max(total_turnaround_time, p[i].completion_time);
    }

    printf("\nAverage wait time = %.2f\n", (float)average_wait_time / n);
    printf("Total turnaround time = %d\n", total_turnaround_time);
    printf("CPU idle time = %d\n", idle_time);
    printf("CPU utilization = %.2f%%\n", (float)(total_turnaround_time - idle_time) * 100 / total_turnaround_time);

    freeMinHeap(event_queue);
    freeQueue(ready_queue);

    for (int i = 0; i < n; i++)
    {
        p[i].current_index = 0;
        p[i].completion_time = 0;
        p[i].turnaround_time = 0;
        p[i].wait_time = 0;
        p[i].arrival = p[i].first_arrival;
        for (int j = 0; j < p[i].num_bursts; j++)
        {
            p[i].temp_bursts[j] = p[i].bursts[j];
        }
    }

    printf("\n");
}

// Main function
int main()
{
    FILE *file = fopen("input.txt", "r");
    if (!file)
    {
        printf("Error opening file\n");
        return 1;
    }

    fscanf(file, "%d", &n);
    p = create_processes(n);

    for (int i = 0; i < n; i++)
    {
        fscanf(file, "%d%d", &p[i].pid, &p[i].arrival);
        p[i].first_arrival = p[i].arrival;
        while (1)
        {
            int num;
            fscanf(file, "%d", &num);
            if (num == -1)
                break;
            p[i].bursts[p[i].num_bursts++] = num;
        }
    }
    fclose(file);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < p[i].num_bursts; j++)
        {
            p[i].temp_bursts[j] = p[i].bursts[j];
            p[i].sum_bursts += p[i].bursts[j];
        }
    }

    // Simulate FCFS
    scheduling(INT_MAX);

    // Simulate Round-Robin with q = 10
    scheduling(10);

    // Simulate Round-Robin with q = 5
    scheduling(5);

    return 0;
}
