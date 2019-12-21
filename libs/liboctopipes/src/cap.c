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

#include <octopipes/cap.h>

#include <string.h>

/**
 * @brief prepare a CAP subscribe payload
 * @param char** groups array
 * @param size_t groups size
 * @param size_t out data size
 * @return uint8_t* data out
 */

uint8_t* octopipes_cap_prepare_subscribe(const char** groups, const size_t groups_size, size_t* data_size) {
  size_t current_size = 2; //Size is at least 2 (descriptor, groups amount)
  //Iterate over groups to get total size
  for (size_t i = 0; i < groups_size; i++) {
    current_size += strlen(groups[i]);
  }
  //Allocate buffer
  uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t) * current_size);
  if (data == NULL) {
    return NULL;
  }
  data[0] = (uint8_t) OCTOPIPES_CAP_SUBSCRIBE;
  data[1] = (uint8_t) groups_size;
  size_t data_ptr = 2;
  for (size_t i = 0; i < groups_size; i++) {
    //Write group size
    const size_t this_group_length = strlen(groups[i]);
    data[data_ptr++] = this_group_length;
    //Then write group
    memcpy(data + data_ptr, groups[i], this_group_length);
    data_ptr += this_group_length;
  }
  *data_size = current_size;
  return data;
}

/**
 * @brief prepare the payload for a CAP assign message
 * @param OctopipesCapError error to return to assignment
 * @param char* fifo tx path
 * @param size_t fifo tx size
 * @param char* fifo rx path
 * @param size_t fifo rx size
 * @param size_t* total data size
 * @return uint8_t*
 */

uint8_t* octopipes_cap_prepare_assign(OctopipesCapError error, const char* fifo_tx, const size_t fifo_tx_size, const char* fifo_rx, const size_t fifo_rx_size, size_t* data_size) {
  *data_size = 4 + fifo_tx_size + fifo_rx_size; //Object + error + fifo tx size + fifo rx size
  uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t) * *data_size);
  if (data == NULL) {
    return NULL;
  }
  data[0] = (uint8_t) OCTOPIPES_CAP_ASSIGNMENT;
  data[1] = (uint8_t) error;
  data[2] = (uint8_t) fifo_tx_size;
  size_t curr_data_ptr = 3;
  memcpy(data + curr_data_ptr, fifo_tx, fifo_tx_size);
  curr_data_ptr += fifo_tx_size;
  data[curr_data_ptr++] = (uint8_t) fifo_rx_size;
  memcpy(data + curr_data_ptr, fifo_rx_size, fifo_rx);
  return data;
}

/**
 * @brief prepare an unsubscribe payload for the CAP
 * @param size_t* data size
 * @return out data
 */

uint8_t* octopipes_cap_prepare_unsubscribe(size_t* data_size) {
  *data_size = 1;
  uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t) * *data_size);
  if (data == NULL) {
    return NULL;
  }
  data[0] = OCTOPIPES_CAP_UNSUBSCRIBE;
  return data;
}

/**
 * @brief get CAP message type from its object
 * @param uint8_t* data
 * @param size_t data size
 * @return OctopipesCapMessage
 */

OctopipesCapMessage octopipes_cap_get_message(const uint8_t* data, const size_t data_size) {
  if (data_size == 0) {
    return OCTOPIPES_CAP_UNKNOWN;
  }
  const uint8_t object = data[0];
  switch (object) {
    case OCTOPIPES_CAP_SUBSCRIBE:
      return OCTOPIPES_CAP_SUBSCRIBE;
    case OCTOPIPES_CAP_UNSUBSCRIBE:
      return OCTOPIPES_CAP_UNSUBSCRIBE;
    case OCTOPIPES_CAP_ASSIGNMENT:
      return OCTOPIPES_CAP_ASSIGNMENT;
    default:
      return OCTOPIPES_CAP_UNKNOWN;
  }
}

/**
 * @brief parse a subscribe package
 * @param uint8_t* data in
 * @param size_t data in size
 * @param char** groups will contain the groups to subscribe to
 * @param size_t* amount of groups
 * @return OctopipesError
 */

OctopipesError octopipes_cap_parse_subscribe(const uint8_t* data, const size_t data_size, char** groups, size_t* groups_amount) {
  if (data_size < 2) {
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (data[0] != OCTOPIPES_CAP_SUBSCRIBE) {
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  *groups_amount = (size_t) data[1];
  size_t data_ptr = 2; //Start from first group size
  size_t curr_group = 0;
  //Allocate groups
  groups = (char**) malloc(sizeof(char*) * *groups_amount);
  while (data_ptr < data_size && curr_group < *groups_amount) {
    //Read group name size
    const size_t this_group_size = data[data_ptr++];
    //Allocate group
    char* this_group = (char*) malloc(sizeof(char) * (this_group_size + 1));
    if (data_ptr + this_group_size >= data_size || this_group == NULL) {
      //Select error
      OctopipesError rc = (this_group == NULL) ? OCTOPIPES_ERROR_BAD_ALLOC : OCTOPIPES_ERROR_BAD_PACKET;
      //Free groups
      for (size_t i = 0; i < curr_group + 1; i++) {
        free(groups[i]);
      }
      free(groups);
      return rc; //Not enough bytes
    }
    //Copy group to group string
    memcpy(this_group, data + data_ptr, this_group_size);
    //Add null terminator to group
    this_group[this_group_size] = 0x00;
    //Assign this group to groups
    groups[curr_group] = this_group;
    //Increment data ptr of this groups size
    data_ptr += this_group_size;
    //Increment current group
    curr_group++;
  }
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief parse an assign CAP message
 * @param uint8_t* data in
 * @param size_t data in size
 * @param OctopipesCapError error
 * @param char** fifo tx
 * @param char** fifo_rx
 * @return OctopipesError
 */

OctopipesError octopipes_cap_parse_assign(const uint8_t* data, const size_t data_size, OctopipesCapError* error, char** fifo_tx, char** fifo_rx) {
  if (data_size < 4) { //Must be at least 4 bytes
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (data[0] != OCTOPIPES_CAP_ASSIGNMENT) {
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  *error = (OctopipesCapError) data[1];
  //Is in error state, just return, there's no need to parse anything else
  if (*error != OCTOPIPES_CAP_ERROR_SUCCESS) {
    return OCTOPIPES_ERROR_SUCCESS;
  }
  //If error state is OK, set pipe paths
  const size_t pipe_tx_size = data[2];
  size_t data_ptr = 3;
  *fifo_tx = (char*) malloc(sizeof(char) * (pipe_tx_size + 1));
  if (*fifo_tx == NULL) {
    return OCTOPIPES_ERROR_BAD_ALLOC;
  } else if (data_ptr + pipe_tx_size >= data_size) {
    free(*fifo_tx);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  memcpy(*fifo_tx, data + 3, pipe_tx_size);
  *fifo_tx[pipe_tx_size] = 0x00;
  data_ptr +=  pipe_tx_size;
  const size_t pipe_rx_size = data[data_ptr];
  data_ptr++;
  *fifo_rx = (char*) malloc(sizeof(char) * (pipe_rx_size + 1));
  if (*fifo_tx == NULL) {
    free(*fifo_tx);
    return OCTOPIPES_ERROR_BAD_ALLOC;
  } else if (data_ptr + pipe_rx_size >= data_size) {
    free(*fifo_tx);
    free(*fifo_rx);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  memcpy(*fifo_rx, data + data_ptr, pipe_rx_size);
  *fifo_rx[pipe_rx_size] = 0x00;
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief parse an unsubscribe CAP message
 * @param uint8_t* data in
 * @param size_t data in size
 * @return OctopipesError
 */

OctopipesError octopipes_cap_parse_unsubscribe(const uint8_t* data, const size_t data_size) {
  if (data_size == 0) {
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (data[0] == OCTOPIPES_CAP_UNSUBSCRIBE) {
    return OCTOPIPES_ERROR_SUCCESS;
  }
}
