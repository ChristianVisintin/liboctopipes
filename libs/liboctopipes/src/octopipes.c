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

#include <string.h>

#define SOH 0x01
#define STX 0x02
#define ETX 0x03

#define DEFAULT_TTL 255

//Private properties and functions
//Threads
void octopipes_loop(OctopipesClient* client);
//Encoding/decoding
OctopipesError octopipes_decode(const uint8_t* data, const size_t data_size, OctopipesMessage** message);
OctopipesError octopipes_encode(OctopipesMessage* message, uint8_t** data, size_t* data_size);
uint8_t calculate_checksum(const OctopipesMessage* message);
//I/O
OctopipesError fifo_receive(const char* fifo, uint8_t** data, size_t* data_size);
OctopipesError fifo_send(const char* fifo, const uint8_t* data, const size_t data_size);

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
  (*client)->on_subscribed = NULL;
  (*client)->on_unsubscribed = NULL;
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
  if (client->state == OCTOPIPES_STATE_SUBSCRIBED) {
    OctopipesError rc;
    if ((rc = octopipes_unsubscribe(client)) != OCTOPIPES_ERROR_SUCCESS) {
      return rc;
    }
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
  client->state = OCTOPIPES_STATE_RUNNING;
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
    client->state = OCTOPIPES_STATE_UNSUBSCRIBED;
  }
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief subscribe to Octopipe
 * @param OctopipesClient*
 * @param char** groups
 * @param size_t groups
 * @return OctopipesError
 */

OctopipesError octopipes_subscribe(OctopipesClient* client, const char** groups, size_t groups_amount) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  //Prepare packet
  OctopipesMessage* subscribe_message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  if (subscribe_message == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Set origin as client
  subscribe_message->version = client->protocol_version;
  subscribe_message->origin_size = client->client_id_size;
  subscribe_message->origin = client->client_id;
  //Server is 0
  subscribe_message->remote_size = 0;
  subscribe_message->remote = NULL;
  //Options
  subscribe_message->ttl = DEFAULT_TTL;
  subscribe_message->options = OCTOPIPES_OPTIONS_NONE;
  //Data
  subscribe_message->data = octopipes_cap_prepare_subscribe(groups, groups_amount, &subscribe_message->data_size);
  if (subscribe_message->data == NULL) {
    free(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Encode message
  size_t out_data_size;
  uint8_t* out_data;
  OctopipesError rc = octopipes_encode(subscribe_message, &out_data, out_data_size);
  free(subscribe_message->data);
  free(subscribe_message);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Send packet
  rc = fifo_send(client->common_access_pipe, out_data, out_data_size);
  free(out_data);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //If packet was sent successfully, then wait for assignment
  uint8_t* in_data;
  size_t in_data_size;
  rc = fifo_receive(client->common_access_pipe, &in_data, &in_data_size);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Parse assignment
  rc = octopipes_cap_parse_assign(in_data, in_data_size, &client->tx_pipe, &client->rx_pipe);
  if (rc == OCTOPIPES_ERROR_SUCCESS) {
    //Enter subscribed state
    client->state = OCTOPIPES_STATE_SUBSCRIBED;
  }
  //Free input data, and return rc
  free(in_data);
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
  if (client->state != OCTOPIPES_STATE_SUBSCRIBED) {
    return OCTOPIPES_ERROR_NOT_SUBSCRIBED;
  }
  OctopipesError rc;
  //Prepare message
  OctopipesMessage* subscribe_message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  if (subscribe_message == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Set origin as client
  subscribe_message->version = client->protocol_version;
  subscribe_message->origin_size = client->client_id_size;
  subscribe_message->origin = client->client_id;
  //Server is 0
  subscribe_message->remote_size = 0;
  subscribe_message->remote = NULL;
  //Options
  subscribe_message->ttl = DEFAULT_TTL;
  subscribe_message->options = OCTOPIPES_OPTIONS_NONE;
  //Data
  subscribe_message->data = octopipes_cap_prepare_unsubscribe(&subscribe_message->data_size);
  if (subscribe_message->data == NULL) {
    free(subscribe_message);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  //Encode message
  size_t out_data_size;
  uint8_t* out_data;
  OctopipesError rc = octopipes_encode(subscribe_message, &out_data, &out_data_size);
  free(subscribe_message->data);
  free(subscribe_message);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Send packet
  rc = fifo_send(client->common_access_pipe, out_data, out_data_size);
  free(out_data);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
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
  if (client != NULL) {
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
  //Fill message structure
  message->version = client->protocol_version;
  message->origin = client->client_id;
  message->origin_size = client->client_id_size;
  message->remote = remote;
  message->remote_size = strlen(remote);
  message->options = options;
  message->ttl = ttl;
  message->data = data;
  message->data_size = data_size;
  //Encode message
  size_t out_data_size;
  uint8_t* out_data;
  OctopipesError rc = octopipes_encode(message, &out_data, &out_data_size);
  free(message);
  if (rc != OCTOPIPES_ERROR_SUCCESS) {
    return rc;
  }
  //Write to FIFO
  rc = fifo_send(client->tx_pipe, out_data, out_data_size);
  free(out_data);
  return rc;
}

/**
 * @brief set the function to call when a message is received by the octopipes client
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesMessage*)) {
  if (client == NULL) {
    return OCTOPIPES_ERROR_UNINITIALIZED;
  }
  client->on_received = on_received;
  return OCTOPIPES_ERROR_SUCCESS; 
}

/**
 * @brief set the function to call when the client subscribe to the server
 * @param OctopipesClient*
 * @param function
 * @return OctopipesError
 */

OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)()) {
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

OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)()) {
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
  //TODO: implement
}

//Internal functions

/**
 * @brief thread loop functions for octopipes client daemon thread
 * @param OctopipesClient*
 */

void octopipes_loop(OctopipesClient* client) {
  //TODO: implement
}

/**
 * @brief decode an octopipe message
 * @param uint8_t* data read from FIFO
 * @param size_t data size
 * @param OctopipesMessage**
 * @return OctopipesError
 */

OctopipesError octopipes_decode(const uint8_t* data, const size_t data_size, OctopipesMessage** message) {
  (*message) = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  size_t current_minimum_size = 17; //Minimum packet size counting static sizes
  size_t data_ptr = 0;
  OctopipesMessage* message_ptr = *message;
  if (message_ptr == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  }
  message_ptr->data = NULL;
  message_ptr->origin = NULL;
  message_ptr->remote = NULL;
  if (data_size < current_minimum_size) { //Minimum packet size
    goto decode_bad_packet;
  }
  if (data[0] != SOH || data[data_size - 1] != ETX) { //Check first and last byte
    goto decode_bad_packet;
  }
  message_ptr->version = (OctopipesVersion) data[1];
  if (message_ptr->version == OCTOPIPES_VERSION_1) { //Octopipes version 1
    //Origin and remote
    message_ptr->origin_size = data[2];
    current_minimum_size += message_ptr->origin_size;
    //Check size for origin
    if (data_size < current_minimum_size) {
      goto decode_bad_packet;
    }
    //Allocate origin size
    data_ptr = 3; //next position after SOH + VERSION + LNS
    message_ptr->origin = (char*) malloc(sizeof(char) * message_ptr->origin_size + 1); //+1 null terminator
    if (message_ptr->origin == NULL) {
      goto decode_bad_alloc;
    }
    memcpy(message_ptr->origin, data + data_ptr, message_ptr->origin_size);
    message_ptr->origin[message_ptr->origin_size] = 0x00;
    data_ptr += message_ptr->origin_size;
    //Remote
    message_ptr->remote_size = data[data_ptr];
    current_minimum_size += message_ptr->remote_size;
    if (data_size < current_minimum_size) {
      goto decode_bad_packet;
    }
    data_ptr++;
    message_ptr->remote = (char*) malloc(sizeof(char) * message_ptr->remote_size + 1);
    if (message_ptr->remote == NULL) {
      goto decode_bad_alloc;
    }
    memcpy(message_ptr->remote, data + data_ptr, message_ptr->remote_size);
    message_ptr->remote[message_ptr->remote_size] = 0x00;
    data_ptr += message_ptr->remote_size;
    //TTL
    message_ptr->ttl = data[data_ptr++];
    //Data size
    message_ptr->data_size = data[data_ptr++]; // 0
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 1
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 2
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 3
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 4
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 5
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 6
    message_ptr->data_size = message_ptr->data_size << 8;
    message_ptr->data_size += data[data_ptr++]; // 7
    //Options
    message_ptr->options = data[data_ptr++];
    //Checksum
    message_ptr->checksum = data[data_ptr++];
    //Verify STX
    if (data[data_ptr] != STX) {
      goto decode_bad_packet;
    }
    //Read data
    current_minimum_size += message_ptr->data_size;
    if (data_size < message_ptr->data_size) {
      goto decode_bad_packet;
    }
    if (message_ptr->data_size > 0) {
      message_ptr->data = (uint8_t*) malloc(sizeof(uint8_t) * message_ptr->data_size);
      if (message_ptr->data == NULL) {
        goto decode_bad_alloc;
      }
      memcpy(message_ptr->data, data + data_ptr, message_ptr->data_size);
    }
    //Verify checksum if required
    if (message_ptr->options & OCTOPIPES_OPTIONS_IGNORE_CHECKSUM != 0) {
      uint8_t verification_checksum = calculate_checksum(message_ptr);
      if (verification_checksum != message_ptr->checksum) {
        goto decode_bad_checksum;
      }
    }
    //@! Decoding OK
  } else { //Unsupported protocol version
    free(message_ptr);
    return OCTOPIPES_ERROR_UNSUPPORTED_VERSION;
  }
  return OCTOPIPES_ERROR_SUCCESS;

decode_bad_packet:
  if (message_ptr->origin != NULL) {
    free(message_ptr->origin);
  }
  if (message_ptr->remote != NULL) {
    free(message_ptr->remote);
  }
  if (message_ptr->data != NULL) {
    free(message_ptr->data);
  }
  free(message_ptr);
  return OCTOPIPES_ERROR_BAD_PACKET;

decode_bad_alloc:
  if (message_ptr->origin != NULL) {
    free(message_ptr->origin);
  }
  if (message_ptr->remote != NULL) {
    free(message_ptr->remote);
  }
  if (message_ptr->data != NULL) {
    free(message_ptr->data);
  }
  free(message_ptr);
  return OCTOPIPES_ERROR_BAD_ALLOC;

decode_bad_checksum:
  if (message_ptr->origin != NULL) {
    free(message_ptr->origin);
  }
  if (message_ptr->remote != NULL) {
    free(message_ptr->remote);
  }
  if (message_ptr->data != NULL) {
    free(message_ptr->data);
  }
  free(message_ptr);
  return OCTOPIPES_ERROR_BAD_CHECKSUM;
}

/**
 * @brief encode an OctopipeMessage structure into the buffer to write to the FIFO
 * @param OctopipesMessage* message structure
 * @param size_t* data size of out buffer
 * @return uint8_t*
 */

OctopipesError octopipes_encode(OctopipesMessage* message, uint8_t** data, size_t* data_size) {
  //Prepare encoding
  uint8_t* out_data;
  if (message->version == OCTOPIPES_VERSION_1) {
    //Calculate required size
    *data_size = 17; //Minimum size
    *data_size += message->remote_size;
    *data_size += message->origin_size;
    *data_size += message->data_size;
    out_data = (uint8_t*) malloc(sizeof(uint8_t) * *data_size);
    if (out_data == NULL) {
      return OCTOPIPES_ERROR_BAD_ALLOC;
    }
    size_t data_ptr = 0;
    //SOH
    data[data_ptr++] = SOH;
    //Version
    data[data_ptr++] = message->version;
    //Origin / origin_size
    data[data_ptr++] = message->origin_size;
    memcpy(data + data_ptr, message->origin, message->origin_size);
    data_ptr += message->origin_size;
    //Remote / remote size
    data[data_ptr++] = message->remote_size;
    memcpy(data + data_ptr, message->remote, message->remote_size);
    data_ptr += message->remote_size;
    //TTL
    data[data_ptr++] = message->ttl;
    //Data size
    data[data_ptr++] = (message->data_size >> 56) & 0xFF;
    data[data_ptr++] = (message->data_size >> 48) & 0xFF;
    data[data_ptr++] = (message->data_size >> 40) & 0xFF;
    data[data_ptr++] = (message->data_size >> 32) & 0xFF;
    data[data_ptr++] = (message->data_size >> 24) & 0xFF;
    data[data_ptr++] = (message->data_size >> 16) & 0xFF;
    data[data_ptr++] = (message->data_size >> 8) & 0xFF;
    data[data_ptr++] = message->data_size & 0xFF;
    //Options
    data[data_ptr++] = message->options;
    //Keep position of checksum, and go ahead
    size_t checksum_ptr = data_ptr;
    data_ptr++;
    //STX
    data[data_ptr++] = STX;
    //Write data
    memcpy(data + data_ptr, message->data, message->data_size);
    //Write ETX
    data[data_ptr++] = ETX;
    //Checksum as last thing
    message->checksum = 0;
    if (message->options & OCTOPIPES_OPTIONS_IGNORE_CHECKSUM != 0) {
      message->checksum = calculate_checksum(message);
    }
    data[checksum_ptr] = message->checksum;
  } else {
    return OCTOPIPES_ERROR_UNSUPPORTED_VERSION;
  }
  //Assign out data to data
  *data = out_data;
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief calculate checksum for message
 * @param OctopipesMessage*
 * @return uint8_t checksum
 */

uint8_t calculate_checksum(const OctopipesMessage* message) {
  uint8_t checksum = SOH; //Start with SOH
  if (message->version == OCTOPIPES_VERSION_1) {
    checksum = checksum ^ message->version;
    checksum = checksum ^ message->origin_size;
    for (size_t i = 0; i < message->origin_size; i++) {
      checksum = checksum ^ message->origin[i];
    }
    checksum = checksum ^ message->remote_size;
    for (size_t i = 0; i < message->remote_size; i++) {
      checksum = checksum ^ message->remote[i];
    }
    checksum = checksum ^ message->ttl;
    //Data size
    checksum = checksum ^ (message->data_size >> 56) & 0xFF;
    checksum = checksum ^ (message->data_size >> 48) & 0xFF;
    checksum = checksum ^ (message->data_size >> 40) & 0xFF;
    checksum = checksum ^ (message->data_size >> 32) & 0xFF;
    checksum = checksum ^ (message->data_size >> 24) & 0xFF;
    checksum = checksum ^ (message->data_size >> 16) & 0xFF;
    checksum = checksum ^ (message->data_size >> 8) & 0xFF;
    checksum = checksum ^ message->data_size & 0xFF;
    checksum = checksum ^ message->options;
    for (size_t i = 0; i < message->data_size; i++) {
      checksum = checksum ^ message->data[i];
    }
  }
  checksum = checksum ^ ETX; //Eventually xor with etx
  return checksum;  
}

OctopipesError fifo_receive(const char* fifo, uint8_t** data, size_t* data_size) {
  //TODO: implement
}

OctopipesError fifo_send(const char* fifo, const uint8_t* data, const size_t data_size) {
  //TODO: implement
}
