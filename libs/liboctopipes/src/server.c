/**
 *   Octopipes
 *   Developed by Christian Visintin
 * 
 * MIT License
 * Copyright (c) 2019-2020 Christian Visintin
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/

#include <octopipes/octopipes.h>
#include <octopipes/cap.h>
#include <octopipes/pipes.h>
#include <octopipes/serializer.h>

#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//@! Privates
//Workers
OctopipesServerError worker_init(OctopipesServerWorker** worker, const char** subcsriptions, const size_t sub_len, const char* client_id, const char* pipe_read, const char* pipe_write);
OctopipesServerError worker_cleanup(OctopipesServerWorker* worker);
//Inbox
OctopipesServerError message_inbox_init(OctopipesServerInbox** inbox);
OctopipesServerError message_inbox_cleanup(OctopipesServerInbox* inbox);
OctopipesServerError message_inbox_expunge(OctopipesServerInbox* inbox);
OctopipesServerError message_inbox_push(OctopipesServerInbox* inbox, OctopipesMessage* message, OctopipesServerError error);
OctopipesServerError message_inbox_remove(OctopipesServerInbox* inbox, const size_t index);
//Thread
void* cap_loop(void* args);
void* worker_loop(void* args);
//FS
int create_clients_dir(const char* directory);
//Others
OctopipesServerError to_server_error(const OctopipesError error);
//Consts
#define TIME_50MS 50000
#define TIME_100MS 100000
#define TIME_200MS 200000
#define TIME_300MS 300000
#define TIME_400MS 400000
#define TIME_500MS 500000
#define TIME_600MS 600000
#define TIME_700MS 700000
#define TIME_800MS 800000
#define TIME_900MS 900000

/**
 * @brief initialize an OctopipesServer
 * @param OctopipesServer** server to initialize
 * @param char* cap path
 * @param char* client directory
 * @param OctopipesVersion version to use
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_init(OctopipesServer** server, const char* cap_path, const char* client_folder, const OctopipesVersion version) {
  //Allocate server
  OctopipesServer* ptr = (OctopipesServer*) malloc(sizeof(OctopipesServer));
  if (ptr == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  ptr->cap_inbox = NULL;
  ptr->cap_pipe = NULL;
  ptr->client_folder = NULL;
  //Allocate CAP
  const size_t cap_len = strlen(cap_path);
  ptr->cap_pipe = (char*) malloc(sizeof(char) * (cap_len + 1));
  if (ptr->cap_pipe == NULL) {
    goto bad_alloc;
  }
  memcpy(ptr->cap_pipe, cap_path, cap_len);
  ptr->cap_pipe[cap_len] = 0x00;
  //Allocate client folder
  const size_t cli_dir_len = strlen(client_folder);
  ptr->client_folder = (char*) malloc(sizeof(char) * (cli_dir_len) + 1);
  if (ptr->client_folder == NULL) {
    goto bad_alloc;
  }
  memcpy(ptr->client_folder, client_folder, cli_dir_len);
  ptr->client_folder[cli_dir_len] = 0x00;
  //Allocate inbox
  if (message_inbox_init(&ptr->cap_inbox) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    goto bad_alloc;
  }
  ptr->state = OCTOPIPES_SERVER_STATE_INIT;
  ptr->version = version;
  ptr->workers = NULL;
  ptr->workers_len = 0;
  *server = ptr;
  return OCTOPIPES_SERVER_ERROR_SUCCESS;

bad_alloc:
  if (ptr->cap_pipe != NULL) {
    free(ptr->cap_pipe);
  }
  if (ptr->client_folder != NULL) {
    free(ptr->client_folder);
  }
  message_inbox_cleanup(ptr->cap_inbox);
  if (ptr != NULL) {
    free(ptr);
  }
  return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
}

/**
 * @brief cleanup an OctopipesServer. It stops the workers and the cap listener if they're running
 * @param OctopipesServer*
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_cleanup(OctopipesServer* server) {
  if (server == NULL) {
    return OCTOPIPES_SERVER_ERROR_SUCCESS;
  }
  if (server->state == OCTOPIPES_SERVER_STATE_RUNNING) {
    server->state = OCTOPIPES_SERVER_STATE_STOPPED; //Stop CAP listener
    if (pthread_join(server->cap_listener, NULL) != 0) {
      return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
    }
    //Iterate over workers
    for (size_t i = 0; i < server->workers_len; i++) {
      OctopipesServerWorker* worker = server->workers[i];
      worker->active = 0; //Set active to false
      //Join worker
      if (pthread_join(worker->worker_listener, NULL) != 0) {
        return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
      }
      //Free inbox
      if (worker->inbox != NULL) {
        message_inbox_cleanup(worker->inbox);
      }
      free(worker);
    }
  }
  free(server);
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief start the CAP listener thread. Before starting the thread it cleans the client directory and creates the CAP pipe
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_start_cap_listener(OctopipesServer* server) {
  //If state is running return error
  if (server->state == OCTOPIPES_SERVER_STATE_RUNNING) {
    return OCTOPIPES_SERVER_ERROR_THREAD_ALREADY_RUNNING;
  }
  //Create directory for client
  if (create_clients_dir(server->client_folder) != 0) {
    return OCTOPIPES_SERVER_ERROR_BAD_CLIENT_DIR;
  }
  //Create CAP pipe
  if (pipe_create(server->cap_pipe) != OCTOPIPES_ERROR_SUCCESS) {
    return OCTOPIPES_SERVER_ERROR_OPEN_FAILED;
  }
  //Set server to running
  server->state = OCTOPIPES_SERVER_STATE_RUNNING;
  //Init mutex
  if (pthread_mutex_init(&server->cap_lock, NULL) != 0) {
    return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
  }
  //Start thread
  if(pthread_create(&server->cap_listener, NULL, cap_loop, server) != 0) {
    return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
  }
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_stop_cap_listener(OctopipesServer* server) {
  if (server->state != OCTOPIPES_SERVER_STATE_RUNNING) {
    return OCTOPIPES_SERVER_ERROR_UNINITIALIZED;
  }
  //Set state to running
  server->state = OCTOPIPES_SERVER_STATE_RUNNING;
  //Join CAP listener
  if (pthread_join(server->cap_listener, NULL) != 0) {
    return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
  }
  pthread_mutex_destroy(&server->cap_lock);
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief lock CAP setting server state to BLOCK
 * @param OctopipesServer* server
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_lock_cap(OctopipesServer* server) {
  if (server->state != OCTOPIPES_SERVER_STATE_RUNNING) {
    return OCTOPIPES_SERVER_ERROR_UNINITIALIZED;
  }
  server->state = OCTOPIPES_SERVER_STATE_BLOCK;
  usleep(TIME_500MS); //Give time to block CAP
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief unlock CAP setting state back to RUNNING
 * @param OctopipesServer*
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_unlock_cap(OctopipesServer* server) {
  if (server->state != OCTOPIPES_SERVER_STATE_BLOCK) {
    return OCTOPIPES_SERVER_ERROR_SUCCESS;
  }
  server->state = OCTOPIPES_SERVER_STATE_RUNNING;
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief write a message to the CAP
 * @param OctopipesServer* server
 * @param char* client name
 * @param uint8_t* data
 * @param size_t data size
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_write_cap(OctopipesServer* server, const char* client, const uint8_t* data, const size_t data_size) {
  if (server->state != OCTOPIPES_SERVER_STATE_RUNNING) {
    return OCTOPIPES_SERVER_ERROR_UNINITIALIZED;
  }
  OctopipesServerError rc;
  //Encode message
  OctopipesMessage* message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  if (message == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  message->data = NULL;
  message->origin = NULL; //None, server has no origin
  message->origin_size = 0;
  message->remote = NULL;
  message->data = (uint8_t*) malloc(sizeof(uint8_t) * data_size);
  if (message->data == NULL) {
    octopipes_cleanup_message(message);
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  memcpy(message->data, data, data_size);
  message->data_size = data_size;
  //Copy remote
  message->remote_size = strlen(client);
  message->remote = (char*) malloc(sizeof(char) * (message->remote_size + 1));
  if (message->remote == NULL) {
    octopipes_cleanup_message(message);
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  memcpy(message->remote, client, message->remote_size);
  message->remote[message->remote_size] = 0x00;
  //Encode message
  OctopipesError err;
  uint8_t* data_out;
  size_t data_out_size;
  if ((err = octopipes_encode(message, &data_out, &data_out_size)) != OCTOPIPES_ERROR_SUCCESS) {
    octopipes_cleanup_message(message);
    return to_server_error(err);
  }
  octopipes_cleanup_message(message); //Clean message
  //Block CAP listener
  if ((rc = octopipes_server_lock_cap(server)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    return rc;
  }
  //Write message
  if ((err = pipe_send(server->cap_pipe, data_out, data_out_size, 5000)) != OCTOPIPES_ERROR_SUCCESS) {
    free(data_out);
    return to_server_error(err);
  }
  //Unlock pipe
  octopipes_server_unlock_cap(server);
  free(data_out);
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief process CAP once
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_cap_once(OctopipesServer* server, size_t* requests) {
  if (server->state != OCTOPIPES_SERVER_STATE_RUNNING) {
    return OCTOPIPES_SERVER_ERROR_UNINITIALIZED;
  }
  if (server->cap_inbox != NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  //Lock cap listener
  pthread_mutex_lock(&server->cap_lock);
  //Process
  *requests = 0;
  OctopipesServerError rc;
  if (server->cap_inbox->inbox_len > 0) {
    OctopipesMessage* message = server->cap_inbox->messages[0];
    //Process message
    if ((rc = octopipes_server_handle_cap_message(server, message)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
      return rc;
    }
    //Remove message from CAP
    if ((rc = message_inbox_remove(server->cap_inbox, 0)) != OCTOPIPES_SERVER_ERROR_SUCCESS) { //Remove first
      return rc;
    }
    *requests++;
  }
  //Unlock mutex
  pthread_mutex_unlock(&server->cap_lock);
  //Return OK
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief process all CAP messages in inbox
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_cap_all(OctopipesServer* server, size_t* requests) {
  if (server->state != OCTOPIPES_SERVER_STATE_RUNNING) {
    return OCTOPIPES_SERVER_ERROR_UNINITIALIZED;
  }
  if (server->cap_inbox != NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  *requests = 0;
  size_t this_session_requests = 0;
  do {
    //Process message
    OctopipesServerError rc;
    if ((rc = octopipes_server_process_cap_once(server, &this_session_requests)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
      return rc;
    }
    *requests += this_session_requests;
  } while(this_session_requests > 0); //While requests is > 0
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_handle_cap_message(OctopipesServer* server, const OctopipesMessage* message) {
  //TODO: handle
}

/**
 * @brief start a new worker with the provided parameters
 * @param OctopipesServer*
 * @param char* client
 * @param char** subscriptions
 * @param size_t subscription length
 * @param char* pipe rx
 * @param char* pipe tx
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_start_worker(OctopipesServer* server, const char* client, const char** subscriptions, const size_t subscription_len, const char* cli_tx_pipe, const char* cli_rx_pipe) {
  //Instance a new worker
  OctopipesServerWorker* new_worker;
  OctopipesServerError rc;
  //Check if a worker with that name exists
  //TODO:
  //Initialize a new worker
  if ((rc = worker_init(&new_worker, client, subscriptions, subscription_len, cli_tx_pipe, cli_tx_pipe)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    return rc;
  }
  //Push worker to current workers
  //TODO:
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_stop_worker(OctopipesServer* server, const char* client) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_dispatch_message(OctopipesServer* server, const OctopipesMessage* message) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_first(OctopipesServer* server, size_t* requests, char** client) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_once(OctopipesServer* server, size_t* requests, char** client) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_all(OctopipesServer* server, size_t* requests, char** client) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_is_subscribed(OctopipesServer* server, const char* client) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_get_subscriptions(OctopipesServer* server, const char* client, char*** subscriptions) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_get_clients(OctopipesServer* server, char*** clients) {
  
}

/**
 * @brief get description for server error
 * @param OctopipesServerError
 * @return const char*
 */

const char* octopipes_server_get_error_desc(const OctopipesServerError error) {

}

//Privates

/**
 * @brief initialize a server worker
 * @param OctopipesServerWorker**
 * @return OctopipesServerError
 */

OctopipesServerError worker_init(OctopipesServerWorker** worker, const char** subscriptions, const size_t sub_len, const char* client_id, const char* pipe_read, const char* pipe_write) {
  OctopipesServerWorker* ptr = (OctopipesServerWorker*) malloc(sizeof(OctopipesServerWorker));
  if (ptr == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  ptr->active = 0;
  ptr->client_id = NULL;
  ptr->inbox = NULL;
  ptr->pipe_read = NULL;
  ptr->pipe_write = NULL;
  ptr->subscriptions_list = NULL;
  ptr->subscriptions = 0;
  //Copy clid
  const size_t clid_len = strlen(client_id);
  ptr->client_id = (char*) malloc(sizeof(char) * (clid_len + 1));
  if (ptr->client_id == NULL) {
    goto worker_bad_alloc;
  }
  memcpy(ptr->client_id, client_id, clid_len);
  ptr->client_id[clid_len] = 0x00;
  //Copy pipe read
  const size_t piper_len = strlen(pipe_read);
  ptr->pipe_read = (char*) malloc(sizeof(char) * (piper_len + 1));
  if (ptr->pipe_read == NULL) {
    goto worker_bad_alloc;
  }
  memcpy(ptr->pipe_read, pipe_read, piper_len);
  ptr->pipe_read[piper_len] = 0x00;
  //Copy pipe write
  const size_t pipew_len = strlen(pipe_write);
  ptr->pipe_write = (char*) malloc(sizeof(char) * (pipew_len + 1));
  if (ptr->pipe_write == NULL) {
    goto worker_bad_alloc;
  }
  memcpy(ptr->pipe_write, pipe_write, pipew_len);
  ptr->pipe_write[pipew_len] = 0x00;
  //Copy subscription list
  ptr->subscriptions_list = (char**) malloc(sizeof(char*) * sub_len);
  if (ptr->subscriptions_list == NULL) {
    goto worker_bad_alloc;
  }
  for (size_t i = 0; i < sub_len; i++) {
    const size_t this_sub_len = strlen(subscriptions[i]);
    ptr->subscriptions_list[i] = (char*) malloc(sizeof(char) * (this_sub_len + 1));
    if (ptr->subscriptions_list[i] == NULL) {
      goto worker_bad_alloc;
    }
    memcpy(ptr->subscriptions_list[i], subscriptions[i], this_sub_len);
    (ptr->subscriptions_list[i])[this_sub_len] = 0x00;
    ptr->subscriptions++;
  }
  //Create mutex
  if (pthread_mutex_init(&ptr->worker_lock, NULL) != 0) {
    goto worker_thread_error;
  }
  //Start thread
  if (pthread_create(&ptr->worker_listener, NULL, worker_loop, ptr) != 0) {
    goto worker_thread_error;
  }
  return OCTOPIPES_SERVER_ERROR_SUCCESS;

worker_bad_alloc:
  if (ptr->client_id != NULL) {
    free(ptr->client_id);
  }
  if (ptr->inbox != NULL) {
    message_inbox_cleanup(ptr->inbox);
  }
  if (ptr->pipe_read != NULL) {
    free(ptr->pipe_read);
  }
  if (ptr->pipe_write != NULL) {
    free(ptr->pipe_write);
  }
  for (size_t i = 0; i < ptr->subscriptions; i++) {
    free(ptr->subscriptions_list[i]);
  }
  if (ptr->subscriptions_list != NULL) {
    free(ptr->subscriptions_list);
  }
  return OCTOPIPES_ERROR_BAD_ALLOC;
worker_thread_error:
  if (ptr->client_id != NULL) {
    free(ptr->client_id);
  }
  if (ptr->inbox != NULL) {
    message_inbox_cleanup(ptr->inbox);
  }
  if (ptr->pipe_read != NULL) {
    free(ptr->pipe_read);
  }
  if (ptr->pipe_write != NULL) {
    free(ptr->pipe_write);
  }
  for (size_t i = 0; i < ptr->subscriptions; i++) {
    free(ptr->subscriptions_list[i]);
  }
  if (ptr->subscriptions_list != NULL) {
    free(ptr->subscriptions_list);
  }
  return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
}

/**
 * @brief cleanup a server worker
 * @param OctopipesServerWorker*
 * @return OctopipesServerError
 */

OctopipesServerError worker_cleanup(OctopipesServerWorker* worker) {
  if (worker->active) {
    //Set worker active to false
    worker->active = 0;
    //Join listener
    if (pthread_join(worker->worker_listener, NULL) != 0) {
      return OCTOPIPES_SERVER_ERROR_THREAD_ERROR;
    }
    pthread_mutex_destroy(&worker->worker_lock);
  }
  free(worker->client_id);
  //Delete subscription list
  for (size_t i = 0; i < worker->subscriptions; i++) {
    char* sub = worker->subscriptions_list[i];
    free(sub);
  }
  free(worker->subscriptions_list);
  //Delete pipes
  free(worker->pipe_read);
  free(worker->pipe_write);
  //Free inbox
  OctopipesServerError rc;
  if ((rc =message_inbox_cleanup(worker->inbox)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    return rc;
  }
  //Eventually free worker and return
  free(worker);
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief initialize a message inbox
 * @param OctopipesServerInbox**
 * @return OctopipesServerError
 */

OctopipesServerError message_inbox_init(OctopipesServerInbox** inbox) {
  OctopipesServerInbox* ptr = (OctopipesServerInbox*) malloc(sizeof(OctopipesServerInbox));
  if (ptr == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  ptr->messages = NULL;
  ptr->inbox_len = 0;
  *inbox = ptr;
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief cleanup a message inbox
 * @param OctopipesServerInbox*
 * @return OctopipesServerError
 */

OctopipesServerError message_inbox_cleanup(OctopipesServerInbox* inbox) {
  OctopipesServerError err;
  if ((err = message_inbox_expunge(inbox)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    return err;
  }
  free(inbox);
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief expunge the server message inbox
 * @param OctopipesServerInbox*
 * @return OctopipesServerError
 */

OctopipesServerError message_inbox_expunge(OctopipesServerInbox* inbox) {
  for (size_t i = 0; i < inbox->inbox_len; i++) {
    OctopipesServerMessage* message = inbox->messages[i];
    //Cleanup message
    if (message->message != NULL) {
      octopipes_cleanup_message(message->message);
    }
    free(message);
  }
}

/**
 * @brief push a new message into the Inbox
 * @param OctopipesServerInbox**
 * @param size_t inbox len
 * @param OctopipesMessage* message to push (or NULL)
 * @param OctopipesServerError error to associate (can be success)
 * @return OctopipesServerError
 */

OctopipesServerError message_inbox_push(OctopipesServerInbox* inbox, OctopipesMessage* message, OctopipesServerError error) {
  //Reallocate inbox messages
  const size_t new_inbox_len = inbox->inbox_len + 1;
  //Instance new OctopipesServerMessage
  OctopipesServerMessage* new_message = (OctopipesServerMessage*) malloc(sizeof(OctopipesServerMessage));
  if (new_message == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  new_message->message = message;
  new_message->error = error;
  inbox->messages = (OctopipesServerMessage**) realloc(inbox->messages, sizeof(OctopipesServerMessage*) * new_inbox_len);
  if (inbox->messages == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  inbox->messages[inbox->inbox_len] = new_message;
  inbox->inbox_len++;
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief remove the message in position index
 * @param OctopipesServerInbox*
 * @param size_t index
 * @return OctopipesServerError
 */

OctopipesServerError message_inbox_remove(OctopipesServerInbox* inbox, const size_t index) {
  //Get the message in the provided position
  if (index >= inbox->inbox_len) {
    return OCTOPIPES_SERVER_ERROR_SUCCESS; //Out of range, just ignore the error
  }
  OctopipesServerMessage* target_message = inbox->messages[index];
  //Free message if not null
  if (target_message->message != NULL) {
    octopipes_cleanup_message(target_message->message);
  }
  //Free target
  free(target_message);
  //Move all the following messages one position behind
  const size_t new_inbox_len = inbox->inbox_len - 1;
  for (size_t i = index; i < new_inbox_len; i++) {
    inbox[i] = inbox[i + 1];
  }
  //Reallocate messages
  inbox->messages = (OctopipesServerMessage**) realloc(inbox->messages, sizeof(OctopipesServerMessage*) * new_inbox_len);
  if (inbox->messages == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  inbox->inbox_len--;
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief loop for CAP listener
 * @param void* args (pointer to server)
 * @return void*
 */

void* cap_loop(void* args) {
  //TODO:
  return NULL;
}

/**
 * @brief loop for worker listener
 * @param void* args (pointer to worker)
 * @return void*
 */

void* worker_loop(void* args) {
  //TODO:
  return NULL;
}


/**
 * @brief create and clean clients directory
 * @param char* directory
 * @return int
 */

int create_clients_dir(const char* directory) {

  struct stat st = {0};
  if (stat(directory, &st) == -1) {
    //Create dir
    if (mkdir(directory, 0755) != 0) {
      return 1;
    }
  } else {
    //Directory already exists, clean directory
    DIR* dir;
    struct dirent *entry;
    struct stat statbuf;
    if((dir = opendir(directory)) == NULL) {
        return 1;
    }
    while((entry = readdir(dir)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if (!S_ISFIFO(statbuf.st_mode)) {
            //Remove file
            unlink(entry->d_name);
        }
    }
    closedir(dir);
  }
  return 0;
}

OctopipesServerError to_server_error(const OctopipesError error) {
  //TODO: implement
}
