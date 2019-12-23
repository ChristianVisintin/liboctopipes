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

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#define DEFAULT_CAP_PATH "/usr/share/octopipes/pipes/cap.fifo"
#define DEFAULT_CLIENT_ID "octopipes-client"
#define DEFAULT_PROTOCOL_VERSION OCTOPIPES_VERSION_1

#define PROGRAM_NAME "octopipes_sub"
#define USAGE PROGRAM_NAME " built against liboctopipes " OCTOPIPES_LIB_VERSION "\n\
Usage: " PROGRAM_NAME " [options...] subscriptions\n\
\t-C <CAP path>\t\tSpecify the Common Access Pipe path\n\
\t-c <count>\t\tIndicates the amount of messages to receive before exiting\n\
\t-i <client id>\t\tSpeicify the client ID\n\
\t-V <protocol version>\tSpecify the protocol version to use (Default: 1)\n\
\t-v\t\t\tVerbose\n\
\t-h\t\t\tShow this page\n\
"

//Options
char** groups = NULL;
size_t groups_count = 0;
int count = -1;
char* cap_path = DEFAULT_CAP_PATH;
char* client_id = DEFAULT_CLIENT_ID;
int protocol_version = DEFAULT_PROTOCOL_VERSION;
bool sigterm_called = false;
bool verbose = false;

int message_counter = 0;

/**
 * @brief on message received callback
 * @param OctopipesMessage*
 */

void on_received(const OctopipesMessage* message) {
  message_counter++;
  if (message_counter >= count && count != -1) {
    return;
  }
  if (message->data_size > 0 && message->data != NULL) {
    if (verbose) {
      printf("%s %s\n", message->origin, message->data);
    } else {
      printf("%s\n", message->data);
    }
  } else {
    if (verbose) {
      printf("%s\n", message->origin);
    }
  }
}

/**
 * @brief on error raised callback
 * @param OctopipesError
 */

void on_error(const OctopipesError error) {
  printf("ERROR: %s\n", octopipes_get_error_desc(error));
}

/**
 * @function handleSigterm
 * @description: terminate main loop in order to terminate einklient
 * @param int
**/

void handleSigterm(int s) {
  sigterm_called = true;
}

int main(int argc, char** argv) {
  int opt;
  //Getopt
  while ((opt = getopt(argc, argv, "C:c:i:V:vh")) != -1) {
    switch (opt) {
    case 'C':
      cap_path = optarg;
      break;
    case 'c':
      count = atoi(optarg);
      break;
    case 'i':
      client_id = optarg;
      break;
    case 'V':
      protocol_version = atoi(optarg);
      break;
    case 'v':
      verbose = true;
      break;
    case 'h':
      printf("%s\n", USAGE);
      return 0;
    }
  }

  groups_count = (argc - optind - 1);
  //Allocate groups
  groups = (char**) malloc(sizeof(char**) * groups_count);
  if (groups == NULL) {
    printf("Could not allocate more memory for groups container\n");
    return 1;
  }
  //Get subscribptions
  for (size_t i; optind < argc; optind++) {
    groups[i++] = argv[optind]; //Won't be freed since not allocated dynamically
  }

  if (verbose) {
    printf("CAP Path: %s\n", cap_path);
    printf("Count: %d\n", count);
    printf("Client ID: %s\n", client_id);
    printf("Protocol Version: %d\n", protocol_version);
    printf("Verbose: %d\n", verbose);
    for (size_t i = 0; i < groups_count; i++) {
      printf("Group: %s\n", groups[i]);
    }
  }

  //SIGTERM handler
  struct sigaction sigTermHandler;
  sigTermHandler.sa_handler = handleSigterm;
  sigemptyset(&sigTermHandler.sa_mask);
  sigTermHandler.sa_flags = 0;
  sigaction(SIGTERM, &sigTermHandler, NULL);
  sigaction(SIGINT, &sigTermHandler, NULL);

  //Initialize client
  OctopipesClient* client;
  OctopipesError rc;
  OctopipesCapError assignment_error;
  if ((rc = octopipes_init(&client, client_id, cap_path, protocol_version)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not initialize OctopipesClient: %s\n", octopipes_get_error_desc(rc));
    client = NULL;
    goto exit;
  }
  if (verbose) {
    printf("Client initialized\n");
  }

  //Set callback
  octopipes_set_received_cb(client, on_received);
  octopipes_set_receive_error_cb(client, on_error);

  //Subscribe
  if ((rc = octopipes_subscribe(client, (const char**) groups, groups_count, &assignment_error)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not subscribe OctopipesClient: %s (CAP Error: %d)\n", octopipes_get_error_desc(rc), assignment_error);
    goto exit;
  }
  if (verbose) {
    printf("Successfully subscribed to Octopipes Server\n");
  }
  //Start loop
  if ((rc = octopipes_loop_start(client)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not start loop: %s\n", octopipes_get_error_desc(rc));
    goto exit;
  }
  if (verbose) {
    printf("Listening for incoming messages\n");
  }

  while (!sigterm_called && (message_counter < count || count == -1)) {
    sleep(1); //Loops
  }

  //Unsubscribe
  if ((rc = octopipes_unsubscribe(client)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not unsubscribe loop: %s\n", octopipes_get_error_desc(rc));
    goto exit;
  }

  //Stop loop
  if ((rc = octopipes_loop_stop(client)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not stop loop: %s\n", octopipes_get_error_desc(rc));
    goto exit;
  }

exit:
  //Free groups
  free(groups);
  //Free client
  octopipes_cleanup(client);
  return rc;
}
