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

#ifdef _WIN32

#pragma message("-Warning: octopipes is not currently supported on Windows systems")

#include <octopipes/pipes.h>

#ifdef _IA64_
#pragma warning (disable: 4311)
#pragma warning (disable: 4312)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <tchar.h>
#include <windows.h>

typedef struct { 
  OVERLAPPED oOverlap; 
  HANDLE hPipeInst; 
  TCHAR chRequest[2048]; 
  DWORD cbRead;
  TCHAR chReply[2048];
  DWORD cbToWrite; 
  DWORD dwState; 
  BOOL fPendingIO; 
} PIPEINST, *LPPIPEINST; 

/**
 * @brief create the fifo described in the fifo parameter
 * @param char* fifo path
 * @return OctopipesError
 */

OctopipesError pipe_create(const char* fifo) {
  return OCTOPIPES_ERROR_SUCCESS;
}

/**
 * @brief delete the provided pipe path
 * @param char* fifo path
 * @return OctopipesError
 */

OctopipesError pipe_delete(const char* fifo) {
  return OCTOPIPES_ERROR_SUCCESS;
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
  /*
  PIPEINST Pipe;
  HANDLE hEvent;

  hEvents = CreateEvent( 
    NULL,    // default security attribute 
    TRUE,    // manual-reset event 
    TRUE,    // initial state = signaled 
    NULL);   // unnamed event object
  
  if (hEvent == NULL) {
    return OCTOPIPES_ERROR_OPEN_FAILED;
  }

  Pipe.oOverlap.hEvent = hEvent;
  Pipe.hPipeInst = CreateNamedPipe( 
    lpszPipename,            // pipe name 
    PIPE_ACCESS_DUPLEX |     // read/write access 
    FILE_FLAG_OVERLAPPED,    // overlapped mode 
    PIPE_TYPE_MESSAGE |      // message-type pipe 
    PIPE_READMODE_MESSAGE |  // message-read mode 
    PIPE_WAIT,               // blocking mode 
    INSTANCES,               // number of instances 
    BUFSIZE*sizeof(TCHAR),   // output buffer size 
    BUFSIZE*sizeof(TCHAR),   // input buffer size 
    PIPE_TIMEOUT,            // client time-out 
    NULL);                   // default security attributes

  if (Pipe.hPipeInst == INVALID_HANDLE_VALUE) {
    return OCTOPIPES_ERROR_OPEN_FAILED;
  }

  Pipe.dwState = Pipe.fPendingIO ?  CONNECTING_STATE : READING_STATE;
  
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
  fds[0].events = POLLIN;
  int time_elapsed = 0;
  //Poll FIFO
  while (time_elapsed < timeout) {
    ret = poll(fds, 1, 50);
    time_elapsed += 100;
    if (ret > 0) {
      // Fifo is available to be read
      if (fds[0].revents & POLLIN) {
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
      if (*data == NULL) {
        continue; //Keep waiting for data
      } else {
        break; //Exit
      }
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
  */
  return OCTOPIPES_ERROR_READ_FAILED;
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
  return OCTOPIPES_ERROR_READ_FAILED;
}

#endif
