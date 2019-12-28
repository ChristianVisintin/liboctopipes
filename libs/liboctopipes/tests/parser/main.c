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
#include <octopipes/serializer.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define PROGRAM_NAME "test_parser"
#define USAGE PROGRAM_NAME "Usage: " PROGRAM_NAME " [Options]\n\
\t -h\t\tShow this page\n\
"

//Colors
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define ORIGIN "test_parser"
#define ORIGIN_SIZE 11
#define REMOTE "BROADCAST"
#define REMOTE_SIZE 9

/**
 * Test Description: test_parser tests the functions which encodes and decodes the payloads; CAP messages are covered too
 * - encodes normal octopipes payloads
 * - parses normal octopipes payloads
 * - encodes all kind of CAP messages
 * - parses all kind of CAP messages
 * Functions covered by this test:
 * - octopipes_decode
 * - octopipes_encode
 * - calculate_checksum
 * - octopipes_cap_prepare_subscription
 * - octopipes_cap_prepare_assign
 * - octopipes_cap_prepare_unsubscription
 * - octopipes_cap_get_message
 * - octopipes_cap_parse_subscribe
 * - octopipes_cap_parse_assign
 * - octopipes_cap_parse_unsubscribe
 * NOTE: This test JUST tests encoding/decoding functions
 */

int test_endecoding() {
  //Test encoding and decoding functions
  OctopipesError rc;
  //Prepare a dummy data
  uint8_t payload[32];
  for (size_t i = 0; i < 32; i++) {
    payload[i] = i;
  }
  OctopipesMessage* message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  message->origin = NULL;
  message->remote = NULL;
  message->data = NULL;
  //Fill message fields
  message->version = OCTOPIPES_VERSION_1;
  message->origin = ORIGIN;
  message->origin_size = ORIGIN_SIZE;
  message->remote = REMOTE;
  message->remote_size = REMOTE_SIZE;
  message->options = 0;
  message->ttl = 60; //60 seconds
  message->data_size = 32;
  message->data = payload;
  //Encode message
  uint8_t* data;
  size_t data_size;
  if ((rc = octopipes_encode(message, &data, &data_size)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not encode message: %s%s\n", KRED, octopipes_get_error_desc(rc), KNRM);
    return rc;
  }
  printf("%sVerifying encoded message%s\n", KYEL, KNRM);
  const uint8_t checksum = calculate_checksum(message);
  //Free message
  //Verify if data is coherent
  printf("%s(Data size: %zu) Data dump: ", KYEL, data_size);
  for (size_t i = 0; i < data_size; i++) {
    printf("%02x ", data[i]);
  }
  printf("%s\n", KNRM);
  free(message); //Don't cleanup since remote, origin and data are in stack
  if (data[0] != 0x01) {
    printf("%sData[0] (SOH) should be 01, but is %02x%s\n", KRED, data[0], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Version
  if (data[1] != OCTOPIPES_VERSION_1) {
    printf("%sData[1] (VER) should be %02x, but is %02x%s\n", KRED, OCTOPIPES_VERSION_1, data[1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Local Node size
  if (data[2] != ORIGIN_SIZE) {
    printf("%sData[2] (LNS) should be %02x, but is %02x%s\n", KRED, ORIGIN_SIZE, data[2], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Local node
  if (memcmp(data + 3, ORIGIN, ORIGIN_SIZE) != 0) {
    printf("%sData[3-13] (LND) should be %s, but is %.*s%s\n", KRED, ORIGIN, ORIGIN_SIZE, data + 3, KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Remote node size
  size_t ptr = 3 + ORIGIN_SIZE;
  if (data[ptr] != REMOTE_SIZE) {
    printf("%sData[%zu] (RNS) should be %02x, but is %02x%s\n", KRED, ptr, REMOTE_SIZE, data[ptr], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  ptr++;
  //Remote node
  if (memcmp(data + ptr, REMOTE, REMOTE_SIZE) != 0) {
    printf("%sData[%zu - %zu] (RND) should be %s, but is %.*s%s\n", KRED, ptr, ptr + REMOTE_SIZE, REMOTE, REMOTE_SIZE, data + ptr, KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  ptr += REMOTE_SIZE;
  //TTL
  if (data[ptr++] != 60) {
    printf("%sData[%zu] (TTL) should be %02x, but is %02x%s\n", KRED, ptr, 60, data[ptr - 1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Data Size (8 bytes; 0th->6th => 0; 7th => 32)
  for (int i = 0; i < 7; i++) {
    if (data[ptr++] != 0x00) { //0th->6th
      printf("%sData[%zu] (DSZ) should be %02x, but is %02x%s\n", KRED, ptr, 0x00, data[ptr - 1], KNRM);
      free(data);
      return OCTOPIPES_ERROR_BAD_PACKET;
    }
  }
  if (data[ptr++] != 32) { //7th
    printf("%sData[%zu] (DSZ) should be %02x, but is %02x%s\n", KRED, ptr, 32, data[ptr - 1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Check Options
  if (data[ptr++] != 0x00) {
    printf("%sData[%zu] (OPT) should be %02x, but is %02x%s\n", KRED, ptr, 0x00, data[ptr - 1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Check Checksum
  if (data[ptr++] != checksum) {
    printf("%sData[%zu] (CHK) should be %02x, but is %02x%s\n", KRED, ptr, checksum, data[ptr - 1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Check STX
  if (data[ptr++] != 0x02) {
    printf("%sData[%zu] (STX) should be %02x, but is %02x%s\n", KRED, ptr, 0x02, data[ptr - 1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Check data
  for (size_t i = 0; i < 32; i++) {
    if (data[ptr++] != i) {
      printf("%sData[%zu] (DAT) should be %02zx, but is %02x%s\n", KRED, ptr, i, data[ptr - 1], KNRM);
      free(data);
      return OCTOPIPES_ERROR_BAD_PACKET;
    }
  }
  //Check ETX
  if (data[ptr++] != 0x03) {
    printf("%sData[%zu] (ETX) should be %02x, but is %02x%s\n", KRED, ptr, 0x03, data[ptr - 1], KNRM);
    free(data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  printf("%sPacket was encoded correctly%s\n", KYEL, KNRM);
  //Now decode packet and check if it's correct
  if ((rc = octopipes_decode(data, data_size, &message)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not decode data: %s%s\n", KRED, octopipes_get_error_desc(rc), KNRM);
    return rc;
  }
  //Free data
  free(data);
  printf("%sVerifying decoded message...%s\n", KYEL, KNRM);
  //Verify message params
  if (message->version != OCTOPIPES_VERSION_1) {
    printf("%sVersion should be %02x but is %02x%s\n", KRED, OCTOPIPES_VERSION_1, message->version, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (message->origin_size != ORIGIN_SIZE) {
    printf("%sOrigin size should be %02x but is %02x%s\n", KRED, OCTOPIPES_VERSION_1, message->version, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (memcmp(message->origin, ORIGIN, ORIGIN_SIZE)) {
    printf("%sOrigin should be %s but is %.*s%s\n", KRED, ORIGIN, message->origin_size, message->origin, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (message->remote_size != REMOTE_SIZE) {
    printf("%sRemote size should be %02x but is %02x%s\n", KRED, REMOTE_SIZE, message->remote_size, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (memcmp(message->remote, REMOTE, REMOTE_SIZE)) {
    printf("%sRemote should be %s but is %.*s%s\n", KRED, REMOTE, message->remote_size, message->remote, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (message->ttl != 60) {
    printf("%sTTLshould be %02x but is %02x%s\n", KRED, 60, message->ttl, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (message->checksum != checksum) {
    printf("%sChecksum should be %02x but is %02x%s\n", KRED, checksum, message->checksum, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (message->data_size != 32) {
    printf("%sData size should be %02x but is %02llx%s\n", KRED, 32, message->data_size, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (memcmp(message->data, payload, 32) != 0) {
    printf("%sData should be ", KRED);
    for (size_t i = 0; i < 32; i++)
      printf("%02x ", (char) payload[i]);
    printf(" but is ");
    for (size_t i = 0; i < message->data_size; i++) {
      printf("%02x ", (char) message->data[i]);
    }
    printf("%s\n", KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  if (message->options != OCTOPIPES_OPTIONS_NONE) {
    printf("%sOptions should be %02x but is %02x%s\n", KRED, OCTOPIPES_OPTIONS_NONE, message->options, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Free message
  octopipes_cleanup_message(message);
  printf("%sDecoding verification succeded%s\n", KYEL, KNRM);
  //Test errors
  printf("%sTesting alternative flows%s\n", KYEL, KNRM);
  if ((rc = octopipes_decode(NULL, 0, &message)) != OCTOPIPES_ERROR_BAD_PACKET) {
    printf("%soctopipes_decode(NULL, 0, &message) should have returned BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  uint8_t bad_soh_data[17] = {0x00};
  if ((rc = octopipes_decode(bad_soh_data, 17, &message)) != OCTOPIPES_ERROR_BAD_PACKET) {
    printf("%soctopipes_decode(bad_soh_data, 17, &message) should have returned BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Unsupported protocol version
  message = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  message->remote = NULL;
  message->origin = NULL;
  message->data = NULL;
  message->version = 0;
  if ((rc = octopipes_encode(message, &data, &data_size)) != OCTOPIPES_ERROR_UNSUPPORTED_VERSION) {
    printf("%soctopipes_encode should have returned OCTOPIPES_ERROR_UNSUPPORTED_VERSION, but returned %d %s\n", KRED, rc, KNRM);
    octopipes_cleanup_message(message);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  octopipes_cleanup_message(message);
  return 0;
}

/**
 * @brief encode and then parse a CAP subscribe payload
 * @return int rc
 */

int test_cap_subscribe() {
  OctopipesError rc;
  printf("%sEncoding a suscribe for groups: 'hardware', 'display', 'drivers'%s\n", KYEL, KNRM);
  char** groups = (char**) malloc(sizeof(char*) * 3);
  const char* group_hardware = "hardware";
  const char* group_display = "display";
  const char* group_drivers = "drivers";
  groups[0] = (char*) group_hardware;
  groups[1] = (char*) group_display;
  groups[2] = (char*) group_drivers;
  //Encode subscribe
  size_t data_size;
  uint8_t* subscribe_data = octopipes_cap_prepare_subscription((const char**) groups, 3, &data_size);
  if (subscribe_data == NULL) {
    printf("%sCould not prepare subscribe; data is invalid%s\n", KRED, KNRM);
  }
  //Dump data
  printf("%ssubscribe data: ", KYEL);
  for (size_t i = 0; i < data_size; i++) {
    printf("%02x ", subscribe_data[i]);
  }
  printf("%s\n", KNRM);
  //Check if it is a subscribe data
  OctopipesCapMessage message_type = octopipes_cap_get_message(subscribe_data, data_size);
  if (message_type != OCTOPIPES_CAP_SUBSCRIBPTION) {
    printf("%sMessage encoded is not subscribed type (%d)%s\n", KRED, message_type, KNRM);
    free(subscribe_data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  printf("%sMessage type is subscribe!%s\n", KYEL, KNRM);
  //Parse subscribe data back
  char** parsed_groups;
  size_t parsed_groups_amount;
  if ((rc = octopipes_cap_parse_subscribe(subscribe_data, data_size, &parsed_groups, &parsed_groups_amount)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not parse subscribe payload: %s%s\n", KRED, octopipes_get_error_desc(rc), KNRM);
    free(subscribe_data);
    return rc;
  }
  free(subscribe_data);
  if (parsed_groups_amount != 3) {
    printf("%sExpected %d groups, but got %zu%s\n", KRED, 3, parsed_groups_amount, KNRM);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Verify groups
  for (size_t i = 0; i < parsed_groups_amount; i++) {
    if (strcmp(groups[i], parsed_groups[i]) != 0) {
      printf("%sGroup %zu mismatched: %s; %s%s\n", KRED, i, groups[i], parsed_groups[i], KNRM);
      for (size_t i = 0; i < parsed_groups_amount; i++) {
        free(parsed_groups[i]);
        free(groups[i]);
      }
      free(parsed_groups);
      free(groups);
      return OCTOPIPES_ERROR_BAD_PACKET;
    } else {
      printf("%sFound correct group %s%s\n", KYEL, parsed_groups[i], KNRM);
    }
  }
  //Free groups
  for (size_t i = 0; i < parsed_groups_amount; i++) {
    char* parsed_group = parsed_groups[i];
    free(parsed_group);
  }
  free(parsed_groups);
  free(groups);
  //CAP Subscribe was successful
  //Test errors
  uint8_t bad_subscribe_data[2] = {0xFF, 0x00};
  if ((rc = octopipes_cap_parse_subscribe(bad_subscribe_data, 2, &parsed_groups, &parsed_groups_amount)) != OCTOPIPES_ERROR_BAD_PACKET) {
    printf("%soctopipes_cap_parse_subscribe should have returned OCTOPIPES_ERROR_BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    return rc;
  }
  return 0;
}

/**
 * @brief encode and then parse a CAP assignment payload
 * @return int rc
 */

int test_cap_assignment() {
  OctopipesError rc;
  printf("%sEncoding an assignment with TX fifo /usr/share/octopipes/test_parser.tx.fifo and RX fifo /usr/share/octopipes/test_parser.rx.fifo %s\n", KYEL, KNRM);
  const char* tx_fifo = "/usr/share/octopipes/test_parser.tx.fifo";
  const char* rx_fifo = "/usr/share/octopipes/test_parser.rx.fifo";
  const size_t tx_fifo_size = strlen(tx_fifo);
  const size_t rx_fifo_size = strlen(rx_fifo);
  //Encode assignment
  size_t data_size;
  OctopipesCapError error = OCTOPIPES_CAP_ERROR_SUCCESS;
  uint8_t* assignment_data = octopipes_cap_prepare_assign(error, tx_fifo, tx_fifo_size, rx_fifo, rx_fifo_size, &data_size);
  if (assignment_data == NULL) {
    printf("%sCould not prepare assignment; data is invalid%s\n", KRED, KNRM);
  }
  //Dump data
  printf("%sassignment data: ", KYEL);
  for (size_t i = 0; i < data_size; i++) {
    printf("%02x ", assignment_data[i]);
  }
  printf("%s\n", KNRM);
  //Check if it is a assignment data
  OctopipesCapMessage message_type = octopipes_cap_get_message(assignment_data, data_size);
  if (message_type != OCTOPIPES_CAP_ASSIGNMENT) {
    printf("%sMessage encoded is not assignment type (%d)%s\n", KRED, message_type, KNRM);
    free(assignment_data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Parse subscribe data back
  char* parsed_tx_fifo;
  char* parsed_rx_fifo;
  if ((rc = octopipes_cap_parse_assign(assignment_data, data_size, &error, &parsed_tx_fifo, &parsed_rx_fifo)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not parse assign payload: %s%s\n", KRED, octopipes_get_error_desc(rc), KNRM);
    free(assignment_data);
    return rc;
  }
  free(assignment_data);
  //Compare fifos
  if (strcmp(parsed_tx_fifo, tx_fifo) != 0) {
    printf("%sparsed TX fifo mismatch from encoded one (%s; %s)%s", KRED, parsed_tx_fifo, tx_fifo, KNRM);
    free(parsed_rx_fifo);
    free(parsed_tx_fifo);
  }
  if (strcmp(parsed_rx_fifo, rx_fifo) != 0) {
    printf("%sparsed RX fifo mismatch from encoded one (%s; %s)%s", KRED, parsed_rx_fifo, rx_fifo, KNRM);
    free(parsed_rx_fifo);
    free(parsed_tx_fifo);
  }
  free(parsed_rx_fifo);
  free(parsed_tx_fifo);
  //CAP Assignment was successful
  //@! Test errors
  uint8_t bad_assign_data[4] = {0x01, 0x00, 0x00, 0x00};
  if ((rc = octopipes_cap_parse_assign(bad_assign_data, 4, &error, &parsed_tx_fifo, &parsed_rx_fifo)) != OCTOPIPES_ERROR_BAD_PACKET) {
    printf("%soctopipes_cap_parse_assign should have returned OCTOPIPES_ERROR_BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    return rc;
  }
  uint8_t bad_assign_data_2[4] = {0xFF, OCTOPIPES_CAP_ERROR_NAME_ALREADY_TAKEN, 0x00, 0x00};
  //Should return SUCCESS, since error is not 0
  if ((rc = octopipes_cap_parse_assign(bad_assign_data_2, 4, &error, &parsed_tx_fifo, &parsed_rx_fifo)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%soctopipes_cap_parse_assign should have returned OCTOPIPES_ERROR_SUCCESS, but returned %d %s\n", KRED, rc, KNRM);
    return rc;
  }
  printf("%sDetected successfull CAP error %d%s\n", KYEL, error, KNRM);
  uint8_t bad_assign_data_3[4] = {0xFF, 0x00, 0xF0, 0xF0};
  //Should return BAD_PACKET, object and error are corrects, but there are no fifos
  if ((rc = octopipes_cap_parse_assign(bad_assign_data_3, 4, &error, &parsed_tx_fifo, &parsed_rx_fifo)) != OCTOPIPES_ERROR_BAD_PACKET) {
    printf("%soctopipes_cap_parse_assign should have returned OCTOPIPES_ERROR_BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    return rc;
  }
  printf("%sSuccessfully detected CAP errors%s\n", KYEL, KNRM);
  return 0;
}

/**
 * @brief encode and parse an unsubscribe payload
 * @return int
 */

int test_cap_unsubscribe() {
  OctopipesError rc;
  printf("%sEncoding an unsubscribe payload%s\n", KYEL, KNRM);
  //Encode assignment
  size_t data_size;
  OctopipesCapError error = OCTOPIPES_CAP_ERROR_SUCCESS;
  uint8_t* unsubscribe_data = octopipes_cap_prepare_unsubscription(&data_size);
  if (unsubscribe_data == NULL) {
    printf("%sCould not prepare unsubscribe; data is invalid%s\n", KRED, KNRM);
  }
  //Dump data
  printf("%sunsubscribe data: ", KYEL);
  for (size_t i = 0; i < data_size; i++) {
    printf("%02x ", unsubscribe_data[i]);
  }
  printf("%s\n", KNRM);
  //Check if it is a assignment data
  OctopipesCapMessage message_type = octopipes_cap_get_message(unsubscribe_data, data_size);
  if (message_type != OCTOPIPES_CAP_UNSUBSCRIBPTION) {
    printf("%sMessage encoded is not of unsubscribe type (%d)%s\n", KRED, message_type, KNRM);
    free(unsubscribe_data);
    return OCTOPIPES_ERROR_BAD_PACKET;
  }
  //Parse unsubscribe data back
  if ((rc = octopipes_cap_parse_unsubscribe(unsubscribe_data, data_size)) != OCTOPIPES_ERROR_SUCCESS) {
    printf("%sCould not parse unsubscribe payload: %s%s\n", KRED, octopipes_get_error_desc(rc), KNRM);
    free(unsubscribe_data);
    return rc;
  }
  free(unsubscribe_data);
  //CAP Unsubscribe was successful
  //@! Test errors
  uint8_t bad_unsubscribe_data[1] = {0x01};
  if ((rc = octopipes_cap_parse_unsubscribe(bad_unsubscribe_data, 1)) != OCTOPIPES_ERROR_BAD_PACKET) {
    printf("%soctopipes_cap_parse_unsubscribe should have returned OCTOPIPES_ERROR_BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    return rc;
  }
  return 0;
}

int main(int argc, char** argv) {
  printf(PROGRAM_NAME " liboctopipes Build: " OCTOPIPES_LIB_VERSION "\n");
  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
    case 'h':
      printf("%s\n", USAGE);
      return 0;
    }
  }
  int rc = 0;
  int ret = 0;
  //Test 1. encodes a payload and verify the data once decoded
  if ((ret = test_endecoding()) != 0) {
    printf("%sEn/Decoding test failed: %d%s\n", KRED, ret, KNRM);
    rc += ret;
  }
  if (ret == 0)
    printf("%sEn/Decoding test passed!%s\n", KGRN, KNRM);
  //Test 2. CAP subscribe test
  if ((ret = test_cap_subscribe()) != 0) {
    printf("%sCAP subscribe test failed: %d%s\n", KRED, ret, KNRM);
    rc += ret;
  }
  if (ret == 0)
    printf("%sCAP subscribe test passed!%s\n", KGRN, KNRM);
  //Test 3. CAP assignment test
  if ((ret = test_cap_assignment()) != 0) {
    printf("%sCAP assignment test failed: %d%s\n", KRED, ret, KNRM);
    rc += ret;
  }
  if (ret == 0)
    printf("%sCAP assignment test passed!%s\n", KGRN, KNRM);
  //Test 4. CAP unsubscribe test
  if ((ret = test_cap_unsubscribe()) != 0) {
    printf("%sCAP unsubscribe test failed: %d%s\n", KRED, ret, KNRM);
    rc += ret;
  }
  if (ret == 0)
    printf("%sCAP unsubscribe test passed!%s\n", KGRN, KNRM);
  return rc; //Sum of error codes
}
