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

#include <octopipes/serializer.h>

#include <string.h>

#define SOH 0x01
#define STX 0x02
#define ETX 0x03

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
  if (data_size < current_minimum_size || data == NULL) { //Minimum packet size
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
    if (message_ptr->origin_size > 0) {
      message_ptr->origin = (char*) malloc(sizeof(char) * message_ptr->origin_size + 1); //+1 null terminator
      if (message_ptr->origin == NULL) {
        goto decode_bad_alloc;
      }
      memcpy(message_ptr->origin, data + data_ptr, message_ptr->origin_size);
      message_ptr->origin[message_ptr->origin_size] = 0x00;
      data_ptr += message_ptr->origin_size;
    } else {
      message_ptr->origin = NULL;
    }
    //Remote
    message_ptr->remote_size = data[data_ptr];
    current_minimum_size += message_ptr->remote_size;
    if (data_size < current_minimum_size) {
      goto decode_bad_packet;
    }
    data_ptr++;
    if (message_ptr->remote_size > 0) {
      message_ptr->remote = (char*) malloc(sizeof(char) * message_ptr->remote_size + 1);
      if (message_ptr->remote == NULL) {
        goto decode_bad_alloc;
      }
      memcpy(message_ptr->remote, data + data_ptr, message_ptr->remote_size);
      message_ptr->remote[message_ptr->remote_size] = 0x00;
    } else {
      message_ptr->remote = NULL;
    }
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
    if (data[data_ptr++] != STX) {
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
    if ((message_ptr->options & OCTOPIPES_OPTIONS_IGNORE_CHECKSUM) == 0) {
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
    out_data[data_ptr++] = SOH;
    //Version
    out_data[data_ptr++] = message->version;
    //Origin / origin_size
    out_data[data_ptr++] = message->origin_size;
    memcpy(out_data + data_ptr, message->origin, message->origin_size);
    data_ptr += message->origin_size;
    //Remote / remote size
    out_data[data_ptr++] = message->remote_size;
    memcpy(out_data + data_ptr, message->remote, message->remote_size);
    data_ptr += message->remote_size;
    //TTL
    out_data[data_ptr++] = message->ttl;
    //Data size
    out_data[data_ptr++] = (message->data_size >> 56) & 0xFF;
    out_data[data_ptr++] = (message->data_size >> 48) & 0xFF;
    out_data[data_ptr++] = (message->data_size >> 40) & 0xFF;
    out_data[data_ptr++] = (message->data_size >> 32) & 0xFF;
    out_data[data_ptr++] = (message->data_size >> 24) & 0xFF;
    out_data[data_ptr++] = (message->data_size >> 16) & 0xFF;
    out_data[data_ptr++] = (message->data_size >> 8) & 0xFF;
    out_data[data_ptr++] = message->data_size & 0xFF;
    //Options
    out_data[data_ptr++] = message->options;
    //Keep position of checksum, and go ahead
    size_t checksum_ptr = data_ptr;
    data_ptr++;
    //STX
    out_data[data_ptr++] = STX;
    //Write data
    memcpy(out_data + data_ptr, message->data, message->data_size);
    data_ptr += message->data_size;
    //Write ETX
    out_data[data_ptr++] = ETX;
    //Checksum as last thing
    message->checksum = 0;
    if ((message->options & OCTOPIPES_OPTIONS_IGNORE_CHECKSUM) == 0) {
      message->checksum = calculate_checksum(message);
    }
    out_data[checksum_ptr] = message->checksum;
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
    checksum = checksum ^ ((message->data_size >> 56) & 0xFF);
    checksum = checksum ^ ((message->data_size >> 48) & 0xFF);
    checksum = checksum ^ ((message->data_size >> 40) & 0xFF);
    checksum = checksum ^ ((message->data_size >> 32) & 0xFF);
    checksum = checksum ^ ((message->data_size >> 24) & 0xFF);
    checksum = checksum ^ ((message->data_size >> 16) & 0xFF);
    checksum = checksum ^ ((message->data_size >> 8) & 0xFF);
    checksum = checksum ^ (message->data_size & 0xFF);
    checksum = checksum ^ message->options;
    checksum = checksum ^ STX;
    for (size_t i = 0; i < message->data_size; i++) {
      checksum = checksum ^ message->data[i];
    }
  }
  checksum = checksum ^ ETX; //Eventually xor with etx
  return checksum;  
}
