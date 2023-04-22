#ifndef SPECIFICATIONS_H
#define SPECIFICATIONS_H

// to fix windows int64_t typedef issues...
#include <stdint.h>
#include <string>
#include <vector>

// Define NA_INT64
const int64_t NA_INT64 = 1ULL << 63;

// the lengths of the message types ordered based on their ASCII table positions
// To get the respective positions of a message 'msg' (e.g., 'Q') use MSG_SIZES[msg - 'A'];
const int MSG_SIZES [] = {
  // A   B   C   D   E   F  G   H   I   J   K   L  M   N  O   P   Q   R   S  T
  36, 19, 36, 19, 31, 40, 0, 25, 50, 35, 28, 26, 0, 20, 0, 44, 40, 39, 12, 0,
  // U   V   W   X   Y  Z  [  \  ]  ^  _  `  a  b  c  d  e  f  g   h
  35, 35, 12, 23, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21
};
// the names of the message types
const unsigned char MSG_NAMES [] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
  '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
};
// the number of message types in MSG_SIZES, MSG_NAMES, value is 40...
const int N_TYPES = sizeof(MSG_SIZES) / sizeof(MSG_SIZES[0]);

// the names of the messages we actually use
const unsigned char ACT_MSG_NAMES [] = {
  'S', 'R', 'H', 'Y', 'L', 'V', 'W', 'K', 'J', 'h', 'A', 'F', 'E', 'C', 'X',
  'D', 'U', 'P', 'Q', 'B', 'I', 'N'
};
const int N_ACT_MSGS = sizeof(ACT_MSG_NAMES) / sizeof(ACT_MSG_NAMES[0]);

// The message classes (or groups)
const std::vector<std::string> MSG_CLASSES {
  "system_events",
  "stock_directory",
  "trading_status",
  "reg_sho",
  "market_participant_states",
  "mwcb",
  "ipo",
  "luld",
  "orders",
  "modifications",
  "trades",
  "noii",
  "rpii"
};

// How many classes there are
const int MSG_CLASS_SIZE = MSG_CLASSES.size();

// translates msg_type to MSG_CLASSES position
// e.g., msg_type 'h' has value 2, belongs to the third class in MSG_CLASSES: trading_status
const int TYPE_CLASS_TRANSLATOR [] = {
// A   B  C  D  E  F   G  H   I  J  K  L   M   N   O   P   Q  R  S  T
   8, 10, 9, 9, 9, 8, -1, 2, 11, 7, 6, 4, -1, 12, -1, 10, 10, 1, 0, -1,
// U  V  W  X  Y   Z   [   \   ]   ^   _   `   a   b   c   d   e   f   g  h
   9, 5, 5, 9, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2
};

#endif //SPECIFICATIONS_H
