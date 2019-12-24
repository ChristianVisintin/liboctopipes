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

#include <octopipespp/octopipespp.hpp>
#include <getopt.h>
#include <cstring>

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
 * - octopipes_cap_prepare_subscribe
 * - octopipes_cap_prepare_assign
 * - octopipes_cap_prepare_unsubscribe
 * - octopipes_cap_get_message
 * - octopipes_cap_parse_subscribe
 * - octopipes_cap_parse_assign
 * - octopipes_cap_parse_unsubscribe
 * NOTE: This test JUST tests encoding/decoding functions
 */

int test_endecoding() {
  //Test encoding and decoding functions
  octopipes::Error rc;
  //Prepare a dummy data
  uint8_t payload[32];
  for (size_t i = 0; i < 32; i++) {
    payload[i] = i;
  }
  //Instance message
  octopipes::Message* message = new octopipes::Message(octopipes::ProtocolVersion::VERSION_1, ORIGIN, REMOTE, payload, 32, octopipes::Options::NONE, 60);
  //Encode message
  uint8_t* data;
  size_t data_size;
  if ((rc = message->encodeData(data, data_size)) != octopipes::Error::SUCCESS) {
    printf("%sCould not encode message: %s%s\n", KRED, octopipes::Client::getErrorDesc(rc).c_str(), KNRM);
    return static_cast<int>(rc);
  }
  printf("%sVerifying encoded message%s\n", KYEL, KNRM);
  const uint8_t checksum = message->getChecksum();
  //Free message
  //Verify if data is coherent
  printf("%s(Data size: %zu) Data dump: ", KYEL, data_size);
  for (size_t i = 0; i < data_size; i++) {
    printf("%02x ", data[i]);
  }
  printf("%s\n", KNRM);
  delete message;
  if (data[0] != 0x01) {
    printf("%sData[0] (SOH) should be 01, but is %02x%s\n", KRED, data[0], KNRM);
    free(data);
    return 1;
  }
  //Version
  if (data[1] != static_cast<uint8_t>(octopipes::ProtocolVersion::VERSION_1)) {
    printf("%sData[1] (VER) should be %02x, but is %02x%s\n", KRED, static_cast<uint8_t>(octopipes::ProtocolVersion::VERSION_1), data[1], KNRM);
    free(data);
    return 1;
  }
  //Local Node size
  if (data[2] != ORIGIN_SIZE) {
    printf("%sData[2] (LNS) should be %02x, but is %02x%s\n", KRED, ORIGIN_SIZE, data[2], KNRM);
    free(data);
    return 1;
  }
  //Local node
  if (memcmp(data + 3, ORIGIN, ORIGIN_SIZE) != 0) {
    printf("%sData[3-13] (LND) should be %s, but is %.*s%s\n", KRED, ORIGIN, ORIGIN_SIZE, data + 3, KNRM);
    free(data);
    return 1;
  }
  //Remote node size
  size_t ptr = 3 + ORIGIN_SIZE;
  if (data[ptr] != REMOTE_SIZE) {
    printf("%sData[%zu] (RNS) should be %02x, but is %02x%s\n", KRED, ptr, REMOTE_SIZE, data[ptr], KNRM);
    free(data);
    return 1;
  }
  ptr++;
  //Remote node
  if (memcmp(data + ptr, REMOTE, REMOTE_SIZE) != 0) {
    printf("%sData[%zu - %zu] (RND) should be %s, but is %.*s%s\n", KRED, ptr, ptr + REMOTE_SIZE, REMOTE, REMOTE_SIZE, data + ptr, KNRM);
    free(data);
    return 1;
  }
  ptr += REMOTE_SIZE;
  //TTL
  if (data[ptr++] != 60) {
    printf("%sData[%zu] (TTL) should be %02x, but is %02x%s\n", KRED, ptr, 60, data[ptr - 1], KNRM);
    free(data);
    return 1;
  }
  //Data Size (8 bytes; 0th->6th => 0; 7th => 32)
  for (int i = 0; i < 7; i++) {
    if (data[ptr++] != 0x00) { //0th->6th
      printf("%sData[%zu] (DSZ) should be %02x, but is %02x%s\n", KRED, ptr, 0x00, data[ptr - 1], KNRM);
      free(data);
      return 1;
    }
  }
  if (data[ptr++] != 32) { //7th
    printf("%sData[%zu] (DSZ) should be %02x, but is %02x%s\n", KRED, ptr, 32, data[ptr - 1], KNRM);
    free(data);
    return 1;
  }
  //Check Options
  if (data[ptr++] != 0x00) {
    printf("%sData[%zu] (OPT) should be %02x, but is %02x%s\n", KRED, ptr, 0x00, data[ptr - 1], KNRM);
    free(data);
    return 1;
  }
  //Check Checksum
  if (data[ptr++] != checksum) {
    printf("%sData[%zu] (CHK) should be %02x, but is %02x%s\n", KRED, ptr, checksum, data[ptr - 1], KNRM);
    free(data);
    return 1;
  }
  //Check STX
  if (data[ptr++] != 0x02) {
    printf("%sData[%zu] (STX) should be %02x, but is %02x%s\n", KRED, ptr, 0x02, data[ptr - 1], KNRM);
    free(data);
    return 1;
  }
  //Check data
  for (size_t i = 0; i < 32; i++) {
    if (data[ptr++] != i) {
      printf("%sData[%zu] (DAT) should be %02zx, but is %02x%s\n", KRED, ptr, i, data[ptr - 1], KNRM);
      free(data);
      return 1;
    }
  }
  //Check ETX
  if (data[ptr++] != 0x03) {
    printf("%sData[%zu] (ETX) should be %02x, but is %02x%s\n", KRED, ptr, 0x03, data[ptr - 1], KNRM);
    free(data);
    return 1;
  }
  printf("%sPacket was encoded correctly%s\n", KYEL, KNRM);
  //Now decode packet and check if it's correct
  message = new octopipes::Message();
  if ((rc = message->decodeData(data, data_size)) != octopipes::Error::SUCCESS) {
    printf("%sCould not decode data: %s%s\n", KRED, octopipes::Client::getErrorDesc(rc).c_str(), KNRM);
    return static_cast<int>(rc);
  }
  //Free data
  free(data);
  printf("%sVerifying decoded message...%s\n", KYEL, KNRM);
  //Verify message params
  if (message->getVersion() != octopipes::ProtocolVersion::VERSION_1) {
    printf("%sVersion should be %02x but is %02x%s\n", KRED, octopipes::ProtocolVersion::VERSION_1, message->getVersion(), KNRM);
    delete message;
    return 1;
  }
  if (message->getOrigin() != ORIGIN) {
    printf("%sOrigin should be %s but is %s%s\n", KRED, ORIGIN, message->getOrigin().c_str(), KNRM);
    delete message;
    return 1;
  }
  if (message->getRemote() != REMOTE) {
    printf("%sRemote should be %s but is %s%s\n", KRED, REMOTE, message->getRemote().c_str(), KNRM);
    delete message;
    return 1;
  }
  if (message->getTTL() != 60) {
    printf("%sTTLshould be %02x but is %02x%s\n", KRED, 60, message->getTTL(), KNRM);
    delete message;
    return 1;
  }
  if (message->getChecksum() != checksum) {
    printf("%sChecksum should be %02x but is %02x%s\n", KRED, checksum, message->getChecksum(), KNRM);
    delete message;
    return 1;
  }
  size_t decoded_payload_size;
  const uint8_t* decoded_payload = message->getPayload(decoded_payload_size);
  if (decoded_payload_size != 32) {
    printf("%sData size should be %02x but is %02zx%s\n", KRED, 32, decoded_payload_size, KNRM);
    delete message;
    return 1;
  }
  if (memcmp(decoded_payload, payload, 32) != 0) {
    printf("%sData should be ", KRED);
    for (size_t i = 0; i < 32; i++)
      printf("%02x ", (char) payload[i]);
    printf(" but is ");
    for (size_t i = 0; i < decoded_payload_size; i++) {
      printf("%02x ", (char) decoded_payload[i]);
    }
    printf("%s\n", KNRM);
    delete message;
    return 1;
  }
  if (message->getOption(octopipes::Options::NONE)) {
    printf("%sOptions should be %02x but is 1%s\n", KRED, 0, KNRM);
    delete message;
    return 1;
  }
  //Free message
  delete message;
  printf("%sDecoding verification succeded%s\n", KYEL, KNRM);
  //Test errors
  printf("%sTesting alternative flows%s\n", KYEL, KNRM);
  message = new octopipes::Message();
  if ((rc = message->decodeData(NULL, 0)) != octopipes::Error::BAD_PACKET) {
    printf("%smessage->decodeData(NULL, 0) should have returned BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    delete message;
    return 1;
  }
  delete message;
  message = new octopipes::Message();
  uint8_t bad_soh_data[17] = {0x00};
  if ((rc = message->decodeData(bad_soh_data, 17)) != octopipes::Error::BAD_PACKET) {
    printf("%smessage->decodeData(bad_soh_data, 17,) should have returned BAD_PACKET, but returned %d %s\n", KRED, rc, KNRM);
    delete message;
    return 1;
  }
  delete message;
  message = new octopipes::Message(static_cast<octopipes::ProtocolVersion>(0), ORIGIN, REMOTE, NULL, 0, octopipes::Options::NONE, 60);
  //Unsupported protocol version
  if ((rc = message->encodeData(data, data_size)) != octopipes::Error::UNSUPPORTED_VERSION) {
    printf("%smessage->encodeData(data, data_size) should have returned UNSUPPORTED_VERSION, but returned %d %s\n", KRED, rc, KNRM);
    delete message;
    return 1;
  }
  delete message;
  return 0;
}

int main(int argc, char* argv[]) {
  printf(PROGRAM_NAME " liboctopipes Build: " OCTOPIPESPP_LIB_VERSION "\n");
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
  return static_cast<int>(rc); //Sum of error codes
}
