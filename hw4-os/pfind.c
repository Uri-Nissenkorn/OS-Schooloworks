#include <threads.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

typedef struct worker
{
    struct worker *next;
    thrd_t thread;
    cnd_t cond;
} worker;

typedef struct workers
{
    worker *first;
    worker *last;
    int count;
} workers;

typedef struct dir
{
    char path[PATH_MAX];
    struct dir *next;
} dir;

typedef struct fifo
{
    dir *first;
    dir *last;
} fifo;


mtx_t lock;
mtx_t waitLock;
int init_faze;
fifo *dirQueue;
int threadNum;

//threads array
int *threadStatus;
thrd_t *threadList;

// inactive threads queue
workers *waitingWorkers;

char term[PATH_MAX];

int count = 0;
int done = 0;


cnd_t addWaitingWorker(thrd_t thread)
{
    waitingWorkers->count++;
    worker *newWorker = malloc(sizeof(worker));
    newWorker->thread = thread;
    cnd_init(&(newWorker->cond));

    newWorker->next = NULL;

    if (waitingWorkers->first == NULL)
    {
        waitingWorkers->first = newWorker;
        waitingWorkers->last = newWorker;
    }
    else
    {
        waitingWorkers->last->next = newWorker;
        waitingWorkers->last = newWorker;
    }
    return newWorker->cond;

}

void popWaitingWorker()
{
    if (waitingWorkers->first == NULL)
    {
        return;
    }
    waitingWorkers->count--;
    printf("______should release %ld\n",waitingWorkers->first->thread);
    if (waitingWorkers->first == waitingWorkers->last)
    {
        worker *temp = waitingWorkers->first;
        free(waitingWorkers->first);
        waitingWorkers->first = NULL;
        waitingWorkers->last = NULL;
        cnd_signal(&(temp->cond));
        free(temp);
    }
    else
    {
        worker *temp;
        temp = waitingWorkers->first;
        waitingWorkers->first = waitingWorkers->first->next;
        cnd_signal(&(temp->cond));
        free(temp);
    }
}


void addToQueue(char *dirPath)
{
    //printf("adding %s\n", dirPath);
    mtx_lock(&lock);

    dir *newDir = malloc(sizeof(dir));
    strcpy(newDir->path, dirPath);
    newDir->next = NULL;

    if (dirQueue->first == NULL)
    {
        dirQueue->first = newDir;
        dirQueue->last = newDir;
    }
    else
    {
        dirQueue->last->next = newDir;
        dirQueue->last = newDir;
    }

    mtx_unlock(&lock);
}

char *popQueue()
{

    mtx_lock(&lock);

    char *path;

    if (dirQueue->first == NULL)
    {
        mtx_unlock(&lock);
        return NULL;
    }
    path = malloc(PATH_MAX); 

    if (dirQueue->first==dirQueue->last)
    {
        strcpy(path,dirQueue->first->path);
        free(dirQueue->first);
        dirQueue->first = NULL;
        dirQueue->last = NULL;
    }
    else
    {
        strcpy(path,dirQueue->first->path);
        dir *temp;
        temp = dirQueue->first;
        dirQueue->first = dirQueue->first->next;
        free(temp);
    }

    mtx_unlock(&lock);

    //printf("removing %s\n", path);


    return path;
}

void updateThreadStatus(int status)
{
    // 1 = active, 0 = inactive
    thrd_t tid = thrd_current();
    int inactive = 0;

    // update status, and count inactive threads
    for (int i = 0; i < threadNum; i++)
    {
        if (thrd_equal(tid, threadList[i]))
        {
            threadStatus[i] = status;
        }
        if (!threadStatus[i])
        {
            inactive++;
        }
    }


    // if all threads are inactive signal to finish
    if (inactive == threadNum)
    {
        done = 1;
    }
    else
    {
        done = 0;
    }
}

void searchDir(char *path)
{
    struct dirent *de;
    struct stat fileinfo;

    DIR *dr = opendir(path);

    // if folder can't be opend
    if (dr == NULL)
    {
        printf("Directory %s: Permission denied. \n", path);
    } else {

    char subPath[PATH_MAX];

    // sub directories
    while ((de = readdir(dr)) != NULL)
    {
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) // ignore "." and ".."
        {

            strcpy(subPath, path);
            strcat(subPath, "/");
            strcat(subPath, de->d_name);
            stat(subPath, &fileinfo);


            // dir or not
            if (S_ISDIR(fileinfo.st_mode))
            {
                addToQueue(subPath);
                // release waiting thread if there is one
                // mtx_lock(&waitLock);
                // printf("pop!\n");
                // popWaitingWorker();
                // mtx_unlock(&waitLock);
                // printf("nopop\n");
            }
            else
            {
                if (strstr(de->d_name, term))
                {
                    count++;
                    printf("%s\n", subPath);
                }
            }
        }
    }

    closedir(dr);

}
}


int threadFunc(void* arg)
{
    // wait for init to finish
    while (init_faze == 1){}

    // start searching
    char *path;
    while (!done)
    {
        path = popQueue();
        if (path == NULL)
        {
            updateThreadStatus(0);
            sleep(1);
            // mtx_lock(&waitLock);
            // cnd_t t = addWaitingWorker(thrd_current());
            // if((waitingWorkers->count)<threadNum) {
            //     cnd_wait(&t, &waitLock);
            // } else {
            //     done=1;
            //     // signal all threads to finish
            //     worker *temp = waitingWorkers->first;
            //     while (temp!=NULL)
            //     {
            //         cnd_signal(&(temp->cond));
            //         temp = temp->next;
            //     }
            // }
            // mtx_unlock(&waitLock);
        }
        else
        {

            updateThreadStatus(1);
            searchDir(path);
        }
    }
    thrd_exit(thrd_success);
}

int main(int argc, char *argv[])
{
    // will init first and only start after all threads are created
    init_faze = 1;

    if (argc != 4)
    {
        fprintf(stderr, "Error: incorrect number of variables\n");
        exit(1);
    }

    strcpy(term, argv[2]);
    threadNum = atoi(argv[3]);


    if (threadNum < 1)
    {
        fprintf(stderr, "Error: no thread numbers\n");
        exit(1);
    }

    if (mtx_init(&lock, mtx_plain) != 0) {
        fprintf(stderr, "mutex init has failed\n");
        exit(1);
    }

     if (mtx_init(&waitLock, mtx_plain) != 0) {
        fprintf(stderr, "mutex init has failed\n");
        exit(1);
    }


    dirQueue = malloc(sizeof(fifo)); // alocate queue memory
    // waitingWorkers = malloc(sizeof(workers)); // alocate queue memory
    // waitingWorkers->count=0;

    searchDir(argv[1]);

    // threads list
    threadList = malloc(sizeof(thrd_t)*threadNum);
    threadStatus = malloc(sizeof(int)*threadNum);

    // create threads
    for (int i = 0; i < threadNum; i++)
    {
        if (thrd_create(&threadList[i], &threadFunc, NULL) != thrd_success)
        {
            fprintf(stderr, "Error: failed to create threads\n");
            exit(1);
        }
        
        threadStatus[i] = 0;
    }

    // start threads
    init_faze = 0;

    // wait for end
   for (int i = 0; i < threadNum; i++)
    {
        thrd_join(threadList[i], NULL);
    }    
    mtx_destroy(&lock);

    printf("Done searching, found %d files\n", count);

    free(threadList);
    free(threadStatus);

    exit(0);
}