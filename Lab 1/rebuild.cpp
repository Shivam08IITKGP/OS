#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sstream>
#include <cstdio>
#include <cstdlib>

using namespace std;

#define VISITED_FILE "done.txt"
#define DEPENDENCY_FILE "foodep.txt"

void procnode(int node, int isroot);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Node number required\n");
        exit(1);
    }

    int node = atoi(argv[1]);
    procnode(node, (argc >= 3) ? 1 : 0);
    exit(0);
}

void procnode(int node, int isroot)
{
    FILE *f_dep;
    f_dep = fopen(DEPENDENCY_FILE, "r");
    if (f_dep == nullptr)
    {
        perror("Failed to open dependency file");
        exit(1);
    }

    int total_nodes;
    fscanf(f_dep, "%d", &total_nodes);
    fgetc(f_dep);
    fclose(f_dep);

    if (isroot == 0)
    {
        FILE *f_visited = fopen(VISITED_FILE, "w");
        if (f_visited == nullptr)
        {
            perror("Failed to open visited file");
            exit(1);
        }
        for (int i = 0; i < total_nodes; i++)
        {
            fprintf(f_visited, "%d", 0);
        }
        fclose(f_visited);
    }

    f_dep = fopen(DEPENDENCY_FILE, "r");
    if (f_dep == nullptr)
    {
        perror("Failed to open dependency file");
        exit(1);
    }

    vector<int> dep;
    fscanf(f_dep, "%d", &total_nodes);
    fgetc(f_dep);
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), f_dep))
    {
        string line = buffer;
        if (line.empty() || line == "\n")
            continue;

        stringstream ss(line);
        int key;
        char colon;
        ss >> key >> colon;

        vector<int> values;
        int value;
        while (ss >> value)
        {
            values.push_back(value);
        }
        if (key == node)
        {
            dep = values;
            break;
        }
    }
    fclose(f_dep);

    for (auto x : dep)
    {
        FILE *f_visited = fopen(VISITED_FILE, "r");
        if (f_visited == nullptr)
        {
            perror("Failed to open visited file");
            exit(1);
        }

        vector<int> visited(total_nodes + 1, 0);
        for (int i = 0; i < total_nodes; i++)
        {
            char c;
            fscanf(f_visited, "%c", &c);
            visited[i + 1] = c - '0';
        }
        fclose(f_visited);

        if (!visited[x])
        {
            int pid = fork();
            if (pid == 0)
            {
                string s = to_string(x);
                execlp("./rebuild", "./rebuild", s.c_str(), "c", NULL);
            }
            else
            {
                wait(0);
            }
        }
    }

    FILE *f_visited = fopen(VISITED_FILE, "r");
    if (f_visited == nullptr)
    {
        perror("Failed to open visited file");
        exit(1);
    }

    vector<int> visited(total_nodes + 1, 0);
    for (int i = 0; i < total_nodes; i++)
    {
        char c;
        fscanf(f_visited, "%c", &c);
        visited[i + 1] = c - '0';
    }
    fclose(f_visited);

    if (!visited[node])
    {
        printf("foo%d rebuilt", node);
        if (!dep.empty())
        {
            printf(" from");
        }
        for (int cnt = 0; cnt < dep.size(); cnt++)
        {
            int x = dep[cnt];
            if (cnt > 0)
            {
                printf(",");
            }
            printf(" foo%d", x);
        }

        printf("\n");
    }

    visited[node] = 1;
    f_visited = fopen(VISITED_FILE, "w");
    if (f_visited == nullptr)
    {
        perror("Failed to open visited file for writing");
        exit(1);
    }

    for (int i = 0; i < total_nodes; i++)
    {
        fprintf(f_visited, "%d", visited[i + 1]);
    }
    fprintf(f_visited, "\n");
    fclose(f_visited);

    return;
}
