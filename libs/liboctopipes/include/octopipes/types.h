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

typedef enum OctopipesError {
  OCTOPIPES_ERROR_SUCCESS,
  OCTOPIPES_ERROR_UNINITIALIZED,
  OCTOPIPES_ERROR_THREAD,
  OCTOPIPES_ERROR_WRITE_FAILED,
  OCTOPIPES_ERROR_READ_FAILED,
  OCTOPIPES_ERROR_NOT_SUBSCRIBED,
  OCTOPIPES_ERROR_NOT_UNSUBSCRIBED,
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

typedef enum OctopipesOptions {
  OCTOPIPES_OPTIONS_NONE = 0,
  OCTOPIPES_OPTIONS_REQUIRE_ACK = 1,
  OCTOPIPES_OPTIONS_ACK = 2,
  OCTOPIPES_OPTIONS_IGNORE_CHECKSUM = 4
} OctopipesOptions;

typedef enum OctopipesVersion {
  OCTOPIPES_VERSION_1 = 1
} OctopipesVersion;

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
  //Pipes paths
  char* common_access_pipe;
  char* tx_pipe;
  char* rx_pipe;
  //Callbacks
  void (*on_received)(const OctopipesMessage*);
  void (*on_subscribed)();
  void (*on_unsubscribed)();
} OctopipesClient;

typedef enum OctopipesCapMessage {
  OCTOPIPES_CAP_SUBSCRIBE = 0x01,
  OCTOPIPES_CAP_ASSIGNMENT = 0xFF,
  OCTOPIPES_CAP_UNSUBSCRIBE = 0x02
} OctopipesCapMessage;

#endif
