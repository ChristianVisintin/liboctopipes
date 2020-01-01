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

#ifndef OCTOPIPES_TYPES_H
#define OCTOPIPES_TYPES_H

#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OctopipesError {
  OCTOPIPES_ERROR_SUCCESS,
  OCTOPIPES_ERROR_UNINITIALIZED,
  OCTOPIPES_ERROR_BAD_PACKET,
  OCTOPIPES_ERROR_BAD_CHECKSUM,
  OCTOPIPES_ERROR_UNSUPPORTED_VERSION,
  OCTOPIPES_ERROR_NO_DATA_AVAILABLE,
  OCTOPIPES_ERROR_OPEN_FAILED,
  OCTOPIPES_ERROR_WRITE_FAILED,
  OCTOPIPES_ERROR_READ_FAILED,
  OCTOPIPES_ERROR_CAP_TIMEOUT,
  OCTOPIPES_ERROR_NOT_SUBSCRIBED,
  OCTOPIPES_ERROR_NOT_UNSUBSCRIBED,
  OCTOPIPES_ERROR_THREAD,
  OCTOPIPES_ERROR_BAD_ALLOC,
  OCTOPIPES_ERROR_UNKNOWN_ERROR
} OctopipesError;

typedef enum OctopipesState {
  OCTOPIPES_STATE_INIT,
  OCTOPIPES_STATE_SUBSCRIBED,
  OCTOPIPES_STATE_RUNNING,
  OCTOPIPES_STATE_UNSUBSCRIBED,
  OCTOPIPES_STATE_STOPPED
} OctopipesState;

typedef enum OctopipesServerState {
  OCTOPIPES_SERVER_STATE_INIT,
  OCTOPIPES_SERVER_STATE_RUNNING,
  OCTOPIPES_SERVER_STATE_BLOCK,
  OCTOPIPES_SERVER_STATE_STOPPED
} OctopipesServerState;

typedef enum OctopipesOptions {
  OCTOPIPES_OPTIONS_NONE = 0,
  OCTOPIPES_OPTIONS_REQUIRE_ACK = 1,
  OCTOPIPES_OPTIONS_ACK = 2,
  OCTOPIPES_OPTIONS_IGNORE_CHECKSUM = 4
} OctopipesOptions;

typedef enum OctopipesVersion {
  OCTOPIPES_VERSION_1 = 1
} OctopipesVersion;

typedef enum OctopipesCapMessage {
  OCTOPIPES_CAP_UNKNOWN = 0x00,
  OCTOPIPES_CAP_SUBSCRIPTION = 0x01,
  OCTOPIPES_CAP_ASSIGNMENT = 0xFF,
  OCTOPIPES_CAP_UNSUBSCRIPTION = 0x02
} OctopipesCapMessage;

typedef enum OctopipesCapError {
  OCTOPIPES_CAP_ERROR_SUCCESS = 0,
  OCTOPIPES_CAP_ERROR_NAME_ALREADY_TAKEN = 1,
  OCTOPIPES_CAP_ERROR_FS = 2
} OctopipesCapError;

typedef struct OctopipesMessage {
  OctopipesVersion version;
  uint8_t origin_size;
  char* origin;
  uint8_t remote_size;
  char* remote;
  uint8_t ttl;
  uint64_t data_size;
  OctopipesOptions options;
  uint8_t checksum;
  uint8_t* data;
} OctopipesMessage;

typedef struct OctopipesClient {
  //State
  OctopipesState state;
  //Thread
  pthread_t loop;
  //Client parameters
  size_t client_id_size;
  char* client_id;
  OctopipesVersion protocol_version;
  //Pipes paths
  char* common_access_pipe;
  char* tx_pipe;
  char* rx_pipe;
  //Callbacks
  void (*on_received)(const struct OctopipesClient* client, const OctopipesMessage*);
  void (*on_sent)(const struct OctopipesClient* client, const OctopipesMessage*);
  void (*on_receive_error)(const struct OctopipesClient* client, const OctopipesError);
  void (*on_subscribed)(const struct OctopipesClient* client);
  void (*on_unsubscribed)(const struct OctopipesClient* client);
  //Extra - can be used to store anything NOTE: must be freed by the user
  void* user_data;
} OctopipesClient;

//@! Server

typedef enum OctopipesServerError {
  OCTOPIPES_SERVER_ERROR_SUCCESS,
  OCTOPIPES_SERVER_ERROR_BAD_ALLOC,
  OCTOPIPES_SERVER_ERROR_UNINITIALIZED,
  OCTOPIPES_SERVER_ERROR_BAD_PACKET,
  OCTOPIPES_SERVER_ERROR_BAD_CHECKSUM,
  OCTOPIPES_SERVER_ERROR_UNSUPPORTED_VERSION,
  OCTOPIPES_SERVER_ERROR_OPEN_FAILED,
  OCTOPIPES_SERVER_ERROR_WRITE_FAILED,
  OCTOPIPES_SERVER_ERROR_READ_FAILED,
  OCTOPIPES_SERVER_ERROR_CAP_TIMEOUT,
  OCTOPIPES_SERVER_ERROR_THREAD_ERROR,
  OCTOPIPES_SERVER_ERROR_THREAD_ALREADY_RUNNING,
  OCTOPIPES_SERVER_ERROR_WORKER_EXISTS,
  OCTOPIPES_SERVER_ERROR_WORKER_NOT_FOUND,
  OCTOPIPES_SERVER_ERROR_WORKER_ALREADY_RUNNING,
  OCTOPIPES_SERVER_ERROR_WORKER_NOT_RUNNING,
  OCTOPIPES_SERVER_ERROR_NO_RECIPIENT,
  OCTOPIPES_SERVER_ERROR_BAD_CLIENT_DIR,
  OCTOPIPES_SERVER_ERROR_NOT_SUBSCRIBED
} OctopipesServerError;

typedef struct OctopipesServerMessage {
  OctopipesMessage* message;
  OctopipesServerError error;
} OctopipesServerMessage;

typedef struct OctopipesServerInbox {
  OctopipesServerMessage** messages;
  size_t inbox_len;
} OctopipesServerInbox;

typedef struct OctopipesServerWorker {
  char* client_id;
  char** subscriptions_list;
  size_t subscriptions;
  //Pipes
  char* pipe_read;
  char* pipe_write;
  //Thread stuff
  pthread_t worker_listener;
  int active;
  OctopipesServerInbox* inbox;
} OctopipesServerWorker;

typedef struct OctopipesServer {
  //Version
  OctopipesVersion version;
  OctopipesServerState state;
  //Pipe
  char* cap_pipe;
  char* client_folder;
  //Thread
  pthread_t cap_listener;
  OctopipesServerInbox* cap_inbox;
  //Workers
  OctopipesServerWorker** workers;
  size_t workers_len;
} OctopipesServer;

#ifdef __cplusplus
}
#endif

#endif
