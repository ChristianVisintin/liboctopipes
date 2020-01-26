# LibOctopipes

[![License: MIT](https://img.shields.io/badge/License-MIT-teal.svg)](https://opensource.org/licenses/MIT) [![Stars](https://img.shields.io/github/stars/ChristianVisintin/liboctopipes.svg)](https://github.com/ChristianVisintin/liboctopipes) [![Issues](https://img.shields.io/github/issues/ChristianVisintin/liboctopipes.svg)](https://github.com/ChristianVisintin/liboctopipes) [![Build](https://api.travis-ci.org/ChristianVisintin/liboctopipes.svg?branch=master)](https://travis-ci.org/ChristianVisintin/liboctopipes)

Current Version: 0.1.0 (13/01/2020)
Developed by *Christian Visintin*

LibOctopipes is a C library which provides functions to implement Octopipes server and clients.

- [LibOctopipes](#liboctopipes)
  - [Build](#build)
  - [C++ Wrapper](#c-wrapper)
  - [Client Implementation](#client-implementation)
  - [Server Implementation](#server-implementation)
  - [Documentation](#documentation)
    - [Data types (types.h)](#data-types-typesh)
      - [OctopipesError](#octopipeserror)
      - [OctopipesOptions](#octopipesoptions)
      - [OctopipesVersion](#octopipesversion)
      - [OctopipesCapError](#octopipescaperror)
      - [OctopipesMessage](#octopipesmessage)
      - [OctopipesClient](#octopipesclient)
      - [OctopipesServerError](#octopipesservererror)
      - [OctopipesServer](#octopipesserver)
      - [OctopipesState](#octopipesstate)
      - [OctopipesCapMessage](#octopipescapmessage)
      - [OctopipesServerState](#octopipesserverstate)
      - [OctopipesServerMessage](#octopipesservermessage)
      - [OctopipesServerInbox](#octopipesserverinbox)
      - [OctopipesServerWorker](#octopipesserverworker)
    - [octopipes.h](#octopipesh)
      - [octopipes_init](#octopipesinit)
      - [octopipes_cleanup](#octopipescleanup)
      - [octopipes_message_cleanup](#octopipesmessagecleanup)
      - [octopipes_loop_start](#octopipesloopstart)
      - [octopipes_loop_stop](#octopipesloopstop)
      - [octopipes_subscribe](#octopipessubscribe)
      - [octopipes_unsubscribe](#octopipesunsubscribe)
      - [octopipes_send](#octopipessend)
      - [octopipes_send_ex](#octopipessendex)
      - [octopipes_set_received_cb](#octopipessetreceivedcb)
      - [octopipes_set_sent_cb](#octopipessetsentcb)
      - [octopipes_set_receive_error_cb](#octopipessetreceiveerrorcb)
      - [octopipes_set_subscribed_cb](#octopipessetsubscribedcb)
      - [octopipes_set_unsubscribed_cb](#octopipessetunsubscribedcb)
      - [octopipes_get_error_desc](#octopipesgeterrordesc)
      - [octopipes_server_init](#octopipesserverinit)
      - [octopipes_server_cleanup](#octopipesservercleanup)
      - [octopipes_server_start_cap_listener](#octopipesserverstartcaplistener)
      - [octopipes_server_stop_cap_listener](#octopipesserverstopcaplistener)
      - [octopipes_server_process_cap_once](#octopipesserverprocesscaponce)
      - [octopipes_server_process_cap_all](#octopipesserverprocesscapall)
      - [octopipes_server_start_worker](#octopipesserverstartworker)
      - [octopipes_server_stop_worker](#octopipesserverstopworker)
      - [octopipes_server_process_first](#octopipesserverprocessfirst)
      - [octopipes_server_process_once](#octopipesserverprocessonce)
      - [octopipes_server_process_all](#octopipesserverprocessall)
      - [octopipes_server_is_subscribed](#octopipesserverissubscribed)
      - [octopipes_server_get_subscriptions](#octopipesservergetsubscriptions)
      - [octopipes_server_get_clients](#octopipesservergetclients)
      - [octopipes_server_get_error_desc](#octopipesservergeterrordesc)
    - [cap.h](#caph)
      - [octopipes_cap_prepare_subscription](#octopipescappreparesubscription)
      - [octopipes_cap_prepare_assign](#octopipescapprepareassign)
      - [octopipes_cap_prepare_unsubscription](#octopipescapprepareunsubscription)
      - [octopipes_cap_get_message](#octopipescapgetmessage)
      - [octopipes_cap_parse_subscribe](#octopipescapparsesubscribe)
      - [octopipes_cap_parse_assign](#octopipescapparseassign)
      - [octopipes_cap_parse_unsubscribe](#octopipescapparseunsubscribe)
    - [pipes.h](#pipesh)
      - [pipe_create](#pipecreate)
      - [pipe_delete](#pipedelete)
      - [pipe_receive](#pipereceive)
      - [pipe_send](#pipesend)
    - [serializer.h](#serializerh)
      - [octopipes_decode](#octopipesdecode)
      - [octopipes_encode](#octopipesencode)
      - [calculate_checksum](#calculatechecksum)
  - [Changelog](#changelog)
  - [License](#license)

## Build

Dependencies:

- Cmake
- pthread

```sh
mkdir build/
cmake ..
make
make install
```

## C++ Wrapper

A C++14 wrapper is provided along with this library. For more information go [Click here](liboctopipespp/README.md)

## Client Implementation

Initialize the client

```c
OctopipesClient* client;
if ((rc = octopipes_init(&client, client_id, cap_path, protocol_version)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Set callbacks (optional)

```c
octopipes_set_received_cb(client, on_received);
octopipes_set_receive_error_cb(client, on_error);
octopipes_set_sent_cb(client, on_sent);
octopipes_set_subscribed_cb(client, on_subscribed);
octopipes_set_unsubscribed_cb(client, on_unsubscribed);

//All callbacks takes in an OctopipesClient; received and sent takes also an OctopipesMessage*, while receive_error the returned error from receive:

OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesClient* client, const OctopipesMessage*));
OctopipesError octopipes_set_sent_cb(OctopipesClient* client, void (*on_sent)(const OctopipesClient* client, const OctopipesMessage*));
OctopipesError octopipes_set_receive_error_cb(OctopipesClient* client, void (*on_receive_error)(const OctopipesClient* client, const OctopipesError));
OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)(const OctopipesClient* client));
OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)(const OctopipesClient* client));
```

Subscribe to server

```c
OctopipesCapError cap_error;
if ((rc = octopipes_subscribe(client, groups, groups_amount, &cap_error)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
  //Check also OctopipesCapError here
}
```

Start loop (after that you will start receiving messages through the on_received callback):

```c
if ((rc = octopipes_loop_start(client)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Send messages

```c
if ((rc = octopipes_send(client, remote, (void*) data, data_size)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Unsubscribe

```c
if ((rc = octopipes_unsubscribe(client)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Free resources

```c
octopipes_cleanup(client);
```

Two clients implementation can be found in and are provided with liboctopipes [Here](https://github.com/ChristianVisintin/Octopipes/tree/master/libs/liboctopipes/clients)

## Server Implementation

Initialize the server.
These parameters are required:

- CAP pipe path
- Directory where the clients pipes will be stored (if doesn't exist, will be created)
- Protocol version

```c
OctopipesServer* octopipesServer;
if ((error = octopipes_server_init(&octopipesServer, cap_path, client_folder, OCTOPIPES_VERSION_1)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
  printf("Could not allocate server: %s\n", octopipes_server_get_error_desc(error));
}
```

Start the CAP listener (in order to receive message on the CAP)

```c
if ((error = octopipes_server_start_cap_listener(server)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
  printf("Could not start CAP listener: %s\n", octopipes_server_get_error_desc(error));
}
```

Server main loop. This simple loop takes care of:

- CAP:
  - listening on the CAP inbox for new messages
  - handling subscription requests
  - handling unsubscription requests
- Clients:
  - Waiting for messages from clients
  - Dispatching the client messages to the other clients subscribed to the associated remote

```c
while (true) {
  //Process CAP
  size_t requests = 0;
  if ((error = octopipes_server_process_cap_oncee(server, &requests)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    printf("Error while processing CAP: %s\n", octopipes_server_get_error_desc(error));
  }
  //Process clients
  const char* faultClient;
  if ((error = octopipes_server_process_once(server, &requests, &faultClient)) != octopipes::ServerError::SUCCESS) {
    printf("Error while processing CLIENT %s: %s\n", faultClient, octopipes_server_get_error_desc(error));
  }
  if (requests > 0) {
    printf("Processed %zu requests\n", requests);
  }
  usleep(100000); //100ms
}
```

Stop server
It's enough to delete the server object.

```c
octopipes_server_cleanup(server);
```

## Documentation

liboctopipes is structured as follows:

- octopipes.h : library access point, the file the final developer must include
  - cap.h : takes care of coding and decoding payloads from CAP
  - pipes.h : takes care of creating/removing/writing to/reading from pipes
  - serializer.h : takes care of encoding/decoding Octopipes Messages
  - types.h : contains all the types of liboctopipes

### Data types (types.h)

#### OctopipesError

*public*
OctopipesError is an enum which describes the error type returned by a function in liboctopipes.
*This kind of error is not used for Servers!* (as we'll see).
In liboctopipes all functions returns an OctopipesError, except some exceptions, but the final developer will always receive one.
It is possible to get a description of the error calling ```const char* octopipes_get_error_desc(const OctopipesError error);```.

```c
typedef enum OctopipesError {
  OCTOPIPES_ERROR_SUCCESS,
  OCTOPIPES_ERROR_UNINITIALIZED,
  OCTOPIPES_ERROR_BAD_PACKET,
  OCTOPIPES_ERROR_BAD_CHECKSUM,
  OCTOPIPES_ERROR_UNSUPPORTED_VERSION,
  OCTOPIPES_ERROR_NO_DATA_AVAILABLE,
  OCTOPIPES_ERROR_OPEN_FAILED,
  OCTOPIPES_ERROR_WRITE_FAILED,
  OCTOPIPES_ERROR_READ_FAILED,
  OCTOPIPES_ERROR_CAP_TIMEOUT,
  OCTOPIPES_ERROR_NOT_SUBSCRIBED,
  OCTOPIPES_ERROR_NOT_UNSUBSCRIBED,
  OCTOPIPES_ERROR_THREAD,
  OCTOPIPES_ERROR_BAD_ALLOC,
  OCTOPIPES_ERROR_UNKNOWN_ERROR
} OctopipesError;
```

#### OctopipesOptions

*public*
OctopipesOptions is the options set for an OctopipesMessage

```c
typedef enum OctopipesOptions {
  OCTOPIPES_OPTIONS_NONE = 0,
  OCTOPIPES_OPTIONS_REQUIRE_ACK = 1,
  OCTOPIPES_OPTIONS_ACK = 2,
  OCTOPIPES_OPTIONS_IGNORE_CHECKSUM = 4
} OctopipesOptions;
```

See the documentation to check what each option means.

#### OctopipesVersion

*public*
OctopipesVersion is describes the protocol version to use

```c
typedef enum OctopipesVersion {
  OCTOPIPES_VERSION_1 = 1
} OctopipesVersion;
```

#### OctopipesCapError

*public*
OctopipesCapError describes the CAP error type

```c
typedef enum OctopipesCapError {
  OCTOPIPES_CAP_ERROR_SUCCESS = 0,
  OCTOPIPES_CAP_ERROR_NAME_ALREADY_TAKEN = 1,
  OCTOPIPES_CAP_ERROR_FS = 2
} OctopipesCapError;
```

#### OctopipesMessage

*public*
OctopipesMessage is a container for an Octopipes Message, which will be encoded when sent or stores the data of a decoded message.

```c
typedef struct OctopipesMessage {
  OctopipesVersion version;
  uint8_t origin_size;
  char* origin;
  uint8_t remote_size;
  char* remote;
  uint8_t ttl;
  uint64_t data_size;
  OctopipesOptions options;
  uint8_t checksum;
  uint8_t* data;
} OctopipesMessage;
```

- version: Protocol Version used by the origin
- origin_size: length of the origin
- origin: message origin
- remote_size: length of the remote
- remote: message remote
- ttl: TTL of the message
- data_size: length of the payload
- options: options of the message
- checksum: message checksum
- data: payload

#### OctopipesClient

*public*
OctopipesClient represents a Client instance. This is the main object which must be used by the developer to implement an Octopipes client.

```c
typedef struct OctopipesClient {
  //State
  OctopipesState state;
  //Thread
  pthread_t loop;
  //Client parameters
  size_t client_id_size;
  char* client_id;
  OctopipesVersion protocol_version;
  //Pipes paths
  char* common_access_pipe;
  char* tx_pipe;
  char* rx_pipe;
  //Callbacks
  void (*on_received)(const struct OctopipesClient* client, const OctopipesMessage*);
  void (*on_sent)(const struct OctopipesClient* client, const OctopipesMessage*);
  void (*on_receive_error)(const struct OctopipesClient* client, const OctopipesError);
  void (*on_subscribed)(const struct OctopipesClient* client);
  void (*on_unsubscribed)(const struct OctopipesClient* client);
  //Extra - can be used to store anything NOTE: must be freed by the user
  void* user_data;
} OctopipesClient;
```

- state: current client state
- loop: loop thread
- client_id_size: length of client id
- client_id: client id
- protocol_version: protocol version used by the client
- common_access_pipe: path of the CAP
- tx_pipe: TX pipe assigned to the client
- rx_pipe: RX pipe assigned to the client
- on_received: callback called when a message is received
- on_sent: callback called when a message is sent
- on_receive_error: callback called when an error is raised while receiving messages
- on_subscribed: callback called when the client subscribes
- on_unsubscribed: callback called when the client unsubscribes
- user_data: a container for custom user data

#### OctopipesServerError

*public*
OtopipesServerError describes the error returned by an operation on an OctopipesServer.
Each function which operates on an OctopipesServer returns an OctopipesServerError.
It is possible to get a description of a ServerError calling ```const char* octopipes_server_get_error_desc(const OctopipesServerError error);```

```c
typedef enum OctopipesServerError {
  OCTOPIPES_SERVER_ERROR_SUCCESS,
  OCTOPIPES_SERVER_ERROR_BAD_ALLOC,
  OCTOPIPES_SERVER_ERROR_UNINITIALIZED,
  OCTOPIPES_SERVER_ERROR_BAD_PACKET,
  OCTOPIPES_SERVER_ERROR_BAD_CHECKSUM,
  OCTOPIPES_SERVER_ERROR_UNSUPPORTED_VERSION,
  OCTOPIPES_SERVER_ERROR_OPEN_FAILED,
  OCTOPIPES_SERVER_ERROR_WRITE_FAILED,
  OCTOPIPES_SERVER_ERROR_READ_FAILED,
  OCTOPIPES_SERVER_ERROR_CAP_TIMEOUT,
  OCTOPIPES_SERVER_ERROR_THREAD_ERROR,
  OCTOPIPES_SERVER_ERROR_THREAD_ALREADY_RUNNING,
  OCTOPIPES_SERVER_ERROR_WORKER_EXISTS,
  OCTOPIPES_SERVER_ERROR_WORKER_NOT_FOUND,
  OCTOPIPES_SERVER_ERROR_WORKER_ALREADY_RUNNING,
  OCTOPIPES_SERVER_ERROR_WORKER_NOT_RUNNING,
  OCTOPIPES_SERVER_ERROR_NO_RECIPIENT,
  OCTOPIPES_SERVER_ERROR_BAD_CLIENT_DIR,
  OCTOPIPES_SERVER_ERROR_UNKNOWN
} OctopipesServerError;
```

#### OctopipesServer

*public*
OctopipesServer represents an Octopipes Server. It contains all the data necessary to run an Octopipes Server indeed.

```c
typedef struct OctopipesServer {
  //Version
  OctopipesVersion version;
  OctopipesServerState state;
  //Pipe
  char* cap_pipe;
  char* client_folder;
  //Thread
  pthread_mutex_t cap_lock;
  pthread_t cap_listener;
  OctopipesServerInbox* cap_inbox;
  //Workers
  OctopipesServerWorker** workers;
  size_t workers_len;
} OctopipesServer;
```

- version: the protocol version used by the server
- state: current server state
- cap_pipe: the path of the CAP pipe
- client_folder: the folder where the clients' pipes are allocated
- cap_lock: mutex for CAP listener
- cap_listener: thread which listens to the CAP
- cap_inbox: CAP message inbox
- workers: array of server workers.
- workers_len: length of workers

#### OctopipesState

*private*
OctopipesState describes the current state of the Octopipes Client.

```c
typedef enum OctopipesState {
  OCTOPIPES_STATE_INIT,
  OCTOPIPES_STATE_SUBSCRIBED,
  OCTOPIPES_STATE_RUNNING,
  OCTOPIPES_STATE_UNSUBSCRIBED,
  OCTOPIPES_STATE_STOPPED
} OctopipesState;
```

- INIT: the client is initialized
- SUBSCRIBED: the client is subscribed to the server
- RUNNING: the client loop is running
- UNSUBSCRIBED: the client is unsubscribed from the server and ready to be freed
- STOPPED: the client loop has been stopped and is ready to be freed

#### OctopipesCapMessage

*private*
OctopipesCapMessage describes the type of a CAP message

```c
typedef enum OctopipesCapMessage {
  OCTOPIPES_CAP_UNKNOWN = 0x00,
  OCTOPIPES_CAP_SUBSCRIPTION = 0x01,
  OCTOPIPES_CAP_ASSIGNMENT = 0xFF,
  OCTOPIPES_CAP_UNSUBSCRIPTION = 0x02
} OctopipesCapMessage;
```

#### OctopipesServerState

*private*
OctopipesServerState describes the current state of the Octopipes Server.

```c
typedef enum OctopipesServerState {
  OCTOPIPES_SERVER_STATE_INIT,
  OCTOPIPES_SERVER_STATE_RUNNING,
  OCTOPIPES_SERVER_STATE_BLOCK,
  OCTOPIPES_SERVER_STATE_STOPPED
} OctopipesServerState;
```

- INIT: the server is initialized
- RUNNING: the server is listening on the CAP
- BLOCK: the server is writing data to the CAP, so the CAP listener must be paused
- STOPPED: the server is stopped and ready to be freed

#### OctopipesServerMessage

*private*
Container for a server message, which is inserted into the server Inbox. The message contains or an OctopipesMessage or an error.

```c
typedef struct OctopipesServerMessage {
  OctopipesMessage* message;
  OctopipesServerError error;
} OctopipesServerMessage;
```

#### OctopipesServerInbox

*private*
The server inbox is used to pass messages from the CAP listener or from a worker to the main thread.

```c
typedef struct OctopipesServerInbox {
  OctopipesServerMessage** messages;
  size_t inbox_len;
} OctopipesServerInbox;
```

#### OctopipesServerWorker

*private*
A worker is a task manager for an individual client subscribed to the server. When a client subscribes a new worker is created and the worker listener thread starts.
The worker is destroyed when a client unsubscribes.

```c
typedef struct OctopipesServerWorker {
  char* client_id;
  char** subscriptions_list;
  size_t subscriptions;
  //Pipes
  char* pipe_read;
  char* pipe_write;
  //Thread stuff
  pthread_t worker_listener;
  pthread_mutex_t worker_lock;
  int active;
  OctopipesServerInbox* inbox;
} OctopipesServerWorker;
```

### octopipes.h

Octopipes contains both the functions to implement a client and a server. The server functions have as prefix ```octopipes_server_FUNCTION_NAME``` while the client's have as prefix just ```octopipes_FUNCTION_NAME```
In the documentation the clients' functions are documented first.

#### octopipes_init

*public*
Initialize an OctopipesClient object.

```c
OctopipesError octopipes_init(OctopipesClient** client, const char* client_id, const char* cap_path, const OctopipesVersion version_to_use);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if was not possible to allocate the client
- OCTOPIPES_ERROR_SUCCESS: if init was successful

#### octopipes_cleanup

*public*
Cleans up an OctopipesClient object.

```c
OctopipesError octopipes_cleanup(OctopipesClient* client);
```

Returns:

- OCTOPIPES_ERROR_SUCCESS: if client was freed

#### octopipes_message_cleanup

*public*
Cleans up an OctopipesMessage object.

```c
OctopipesError octopipes_cleanup_message(OctopipesMessage* message);
```

#### octopipes_loop_start

*public*
Starts the client loop which listens for new messages. To start the loop, the client must be subscribed to the server.

```c
OctopipesError octopipes_loop_start(OctopipesClient* client);
```

Returns:

- OCTOPIPES_ERROR_THREAD: if it was not possible to start the loop thread
- OCTOPIPES_ERROR_NOT_SUBSCRIBED: if client is not subscribed yet
- OCTOPIPES_ERROR_SUCCESS: if loop started
- OCTOPIPES_ERROR_UNINITIALIZED: if client is NULL

#### octopipes_loop_stop

*public*
Stops the client loop. To stop the loop the client must be unsubscribed from the server.

```c
OctopipesError octopipes_loop_stop(OctopipesClient* client);
```

Returns:

- OCTOPIPES_ERROR_THREAD: if it was not possible to join the thread
- OCTOPIPES_ERROR_NOT_SUBSCRIBED: if client is not subscribed yet
- OCTOPIPES_ERROR_SUCCESS: if loop started
- OCTOPIPES_ERROR_UNINITIALIZED: if client is NULL

#### octopipes_subscribe

*public*
Subscribe to the server.

```c
OctopipesError octopipes_subscribe(OctopipesClient* client, const char** groups, size_t groups_amount, OctopipesCapError* assignment_error);
```

Returns:

- OCTOPIPES_ERROR_BAD_PACKET: if the assignment packet has a bad syntax
- OCTOPIPES_ERROR_BAD_ALLOC: if was not possible to allocate more memory
- OCTOPIPES_ERROR_OPEN_FAILED: if pipe_send failed
- OCTOPIPES_ERROR_SUCCESS: if the client successfully subscribed
- OCTOPIPES_ERROR_UNINITIALIZED: if the client is NULL
- OCTOPIPES_ERROR_WRITE_FAILED: if pipe_send failed

#### octopipes_unsubscribe

*public*
Unsubscribe from the server.

```c
OctopipesError octopipes_unsubscribe(OctopipesClient* client);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if it was not possible to allocate more memory
- OCTOPIPES_ERROR_NOT_SUBSCRIBED: if the client is not subscribed
- OCTOPIPES_ERROR_OPEN_FAILED: if pipe_send failed
- OCTOPIPES_ERROR_SUCCESS: if the client successfully unsubscribed
- OCTOPIPES_ERROR_UNINITIALIZED: if the client is NULL
- OCTOPIPES_ERROR_WRITE_FAILED: if pipe_send failed

#### octopipes_send

*public*
Send a message to a certain remote (without any particular options).

```c
OctopipesError octopipes_send(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size);
```

Returns: see octopipes_send_ex

#### octopipes_send_ex

*public*
Send a message to a certain remote; allows ttl and options.

```c
OctopipesError octopipes_send_ex(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size, const uint8_t ttl, const OctopipesOptions options);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if it was not possible to allocate more memory
- OCTOPIPES_ERROR_NOT_SUBSCRIBED: if the client is not subscribed
- OCTOPIPES_ERROR_OPEN_FAILED: if pipe_send failed
- OCTOPIPES_ERROR_SUCCESS: if the client successfully unsubscribed
- OCTOPIPES_ERROR_UNINITIALIZED: if the client is NULL
- OCTOPIPES_ERROR_WRITE_FAILED: if pipe_send failed

#### octopipes_set_received_cb

*public*
Set the function to call when a message is received.

```c
OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesClient* client, const OctopipesMessage*));
```

#### octopipes_set_sent_cb

*public*
Set the function to call when a message is sent.

```c
OctopipesError octopipes_set_sent_cb(OctopipesClient* client, void (*on_sent)(const OctopipesClient* client, const OctopipesMessage*));
```

#### octopipes_set_receive_error_cb

*public*
Set the function to call when an error is raised while receiving messages.

```c
OctopipesError octopipes_set_receive_error_cb(OctopipesClient* client, void (*on_receive_error)(const OctopipesClient* client, const OctopipesError));
```

#### octopipes_set_subscribed_cb

*public*
Set the function to call when the client subscribes.

```c
OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)(const OctopipesClient* client));
```

#### octopipes_set_unsubscribed_cb

*public*
Set the function to call when the client unsubscribes.

```c
OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)(const OctopipesClient* client));
```

#### octopipes_get_error_desc

*public*
Get the error description for a certain OctopipesError

```c
const char* octopipes_get_error_desc(const OctopipesError error);
```

#### octopipes_server_init

*public*
Initialize an OctopipesServer object.

```c
OctopipesServerError octopipes_server_init(OctopipesServer** server, const char* cap_path, const char* client_folder, const OctopipesVersion version);
```

Returns:

- OCTOPIPES_SERVER_ERROR_BAD_ALLOC: if it was not possible to allocate the server object.
- OCTOPIPES_SERVER_ERROR_SUCCESS: if succeded

#### octopipes_server_cleanup

*public*
Cleans up an OctopipesServer object.

```c
OctopipesServerError octopipes_server_cleanup(OctopipesServer* server);
```

#### octopipes_server_start_cap_listener

*public*
Starts the CAP listener thread, which reads from the CAP for incoming messages.

```c
OctopipesServerError octopipes_server_start_cap_listener(OctopipesServer* server);
```

Returns:

- OCTOPIPES_SERVER_ERROR_BAD_CLIENT_DIR: if it was not possible to create the Client directory
- OCTOPIPES_SERVER_ERROR_OPEN_FAILED: if it wasn't possible to create the CAP
- OCTOPIPES_SERVER_ERROR_SUCCESS: if it succeded
- OCTOPIPES_SERVER_ERROR_THREAD_ALREADY_RUNNING: if the CAP listener is already running
- OCTOPIPES_SERVER_ERROR_THREAD_ERROR: if it wasn't possible to start the CAP thread

#### octopipes_server_stop_cap_listener

*public*
Stops the CAP listener thread.

```c
OctopipesServerError octopipes_server_stop_cap_listener(OctopipesServer* server);
```

Returns:

- OCTOPIPES_SERVER_ERROR_UNINITIALIZED: if the server CAP listener is not running
- OCTOPIPES_SERVER_ERROR_THREAD_ERROR: if it wasn't possible to join the cap listener thread
- OCTOPIPES_SERVER_ERROR_SUCCESS: if it succeded

#### octopipes_server_process_cap_once

*public*
Processes, if found in the CAP inbox, one message from CAP.
This function takes care of processing the entire request and popping out the message from the inbox.

```c
OctopipesServerError octopipes_server_process_cap_once(OctopipesServer* server, size_t* requests);
```

Returns:

- OCTOPIPES_SERVER_ERROR_UNINITIALIZED: If the CAP listener is not running or the inbox is not initialized
- OCTOPIPES_SERVER_ERROR_BAD_PACKET: if the message has bad syntax
- OCTOPIPES_SERVER_ERROR_BAD_ALLOC: if couldn't allocate space for workers
- OCTOPIPES_SERVER_ERROR_WORKER_EXISTS: if the worker already exists
- OCTOPIPES_SERVER_ERROR_SUCCESS: if succeded

#### octopipes_server_process_cap_all

*public*
Process all the messages found in the CAP inbox; returns only when the inbox is finally empty.

```c
OctopipesServerError octopipes_server_process_cap_all(OctopipesServer* server, size_t* requests);
```

Returns:

- OCTOPIPES_SERVER_ERROR_UNINITIALIZED: If the CAP listener is not running or the inbox is not initialized
- OCTOPIPES_SERVER_ERROR_BAD_PACKET: if the message has bad syntax
- OCTOPIPES_SERVER_ERROR_BAD_ALLOC: if couldn't allocate space for workers
- OCTOPIPES_SERVER_ERROR_WORKER_EXISTS: if the worker already exists
- OCTOPIPES_SERVER_ERROR_SUCCESS: if succeded

#### octopipes_server_start_worker

*public, unsafe*
Starts a new worker whith a certain name
**WARNING**: this function should be considered UNSAFE. The CAP process functions already take care of this task for a new subscription, so this should only used to force a subscription of a certain client.

```c
OctopipesServerError octopipes_server_start_worker(OctopipesServer* server, const char* client, char** subscriptions, const size_t subscription_len, const char* cli_tx_pipe, const char* cli_rx_pipe);
```

Returns:

- OCTOPIPES_SERVER_ERROR_BAD_ALLOC: if workers couldn't be reallocated
- OCTOPIPES_SERVER_ERROR_SUCCESS: if succeded
- OCTOPIPES_SERVER_ERROR_WORKER_EXISTS: if the worker already exists

#### octopipes_server_stop_worker

*public, unsafe*
Stops a running worker with a certain name
**WARNING**: this function should be considered UNSAFE. The CAP process functions already take care of this task in case of an unsubscription, so this should only used to force an unsubscription of a certain client.

```c
OctopipesServerError octopipes_server_stop_worker(OctopipesServer* server, const char* client);
```

Returns:

- OCTOPIPES_SERVER_ERROR_BAD_ALLOC: if workers couldn't be reallocated
- OCTOPIPES_SERVER_ERROR_SUCCESS: if succeded
- OCTOPIPES_SERVER_ERROR_WORKER_NOT_FOUND: if the worker doesn't exist

#### octopipes_server_process_first

*public*
Process the first available message on any worker inbox.

```c
OctopipesServerError octopipes_server_process_first(OctopipesServer* server, size_t* requests, const char** client);
```

#### octopipes_server_process_once

*public*
Process one message for each worker inbox (if possible).

```c
OctopipesServerError octopipes_server_process_once(OctopipesServer* server, size_t* requests, const char** client);
```

#### octopipes_server_process_all

*public*
Process all the available messages on each worker inbox.
**WARNING**: octopipes_server_process_once should be preferred

```c
OctopipesServerError octopipes_server_process_all(OctopipesServer* server, size_t* requests, const char** client);
```

#### octopipes_server_is_subscribed

*public*
Returns whether a certain client is subscribed.

```c
OctopipesServerError octopipes_server_is_subscribed(OctopipesServer* server, const char* client);
```

#### octopipes_server_get_subscriptions

*public*
Returns the subscriptions for a certain client.

```c
OctopipesServerError octopipes_server_get_subscriptions(OctopipesServer* server, const char* client, char*** subscriptions, size_t* sub_len);
```

#### octopipes_server_get_clients

*pbblic*
Returns a list with all the subscribed clients.

```c
OctopipesServerError octopipes_server_get_clients(OctopipesServer* server, char*** clients, size_t* cli_len);
```

#### octopipes_server_get_error_desc

*public*
Get the error description for a certain OctopipesServerError

```c
const char* octopipes_server_get_error_desc(const OctopipesServerError error);
```

### cap.h

#### octopipes_cap_prepare_subscription

*private*
Encodes a payload for a subscription request.

```c
uint8_t* octopipes_cap_prepare_subscription(const char** groups, const size_t groups_size, size_t* data_size);
```

#### octopipes_cap_prepare_assign

*private*
Encodes a payload for an assignment.

```c
uint8_t* octopipes_cap_prepare_assign(OctopipesCapError error, const char* fifo_tx, const size_t fifo_tx_size, const char* fifo_rx, const size_t fifo_rx_size, size_t* data_size);
```

#### octopipes_cap_prepare_unsubscription

*private*
Encodes a payload for an unsubscription.

```c
uint8_t* octopipes_cap_prepare_unsubscription(size_t* data_size);
```

#### octopipes_cap_get_message

*private*
Returns the type of the message received from the CAP

```c
OctopipesCapMessage octopipes_cap_get_message(const uint8_t* data, const size_t data_size);
```

#### octopipes_cap_parse_subscribe

*private*
Get the subscription request parameters from a subscribe payload

```c
OctopipesError octopipes_cap_parse_subscribe(const uint8_t* data, const size_t data_size, char*** groups, size_t* groups_amount);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if was not possible to allocate groups
- OCTOPIPES_ERROR_BAD_PACKET: if the payload has an invalid syntax
- OCTOPIPES_ERROR_SUCCESS: if subscription was successfully parsed

#### octopipes_cap_parse_assign

*private*
Get the assignment parameters from an assignment payload

```c
OctopipesError octopipes_cap_parse_assign(const uint8_t* data, const size_t data_size, OctopipesCapError* error, char** fifo_tx, char** fifo_rx);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if was not possible to pipes
- OCTOPIPES_ERROR_BAD_PACKET: if the payload has an invalid syntax
- OCTOPIPES_ERROR_SUCCESS: if assignment was successfully parsed

#### octopipes_cap_parse_unsubscribe

*private*
Checks if an unsubscription request has a valid syntax

```c
OctopipesError octopipes_cap_parse_unsubscribe(const uint8_t* data, const size_t data_size);
```

Returns:

- OCTOPIPES_ERROR_BAD_PACKET: if the payload has an invalid syntax
- OCTOPIPES_ERROR_SUCCESS: if unsubscription was successfully parsed

### pipes.h

#### pipe_create

*private*
Creates a Pipe; if Pipe already exists returns success anyway.

```c
OctopipesError pipe_create(const char* fifo);
```

Returns:

- OCTOPIPES_ERROR_OPEN_FAILED: if was not possible to create the pipe
- OCTOPIPES_ERROR_SUCCESS: if pipe was created or already existed

#### pipe_delete

*private*
Deletes a pipe; if pipe doesn't exist returns success anyway.

```c
OctopipesError pipe_delete(const char* fifo);
```

Returns:

- OCTOPIPES_ERROR_OPEN_FAILED: if was not possible to delete the pipe
- OCTOPIPES_ERROR_SUCCESS: if pipe was deleted or didn't exist

#### pipe_receive

*private*
Try to read data from a certain pipe. This function will try to read data until all the data has been read or if the elapsed time reaches timeout. Timeout is expressed in **milliseconds**.

```c
OctopipesError pipe_receive(const char* fifo, uint8_t** data, size_t* data_size, const int timeout);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if was not possible to allocate the buffer which contains the data read
- OCTOPIPES_ERROR_NO_DATA_AVAILABLE: if there was no data to be read; is not really an error, if you think about it.
- OCTOPIPES_ERROR_OPEN_FAILED: if was not possible to open the pipe
- OCTOPIPES_ERROR_READ_FAILED: if was not possible to read from the pipe
- OCTOPIPES_ERROR_SUCCESS: if at least 1 byte has been read

#### pipe_send

*private*
Send some data through a certain pipe. This function will try to write data until all data has been written or if the elapsed time reaches timeout. Timeout is expressed in **milliseconds**.

```c
OctopipesError pipe_send(const char* fifo, const uint8_t* data, const size_t data_size, const int timeout);
```

Returns:

- OCTOPIPES_ERROR_OPEN_FAILED: if the pipe doesn't exist or if there's nobody reading the pipe.
- OCTOPIPES_ERROR_SUCCESS: if all data has been written
- OCTOPIPES_ERROR_WRITE_FAILED: if it was not possible to write data

### serializer.h

#### octopipes_decode

*private*
Decodes a buffer to an OctopipesMessage.

```c
OctopipesError octopipes_decode(const uint8_t* data, const size_t data_size, OctopipesMessage** message);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if couldn't allocate OctopipesMessage
- OCTOPIPES_ERROR_BAD_CHECKSUM: when the message has a bad checksum
- OCTOPIPES_ERROR_BAD_PACKET: when the message has invalid syntax
- OCTOPIPES_ERROR_SUCCESS: when decoding was successful
- OCTOPIPES_ERROR_UNSUPPORTED_VERSION: when the version of the message is not supported by the library

#### octopipes_encode

*private*
Encodes an OctopipesMessage to a buffer to write to pipe.

```c
OctopipesError octopipes_encode(OctopipesMessage* message, uint8_t** data, size_t* data_size);
```

Returns:

- OCTOPIPES_ERROR_BAD_ALLOC: if couldn't allocate the uint8_t* data buffer
- OCTOPIPES_ERROR_SUCCESS: if encoding was successful
- OCTOPIPES_ERROR_UNSUPPORTED_VERSION: if message has an unsupported version

#### calculate_checksum

*private*
Calculate the checksum for an OctopipesMessage

```c
uint8_t calculate_checksum(const OctopipesMessage* message);
```

---

## Changelog

---

## License

```txt
MIT License

Copyright (c) 2019-2020 Christian Visintin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
