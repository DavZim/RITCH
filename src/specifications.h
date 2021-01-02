#ifndef SPECIFICATIONS_H
#define SPECIFICATIONS_H

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
const char MSG_NAMES [] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
  '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
};
// the number of message types in MSG_SIZES, MSG_NAMES, value is 40...
const int N_TYPES = sizeof(MSG_SIZES) / sizeof(MSG_SIZES[0]);

// the names of the messages we actually use
const char ACT_MSG_NAMES [] = {
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

// small helper function to get the message size for a char
inline int get_message_size(const char msg) {
  return MSG_SIZES[msg - 'A'] + 2;
}

/*
 * @brief      Formats an integer number to a std::string with thousands separator
 *
 * @param      num    The number to format
 * @param      sep    The thousands separator, default value is a comma
 * @param      s      The return string, this is only used internally, as the function
 *                    is called recursively
 *
 * @return       The number as a string
 */
inline std::string format_thousands(int64_t num,
                            const std::string sep = ",",
                            const std::string s = "") {
  if (num < 1000) {
    return std::to_string(num) + s;
  } else {
    std::string last_three = std::to_string(num % 1000);
    const int num_zeros = 3 - last_three.length();
    last_three = std::string(num_zeros, '0').append(last_three);
    
    const int64_t remainder = (int64_t) num / 1000;
    const std::string res = sep + last_three + s;
    return format_thousands(remainder, sep, res);
  }
}

#endif //SPECIFICATIONS_H
