#include "helper_functions.h"


// small helper function to get the message size for a char
int get_message_size(const char msg) {
  return MSG_SIZES[msg - 'A'] + 2;
}

// the count_messages_internal function is optimized and therefore contains
// unused messages (they are used for faster access speeds!) 
// (see also Specifications.h)
// this function extracts the needed message classes from the raw vector
std::vector<int64_t> take_needed_messages(std::vector<int64_t> &v) {
  std::vector<int64_t> res;
  for (const char act_msg : ACT_MSG_NAMES) {
    size_t i = 0;
    for (const char msg : MSG_NAMES) {
      if (msg == act_msg) {
        res.push_back(v[i]);
        break;
      }
      i++;
    }
  }
  return res;
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
std::string format_thousands(int64_t num,
                             const std::string sep,
                             const std::string s) {
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

// #############################################################################
// small internal helper function to convert bytes etc
// #############################################################################

/**
 * @brief      Converts 2 bytes from a buffer in big endian to an int32_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted integer
 */
int32_t get2bytes(char* buf) {
  return __builtin_bswap16(*reinterpret_cast<uint16_t*>(&buf[0]));
}

/**
 * @brief      Converts 4 bytes from a buffer in big endian to an int32_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted integer
 */
int32_t get4bytes(char* buf) {
  return __builtin_bswap32(*reinterpret_cast<uint32_t*>(&buf[0]));
}

/**
 * @brief      Converts 6 bytes from a buffer in big endian to an int64_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted int64_t
 */
int64_t get6bytes(char* buf) {
  return (__builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0])) & 0xFFFFFFFFFFFF0000) >> 16;
}

/**
 * @brief      Converts 8 bytes from a buffer in big endian to an int64_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted int64_t
 */
int64_t get8bytes(char* buf) {
  return __builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0]));
}

// return N bytes of a buffer as a string
std::string getNBytes(char* buf, const int n, const char empty) {
  std::string res;
  for (int i = 0; i < n; ++i) if (buf[i] != empty) res += buf[i];
  return res;
}

// converts a Numeric Vector to int64
Rcpp::NumericVector to_int64(Rcpp::NumericVector v) {
  v.attr("class") = "integer64";
  return v;
}

// helper functions that check if a buffer value is in a vector of filters 
// equivalent of R buf_val %in% filter
bool passes_filter(char* buf, std::vector<char> &filter) {
  if (filter.size() == 0) return true;
  for (char cc : filter) if (cc == *buf) return true;
  return false;
}
// same helper function as before but for int vector
bool passes_filter(char* buf, std::vector<int> &filter) {
  if (filter.size() == 0) return true;
  const int val = get2bytes(&buf[0]);
  for (int cc : filter) if (cc == val) return true;
  return false;
}
// check larger/smaller inclusive for 8 byte numbers (timestamp)
// equivalent to R (buf_val >= lower & buf_val <= upper)
bool passes_filter_in(char* buf,
                      std::vector<int64_t> &lower, 
                      std::vector<int64_t> &upper) {
  // lower and upper have the same size!
  if (lower.size() == 0) return true;
  const int64_t val = get6bytes(&buf[0]);
  for (size_t i = 0; i < lower.size(); i++) {
    if (val >= lower[i] && val <= upper[i]) return true;
  }
    
  return false;
}

// sets inside a char buffer b, 2 bytes from the value val, returns number of bytes changed
// i.e., convert val = 8236 to 0x202c
uint64_t set2bytes(char* b, int32_t val) {
  b[1] = val         & 0xff;
  b[0] = (val >> 8)  & 0xff;
  // Rprintf("Converting: %15i -> 0x %02x %02x\n",
  //         val, b[0], b[1]);
  return 2;
}

// sets inside a char buffer b, 4 bytes from the value val, returns number of bytes changed
// i.e., convert val = 11900 to 0x00002e7c
uint64_t set4bytes(char* b, int32_t val) {
  b[3] = val         & 0xffff;
  b[2] = (val >> 8)  & 0xffff;
  b[1] = (val >> 16) & 0xffff;
  b[0] = (val >> 24) & 0xffff;
  // Rprintf("Converting: %15i -> 0x %02x %02x %02x %02x\n",
  //         val, b[0], b[1], b[2], b[3]);
  return 4;
}
// sets inside a char buffer b, 6 bytes from the value val, returns number of bytes changed
// i.e., 25200002107428 to 0x16eb552c8824
uint64_t set6bytes(char* b, int64_t val) {
  b[5] = val         & 0xffffff;
  b[4] = (val >> 8)  & 0xffffff;
  b[3] = (val >> 16) & 0xffffff;
  b[2] = (val >> 24) & 0xffffff;
  b[1] = (val >> 32) & 0xffffff;
  b[0] = (val >> 40) & 0xffffff;
  // Rprintf("Converting: %15lld -> 0x %02x %02x %02x %02x %02x %02x\n",
  //         (long long) val, b[0], b[1], b[2], b[3], b[4], b[5]);
  return 6;
}
// sets inside a char buffer b, 8 bytes from the value val, returns number of bytes changed
// i.e., 4 to 0x0000000000000004
uint64_t set8bytes(char* b, int64_t val) {
  b[7] = val         & 0xffffffff;
  b[6] = (val >> 8)  & 0xffffffff;
  b[5] = (val >> 16) & 0xffffffff;
  b[4] = (val >> 24) & 0xffffffff;
  b[3] = (val >> 32) & 0xffffffff;
  b[2] = (val >> 40) & 0xffffffff;
  b[1] = (val >> 48) & 0xffffffff;
  b[0] = (val >> 56) & 0xffffffff;
  // Rprintf("Converting: %15lld -> 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  //         (long long) val, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
  return 8;
}
// sets inside a char buffer b, n bytes from the string x, returns number of bytes changed
// i.e., "UFO" with 8 to 0x55534f2020202020 (filled with whitespaces)
uint64_t setCharBytes(char* b, std::string x, uint64_t n) {
  char *st = new char[n + 1];
  if (x.size() > n) Rprintf("ERROR: setChar Bytes for string '%s' larger than capacity %i\n", x.c_str(), n);
  for (uint64_t j = 0; j < n; j++) st[j] = ' '; // fill with n spaces
  for (uint64_t j = 0; j < x.size(); j++) st[j] = x[j]; // copy the string x
  memcpy(b, st, n);
  // Rprintf("Set %i char Bytes from '%s' -> 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  //         n, x.c_str(), b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
  return n;
}
