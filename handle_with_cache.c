#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h>

#include "shm_channel.h"

#include "gfserver.h"
//Replace with an implementation of handle_with_curl and any other
//functions you may need.

struct MemoryStruct{
	char *memory;
	size_t size;
};

size_t write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp);
int send_contents(gfcontext_t *ctx, struct MemoryStruct * data);
void mem_struct_init(struct MemoryStruct *mem);

pthread_mutex_t MUTEX_SHM;
ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg)
{

	char buffer[BUFFER_LEN];
	char *data_dir = arg;
	struct MemoryStruct data;
	int msgq_glob;
	int msgq_thd;
	int msg_key;
	int msgsend_ret;
	char_msgbuf msg;
	struct MemoryStruct * data;
	mem_struct_init(&data);

	mem_struct_init(&data);
	strcpy(buffer,path);
	//strcat(buffer,path);
	msg.mtype = 2;
	strcpy(msg.mtext, buffer);
	msg.mkey = (int *)arg;

	fprintf(stdout, "cur_easy_perform.path = %s\n", buffer);
	//Create global message queue. Check if msgget performed okay
	msgq_glob = msgget(MESSAGE_KEY, 0777 | IPC_CREAT);
	if (msgq_glob == -1)
		perror("msgget");
	msgq_thd = msgget(msg_key, 0777 | IPC_CREAT);
	if (msgq_thd == -1)
		perror("msgget");
	//Add message struct (path to query) to the queue
	msgsend_ret = msgsnd(msgq_glob, &msg, char_msgbuff_sizeof(), 0);
	if (msgsend_ret != 0)
		perror("msgsnd");

	//wait on msgrcv. Upon rcv wil know if file exists and what
	//shm data struct to access.

	msgsend_ret = msgrcv(msgq_glob, &msg, char_msgbuff_sizeof(), 0, 0);
	//if char_msgbuf.shmkey is = 0 (default) then cache file did not exist
	//according to doc this should retun GF_FILE_NOT_FOUND
	if (msgsend_ret == -1)
		return EXIT_FAILURE;
	if (msg.shmkey == 0)
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	else
	{
		shm_ret = shmget(msg.shmkey, msg.size_seg, 0755 | IPC_CREAT);
		//shgmget returns -1 on failure
		if (shm_ret == -1)
			perror("shmget");
		shm_data_p = (shm_data_t *)shmat(shm_ret, (void *)0, 0);
		if (data == (shm_data_t *-1))
			perror("shm_data_p");
		while(1)
		{
			pthread_mutex_lock(&(pthread_shm_data_p->mutex));
				write_memory_cb((void *)shm_data_p, shm_data_p->data_size, 1, (void *)data);
			pthread_mutex_unlock(&(pthread_shm_data_p->mutex));
			//when size of local data == size of file break for loop
			if (data->size == shm_data_p->fsize)
				break;
		}
		gfs_sendheader(ctx, GF_OK, data.size);
		/* Sending the file contents chunk by chunk. */
		int ret_sc = send_contents(ctx, &data);
		if (ret_sc == 0)
			return data.size;
		else
			return EXIT_FAILURE;
	}
}
int send_contents(gfcontext_t *ctx, struct MemoryStruct * data)
/*
*arguments*
gfcontext_t
MemoryStruct -> struct containing data to send and its size

*synopsis*
Sends chunks of data-> memory until bytes_transferred is less than
the size referenced by data struct.

*return*
0--If no error
1--if a gfs_send sends lses than the write_len_blk
*/
{
	ssize_t bytes_transferred = 0;
	ssize_t remaining_bytes, write_len_blk, write_len;
	while(bytes_transferred < data->size)
	{
		remaining_bytes = data->size - bytes_transferred;
		write_len_blk = (BUFFER_LEN > remaining_bytes) ? remaining_bytes : BUFFER_LEN;
		write_len = gfs_send(ctx, data->memory + bytes_transferred, write_len_blk);
		if (write_len != write_len_blk){
			fprintf(stderr, "handle_with_file write error");
			return EXIT_FAILURE;
		}
		bytes_transferred += write_len;
	}
	return 0;
}
size_t write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
void mem_struct_init(struct MemoryStruct *mem){
	mem->memory = malloc(1);
	mem->size = 0;
}
