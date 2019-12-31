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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_CAP_PATH "/usr/share/octopipes/pipes/cap.fifo"
#define DEFAULT_CLIENT_ID "octopipes-client-"
#define DEFAULT_PROTOCOL_VERSION OCTOPIPES_VERSION_1

#define PROGRAM_NAME "octopipes_send"
#define USAGE PROGRAM_NAME " built against liboctopipes " OCTOPIPES_LIB_VERSION "\n\
Usage: " PROGRAM_NAME " [options...] payload\n\
\t-C <CAP path>\t\tSpecify the Common Access Pipe path\n\
\t-r <remote>\t\tSpeicify the remote (or group) to send the payload to\n\
\t-i <client id>\t\tSpeicify the client ID\n\
\t-V <protocol version>\tSpecify the protocol version to use (Default: 1)\n\
\t-v\t\t\tVerbose\n\
\t-h\t\t\tShow this page\n\
"

//Options
char* cap_path = DEFAULT_CAP_PATH;
char* remote = NULL;
char* client_id = NULL;
char* payload = NULL;
int protocol_version = DEFAULT_PROTOCOL_VERSION;
bool sigterm_called = false;
bool verbose = false;

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
 * @brief generates a random client id
 * @return char*
 */

char* gen_random_clid() {
  char* clid = (char*) malloc(sizeof(char) * 26); //octopipes-client + 8 random bytes + 0x00
  memcpy(clid, DEFAULT_CLIENT_ID, 17);
  gen_rand_string(clid + 17, 8);
  return clid;
}

int main(int argc, char** argv) {
  int opt;
  //Getopt
  while ((opt = getopt(argc, argv, "C:r:i:V:vh")) != -1) {
    switch (opt) {
    case 'C':
      cap_path = optarg;
      break;
    case 'r':
      remote = optarg;
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

  if (optind < argc) {
    payload = argv[optind];
  } else {
    printf("Missing payload\n");
    return 1;
  }

  if (remote == NULL) {
    printf("Missing remote\n");
    return 1;
  }

  char* to_free_clid = NULL;
  if (client_id == NULL) {
    client_id = gen_random_clid();
    to_free_clid = client_id;
  }

  if (verbose) {
    printf("CAP Path: %s\n", cap_path);
    printf("Client ID: %s\n", client_id);
    printf("Payload: %s\n", payload);
    printf("Protocol Version: %d\n", protocol_version);
    printf("Remote: %s\n", remote);
    printf("Verbose: %d\n", verbose);
  }

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

  //Subscribe
  if ((rc = octopipes_subscribe(client, NULL, 0, &assignment_error)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not subscribe OctopipesClient: %s (CAP Error: %d)\n", octopipes_get_error_desc(rc), assignment_error);
    goto exit;
  }
  if (verbose) {
    printf("Successfully subscribed to Octopipes Server\n");
  }

  //Send message
  if ((rc = octopipes_send(client, remote, (void*) payload, strlen(payload))) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not send message to Octopipes Server: %s\n", octopipes_get_error_desc(rc));
    goto exit;
  }

  //Unsubscribe
  if ((rc = octopipes_unsubscribe(client)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("Could not unsubscribe loop: %s\n", octopipes_get_error_desc(rc));
    goto exit;
  }

exit:
  //Free client
  octopipes_cleanup(client);
  if (to_free_clid != NULL) {
    free(to_free_clid);
  }
  return rc;
}
