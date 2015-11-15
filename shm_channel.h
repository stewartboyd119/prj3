#ifndef SHM_CHANNEL_H
#define SHM_CHANNEL_H

#include <sys/msg.h>
#define BUFFER_LEN 4096
#define SHM_KEY 66666
#define MESSAGE_KEY 9999
#define CHAR_MTYPE 1111
#define KEY_MYTPE 2222
#define NOTEXISTS 0
#define EXISTS 1
#define READ_STATUS 0
#define WRITE_STATUS 1

typedef struct shm_key_strct
{
	int shm_key;
}shm_key_strct;

typedef struct char_msgbuf
{
	/*
	This struct is used to from handle_with_cache
	to simplecached the initial job. Because of this
	it contains the queried path (mtext) and size of shared mem
	segment (size_seg) and the message que key (mkey) which the
	thread will be listening on.

	Additionally, it used to comm back to handle_with_cache, from simple_cached,
	that the job is ready to be executed and where to check for data (shmkey).
	*/
    long mtype;
    char mtext[BUFFER_LEN];
    key_t mkey;
    key_t shmkey;
    int existance;
}char_msgbuf;
void char_msgbuf_prnt(char_msgbuf *self);
//0 sets contained attributes
void char_msgbuf_init(char_msgbuf *self, char *mtext, key_t mkey, key_t shmkey, int existance);

//Returns the sizeof the char_msgbuf struct (minus the long)
size_t char_msgbuff_sizeof();

/*
Used to communicate information about shm and message queue.
The key for both can be dictated by the key start and count (key_end is redundant).
This is because the keys are a continous sequence. This allows simplecached 
to know about the shared memory and message queues created in webproxy
*/
typedef struct key_msgbuff
{
	//This struct is used to communicate key ids
	//key ids are a continous set of integers
	//given key_start and the number of keys key_count
    long mtype;
    size_t size_seg; //only applicable when used with sharedmemory
    int key_count;
    int key_start;
    int key_end;
}key_msgbuff;

// print key_msgbuff attributes
void key_msgbuff_prnt(key_msgbuff *self);
//Initializes structs to arguments. key_end determined from key_start and key_count
void key_msgbuff_init(key_msgbuff *self, size_t size_seg, int key_count, int key_start);

//Sets self to have attributes equal to that of set_eq_to
void key_msgbuff_seteq(key_msgbuff *self, key_msgbuff *set_eq_to);
//returns size of struct sans long
int key_msgbuff_sizeof();

/*This is the struct passed to shared memory. Dynamic data is written
 * directly following the structs location in shm.
*/
typedef struct shm_struct
{
	pthread_mutex_t mutex; //mutex used for access to shared memory (read or write)
	pthread_cond_t cond_read; //signals that reading is now allowed
	pthread_cond_t cond_write; //signals that writing is now allowed
	int rw_status; // 0 (READ_STATUS) or 1 (WRITE_STATUS) ->indicates whether shm_data is in read or write state
	char path[256]; //initilization of path
	int fexist; //0 --- file does not exist and 1--- file path exists
	size_t shm_size; //total shared memory size
	size_t allwd_data_size; //allowed size of dynamic char *data. This is a function of shared memory size minus size of other struct attributes
	size_t fsize; //Should be set to total data size to be written (could be larger than block)
	ssize_t data_size; //size of data that currently resides in data block
}shm_struct;

//this function zero sets shm_struct attributes
void shm_clean(shm_struct *self);
//initailzes mutexes and conditional variables. Calculates size attributes
//given prescribed shared memory size 
void shm_init(shm_struct *self, size_t presc_size);

//calcs size info aobut shared memory struct
void shm_sizecalc(shm_struct *self, size_t presc_size);
//initializes a conidition variable to be shared across processes
int _shm_cond_var_init(pthread_cond_t *c);
//initializes mutex to be shared across prcesses
int _shm_mutex_var_init(pthread_mutex_t *m);

void shm_data_prnt(shm_struct *self);

shm_struct * shm_get(key_t shm_key, size_t size_segments);

typedef struct thread_arg_strct
{
	key_t msg_key;
	size_t size_segs;

}thread_arg_strct;

#endif
