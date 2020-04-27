#include "MessageTypes.h"

/**
 * @brief      Converts 2 bytes from a buffer in big endian to an int32_t
 * 
 * @param      buf   The buffer as a pointer to an array of unsigned chars
 *
 * @return     The converted integer
 */
int32_t get2bytes(unsigned char* buf) {
  return __builtin_bswap16(*reinterpret_cast<uint16_t*>(&buf[0]));
}

/**
 * @brief      Converts 4 bytes from a buffer in big endian to an int32_t
 * 
 * @param      buf   The buffer as a pointer to an array of unsigned chars
 *
 * @return     The converted integer
 */
int32_t get4bytes(unsigned char* buf) {
  return __builtin_bswap32(*reinterpret_cast<uint32_t*>(&buf[0]));
}

/**
 * @brief      Converts 6 bytes from a buffer in big endian to an int64_t
 * 
 * @param      buf   The buffer as a pointer to an array of unsigned chars
 *
 * @return     The converted int64_t
 */
int64_t get6bytes(unsigned char* buf) {
  return (__builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0])) & 0xFFFFFFFFFFFF0000) >> 16;
}

/**
 * @brief      Converts 8 bytes from a buffer in big endian to an int64_t
 * 
 * @param      buf   The buffer as a pointer to an array of unsigned chars
 *
 * @return     The converted int64_t
 */
int64_t get8bytes(unsigned char* buf) {
  return __builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0]));
}

/**
 * @brief      Counts the number of valid messages for this messagetype, given a count-vector
 *
 * @param[in]  count  The count vecotr
 *
 * @return     The Number of valid messages.
 */
int64_t MessageType::countValidMessages(std::vector<int64_t> count) { 
  int64_t total = 0;
  for (int typePos : typePositions) {
      total += count[typePos];
  }
  return total; 
}

// virtual functions of the class MessageType, will be overloaded by the other classes
bool MessageType::loadMessage(unsigned char* buf) { return bool(); }
Rcpp::DataFrame MessageType::getDF() { return Rcpp::DataFrame(); }
void MessageType::reserve(int64_t size) {}


/**
 * @brief      Sets the boundaries, i.e., which message numbers should be actually parsed
 *
 * @param[in]  startMsgCount  The start message count, i.e., what is the first message to be parsed,
 *                             defaults to 0 (the first message)
 * @param[in]  endMsgCount    The end message count, i.e., what is the last message to be parsed,
 *                             defaults to +Inf (the last message)
 */
void MessageType::setBoundaries(int64_t startMsgCount, int64_t endMsgCount) {
  this->startMsgCount = startMsgCount;
  this->endMsgCount   = endMsgCount;
}

// ################################################################################
// ################################ ORDERS ########################################
// ################################################################################

/**
 * @brief      Loads the information from an order into the class, either of type 'A' or 'F'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool Orders::loadMessage(unsigned char* buf) {

  // first check if this is the wrong message
  bool rightMessage = false;
  for (unsigned char type : validTypes) {
    rightMessage = rightMessage || buf[0] == type;
  }
  
  // if the message is of the wrong type, terminate here, but continue with the next message
  if (!rightMessage) return true;
  
  // if the message is out of bounds (i.e., we dont want to collect it yet!)
  if (messageCount < startMsgCount) {
    ++messageCount;
    return true;
  }

  // if the message is out of bounds (i.e., we dont want to collect it ever, 
  // thus aborting the information gathering (return false!))
  // no need to iterate over all the other messages.
  if (messageCount > endMsgCount) return false;
  
  // begin parsing the messages
  // else, we can continue to parse the message to the content vectors
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  int64_t ts = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &ts, sizeof(double));
  int64_t od = get8bytes(&buf[11]);
  std::memcpy(&(order_ref[current_idx]), &od, sizeof(double));
  buy[current_idx]             = buf[19] == 'B';
  shares[current_idx]          = get4bytes(&buf[20]);

  // 8 characters make up the stockname
  std::string stock_string;
  const unsigned char white = ' ';
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[24 + i] != white) stock_string += buf[24 + i];
  }
  stock[current_idx] = stock_string;
  price[current_idx] = (double) get4bytes(&buf[32]) / 10000.0;
  
  // 4 characters make up the MPID-string (if message type 'F')
  std::string mpid_string = "";
  if (buf[0] == 'F') { // type 'F' is an MPID order
    for (unsigned int i = 0; i < 4U; ++i) {
      if (buf[36 + i] != white) mpid_string += buf[36 + i];
    }
    mpid[current_idx] = mpid_string;
  } else {
    mpid[current_idx] = NA_STRING;
  }
  
  // increase the number of this message type
  ++messageCount;
  ++current_idx;
  return true;
}

/**
 * @brief      Converts the stored information into an Rcpp::DataFrame
 *
 * @return     The Rcpp::DataFrame
 */
Rcpp::DataFrame Orders::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  Rcpp::NumericVector oref = data["order_ref"];
  oref.attr("class") = "integer64";
  
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void Orders::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type        = data["msg_type"]        = Rcpp::CharacterVector(size);
  locate_code     = data["locate_code"]     = Rcpp::IntegerVector(size);
  tracking_number = data["tracking_number"] = Rcpp::IntegerVector(size);
  timestamp       = data["timestamp"]       = Rcpp::NumericVector(size);
  order_ref       = data["order_ref"]       = Rcpp::NumericVector(size);
  buy             = data["buy"]             = Rcpp::LogicalVector(size);
  shares          = data["shares"]          = Rcpp::IntegerVector(size);
  stock           = data["stock"]           = Rcpp::CharacterVector(size);
  price           = data["price"]           = Rcpp::NumericVector(size);
  mpid            = data["mpid"]            = Rcpp::CharacterVector(size);
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
}


// ################################################################################
// ################################ Trades ########################################
// ################################################################################

/**
 * @brief      Loads the information from an trades into the class, either of type 'P', 'Q', or 'B'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool Trades::loadMessage(unsigned char* buf) {

  // first check if this is the wrong message
  bool rightMessage = false;
  for (unsigned char type : validTypes) {
    rightMessage = rightMessage || buf[0] == type;
  }
  
  // if the message is of the wrong type, terminate here, but continue with the next message
  if (!rightMessage) return true;
  
  // if the message is out of bounds (i.e., we dont want to collect it yet!)
  if (messageCount < startMsgCount) {
    ++messageCount;
    return true;
  }

  // if the message is out of bounds (i.e., we dont want to collect it ever, 
  // thus aborting the information gathering (return false!))
  // no need to iterate over all the other messages.
  if (messageCount > endMsgCount) return false;

  // begin parsing the messages
  // else, we can continue to parse the message to the content vectors
  int64_t tmp;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  std::string stock_string;
  const unsigned char white = ' ';
  // only used in Message Q
  int64_t cross_shares;

  switch (buf[0]) {
    case 'P':
      tmp = get8bytes(&buf[11]);
      std::memcpy(&(order_ref[current_idx]), &tmp, sizeof(double));

      buy[current_idx]    = buf[19] == 'B';
      shares[current_idx] = get4bytes(&buf[20]);

      // 8 characters make up the stockname
      for (unsigned int i = 0; i < 8U; ++i) {
        if (buf[24 + i] != white) stock_string += buf[24 + i];
      }
      stock[current_idx]  = stock_string;
      price[current_idx]  = (double) get4bytes(&buf[32]) / 10000.0;
      tmp = get8bytes(&buf[36]);
      std::memcpy(&(match_number[current_idx]), &tmp, sizeof(double));
      // empty assigns
      cross_type[current_idx] = NA_STRING;
      break;

    case 'Q':
      cross_shares   = get8bytes(&buf[11]); // only Q has 8 byte shares... otherwise 4 bytes
      if (cross_shares >= INT32_MAX) Rcpp::Rcout <<
        "Warning, overflow for shares on message 'Q' at position " << 
          current_idx << "\n",
      shares[current_idx] = (int32_t) cross_shares;

      for (unsigned int i = 0; i < 8U; ++i) {
        if (buf[19 + i] != white) stock_string += buf[19 + i];
      }
      stock[current_idx]      = stock_string;
      price[current_idx]      = (double) get4bytes(&buf[27]) / 10000.0;
      tmp = get8bytes(&buf[31]);
      std::memcpy(&(match_number[current_idx]), &tmp, sizeof(double));
      cross_type[current_idx] = buf[39];
      //empty assigns
      std::memcpy(&(order_ref[current_idx]), &NA_INT64, sizeof(double));
      buy[current_idx] = NA_LOGICAL;
      break;

    case 'B': 
      tmp = get8bytes(&buf[11]);
      std::memcpy(&(match_number[current_idx]), &tmp, sizeof(double));
      // empty assigns
      std::memcpy(&(order_ref[current_idx]), &NA_INT64, sizeof(double));
      buy[current_idx]        = NA_LOGICAL;
      shares[current_idx]     = NA_INTEGER;
      stock[current_idx]      = NA_STRING;
      price[current_idx]      = NA_REAL;
      cross_type[current_idx] = NA_STRING;
      break;

    default:
      Rcpp::Rcout << "Unkown Type: " << buf[0] << "\n";
      break;
  }

  // increase the number of this message type
  ++messageCount;
  ++current_idx;
  return true;
}

/**
 * @brief      Converts the stored information into an Rcpp::DataFrame
 *
 * @return     The Rcpp::DataFrame
 */
Rcpp::DataFrame Trades::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  Rcpp::NumericVector oref = data["order_ref"];
  oref.attr("class") = "integer64";
  Rcpp::NumericVector mtch = data["match_number"];
  mtch.attr("class") = "integer64";

  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void Trades::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type        = data["msg_type"]        = Rcpp::CharacterVector(size);
  locate_code     = data["locate_code"]     = Rcpp::IntegerVector(size);
  tracking_number = data["tracking_number"] = Rcpp::IntegerVector(size);
  timestamp       = data["timestamp"]       = Rcpp::NumericVector(size);
  order_ref       = data["order_ref"]       = Rcpp::NumericVector(size);
  buy             = data["buy"]             = Rcpp::LogicalVector(size);
  shares          = data["shares"]          = Rcpp::IntegerVector(size);
  stock           = data["stock"]           = Rcpp::CharacterVector(size);
  price           = data["price"]           = Rcpp::NumericVector(size);
  match_number    = data["match_number"]    = Rcpp::NumericVector(size);
  cross_type      = data["cross_type"]      = Rcpp::CharacterVector(size);
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
}


// ################################################################################
// ################################ Modifications #################################
// ################################################################################

/**
 * @brief      Loads the information from an trades into the class, either of type 'P', 'Q', or 'B'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool Modifications::loadMessage(unsigned char* buf) {

  // first check if this is the wrong message
  bool rightMessage = false;
  for (unsigned char type : validTypes) {
    rightMessage = rightMessage || buf[0] == type;
  }
  
  // if the message is of the wrong type, terminate here, but continue with the next message
  if (!rightMessage) return true;

  // if the message is out of bounds (i.e., we dont want to collect it yet!)
  if (messageCount < startMsgCount) {
    ++messageCount;
    return true;
  }

  // if the message is out of bounds (i.e., we dont want to collect it ever, 
  // thus aborting the information gathering (return false!))
  // no need to iterate over all the other messages.
  if (messageCount > endMsgCount) return false;
  
  // begin parsing the messages
  // else, we can continue to parse the message to the content vectors
  int64_t tmp;

  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  tmp = get8bytes(&buf[11]);
  std::memcpy(&(order_ref[current_idx]), &tmp, sizeof(double));
  
  switch (buf[0]) {
    case 'E':
      shares[current_idx]       = get4bytes(&buf[19]);// executed shares

      tmp = get8bytes(&buf[23]);
      std::memcpy(&(match_number[current_idx]), &tmp, sizeof(double));
      // empty assigns
      printable[current_idx]    = NA_LOGICAL;
      price[current_idx]        = NA_REAL;
      std::memcpy(&(new_order_ref[current_idx]), &NA_INT64, sizeof(double));
      break;

    case 'C':
      shares[current_idx]       = get4bytes(&buf[19]);// executed shares
      tmp = get8bytes(&buf[23]);
      std::memcpy(&(match_number[current_idx]), &tmp, sizeof(double));
      printable[current_idx]    = buf[31] == 'P';
      price[current_idx]        = (double) get4bytes(&buf[32]) / 10000.0;
      // empty assigns
      std::memcpy(&(new_order_ref[current_idx]), &NA_INT64, sizeof(double));
      break;

    case 'X':
      shares[current_idx] = get4bytes(&buf[19]); // cancelled shares
      // empty assigns
      std::memcpy(&(match_number[current_idx]), &NA_INT64, sizeof(double));
      printable[current_idx] = NA_LOGICAL;
      price[current_idx]     = NA_REAL;
      std::memcpy(&(new_order_ref[current_idx]), &NA_INT64, sizeof(double));
      break;

    case 'D':
      // empty assigns
      shares[current_idx]    = NA_INTEGER;
      std::memcpy(&(match_number[current_idx]), &NA_INT64, sizeof(double));
      printable[current_idx] = NA_LOGICAL;
      price[current_idx]     = NA_REAL;
      std::memcpy(&(new_order_ref[current_idx]), &NA_INT64, sizeof(double));
      break;

    case 'U':
      // the order ref is the original order reference, 
      // the new order reference is the new order reference
      tmp = get8bytes(&buf[19]);
      std::memcpy(&(new_order_ref[current_idx]), &tmp, sizeof(double));
      shares[current_idx] = get4bytes(&buf[27]);
      price[current_idx]  = (double) get4bytes(&buf[31]) / 10000.0;
      // empty assigns
      std::memcpy(&(match_number[current_idx]), &NA_INT64, sizeof(double));
      printable[current_idx] = NA_LOGICAL;
      break;

    default:
      Rcpp::Rcout << "Unkown message type: " << buf[0] << "\n";
    break;
  }

  // increase the number of this message type
  ++messageCount;
  ++current_idx;
  return true;
}

/**
 * @brief      Converts the stored information into an Rcpp::DataFrame
 *
 * @return     The Rcpp::DataFrame
 */
Rcpp::DataFrame Modifications::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  Rcpp::NumericVector oref = data["order_ref"];
  oref.attr("class") = "integer64";
  Rcpp::NumericVector mtch = data["match_number"];
  mtch.attr("class") = "integer64";
  Rcpp::NumericVector nor = data["new_order_ref"];
  nor.attr("class") = "integer64";

  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void Modifications::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type        = data["msg_type"]        = Rcpp::CharacterVector(size);
  locate_code     = data["locate_code"]     = Rcpp::IntegerVector(size);
  tracking_number = data["tracking_number"] = Rcpp::IntegerVector(size);
  timestamp       = data["timestamp"]       = Rcpp::NumericVector(size);
  order_ref       = data["order_ref"]       = Rcpp::NumericVector(size);
  shares          = data["shares"]          = Rcpp::IntegerVector(size);
  match_number    = data["match_number"]    = Rcpp::NumericVector(size);
  printable       = data["printable"]       = Rcpp::LogicalVector(size);
  price           = data["price"]           = Rcpp::NumericVector(size);
  new_order_ref   = data["new_order_ref"]   = Rcpp::NumericVector(size);
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
}
