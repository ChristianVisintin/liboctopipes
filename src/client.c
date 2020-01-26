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
#include <unistd.h>

#define DEFAULT_TTL 60

//Private properties and functions
//Threads
void* octopipes_loop(void* args);

/**
 * @brief initializes a OctopipesClient instance, the client mustn't be allocated before call, the client_id and cap_path are copied, so must be freed by the user later
 * @param OctopipesClient
 * @param char* client id
 * @param char* common access pipe path
 * @param OctopipesVersion verso to use to communicate
 * @return OctopipesError
 */

OctopipesError octopipes_init(OctopipesClient** client, const char* client_id, const char* cap_path, const OctopipesVersion version) {
  *client = (OctopipesClient*) malloc(sizeof(OctopipesClient));
  if (*client == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  (*client)->client_id_size = strlen(client_id);
  (*client)->client_id = (char*) malloc(sizeof(char) * ((*client)->client_id_size + 1));
  if ((*client)->client_id == NULL) {
    free(*client);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  strcpy((*client)->client_id, client_id);
  (*client)->common_access_pipe = (char*) malloc(sizeof(char) * (strlen(cap_path) + 1));
  if ((*client)->common_access_pipe == NULL) {
    free((*client)->client_id);
    free(*client);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  strcpy((*client)->common_access_pipe, cap_path);
  (*client)->protocol_version = version;
  (*client)->rx_pipe = NULL;
  (*client)->tx_pipe = NULL;
  (*client)->state = OCTOPIPES_STATE_INIT;
  (*client)->on_received = NULL;
  (*client)->on_sent = NULL;
  (*client)->on_receive_error = NULL;
  (*client)->on_subscribed = NULL;
  (*client)->on_unsubscribed = NULL;
  (*client)->user_data = NULL;
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief free a octopipes client instance
 * @param OctopipesClient*
 * @return OctopipesError
 */

OctopipesError octopipes_cleanup(OctopipesClient* client) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_SUCCESS;
  }
  //Disconnect if not disconnected yet
  if (client->state == OCTOPIPES_STATE_SUBSCRIBED || client->state == OCTOPIPES_STATE_RUNNING) {
    octopipes_unsubscribe(client);
  }
  free(client->client_id);
  free(client->common_access_pipe);
  if (client->rx_pipe != NULL) {
    free(client->rx_pipe);
  }
  if (client->tx_pipe != NULL) {
    free(client->tx_pipe);
  }
  free(client);
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief cleanup an OctopipesMessage struct
 * @param OctopipesMessage*
 * @return OctopipesError
 */

OctopipesError octopipes_cleanup_message(OctopipesMessage* message) {
  if (message == NULL) {
    return OCTOPIPES_ERROR_SUCCESS;
  }
  if (message->origin_size > 0 && message->origin != NULL) {
    free(message->origin);
  }
  if (message->remote_size > 0 && message->remote != NULL) {
    free(message->remote);
  }
  if (message->data_size > 0 && message->data != NULL) {
    free(message->data);
  }
  free(message);
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief start the octopipes client loop thread
 * @param OctopipesClient
 * @return OctopipeError
 */

OctopipesError octopipes_loop_start(OctopipesClient* client) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  if (client->state != OCTOPIPES_STATE_SUBSCRIBED) {
    return OCTOPIPES_ERROR_NOT_SUBSCRIBED;
  }
  if(pthread_create(&client->loop, NULL, octopipes_loop, client) != 0) {
    return OCTOPIPES_ERROR_THREAD;
  }
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief stop octopipes client loop thread
 * @param OctopipesClient
 * @return OctopipesError
 */

OctopipesError octopipes_loop_stop(OctopipesClient* client) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  if (client->state != OCTOPIPES_STATE_UNSUBSCRIBED) {
    return OCTOPIPES_ERROR_NOT_UNSUBSCRIBED;
  }
  //Join thread
  client->state = OCTOPIPES_STATE_STOPPED;
  if (pthread_join(client->loop, NULL) != 0) {
    client->state = OCTOPIPES_STATE_UNSUBSCRIBED; //Set state back to UNSUBSCRIBED
    return OCTOPIPES_ERROR_THREAD;
  }
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief subscribe to Octopipe
 * @param OctopipesClient*
 * @param char** groups
 * @param size_t groups
 * @param OctopipesCapError assignment error
 * @return OctopipesError
 */

OctopipesError octopipes_subscribe(OctopipesClient* client, const char** groups, size_t groups_amount, OctopipesCapError* assignment_error) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  //Prepare packet
  OctopipesMessage* subscribe_message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  if (subscribe_message == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  subscribe_message->origin = NULL;
  subscribe_message->remote = NULL;
  subscribe_message->data = NULL;
  //Set origin as client
  subscribe_message->version = client->protocol_version;
  subscribe_message->origin_size = client->client_id_size;
  subscribe_message->origin = (char*) malloc(sizeof(char) * (client->client_id_size + 1));
  if (subscribe_message->origin == NULL) {
    octopipes_cleanup_message(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  memcpy(subscribe_message->origin, client->client_id, client->client_id_size);
  subscribe_message->origin[client->client_id_size] = 0x00;
  //Server is 0
  subscribe_message->remote_size = 0;
  subscribe_message->remote = NULL;
  //Options
  subscribe_message->ttl = DEFAULT_TTL;
  subscribe_message->options = OCTOPIPES_OPTIONS_NONE;
  //Data
  subscribe_message->data = octopipes_cap_prepare_subscription(groups, groups_amount, (size_t*) &subscribe_message->data_size);
  if (subscribe_message->data == NULL) {
    octopipes_cleanup_message(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Encode message
  size_t out_data_size;
  uint8_t* out_data;
  OctopipesError rc = octopipes_encode(subscribe_message, &out_data, &out_data_size);
  octopipes_cleanup_message(subscribe_message);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Send packet
  rc = pipe_send(client->common_access_pipe, out_data, out_data_size, 5000);
  free(out_data);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  usleep(100000);
  //If packet was sent successfully, then wait for assignment
  uint8_t* in_data;
  size_t in_data_size;
  rc = pipe_receive(client->common_access_pipe, &in_data, &in_data_size, 5000);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  OctopipesMessage* cap_message = NULL;
  rc = octopipes_decode(in_data, in_data_size, &cap_message);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    free(in_data);
    return rc;
  }
  free(in_data);
  //Verify remote
  if (cap_message->remote == NULL) {
    octopipes_cleanup_message(cap_message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (strcmp(cap_message->remote, client->client_id) != 0) {
    octopipes_cleanup_message(cap_message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Verify packet type
  OctopipesCapMessage message_type = octopipes_cap_get_message(cap_message->data, cap_message->data_size);
  if (message_type != OCTOPIPES_CAP_ASSIGNMENT) { //Return bad packet, the user must redo the operation
    octopipes_cleanup_message(cap_message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Stop running thread and free previous pipe
  if (client->state == OCTOPIPES_STATE_RUNNING) {
    octopipes_loop_stop(client);
  }
  if (client->state == OCTOPIPES_STATE_SUBSCRIBED || client->state == OCTOPIPES_STATE_RUNNING) {
    free(client->tx_pipe);
    free(client->rx_pipe);
  }
  //Parse assignment
  rc = octopipes_cap_parse_assign(cap_message->data, cap_message->data_size, assignment_error, &client->tx_pipe, &client->rx_pipe);
  if (rc == OCTOPIPES_ERROR_SUCCESS) {
    //Enter subscribed state
    client->state = OCTOPIPES_STATE_SUBSCRIBED;
    //Call on unsubscribed callback
    if (client->on_subscribed != NULL) {
      client->on_subscribed(client);
    }
  }
  //Free input data, and return rc
  octopipes_cleanup_message(cap_message);
  return rc;
}

/**
 * @brief unsubscribe from octopipe server
 * @param OctopipesClient
 * @return OctopipesError
 */

OctopipesError octopipes_unsubscribe(OctopipesClient* client) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  if (client->state != OCTOPIPES_STATE_SUBSCRIBED && client->state != OCTOPIPES_STATE_RUNNING) {
    return OCTOPIPES_ERROR_NOT_SUBSCRIBED;
  }
  OctopipesError rc;
  //Prepare message
  OctopipesMessage* subscribe_message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  subscribe_message->origin = NULL;
  subscribe_message->remote = NULL;
  subscribe_message->data = NULL;
  if (subscribe_message == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Set origin as client
  subscribe_message->version = client->protocol_version;
  subscribe_message->origin_size = client->client_id_size;
  subscribe_message->origin = (char*) malloc(sizeof(char) * (subscribe_message->origin_size + 1));
  if (subscribe_message->origin == NULL) {
    octopipes_cleanup_message(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  if (subscribe_message->origin == NULL) {
    octopipes_cleanup_message(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  memcpy(subscribe_message->origin, client->client_id, subscribe_message->origin_size);
  subscribe_message->origin[subscribe_message->origin_size] = 0x00;
  //Server is 0
  subscribe_message->remote_size = 0;
  subscribe_message->remote = NULL;
  //Options
  subscribe_message->ttl = DEFAULT_TTL;
  subscribe_message->options = OCTOPIPES_OPTIONS_NONE;
  //Data
  subscribe_message->data = octopipes_cap_prepare_unsubscription((size_t*) &subscribe_message->data_size);
  if (subscribe_message->data == NULL) {
    octopipes_cleanup_message(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Encode message
  size_t out_data_size;
  uint8_t* out_data;
  rc = octopipes_encode(subscribe_message, &out_data, &out_data_size);
  octopipes_cleanup_message(subscribe_message);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Send packet
  rc = pipe_send(client->common_access_pipe, out_data, out_data_size, 5000);
  free(out_data);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Stop loop
  octopipes_loop_stop(client);
  //Call on unsubscribed callback
  if (client->on_unsubscribed != NULL) {
    client->on_unsubscribed(client);
  }
  //Set state to unsubscribed
  if (rc == OCTOPIPES_ERROR_SUCCESS) {
    client->state = OCTOPIPES_STATE_UNSUBSCRIBED;
  }
  return rc;
}

/**
 * @brief send a packet to remote
 * @param OctopipesClient* client
 * @param char* remote node
 * @param void* data
 * @param uint64_t data size
 * @return OctopipesError
 */

OctopipesError octopipes_send(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size) {
  return octopipes_send_ex(client, remote, data, data_size, DEFAULT_TTL, OCTOPIPES_OPTIONS_NONE);
}

/**
 * @brief send a packet to remote
 * @param OctopipesClient* client
 * @param char* remote node
 * @param void* data
 * @param uint64_t data size
 * @param uint8_t ttl
 * @param OctopipesOptions options
 * @return OctopipesError
 */

OctopipesError octopipes_send_ex(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size, const uint8_t ttl, const OctopipesOptions options) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  //Check if state is running or subscribed
  if (client->state != OCTOPIPES_STATE_RUNNING && client->state != OCTOPIPES_STATE_SUBSCRIBED) {
    return OCTOPIPES_ERROR_NOT_SUBSCRIBED;
  }
  //Prepare packet
  OctopipesMessage* message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  if (message == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  message->origin = NULL;
  message->remote = NULL;
  message->data = NULL;
  //Fill message structure
  message->version = client->protocol_version;
  message->origin_size = client->client_id_size;
  message->origin = (char*) malloc(sizeof(char) * (message->origin_size + 1));
  if (message->origin == NULL) {
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  memcpy(message->origin, client->client_id, message->origin_size);
  message->origin[message->origin_size] = 0x00;
  message->remote_size = strlen(remote);
  message->remote = (char*) malloc(sizeof(char) * (message->remote_size + 1));
  if (message->remote == NULL) {
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  memcpy(message->remote, remote, message->remote_size);
  message->origin[message->remote_size] = 0x00;
  message->options = options;
  message->ttl = ttl;
  message->data_size = data_size;
  message->data = (uint8_t*) malloc(sizeof(uint8_t) * message->data_size);
  if (message->data == NULL) {
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  memcpy(message->data, (uint8_t*) data, message->data_size);
  //Encode message
  size_t out_data_size;
  uint8_t* out_data;
  OctopipesError rc = octopipes_encode(message, &out_data, &out_data_size);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    octopipes_cleanup_message(message);
    return rc;
  }
  //Write to FIFO
  rc = pipe_send(client->tx_pipe, out_data, out_data_size, ttl * 1000);
  //Call on sent callback if necessary
  if (rc == OCTOPIPES_ERROR_SUCCESS && client->on_sent != NULL) {
    client->on_sent(client, message);
  }
  //Call on sent
  octopipes_cleanup_message(message);
  free(out_data);
  return rc;
}

/**
 * @brief set the function to call when a message is received by the octopipes client
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesClient* client, const OctopipesMessage*)) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  client->on_received = on_received;
  return OCTOPIPES_ERROR_SUCCESS; 
}

/**
 * @brief set the function to call when a message is successfully sent to the octopipes client
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_sent_cb(OctopipesClient* client, void (*on_sent)(const OctopipesClient* client, const OctopipesMessage*)) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  client->on_sent = on_sent;
  return OCTOPIPES_ERROR_SUCCESS; 
}

/**
 * @brief set the function to call when an error is returned during the receive of a message
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_receive_error_cb(OctopipesClient* client, void (*on_receive_error)(const OctopipesClient* client, const OctopipesError)) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  client->on_receive_error = on_receive_error;
  return OCTOPIPES_ERROR_SUCCESS; 
}

/**
 * @brief set the function to call when the client subscribe to the server
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)(const OctopipesClient* client)) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  client->on_subscribed = on_subscribed;
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief set the function to call when the client unsubscribe from the server
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)(const OctopipesClient* client)) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  client->on_unsubscribed = on_unsubscribed;
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief get error description from the provided error code
 * @param OctopipesError
 * @return const char*
 */

const char* octopipes_get_error_desc(const OctopipesError error) {
  switch (error) {
    case OCTOPIPES_ERROR_BAD_ALLOC:
      return "Could not allocate more memory in the heap";
    case OCTOPIPES_ERROR_BAD_CHECKSUM:
      return "The last packet has a bad checksum and the ignore checksum flag is set to 0";
    case OCTOPIPES_ERROR_BAD_PACKET:
      return "The packet syntax is invalid or is unexpected";
    case OCTOPIPES_ERROR_CAP_TIMEOUT:
      return "The CAP timeout. The request hasn't been fulfilled in time.";
    case OCTOPIPES_ERROR_NO_DATA_AVAILABLE:
      return "There's no data available to be read";
    case OCTOPIPES_ERROR_NOT_SUBSCRIBED:
      return "The client is not subscribed yet to Octopipes server. Sending of messages to server is allowed only using the CAP.";
    case OCTOPIPES_ERROR_NOT_UNSUBSCRIBED:
      return "This operation is not permitted, since the client isn't unsubscribed";
    case OCTOPIPES_ERROR_OPEN_FAILED:
      return "Could not open the FIFO";
    case OCTOPIPES_ERROR_READ_FAILED:
      return "An error occurred while trying to read from FIFO";
    case OCTOPIPES_ERROR_SUCCESS:
      return "Not an error";
    case OCTOPIPES_ERROR_THREAD:
      return "Could not start loop thread";
    case OCTOPIPES_ERROR_UNINITIALIZED:
      return "The OctopipesClient must be initialized calling octopipes_init() first";
    case OCTOPIPES_ERROR_UNSUPPORTED_VERSION:
      return "This protocol version is unsupported by Octopipes Version " OCTOPIPES_LIB_VERSION;
    case OCTOPIPES_ERROR_WRITE_FAILED:
      return "Could not write data to FIFO";
    case OCTOPIPES_ERROR_UNKNOWN_ERROR:
    default:
      return "Unknown error";
  }
}

//Internal functions

/**
 * @brief thread loop functions for octopipes client daemon thread
 * @param OctopipesClient*
 */

void* octopipes_loop(void* args) {
  OctopipesClient* client = (OctopipesClient*) args;
  client->state = OCTOPIPES_STATE_RUNNING;
  while (client->state == OCTOPIPES_STATE_RUNNING) {
    //Check if there are available messages to be read
    uint8_t* data_in;
    size_t data_in_size;
    OctopipesError rc = pipe_receive(client->rx_pipe, &data_in, &data_in_size, 500); //500 ms
    if (rc == OCTOPIPES_ERROR_SUCCESS) {
      //Parse data
      OctopipesMessage* message;
      if ((rc = octopipes_decode(data_in, data_in_size, &message)) == OCTOPIPES_ERROR_SUCCESS) {
        //Decoding was successful, report received message
        if (client->on_received != NULL) {
          client->on_received(client, message); //@! Success
        }
        //If RCK, send ACK
        if ((message->options & OCTOPIPES_OPTIONS_REQUIRE_ACK) != 0) {
          //Prepare and send ACK message
          octopipes_send_ex(client, message->origin, NULL, 0, 255, OCTOPIPES_OPTIONS_ACK);
        }
        octopipes_cleanup_message(message);
      } else {
        //@! Report error
        if (client->on_receive_error != NULL) {
          client->on_receive_error(client, rc);
        }
      }
      //Free data
      free(data_in);
    } else if (rc == OCTOPIPES_ERROR_NO_DATA_AVAILABLE) {
      //It's ok, just keep waiting
    } else {
      //@! Report error
      if (client->on_receive_error != NULL) {
        client->on_receive_error(client, rc);
      }
    }
    //Nothing else to do
  }
  return NULL;
}
