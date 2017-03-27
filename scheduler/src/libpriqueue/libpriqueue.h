/** @file libpriqueue.h
 */

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_
#define MAX_QUEUE_SIZE 128

/**
  Priqueue Data Structure
*/
typedef struct _priqueue_t
{
	//Might have to malloc() the array.
	//Don't know where or how though.
	//current size of array
	int m_size;//=MAX_QUEUE_SIZE;
	//current number of entries
	int m_num_entries;
	//function pointer to the comparison function
	//returns -1 if the first argument is smaller
	//1 if the first argument is larger
	//or zero if they are equal
	int(*compare_func)(const void *, const void *);

	void ** m_array;
	//Array of void pointers?
	//could just make it a fixed, large size.
	//(like 128 or something)
	//dynamic sizing is creating issues.
	//int m_size
	//int m_num_entries
	//function pointer to the compare function
} priqueue_t;


void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));

int    priqueue_offer    (priqueue_t *q, void *ptr);
void * priqueue_peek     (priqueue_t *q);
void * priqueue_poll     (priqueue_t *q);
void * priqueue_at       (priqueue_t *q, int index);
int    priqueue_remove   (priqueue_t *q, void *ptr);
void * priqueue_remove_at(priqueue_t *q, int index);
int    priqueue_size     (priqueue_t *q);

void   priqueue_destroy  (priqueue_t *q);

#endif /* LIBPQUEUE_H_ */
