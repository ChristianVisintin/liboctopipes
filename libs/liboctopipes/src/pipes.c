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

#include <octopipes/pipes.h>

#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h> 

/**
 * @brief create the fifo described in the fifo parameter
 * @param char* fifo path
 * @return OctopipesError
 */

OctopipesError pipe_create(const char* fifo) {
  return (mkfifo(fifo, 666) == 0) ? OCTOPIPES_ERROR_SUCCESS : OCTOPIPES_ERROR_OPEN_FAILED;
}

/**
 * @brief poll fifo to check if new messages has arrived, in case something arrived, the fifo will be read
 * @param char* fifo path
 * @param uint8_t** buffer to store received data
 * @param size_t data buffer size
 * @param int timeout in milliseconds
 * @return OctopipesError
 */

OctopipesError pipe_receive(const char* fifo, uint8_t** data, size_t* data_size, const int timeout) {
  struct pollfd fds[1];
  int ret;
  OctopipesError rc = OCTOPIPES_ERROR_NO_DATA_AVAILABLE;
  *data = NULL; //Initialize data to NULL
  *data_size = 0;
  //Open FIFO
  fds[0].fd = open(fifo, O_RDONLY | O_NONBLOCK);
  if (fds[0].fd == -1) {
    //Open failed
    return OCTOPIPES_ERROR_OPEN_FAILED;
  }
  fds[0].events = POLLIN | POLLRDBAND;
  //Poll FIFO
  while (1) {
    ret = poll(fds, 1, timeout);
    if (ret > 0) {
      // Fifo is available to be read
      if ((fds[0].revents & POLLRDBAND) || (fds[0].revents & POLLIN) ) {
        //Read from FIFO
        uint8_t buffer[2048];
        const size_t bytes_read = read(fds[0].fd, buffer, 2048);
        if (bytes_read == -1) {
          rc = OCTOPIPES_ERROR_READ_FAILED;
          break;
        }
        //Copy data to data
        //Track current data index
        const size_t curr_data_ptr = *data_size;
        //Increment data size of bytes read
        *data_size += bytes_read;
        *data = (uint8_t*) realloc(*data, sizeof(uint8_t) * *data_size);
        if (*data == NULL) { //Bad alloc
          rc = OCTOPIPES_ERROR_BAD_ALLOC;
          break;
        }
        //Copy new data to data buffer
        memcpy(*data + curr_data_ptr, buffer, bytes_read);
        //Keep iterating
      } else if (fds[0].revents & POLLERR) {
        //FIFO is in error state
        rc = OCTOPIPES_ERROR_READ_FAILED;
        break;
      }
    } else if (ret == 0) {
      //Break if no data is available
      break;
    } else { //Ret == -1
      if (errno == EAGAIN) {
        //Break if no data is available
      } else {
        //Set error state
        rc = OCTOPIPES_ERROR_READ_FAILED;
      }
      break;
    }
  }
  //Close pipe
  close(fds[0].fd);
  if (*data_size > 0) {
    rc = OCTOPIPES_ERROR_SUCCESS;
  } else if (rc != OCTOPIPES_ERROR_SUCCESS && *data != NULL) { //In case of error free data if was allocated
    free(*data);
    *data = NULL;
    *data_size = 0;
  }
  return rc;
}

/**
 * @brief send a message through a FIFO
 * @param char* fifo file path
 * @param uint8_t* data to send
 * @param size_t data size
 * @param int write timeout
 * @return OctopipesError
 */

OctopipesError pipe_send(const char* fifo, const uint8_t* data, const size_t data_size, const int timeout) {
  struct pollfd fds[1];
  int ret;
  OctopipesError rc = OCTOPIPES_ERROR_NO_DATA_AVAILABLE;
  size_t total_bytes_written = 0; //Must be == data_size to succeed
  //Open FIFO
  fds[0].fd = open(fifo, O_WRONLY | O_NONBLOCK);
  if (fds[0].fd == -1) {
    //Open failed
    return OCTOPIPES_ERROR_OPEN_FAILED;
  }
  fds[0].events = POLLOUT | POLLWRBAND;
  //Poll FIFO
  while (total_bytes_written < data_size) {
    ret = poll(fds, 1, timeout);
    if (ret > 0) {
      // Fifo is available to be written
      if ((fds[0].revents & POLLOUT) || (fds[0].revents & POLLWRBAND) ) {
        //Write data to FIFO
        const size_t remaining_bytes = data_size - total_bytes_written;
        //It's not obvious the data will be written in one shot, so just in case sum total_bytea_written to buffer index and write only remaining bytes
        const size_t bytes_written = write(fds[0].fd, data + total_bytes_written, remaining_bytes);
        //Then sum bytes written to total bytes written
        total_bytes_written += bytes_written;
      }
    } else {
      //Could not write or nobody was listening
      rc = OCTOPIPES_ERROR_WRITE_FAILED;
    }
  }
  close(fds[0].fd);
  return rc;
}
