/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	q->m_size = 64 ; //initialize at 64 because it's a good number
	q->m_num_entries = 0; //currently empty
	//q->m_array[20] = {};//this is an array of void pointers, I think.
	q->m_array = malloc(64 * sizeof(*q->m_array) );//creates array of size 20
	
	for(int x = 0; x < q->m_size; x++)
	{
		q->m_array[x] = NULL;
	} //fill it with NULL
	
	q->compare_func = comparer;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	
	//start at back of queue	
	if(0 == q->m_num_entries)
	{
		q->m_array[0] = ptr;
		q->m_num_entries = 1;
		return(0);
	}
	else
	{
		int x = q->m_num_entries - 1; //last element of array location
		while( 0 > q->compare_func( ptr , q->m_array[x] ) && x >= 0 ) 
			//"true" while array element is less than ptr
			//i.e. this is a min priority queue
		{
			x = x-1; //decrement x
			if( -1 == x) //out of bounds, ptr is greater than every other array member
			{
				break;
			}
		}

		//x will either be the location of the first >= element,
		//OR it will be -1. Should work from here on out

		//At this point, x is the first location greater than or equal
		//to ptr (from the back of the array)
		//This means that ptr should be inserted immediately after x

		//first relocate necessary elements
		for( int z = q->m_num_entries-1 ; z > x ; z--)
		{
			q->m_array[z+1] = q->m_array[z]; //move up an index
		}
		q->m_array[x+1] = ptr;
		q->m_num_entries = q->m_num_entries + 1; //increment number of entries

		if(q->m_num_entries == q->m_size)
		{
			//Refactor / double size of array
			void ** replacement_array = malloc(2 * sizeof(*q->m_array) );
			int y;
			for( y = 0; y < q->m_num_entries; y++)
			{
				replacement_array[y] = q->m_array[y];
			}
			while(y < 2*q->m_size)
			{
				replacement_array[y] = NULL; //turn rest of entries to null

			}
			free(q->m_array); //free old array
			q->m_array = replacement_array; //replace with new, larger array
			q->m_size = 2*q->m_size; //double m_size
		}
		return(x+1); //x+1 is index of inserted pointer
	}
		
	
	return -1;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	if( 0 == q->m_num_entries)
	{
		return NULL;
	}
	else if(0 < q->m_size)
	{
		return(q->m_array[0]); //returns front of queue
	}
	else
	{
		puts("Something has gone wrong, and m_size is less than zero");
	}

	return(NULL);
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	if(0 == q->m_num_entries)
	{//is empty
		return NULL;
	}
	else if( 0 < q->m_size)
	{
		void *temp = q->m_array[0];
		for(int x=0; x < q->m_num_entries ; x++)
		{
			if( x == q->m_size - 1) //if x is the very last entry,
						//and the queue was full
			{
				break;
			}
			q->m_array[x] = q->m_array[x+1];
		}
		q->m_array[ q->m_num_entries - 1 ] = NULL;
		//sets last entry (formerly a pointer) to NULL
		q->m_num_entries = q->m_num_entries - 1;
		//decrements the recorded number of entries
		return(temp);
	}
	return(NULL);
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	if(index > q->m_num_entries)
	{
		return(NULL);
	}
	else
	{
		return(q->m_array[index]);
	}

	return NULL;
}


/**
  Removes all instances of ptr from the queue. 
  
  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	int number_of_instances = 0;
	int first_location = 0;
	
	if(0 == q->m_size)
	{
		return(0);
	}
	else
	{
		int x = 0;
		//while entry != ptr and we're still in array
		while( q->m_array[x] != ptr && x < q->m_num_entries)
		{
			x = x + 1; //increment x
		}

		if(x == q->m_num_entries)
		{ //none were in the array
			return(0);
		}
		else
		{ //x will be at first location of ptr
		  //Note that all ptrs will be next to eachother if there are
		  //multiples
		  	number_of_instances = 1;
			first_location = x;
			x++;
			while(q->m_array[x] == ptr && x < q->m_num_entries)
			{ //while in entries and ptr is in array
				x++;
				number_of_instances++;
			}
			for(int y = first_location;
			    y < q->m_num_entries - number_of_instances;
			    y++)
			{
				//Move all at and past first appearance point
				//back number_of_instance spaces
				q->m_array[y] = q->m_array[ y + number_of_instances];
			}
			for(int y = q->m_num_entries - number_of_instances; 
			    y < q->m_num_entries;
			    y++)
			{
				//Replace last number_of_instance spots with
				//NULL
				q->m_array[y] = NULL;
			}
			q->m_num_entries = q->m_num_entries - number_of_instances;

			return(number_of_instances);
		}
	}


	return 0;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	if(index > q->m_num_entries)
	{
		return(NULL);
	}
	else
	{
		void* removed_value = q->m_array[index];
		int x = index;
		while( x < q->m_num_entries)
		{
			//shuffle all entries after "index" back one
			q->m_array[x] = q->m_array[x+1];
		}
		q->m_array[ q->m_num_entries - 1 ] = NULL; //make last entry NULL
		q->m_num_entries = q->m_num_entries - 1; //decrement m_num_entries
		return(removed_value);
	}

	return NULL; //A catch-all return just in case. Shouldn't ever prompt.
}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->m_num_entries;
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	free(q->m_array);
//	free(q);
}
