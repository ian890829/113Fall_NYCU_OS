#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <string>
#include <ctime>
#include <unistd.h>
#include <vector>
#include <sstream>

using namespace std;
pthread_barrier_t barrier;

struct threadArgs
{
    int thread_id;
    int policy;
    int priority;
    double busy_time;
};
void *thread_function(void *arg)
{
    threadArgs *thread=(threadArgs *)arg;
    
    pthread_barrier_wait(&barrier);

    for (int i=0;i<3;i++)
    {
        timespec starttime, currenttime;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &starttime);

        printf("Thread %d is starting\n", thread->thread_id);
        long long start=starttime.tv_sec*1e9+starttime.tv_nsec; //ns level
        while(1)
        {
            clock_gettime(CLOCK_THREAD_CPUTIME_ID,&currenttime);
            //calculate the time diff
            long long current=currenttime.tv_sec*1e9+currenttime.tv_nsec;
            long long end=thread->busy_time*1e9;
            if(current-start>=end)
                break;
        }
        sched_yield();
    }
    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
    int num_threads=0;
    double busy_time=0.0;
    string scheduling_policies;
    string priorities;
    int opt;
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
            case 'n':
                num_threads = std::stoi(optarg);
                break;
            case 't':
                busy_time = std::stod(optarg);
                break;
            case 's':
                scheduling_policies = optarg;
                break;
            case 'p':
                priorities = optarg;
                break;
        }
    }
    vector<int>policies;
    vector<int>prios;
    istringstream policy_stream(scheduling_policies);
    istringstream priority_stream(priorities);
    string token;
    while (getline(policy_stream,token,','))
    {
        //policies : 1 for FIFO 0 for NORMAL(OTHER)
        policies.push_back((token=="FIFO")?SCHED_FIFO:SCHED_OTHER);
    }

    while (getline(priority_stream,token,','))
    {
        prios.push_back(stoi(token));
    }

    pthread_barrier_init(&barrier, nullptr, num_threads);

    vector<pthread_t> threads(num_threads);
    vector<threadArgs> thread_args(num_threads);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0,&cpuset); //add cpu0 to cpuset
    sched_setaffinity(0,sizeof(cpu_set_t),&cpuset); // set the all process to execute on cpu0 
    for (int i=0;i<num_threads;++i)
    {
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, policies[i]);    

        if (policies[i]== SCHED_FIFO)
        {                                           //struct sched_param{ int sched_priority};
        sched_param param{}; 
        param.sched_priority = prios[i];
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); // set the created thread to not inherit the main thread
        pthread_attr_setschedparam(&attr,&param);
        }

        thread_args[i]= {i,policies[i],prios[i],busy_time};

        if(pthread_create(&threads[i],&attr,thread_function, &thread_args[i])!=0)
        {
            perror("pthread create fail");
            exit(1);
        }
        pthread_attr_destroy(&attr);
    }
    
    //waiting for all threads completed
    for (auto &thread : threads)
    {
        pthread_join(thread,nullptr);
    }

    pthread_barrier_destroy(&barrier);


}
