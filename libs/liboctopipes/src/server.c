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

#include <string.h>

//@! Privates
//Workers
OctopipesServerError worker_init(OctopipesServerWorker** worker);
OctopipesServerError worker_cleanup(OctopipesServerWorker* worker);
//Inbox
OctopipesServerError message_inbox_init(OctopipesServerInbox** inbox);
OctopipesServerError message_inbox_cleanup(OctopipesServerInbox* inbox);
OctopipesServerError message_inbox_expunge(OctopipesServerInbox* inbox);
OctopipesServerError message_inbox_push(OctopipesServerInbox* inbox, const size_t inbox_len, OctopipesMessage* message, OctopipesServerError error);
OctopipesServerError message_inbox_remove(OctopipesServerInbox* inbox, const size_t inbox_len, const size_t index);

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
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_start_cap_listener(OctopipesServer* server) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_stop_cap_listener(OctopipesServer* server) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_lock_cap(OctopipesServer* server) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_unlock_cap(OctopipesServer* server) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_write_cap(OctopipesServer* server, const char* client, const uint8_t* data, const size_t data_size) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_cap_once(OctopipesServer* server, size_t* requests) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_process_cap_all(OctopipesServer* server, size_t* requests) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_handle_cap_message(OctopipesServer* server, const OctopipesMessage* message) {
  
}

/**
 * @brief
 * @return OctopipesServerError
 */

OctopipesServerError octopipes_server_start_worker(OctopipesServer* server, const char* client, const char** subscriptions, const size_t subscription_len, const char* cli_tx_pipe, const char* cli_rx_pipe) {
  
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

OctopipesServerError worker_init(OctopipesServerWorker** worker) {
  //TODO:
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
  //TODO: implement
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

OctopipesServerError message_inbox_push(OctopipesServerInbox* inbox, const size_t inbox_len, OctopipesMessage* message, OctopipesServerError error) {
  //Reallocate inbox messages
  const size_t new_inbox_len = inbox_len + 1;
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
  inbox->messages[inbox_len] = new_message;
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief remove the message in position index
 * @param OctopipesServerInbox*
 * @param size_t index
 * @return OctopipesServerError
 */

OctopipesServerError message_inbox_remove(OctopipesServerInbox* inbox, const size_t inbox_len, const size_t index) {
  //Get the message in the provided position
  if (index >= inbox_len) {
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
  const size_t new_inbox_len = inbox_len - 1;
  for (size_t i = index; i < new_inbox_len; i++) {
    inbox[i] = inbox[i + 1];
  }
  //Reallocate messages
  inbox->messages = (OctopipesServerMessage**) realloc(inbox->messages, sizeof(OctopipesServerMessage*) * new_inbox_len);
  if (inbox->messages == NULL) {
    return OCTOPIPES_SERVER_ERROR_BAD_ALLOC;
  }
  return OCTOPIPES_SERVER_ERROR_SUCCESS;
}
