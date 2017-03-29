/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
	int job_id;
	int core_id; //-1 for "inactive"
	int arrival_time; //Time that the job was created
	int completion_time; //will be filled once job completes,
			     //is the time when job has expired/completed
	int length; //the number of time units required for the job to complete
	int priority; //only used for PRI and PPRI schemes,
		      //lower values = higher priority, hence the queue
		      //being a min priority queue
	int time_running; //the cumulative units of time this job has been running	
	int time_last_scheduled; //the time slice it was last put in a cpu
				 //-1 for never scheduled yet
	int time_first_scheduled;//self-explanatory, used to calculate response time
				 //-1 for never scheduled yet
} job_t;

scheme_t active_scheme;

typedef struct _core_t
{
	int core_id;
	int active_job_id; //-1 for no active job
	job_t* active_job;
} core_t;

//Array of cores

core_t ** core_array;
int m_num_cores;
//priority queue
priqueue_t* queue;

//priority queue for the completed jobs
//note that the priority of these is entirely irrelevant, but this is
//convenient
priqueue_t* completed_queue;


//the general function pointer
int (*compare_func)(const void *, const void *);

//more specialized function declarations, definitions at end of file
int compare_FCFS(const void *a, const void *b);
int compare_SJF(const void *a, const void *b);
int compare_PRI(const void *a, const void *b);
int compare_RR(const void *a, const void *b);

/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
	active_scheme = scheme;
	//I probably want to use the scheme to decide which comparing function
	//to use for the rest of the program.
	//Also which comparison function to pass to the priority queue
	
	if(active_scheme == FCFS)
	{
		compare_func = &compare_FCFS;
	}
	else if(active_scheme == SJF || active_scheme == PSJF)
	{
		compare_func = &compare_SJF;
	}
	else if(active_scheme == PRI || active_scheme == PPRI)
	{
		compare_func = &compare_PRI;
	}
	else
	{
		compare_func = &compare_RR;
	}
	m_num_cores = cores;
	core_array = malloc(cores * sizeof(core_t*) ); //initializes the array of cores
	for(int x=0; x < cores; x++)
	{
		core_t* new_core = malloc(sizeof(core_t));
		new_core->core_id = x;
		new_core->active_job_id = -1;
		new_core->active_job = NULL;
		core_array[x] = new_core;
		//core_array[x]->core_id = x;
		//core_array[x]->active_job_id = -1;
		//core_array[x]->active_job = NULL;
	}

	queue = malloc(sizeof(priqueue_t));
	completed_queue = malloc(sizeof(priqueue_t));
	priqueue_init(completed_queue, &compare_FCFS);//FCFS to sort by arrival time
	priqueue_init(queue, compare_func);

}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
	//First: create a new job
	job_t* new_job = malloc(sizeof(job_t));
	new_job->job_id = job_number;
	new_job->arrival_time = time;
	new_job->length = running_time;
	new_job->priority = priority;
	new_job->time_running = 0; //fresh off the presses
	new_job->time_last_scheduled = -1;
	new_job->time_first_scheduled = -1;
	//Second: check for an empty core

	int x = 0;
	for( x = 0; x < m_num_cores; x++)
	{
		if( -1 == core_array[x]->active_job_id)
		{ //if the core is unoccupied, fill it
			core_array[x]->active_job_id = job_number;
			core_array[x]->active_job = new_job;
			new_job->core_id = x;
			new_job->time_last_scheduled = time;
			new_job->time_first_scheduled = time;

			return(x);
		}
	}
	//at this point we know no core is free
	//check for preemption, if applicable
	if(PPRI == active_scheme || PSJF == active_scheme)
	{ //a preemptive scheme
		for(x = 0; x < m_num_cores; x++)
		{
			if(0 > compare_func(new_job , core_array[x]->active_job))
			{//the new job preempts the current one
				//update old job
				job_t* temp = core_array[x]->active_job;
				int time_run = temp->time_running + (time -
						temp->time_last_scheduled);

				temp->time_running = time_run;
				temp->core_id = -1;
				//add to queue
				priqueue_offer(queue, temp);

				//put new job onto core
				core_array[x]->active_job = new_job;
				new_job->core_id = x;
				new_job->time_last_scheduled = time;
				new_job->time_first_scheduled = time;
				core_array[x]->active_job_id = job_number;
				return(x); //return core it's running on
			}
		}
		//At this point, it couldn't get scheduled, so add to queue
		priqueue_offer(queue, new_job);
		return(-1);
	}
	else
	{ //nothing it can or will preempt, add to job queue, return -1
		priqueue_offer(queue, new_job);
		return(-1);
	}
	//this shouldn't prompt, but it will silence the compile warning
	return(-1);
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
	//update and store the completed job into the completed jobs queue
	job_t* finished_job = core_array[core_id]->active_job;
	finished_job->completion_time = time;
	finished_job->core_id = -1; //not being scheduled anymore
	finished_job->time_running = finished_job->length; //run the entire course
	finished_job->time_last_scheduled = time;
	priqueue_offer(completed_queue, finished_job);


	core_array[core_id]->active_job = NULL;
	core_array[core_id]->active_job_id = -1;
	//check for what should be run next
	if( NULL == priqueue_peek(queue) )
	{ //there's nothing else to run
		return(-1);
	}
	else
	{ //there are other jobs to run
		//remove the head of the priority queue and place in "temp"
		job_t* temp = priqueue_poll(queue);
		temp->core_id = core_id;
		temp->time_last_scheduled = time;
	
		if( -1 == temp->time_first_scheduled)
		{//never been scheduled before, update that
			temp->time_first_scheduled = time;
		}

		//after updating the job, place it into the core array
		core_array[core_id]->active_job_id = temp->job_id;
		core_array[core_id]->active_job = temp;
		return(temp->job_id); //return the running job id
	}

	//this should never prompt, but it silences the warning
	return -1;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	//note that this function will never be called at the same time unit
	//that the job completes, as is said in the documentation
	
	if( NULL == priqueue_peek(queue) && -1 == core_array[core_id]->active_job_id )
	{//queue is empty and this one is idle
		return(-1);//remain idle
	}
	else if(NULL == priqueue_peek(queue) && -1 != core_array[core_id]->active_job_id)
	{
		//queue is empty and there is an active job running
		return(core_array[core_id]->active_job_id); //keep running this one
	}

	//implicit else
	
	job_t* old_job = core_array[core_id]->active_job;//get the former job
	//reset the core variables
	core_array[core_id]->active_job = NULL;
	core_array[core_id]->active_job_id = -1;

	//update the old job
	old_job->time_running = old_job->time_running + (time -
				old_job->time_last_scheduled);
	
	old_job->time_last_scheduled = time;

	priqueue_offer(queue,old_job); //place back on queue

	job_t* new_job = priqueue_poll(queue); //get front of queue

	//Update core
	core_array[core_id]->active_job = new_job;
	core_array[core_id]->active_job_id = new_job->job_id;

	//update new job
	new_job->time_last_scheduled = time;
	new_job->core_id = core_id;

	if( -1 == new_job->time_first_scheduled )
	{ //job has never been scheduled
		new_job->time_first_scheduled = time; //update
	}

	return(new_job->job_id);

}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	//Average waiting time is for the time spent in the queue after being
	//created.
	int total_jobs = completed_queue->m_num_entries;

	int total_waiting_time = 0;
	for(int x = 0; x < total_jobs; x++)
	{
		job_t* temp = priqueue_at(completed_queue, x);
		if(temp == NULL)
		{
			printf("Error in calculating average, ");
			//printf("%2d: %s\n",x);
			break;
		}
		total_waiting_time = total_waiting_time + (temp->completion_time
					- temp->arrival_time - temp->length);
	}

	float average = (float)total_waiting_time / (float)total_jobs;


	return(average);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{

	int total_jobs = completed_queue->m_num_entries;

	int total_turnaround_time = 0;
	for(int x = 0; x < total_jobs; x++)
	{
		job_t* temp = priqueue_at(completed_queue, x);
		total_turnaround_time = total_turnaround_time + (temp->completion_time
					- temp->arrival_time);
	}

	float average = (float)total_turnaround_time / (float)total_jobs;

	return(average);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	//response time is time from creation to first scheduling
	
	int total_jobs = completed_queue->m_num_entries;

	int total_response_time = 0;

	for(int x = 0; x < total_jobs; x++)
	{
		job_t* temp = priqueue_at(completed_queue, x);
		total_response_time = total_response_time +
				(temp->time_first_scheduled - temp->arrival_time);
	}

	float average = (float)total_response_time / (float)total_jobs;

	return(average);
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	free(queue);//empty at this point, no need to iterate through the waiting queue

	int finished_size = completed_queue->m_size;

	for(int x = 0; x < finished_size; x++)
	{
		free(priqueue_poll(completed_queue));
	}
	free(completed_queue);
	
	for(int x = 0; x < m_num_cores; x++)
	{
		free(core_array[x]);
	}
	free(core_array);


	//need to free the cores and the jobs, if not freed already
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}

int compare_FCFS(const void *a , const void *b)
{
	//checks the arrival time. Lower arrival time = higher priority
	job_t* job_a = (job_t *) a;
	job_t* job_b = (job_t *) b;
	
	//no two times should ever be the same
	return(job_a->arrival_time - job_b->arrival_time);

}

int compare_SJF(const void *a , const void *b)
{
	//checks time to completion, how much time is left to finish
	//lower time left = higher priority
	job_t* job_a = (job_t *) a;
	job_t* job_b = (job_t *) b;

	int a_remainder = job_a->length - job_a->time_running;
	int b_remainder = job_b->length - job_b->time_running;

	if(a_remainder == b_remainder)
	{
		//If a arrived sooner, then it will be propagated up the queue
		return(job_a->arrival_time - job_b->arrival_time);
	}
	else
	{
		//if a has less time left, returns a negative value and will
		//propogate job_a further up the queue.
		return(a_remainder - b_remainder);
	}
}

int compare_PRI(const void *a, const void *b)
{
	//Checks priority value. Lower number = higher priority
	//tiebreak with arrival time.
	job_t* job_a = (job_t *) a;
	job_t* job_b = (job_t *) b;

	int return_value = job_a->priority - job_b->priority;
	if(return_value == 0)
	{ //both priorities are identical
		return_value = job_a->arrival_time - job_b->arrival_time;
		//both arrival times shouldn't be identical, as said by the
		//documentation
		return(return_value);
	}
	else
	{	
		//recall smaller values go to front of priority queue
		//If a is less than b, this returns a negative value,
		//signaling that a should propagate further up the queue
		return(return_value);
	}
}

int compare_RR(const void *a, const void *b)
{
	//the new should always go on the end of the queue, so...
	return(0);
	//The new task should always get put on the end of the queue
	//(because the insert function in priqueue starts at the end of the
	//queue, inserting only when the addition's priority does not
	//supercede the others)
	//meaning that it will always insert at the end of the queue
}
