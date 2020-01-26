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

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define PROGRAM_NAME "test_client"
#define USAGE PROGRAM_NAME "Usage: " PROGRAM_NAME " [Options]\n\
\t -t <txPipePath>\tSpecify RX Pipe for this instance\n\
\t -r <rxPipePath>\tSpecify RX Pipe for this instance\n\
\t -c <capPath>\t\tSpecify the CAP Pipe for this instance\n\
\t -h\t\t\tShow this page\n\
"

#define WRITES_AMOUNT 10
#define PIPE_TIMEOUT 5000
#define CLIENT_NAME "test_client"
#define CLIENT_NAME_SIZE 11
#define FAKE_CLIENT_NAME "fake_client"
#define FAKE_CLIENT_NAME_SIZE 11

//Colors
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

char* txPipe = NULL;
char* rxPipe = NULL;
char* capPipe = NULL;
int messages_received = 0;

/**
 * Test Description: test_client simulates the connection steps with the server (subscription, assignment, ipc, unsubscription), the test consists in:
 * - crating new pipes
 * - read from pipe
 * - write to pipe
 * - subscribe to the server
 * - parse an assignment
 * - start looping on the pipe
 * - stop looping
 * - unsubscribe from the server
 * Functions covered by this test (including CAP and pipes):
 * - octopipes_init
 * - octopipes_cleanup
 * - octopipes_cleanup_message
 * - octopipes_loop_start
 * - octopipes_loop_stop
 * - octopipes_subscribe
 * - octopipes_unsubscribe
 * - octopipes_send
 * - octopipes_send_ex
 * - octopipes_set_received_cb
 * - octopipes_set_sent_cb
 * - octopipes_set_receive_error_cb
 * - octopipes_set_subscribed_cb
 * - octopipes_set_unsubscribed_cb
 * - octopipes_get_error_desc
 * NOTE: This test forks itself to create a dummy client (so it doesn't run on Windows...)
 */

typedef enum ClientStep {
  INITIALIZED,
  SUBSCRIBED,
  RUNNING,
  UNSUBSCRIBE,
  TERMINATED
} ClientStep;

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

void dump_data(const char* color, const uint8_t* data, const size_t data_size) {
  printf("%s", color);
  for (size_t i = 0; i < data_size; i++) {
    printf("%02x ", data[i]);
  }
  printf("%s\n", KNRM);
}

void on_subscribed(const OctopipesClient* client) {
  printf("%son_subscribed: Client %s SUBSCRIBED to the server%s\n", KYEL, client->client_id, KNRM);
}

void on_unsubscribed(const OctopipesClient* client) {
  printf("%son_unsubscribed: Client %s UNSUBSCRIBED to the server%s\n", KYEL, client->client_id, KNRM);
}

void on_sent(const OctopipesClient* client, const OctopipesMessage* message) {
  printf("%son_sent: Client %s SENT message with size %llu bytes%s\n", KMAG, client->client_id, message->data_size, KNRM);
}

void on_received(const OctopipesClient* client, const OctopipesMessage* message) {
  printf("%son_received: Client %s RECEIVED message from %s with data %.*s%s\n", KYEL, client->client_id, message->origin, (int) message->data_size, message->data, KNRM);
  //Use extra data to count messages
  messages_received++;
}

void on_receive_error(const OctopipesClient* client, const OctopipesError error) {
  printf("%son_receive_error: Client %s ERROR: %s%s\n", KRED, client->client_id, octopipes_get_error_desc(error), KNRM);
}

#ifndef _WIN32

/**
 * @brief main for second child (Fake client which sends a message)
 * @return int
 */

int main_fake_client() {
  //Initialize a fake client
  OctopipesClient* fake_client;
  OctopipesError ret;
  printf("%sInitializing FAKE client%s\n", KMAG, KNRM);
  if ((ret = octopipes_init(&fake_client, FAKE_CLIENT_NAME, capPipe, OCTOPIPES_VERSION_1)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not initialize octopipes client: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
    return 1;
  }
  printf("%sFAKE client initialized; Forcing subscription state%s\n", KMAG, KNRM);
  //Set manually the params for the fake client (the pipes between client and fake clients will be shared)
  fake_client->tx_pipe = rxPipe;
  fake_client->rx_pipe = txPipe;
  //Force state to SUBSCRIBED ;)
  fake_client->state = OCTOPIPES_STATE_SUBSCRIBED;
  fake_client->on_sent = on_sent;
  //Send a message
  uint8_t* out_payload = (uint8_t*) malloc(sizeof(uint8_t) * 33);
  out_payload = (uint8_t*) gen_rand_string((char*) out_payload, 33);
  printf("%sGenerated a random payload of 32 bytes: %s%s\n", KMAG, out_payload, KNRM);
  if ((ret = octopipes_send(fake_client, CLIENT_NAME, out_payload, 32)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not send data to octopipes the other client: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
    free(fake_client->client_id);
    free(fake_client->common_access_pipe);
    free(fake_client);
    free(out_payload);
    return 1;
  }
  printf("%sSent data out to client%s\n", KMAG, KNRM);
  free(out_payload);
  free(fake_client->client_id);
  free(fake_client->common_access_pipe);
  free(fake_client);
  printf("%sFreed fake client; returning 0%s\n", KMAG, KNRM);
  return 0;
}

#endif

/**
 * @brief main for parent process (parent is client)
 * @param char* txPipe
 * @param char* rxPipe
 * @return int
 */

int main_client() {
  printf("%sPARENT (client): Starting main in 3.0 seconds%s\n", KYEL, KNRM);
  sleep(3);
  unsigned long int total_time_elapsed = 0;
  ClientStep current_step = INITIALIZED;
  OctopipesClient* client;
  OctopipesError ret;
  //Initialize client
  if ((ret = octopipes_init(&client, CLIENT_NAME, capPipe, OCTOPIPES_VERSION_1)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not initialize octopipes client: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
    return 1;
  }
  //Set callbacks
  octopipes_set_receive_error_cb(client, on_receive_error);
  octopipes_set_received_cb(client, on_received);
  octopipes_set_sent_cb(client, on_sent);
  octopipes_set_subscribed_cb(client, on_subscribed);
  octopipes_set_unsubscribed_cb(client, on_unsubscribed);
  printf("%sOctopipes client initialized%s\n", KYEL, KNRM);
  while (current_step != TERMINATED) {
    switch(current_step) {
      case INITIALIZED: {
        //Send subscribe
        OctopipesCapError cap_error;
        printf("%sPreparing SUBSCRIBE request%s\n", KYEL, KNRM);
        if ((ret = octopipes_subscribe(client, NULL, 0, &cap_error)) != OCTOPIPES_ERROR_SUCCESS) {
          printf("%sCould not subscribe octopipes client: %s (CAP error: %d)%s\n", KRED, octopipes_get_error_desc(ret), cap_error, KNRM);
          current_step = TERMINATED;
          continue;
        }
        printf("%sSuccessfully SUBSCRIBED to the server! (TX: %s; RX: %s)%s\n", KYEL, client->tx_pipe, client->rx_pipe, KNRM);
        current_step = SUBSCRIBED;
        //Start loop
        break;
      }
      case SUBSCRIBED: {
        //Start loop
        if ((ret = octopipes_loop_start(client)) != OCTOPIPES_ERROR_SUCCESS) {
          printf("%sCould not start octopipes client loop: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
          current_step = TERMINATED;
          continue;
        }
        printf("%sLoop started%s\n", KYEL, KNRM);
        current_step = RUNNING;
        break;
      }
      case RUNNING: {
#ifndef _WIN32
        printf("%sPreparing an IPC simulation%s\n", KYEL, KNRM);
        //Fork itself and simulate a messange exchange
        pid_t child_pid;
        if ((child_pid = fork()) == 0) {
          printf("%sSecond child (Fake Client) process started%s\n", KMAG, KNRM);
          exit(main_fake_client());
        }
        int timeout = 5000000;
        int elapsed_time = 0;
        while (messages_received == 0 && elapsed_time < timeout) { //Wait for message
          usleep(500000); //500ms
          printf("%sWaiting for FAKE client message...%s\n", KYEL, KNRM);
          elapsed_time += 500000;
        }
        if (messages_received == 0) {
          printf("%sNEVER RECEIVED A SINGLE MESSAGE!!!%s\n", KRED, KNRM);
        }
#endif
        current_step = UNSUBSCRIBE;
        break;
      }
      case UNSUBSCRIBE: {
        printf("%sPreparing UNSUBSCRIBE request%s\n", KYEL, KNRM);
        //Unsubscribe
        if ((ret = octopipes_unsubscribe(client)) != OCTOPIPES_ERROR_SUCCESS) {
          printf("%sCould not unsubscribe octopipes client: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
          continue;
        }
        printf("%sSuccessfully UNSUBSCRIBED from the server!%s\n", KYEL, KNRM);
        current_step = TERMINATED;
        break;
      }
      
      case TERMINATED: {
        break;
      }
    }
  }
  printf("%sTest terminated; about to cleanup Octopipes Client%s\n", KYEL, KNRM);
  //Cleanup client
  if ((ret = octopipes_cleanup(client)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not cleanup octopipes client: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
    return 1;
  }
  printf("%sOctopipesClient cleaned up%s\n", KYEL, KNRM);
  sleep(2); //Give to the server the time to terminate
  return 0;
}

/**
 * @brief main for child process (child is a simualated server)
 * @param char* txPipe
 * @param char* rxPipe
 * @return int
 */

int main_server() {
  printf("%sCHILD (server): Starting main in 2.8 seconds%s\n", KCYN, KNRM);
  usleep(2800000);
  unsigned long int total_time_elapsed = 0;
  bool client_unsubscribed = false;
  //Read from pipes
  while(!client_unsubscribed) {
    //Read from pipe
    uint8_t* data_in = NULL;
    size_t data_in_size;
    OctopipesError ret;
    struct timeval t_start, t_read, t_write, t_end;
    gettimeofday(&t_start, NULL);
    ret = pipe_receive(capPipe, &data_in, &data_in_size, 5000);
    gettimeofday(&t_read, NULL);
    const time_t read_time = t_read.tv_usec - t_start.tv_usec;
    total_time_elapsed += read_time;
    printf("%sTotal elapsed time (microseconds): %lu; read time: %lu%s\n", KCYN, total_time_elapsed, read_time, KNRM);
    //Parse message
    if (ret == OCTOPIPES_ERROR_SUCCESS) {
      //Dump data
      printf("%sReceived data: ", KCYN);
      for (size_t i = 0; i < data_in_size; i++) {
        printf("%02x ", data_in[i]);
      }
      printf("%s\n", KNRM);
      OctopipesMessage* message;
      if ((ret = octopipes_decode(data_in, data_in_size, &message)) != OCTOPIPES_ERROR_SUCCESS) {
        printf("%sCould not decode CAP message: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
        free(data_in);
        continue;
      }
      //Parse message
      OctopipesCapMessage message_type = octopipes_cap_get_message(message->data, message->data_size);
      //Switch on message type
      switch (message_type) {
        case OCTOPIPES_CAP_SUBSCRIPTION: { //@!SUBSCRIBE
          printf("%sReceived SUBSCRIBE request from %s%s\n", KCYN, message->origin, KNRM);
          //Parse subscribe
          char** groups = NULL;
          size_t groups_amount;
          if ((ret = octopipes_cap_parse_subscribe(data_in, data_in_size, &groups, &groups_amount)) != OCTOPIPES_ERROR_SUCCESS) {
            printf("%sCould not parse subscribe message: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
            octopipes_cleanup_message(message);
            free(data_in);
            return 1;
          }
          printf("%sSUBSCRIBE message successfully parsed%s\n", KCYN, KNRM);
          //Free groups
          if (groups != NULL) {
            for (size_t i = 0; i < groups_amount; i++) {
              free(groups[i]);
            }
            free(groups);
          }
          //Prepare assign
          uint8_t* out_payload;
          size_t out_payload_size;
          out_payload = octopipes_cap_prepare_assign(OCTOPIPES_CAP_ERROR_SUCCESS, txPipe, strlen(txPipe), rxPipe, strlen(rxPipe), &out_payload_size);
          printf("%sPrepared ASSIGNMENT payload%s\n", KCYN, KNRM);
          //Send data
          OctopipesMessage* assignment_message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
          if (assignment_message == NULL) {
            printf("%sCould not allocate assignment message%s\n", KRED, KNRM);
            free(out_payload);
            octopipes_cleanup_message(message);
            free(data_in);
            return 1;
          }
          assignment_message->version = OCTOPIPES_VERSION_1;
          assignment_message->origin = NULL;
          assignment_message->origin_size = 0; //Server
          assignment_message->remote = CLIENT_NAME;
          assignment_message->remote_size = CLIENT_NAME_SIZE;
          assignment_message->ttl = 60;
          assignment_message->data_size = out_payload_size;
          assignment_message->data = out_payload;
          assignment_message->options = OCTOPIPES_OPTIONS_NONE;
          assignment_message->checksum = calculate_checksum(assignment_message);
          //Encode message
          uint8_t* out_data;
          size_t out_data_size;
          printf("%sEncoding ASSIGNMENT message%s\n", KCYN, KNRM);
          if ((ret = octopipes_encode(assignment_message, &out_data, &out_data_size)) != OCTOPIPES_ERROR_SUCCESS) {
            printf("%sCould not encode assignment message: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
            free(assignment_message);
          }
          free(assignment_message);
          free(out_payload);
          //Send assignment
          printf("%sSending ASSIGNMENT%s\n", KCYN, KNRM);
          printf("%sAssignment data dump: ", KCYN);
          for (size_t i = 0; i < out_data_size; i++) {
            printf("%02x ", out_data[i]);
          }
          printf("%s\n", KNRM);
          if ((ret = pipe_send(capPipe, out_data, out_data_size, 5000)) != OCTOPIPES_ERROR_SUCCESS) {
            printf("%sCould not send assignment to client: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
            free(out_data);
            octopipes_cleanup_message(message);
            free(data_in);
            return 1;
          }
          gettimeofday(&t_write, NULL);
          const time_t write_time = t_write.tv_usec - t_read.tv_usec;
          total_time_elapsed += write_time;
          printf("%sTotal elapsed time (microseconds): %lu; write time: %lu%s\n", KCYN, total_time_elapsed, write_time, KNRM);
          printf("%sSent ASSIGNMENT to %s%s\n", KCYN, message->origin, KNRM);
          //Free out data
          free(out_data);
          break;
        }
        case OCTOPIPES_CAP_UNSUBSCRIPTION: {
          printf("%sReceived UNSUBSCRIBE request from %s%s\n", KCYN, message->origin, KNRM);
          if ((ret = octopipes_cap_parse_unsubscribe(message->data, message->data_size)) != OCTOPIPES_ERROR_SUCCESS) {
            printf("%sCould not parse unsubscribe message%s\n", KRED, KNRM);
            octopipes_cleanup_message(message);
            free(data_in);
            return 1;
          }
          client_unsubscribed = true;
          gettimeofday(&t_write, NULL);
          break;
        }
        default: {
          gettimeofday(&t_write, NULL);
          printf("%sUnknown message type on CAP: %d%s\n", KRED, message_type, KNRM);
          break;
        }
      }
      //Cleanup message
      octopipes_cleanup_message(message);
      free(data_in);
    } else if (ret == OCTOPIPES_ERROR_NO_DATA_AVAILABLE) {
      printf("%sThere is no data available%s\n", KCYN, KNRM);
      continue;
    } else {
      printf("%sCAP Error: %s%s\n", KRED, octopipes_get_error_desc(ret), KNRM);
      return 1;
    }
    gettimeofday(&t_end, NULL);
    const time_t end_time = t_end.tv_usec - t_write.tv_usec;
    total_time_elapsed += end_time;
    printf("%sTotal elapsed time (microseconds): %lu; request processing time: %lu%s\n", KCYN, total_time_elapsed, end_time, KNRM);
  }
  return 0;
}

int main(int argc, char** argv) {
#ifdef _WIN32
#pragma message("-Warning: test client doesn't run on Windows")
  return 0;
#else
  printf(PROGRAM_NAME " liboctopipes Build: " OCTOPIPES_LIB_VERSION "\n");
  int opt;
  int is_child = 0; //This is for indicating the client type
  while ((opt = getopt(argc, argv, "t:r:c:h")) != -1) {
    switch (opt) {
    case 't':
      txPipe = optarg;
      break;
    case 'r':
      rxPipe = optarg;
      break;
    case 'c':
      capPipe = optarg;
      break;
    case 'h':
      printf("%s\n", USAGE);
      return 0;
    }
  }

  //Check if tx pipe and rx pipe are set
  if (txPipe == NULL || rxPipe == NULL || capPipe == NULL) {
    printf("Missing TX Pipe (%p) or RX Pipe (%p) or CAP Pipe (%p)\n%s\n", txPipe, rxPipe, capPipe, USAGE);
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
  if ((rc = pipe_create(capPipe)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not create CAP pipe (%s): %s\n", capPipe, octopipes_get_error_desc(rc));
    //Remote tx pipe
    pipe_delete(txPipe);
    pipe_delete(rxPipe);
    return 1;
  }

  pid_t child_pid;
  if ((child_pid = fork()) == 0) {
    printf("Child process started\n");
    is_child = 1; //Is fork process
  } else if (child_pid > 0) {
    printf("Second client (child process) started with pid %d\n", child_pid);
  } else {
    printf("Could not fork process\n");
    return 1;
  }

  //Main for child and parent
  int ret;
  if (is_child == 0) {
    ret = main_client();
    //Remove pipes
    printf("Removing TX and RX pipes\n");
    if ((rc = pipe_delete(txPipe)) != OCTOPIPES_ERROR_SUCCESS) {
      printf("Could not delete TX pipe (%s): %s\n", txPipe, octopipes_get_error_desc(rc));
    }
    if ((rc = pipe_delete(rxPipe)) != OCTOPIPES_ERROR_SUCCESS) {
      printf("Could not delete RX pipe (%s): %s\n", rxPipe, octopipes_get_error_desc(rc));
    }
    if ((rc = pipe_delete(capPipe)) != OCTOPIPES_ERROR_SUCCESS) {
      printf("Could not delete CAP pipe (%s): %s\n", rxPipe, octopipes_get_error_desc(rc));
    }
    printf("Parent (client) process exited with code %d\n", ret);
  } else {
    ret = main_server();
    printf("Child (server) process exited with code %d\n", ret);
  }
  return ret;
#endif
}
