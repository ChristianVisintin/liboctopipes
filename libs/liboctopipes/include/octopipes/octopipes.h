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

#ifndef OCTOPIPES_H
#define OCTOPIPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

//Version defines
#define OCTOPIPES_LIB_VERSION "0.1.0"
#define OCTOPIPES_LIB_VERSION_MAJOR 0
#define OCTOPIPES_LIB_VERSION_MINOR 1

//@! Client

//Functions
//Alloc operations
OctopipesError octopipes_init(OctopipesClient** client, const char* client_id, const char* cap_path, const OctopipesVersion version_to_use);
OctopipesError octopipes_cleanup(OctopipesClient* client);
OctopipesError octopipes_cleanup_message(OctopipesMessage* message);
//Thread operations
OctopipesError octopipes_loop_start(OctopipesClient* client);
OctopipesError octopipes_loop_stop(OctopipesClient* client);
//Cap operartions
OctopipesError octopipes_subscribe(OctopipesClient* client, const char** groups, size_t groups_amount, OctopipesCapError* assignment_error);
OctopipesError octopipes_unsubscribe(OctopipesClient* client);
//Tx operations
OctopipesError octopipes_send(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size);
OctopipesError octopipes_send_ex(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size, const uint8_t ttl, const OctopipesOptions options);
//Callbacks
OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesClient* client, const OctopipesMessage*));
OctopipesError octopipes_set_sent_cb(OctopipesClient* client, void (*on_sent)(const OctopipesClient* client, const OctopipesMessage*));
OctopipesError octopipes_set_receive_error_cb(OctopipesClient* client, void (*on_receive_error)(const OctopipesClient* client, const OctopipesError));
OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)(const OctopipesClient* client));
OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)(const OctopipesClient* client));

//@! Server

//Alloc
OctopipesServerError octopipes_server_init(OctopipesServer** server, const char* cap_path, const char* client_folder, const OctopipesVersion version);
OctopipesServerError octopipes_server_cleanup(OctopipesServer* server);
//CAP
OctopipesServerError octopipes_server_start_cap_listener(OctopipesServer* server);
OctopipesServerError octopipes_server_stop_cap_listener(OctopipesServer* server);
OctopipesServerError octopipes_server_lock_cap(OctopipesServer* server);
OctopipesServerError octopipes_server_unlock_cap(OctopipesServer* server);
OctopipesServerError octopipes_server_write_cap(OctopipesServer* server, const char* client, const uint8_t* data, const size_t data_size);
OctopipesServerError octopipes_server_process_cap_once(OctopipesServer* server, size_t* requests);
OctopipesServerError octopipes_server_process_cap_all(OctopipesServer* server, size_t* requests);
OctopipesServerError octopipes_server_handle_cap_message(OctopipesServer* server, const OctopipesMessage* message);
//Workers
OctopipesServerError octopipes_server_start_worker(OctopipesServer* server, const char* client, const char** subscriptions, const size_t subscription_len, const char* cli_tx_pipe, const char* cli_rx_pipe);
OctopipesServerError octopipes_server_stop_worker(OctopipesServer* server, const char* client);
OctopipesServerError octopipes_server_dispatch_message(OctopipesServer* server, const OctopipesMessage* message, const char** worker);
OctopipesServerError octopipes_server_process_first(OctopipesServer* server, size_t* requests, const char** client);
OctopipesServerError octopipes_server_process_once(OctopipesServer* server, size_t* requests, const char** client);
OctopipesServerError octopipes_server_process_all(OctopipesServer* server, size_t* requests, const char** client);
//Getters
OctopipesServerError octopipes_server_is_subscribed(OctopipesServer* server, const char* client);
OctopipesServerError octopipes_server_get_subscriptions(OctopipesServer* server, const char* client, char*** subscriptions, size_t* sub_len);
OctopipesServerError octopipes_server_get_clients(OctopipesServer* server, char*** clients, size_t* cli_len);

//Errors
const char* octopipes_get_error_desc(const OctopipesError error);
const char* octopipes_server_get_error_desc(const OctopipesServerError error);

#ifdef __cplusplus
}
#endif

#endif
