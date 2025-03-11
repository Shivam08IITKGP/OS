#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILENAME "foodep.txt"
#define VISFILE "done.txt"
int status;

void createVisFile(const char *filename, int n){
    FILE *file = fopen(VISFILE, "w"); 
    if (file == NULL) {
        printf("Error opening the file.\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        fprintf(file, "0 ");  
    }

    fclose(file);  
}

void storeVisFile(const char *filename,int*vis_node,int n){
    FILE *file = fopen(filename, "w"); 
    if (file == NULL) {
        printf("Error opening the file.\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        // printf("%d ",vis_node[i]);
        fprintf(file, "%d ", vis_node[i]);
    }
    // printf("\n");
    fprintf(file,"\n");

    fclose(file);  
}

int isdigit(char c){
    return (c>='0' && c<='9');
}

int* readDependecies(int target,int*count){
    FILE *file;
    int *numbers = NULL;
    int number, capacity = 100, index = 0;
    char line[256]; 

    file = fopen(FILENAME, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        return NULL;
    }

    numbers = (int*)malloc(capacity * sizeof(int));
    if (numbers == NULL) {
        printf("Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    while (fgets(line, sizeof(line), file)) {
        char *ptr = line;
        // printf("%s",ptr);
        
        while (*ptr==' ') ptr++;
        char target_str[20];
        snprintf(target_str, sizeof(target_str), "%d:", target);
        // printf("%s\n\n",target_str);
        if (strncmp(ptr, target_str,strlen(target_str)) == 0) {
            ptr+=strlen(target_str);
            while (sscanf(ptr, "%d", &number) == 1) {
                // printf("%d ",number);
                numbers[index++] = number; 
                if (index >= capacity) {
                    capacity *= 2;
                    numbers = (int*)realloc(numbers, capacity * sizeof(int));
                    if (numbers == NULL) {
                        printf("Memory reallocation failed.\n");
                        fclose(file);
                        return NULL;
                    }
                }
                while (isdigit(*ptr)) ptr++;
                while (*ptr==' ') ptr++;
            }
            break;  
        }
    }

    if (index == 0) {
        free(numbers);
        numbers = NULL;
    }
    fclose(file);
    *count = index;
    return numbers;
}

int *readVisFile(const char*filename,int*size){
    FILE *file = fopen(filename, "r");  
    if (file == NULL) {
        printf("Error opening the file.\n");
        *size = 0;  
        return NULL;
    }

    int count = 0;
    int num;
    while (fscanf(file, "%d", &num) == 1) {
        count++;
    }

    rewind(file);

    int *arr = (int *)malloc(count * sizeof(int));
    if (arr == NULL) {
        printf("Memory allocation failed.\n");
        fclose(file);
        *size = 0;
        return NULL;
    }

    int i = 0;
    while (fscanf(file, "%d", &arr[i]) == 1) {
        i++;
    }

    fclose(file);  

    *size = count;  
    return arr;
}

void procnode(int node,int flag){
    FILE*file;
    int n;
    file=fopen(FILENAME,"r");
    if(file==NULL){
        printf("Error opening the file.\n");
        return;
    }
    fscanf(file,"%d",&n);
    fclose(file);
    if(!flag){
        createVisFile(VISFILE,n);
    }

    int e;
    int *dependencies=readDependecies(node,&e);
    // printf("%d: ",node);
    // for(int i=0;i<e;i++){
    //     printf("%d ",dependencies[i]);
    // }
    // printf("\n");
    // if(e==0){
    //     printf("hi\n");
    // }
    // for(int i=1;i<e;i++){
    //     printf("%d ",dependecies[i]);
    // }
    for(int i=1;i<e;i++){
        int check=-1;
        int *vis=readVisFile(VISFILE,&check);
        if(!vis[dependencies[i]-1]){
            int pid=fork();
            if(pid==0){
                char target_str[20];
                snprintf(target_str, sizeof(target_str), "%d", dependencies[i]);
                // printf("%s\n",target_str);
                execlp("./rebuild","./rebuild",target_str,"c",NULL);
            }
            else{
                wait(0);
            }
        }
    }
    int check=-1;
    int *vis_node=readVisFile(VISFILE,&check);
    // for(int i=0;i<n;i++) printf("%d ",vis_node[i]);
    // printf("\n");
    if(!vis_node[node-1])
    {
        printf("foo%d rebuilt",node);
        if(e>=1) printf(" from ");
        for(int i=1;i<e;i++){
            printf(" foo%d ",dependencies[i]);
        }
        printf("\n");
    }
    vis_node[node-1]=1;
    // for(int i=0;i<n;i++) printf("%d ",vis_node[i]);
    // printf("\n");
    // printf("%d\n",node-1);
    storeVisFile(VISFILE,vis_node,n);

    return;
}


int main ( int argc, char *argv[] )
{
   if (argc == 1) {
      fprintf(stderr, "Run with a node name\n");
      exit(1);
   }

//    sleep(1);

   procnode(atoi(argv[1]) , (argc >= 3) ?1:0);

   exit(0);
}