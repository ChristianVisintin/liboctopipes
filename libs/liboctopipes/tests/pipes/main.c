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
#include <octopipes/pipes.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define PROGRAM_NAME "test_pipes"
#define USAGE PROGRAM_NAME "Usage: " PROGRAM_NAME " [Options]\n\
\t -t <txPipePath>\tSpecify RX Pipe for this instance\n\
\t -r <rxPipePath>\tSpecify RX Pipe for this instance\n\
\t -h\t\tShow this page\n\
"

#define WRITES_AMOUNT 10
#define PIPE_TIMEOUT 5000

//Colors
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

/**
 * Test Description: test_pipes tests the functions ins pipes.h, which consists in:
 * - crating new pipes
 * - read from pipe
 * - write to pipe
 * NOTE: This test JUST tests the PIPES, not the protocol!
 * NOTE: This test forks itself to create a dummy client
 */

/**
 * @brief generate a random string of alphanumerical characters
 * @param char* str
 * @param size_t size of string
 * @return char*
 */

static char* gen_rand_string(char* str, size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  if (size) {
    --size;
    for (size_t n = 0; n < size; n++) {
      int key = rand() % (int) (sizeof(charset) - 1);
      str[n] = charset[key];
    }
    str[size] = '\0';
  }
  return str;
}

/**
 * @brief main for parent process (parent writes first and reads later)
 * @param char* txPipe
 * @param char* rxPipe
 * @return int
 */

int main_parent(const char* txPipe, const char* rxPipe) {
  printf("%sPARENT: Starting main in 3 seconds%s\n", KYEL, KNRM);
  sleep(3);
  size_t buffer_size = 16; //Will be multiplied by two at each step
  unsigned long int total_time_elapsed = 0;
  for (int i = 0; i < WRITES_AMOUNT; i++) {
    struct timeval start, end, start2, end2;
    unsigned long int time_elapsed;
    printf("%sPARENT: Preparing a random buffer with size of %lu bytes%s\n", KYEL, buffer_size, KNRM);
    char* buffer = (char*) malloc(sizeof(char) * buffer_size);
    buffer = gen_rand_string(buffer, buffer_size);
    printf("%sPARENT: About to write %lu bytes%s\n", KYEL, buffer_size, KNRM);
    OctopipesError rc;
    gettimeofday(&start, NULL);
    rc = pipe_send(txPipe, (const uint8_t*) buffer, buffer_size, PIPE_TIMEOUT);
    if (rc != OCTOPIPES_ERROR_SUCCESS) {
      printf("%sPARENT: Error while writing to pipe: %s%s\n", KYEL, octopipes_get_error_desc(rc), KNRM);
      return (int) rc;
    }
    gettimeofday(&end, NULL);
    time_elapsed = end.tv_usec - start.tv_usec;
    total_time_elapsed += time_elapsed;
    printf("%sPARENT: Written %lu bytes%s\n", KYEL, buffer_size, KNRM);
    printf("%sPARENT: Total time elapsed (microseconds) %lu; time elapsed for this write %lu%s\n", KYEL, total_time_elapsed, time_elapsed, KNRM);
    //Wait for buffer back
    uint8_t* data_in;
    size_t data_in_size;
    gettimeofday(&start2, NULL);
    rc = pipe_receive(rxPipe, &data_in, &data_in_size, PIPE_TIMEOUT);
    if (rc != OCTOPIPES_ERROR_SUCCESS) {
      printf("%sPARENT: Error while reading from pipe: %s%s\n", KYEL, octopipes_get_error_desc(rc), KNRM);
      return (int) rc;
    }
    gettimeofday(&end2, NULL);
    time_elapsed = end2.tv_usec - start2.tv_usec;
    total_time_elapsed += time_elapsed;
    //Verify data in and data out
    if (strcmp(buffer, (const char*) data_in) != 0 || buffer_size != data_in_size) {
      printf("%sBuffer out (%lu bytes) and data read (%lu bytes) mismatch!%s\n", KYEL, buffer_size, data_in_size, KNRM);
      free(data_in);
      free(buffer);
      return 1;
    }
    printf("%sPARENT: Read %lu bytes%s\n", KYEL, data_in_size, KNRM);
    printf("%sPARENT: Total time elapsed (microseconds) %lu; time elapsed for this read %lu%s\n", KYEL, total_time_elapsed, time_elapsed, KNRM);
    free(data_in);
    free(buffer);
    buffer_size *= 2;
  }
  return 0;
}

/**
 * @brief main for child process (parent reads first and writes later)
 * @param char* txPipe
 * @param char* rxPipe
 * @return int
 */

int main_child(const char* txPipe, const char* rxPipe) {
  printf("%sCHILD: Starting main in 3 seconds%s\n", KCYN, KNRM);
  sleep(3);
  unsigned long int total_time_elapsed = 0;
  for (int i = 0; i < WRITES_AMOUNT; i++) {
    struct timeval start, end, start2, end2;
    unsigned long int time_elapsed;
    //Read from pipe
    OctopipesError rc;
    uint8_t* data;
    size_t data_size;
    gettimeofday(&start, NULL);
    rc = pipe_receive(rxPipe, &data, &data_size, PIPE_TIMEOUT);
    if (rc != OCTOPIPES_ERROR_SUCCESS) {
      printf("%sCHILD: Error while reading from pipe: %s%s\n", KCYN, octopipes_get_error_desc(rc), KNRM);
      return (int) rc;
    }
    gettimeofday(&end, NULL);
    time_elapsed = end.tv_usec - start.tv_usec;
    total_time_elapsed += time_elapsed;
    printf("%sCHILD: Read %lu bytes%s\n", KCYN, data_size, KNRM);
    printf("%sCHILD: Total time elapsed (microseconds) %lu; time elapsed for this read %lu%s\n", KCYN, total_time_elapsed, time_elapsed, KNRM);
    //Write data back
    gettimeofday(&start2, NULL);
    rc = pipe_send(txPipe, data, data_size, PIPE_TIMEOUT);
    if (rc != OCTOPIPES_ERROR_SUCCESS) {
      printf("%sCHILD: Error while writing data back to pipe: %s%s\n", KCYN, octopipes_get_error_desc(rc), KNRM);
      free(data);
      return (int) rc;
    }
    gettimeofday(&end2, NULL);
    time_elapsed = end2.tv_usec - start2.tv_usec;
    total_time_elapsed += time_elapsed;
    printf("%sCHILD: Written %lu bytes%s\n", KCYN, data_size, KNRM);
    printf("%sCHILD: Total time elapsed (microseconds) %lu; time elapsed for this write %lu%s\n", KCYN, total_time_elapsed, time_elapsed, KNRM);
    //Free data
    free(data);
  }
  return 0;
}

int main(int argc, char** argv) {
  printf(PROGRAM_NAME "liboctopipes Build: " OCTOPIPES_LIB_VERSION "\n");
  int opt;
  char* txPipe = NULL;
  char* rxPipe = NULL;
  int is_child = 0; //This is for indicating the client type
  while ((opt = getopt(argc, argv, "t:r:h")) != -1) {
    switch (opt) {
    case 't':
      txPipe = optarg;
      break;
    case 'r':
      rxPipe = optarg;
      break;
    case 'h':
      printf("%s\n", USAGE);
      return 0;
    }
  }

  //Check if tx pipe and rx pipe are set
  if (txPipe == NULL || rxPipe == NULL) {
    printf("Missing TX Pipe (%p) or RX Pipe (%p)\n%s\n", txPipe, rxPipe, USAGE);
    return 1;
  }

  //Create pipes
  OctopipesError rc;
  if ((rc = pipe_create(txPipe)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not create TX pipe (%s): %s\n", txPipe, octopipes_get_error_desc(rc));
    return 1;
  }
  if ((rc = pipe_create(rxPipe)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not create RX pipe (%s): %s\n", rxPipe, octopipes_get_error_desc(rc));
    //Remote tx pipe
    pipe_delete(txPipe);
    return 1;
  }

  pid_t child_pid;
  if ((child_pid = fork()) == 0) {
    printf("Child process started\n");
    is_child = 1; //Is fork process
    //Exchange tx with rx
    char* tmp = rxPipe;
    rxPipe = txPipe;
    txPipe = tmp;
  } else if (child_pid > 0) {
    printf("Second client (child process) started with pid %d\n", child_pid);
  } else {
    printf("Could not fork process\n");
    return 1;
  }

  //Main for child and parent
  int ret;
  if (is_child == 0) {
    ret = main_parent(txPipe, rxPipe);
    //Remove pipes
    printf("Removing TX and RX pipes\n");
    if ((rc = pipe_delete(txPipe)) != OCTOPIPES_ERROR_SUCCESS) {
      printf("Could not delete TX pipe (%s): %s\n", txPipe, octopipes_get_error_desc(rc));
    }
    if ((rc = pipe_delete(rxPipe)) != OCTOPIPES_ERROR_SUCCESS) {
      printf("Could not delete RX pipe (%s): %s\n", rxPipe, octopipes_get_error_desc(rc));
    }
    printf("Parent process exited with code %d\n", ret);
  } else {
    ret = main_child(txPipe, rxPipe);
    printf("Child process exited with code %d\n", ret);
  }
  return ret;
}
