#include "MessageTypes.h"

/**
 * @brief      Converts 2 bytes from a buffer in big endian to an unsigned integer
 * 
 * @param      buf   The buffer as a pointer to an array of unsigned chars
 *
 * @return     The converted unsigned integer
 */
unsigned int get2bytes(unsigned char* buf) {
  return __builtin_bswap16(*reinterpret_cast<uint16_t*>(&buf[0]));
}

/**
 * @brief      Converts 4 bytes from a buffer in big endian to an unsigned integer
 * 
 * @param      buf   The buffer as a pointer to an array of unsigned chars
 *
 * @return     The converted unsigned integer
 */
unsigned int get4bytes(unsigned char* buf) {
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
 * @brief copies a 64bit value (or multiple) into a numeric vector
 * @param vec the target vector
 * @param idx the starting position
 * @param val the value to copy
 */
void copy64bit(Rcpp::NumericVector vec, int64_t idx, int64_t val) {
  const int64_t tmp = val;
  std::memcpy(&(vec[idx]), &(tmp), sizeof(double));
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
bool MessageType::loadMessages(unsigned char* buf) { return bool(); }
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
bool Orders::loadMessages(unsigned char* buf) {

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
  copy64bit(timestamp, current_idx, get6bytes(&buf[5]));
  copy64bit(order_ref, current_idx, get8bytes(&buf[11]));
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
  }
  mpid[current_idx] = mpid_string;
  
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
bool Trades::loadMessages(unsigned char* buf) {

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
  copy64bit(timestamp, current_idx, get6bytes(&buf[5]));
  
  std::string stock_string;
  const unsigned char white = ' ';
  // only used in Message Q
  int64_t cross_shares;

  switch (buf[0]) {
    case 'P':
      copy64bit(order_ref, current_idx, get8bytes(&buf[11]));
      buy[current_idx]    = buf[19] == 'B';
      shares[current_idx] = get4bytes(&buf[20]);

      // 8 characters make up the stockname
      for (unsigned int i = 0; i < 8U; ++i) {
        if (buf[24 + i] != white) stock_string += buf[24 + i];
      }
      stock[current_idx]  = stock_string;
      price[current_idx]  = (double) get4bytes(&buf[32]) / 10000.0;
      
      copy64bit(match_number, current_idx, get8bytes(&buf[36]));
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
      copy64bit(match_number, current_idx, get8bytes(&buf[31]));
      cross_type[current_idx] = buf[39];
      //empty assigns
      copy64bit(order_ref, current_idx, NA_INT64);
      buy[current_idx] = NA_LOGICAL;
      break;

    case 'B': 
      copy64bit(match_number, current_idx, get8bytes(&buf[11]));
      // empty assigns
      copy64bit(order_ref, current_idx, NA_INT64);
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
bool Modifications::loadMessages(unsigned char* buf) {

  // first check if this is the wrong message
  bool wrongMessage = false;
  for (unsigned char type : validTypes) {
    wrongMessage = wrongMessage || buf[0] == type;
  }
  
  // if the message is of the wrong type, terminate here, but continue with the next message
  if (!wrongMessage) return true;

  // if the message is out of bounds (i.e., we dont want to collect it yet!)
  if (messageCount < startMsgCount) {
    ++messageCount;
    return true;
  }

  // if the message is out of bounds (i.e., we dont want to collect it ever, 
  // thus aborting the information gathering (return false!))
  // no need to iterate over all the other messages.
  if (messageCount > endMsgCount) return false;
  
  // else, we can continue to parse the message to the content vectors
  type.push_back(           buf[0] );
  locateCode.push_back(     get2bytes(&buf[1]) );
  trackingNumber.push_back( get2bytes(&buf[3]) );
  timestamp.push_back(      get6bytes(&buf[5]) );
  orderRef.push_back(       get8bytes(&buf[11]) );
  
  switch (buf[0]) {
    case 'E':
      shares.push_back(      get4bytes(&buf[19]) ); // executed shares
      matchNumber.push_back( get8bytes(&buf[23]) );
      // empty assigns
      printable.push_back(   'N' );
      price.push_back(       0.0 );
      newOrderRef.push_back( 0ULL );
      break;

    case 'C':
      shares.push_back(      get4bytes(&buf[19]) ); // executed shares
      matchNumber.push_back( get8bytes(&buf[23]) );
      printable.push_back(   buf[31] );
      price.push_back(       (double) get4bytes(&buf[32]) / 10000.0 );
      // empty assigns
      newOrderRef.push_back( 0ULL );
      break;

    case 'X':
      shares.push_back(      get4bytes(&buf[19]) ); // cancelled shares
      // empty assigns
      matchNumber.push_back( 0ULL);
      printable.push_back(   false );
      price.push_back(       0.0 );
      newOrderRef.push_back( 0ULL );
      break;

    case 'D':
      // empty assigns
      shares.push_back(      0ULL); 
      matchNumber.push_back( 0ULL);
      printable.push_back(   false );
      price.push_back(       0.0 );
      newOrderRef.push_back( 0ULL );
      break;

    case 'U':
      // the order ref is the original order reference, 
      // the new order reference is the new order reference
      newOrderRef.push_back( get8bytes(&buf[19]) );
      shares.push_back(      get4bytes(&buf[27]) );
      price.push_back(       (double) get4bytes(&buf[31]) / 10000.0 );
      // empty assigns
      matchNumber.push_back( 0ULL);
      printable.push_back(   false );
      break;

    default:
      Rcpp::Rcout << "Unkown message type: " << buf[0] << "\n";
    break;
  }

  // increase the number of this message type
  ++messageCount;
  return true;
}

/**
 * @brief      Converts the stored information into an Rcpp::DataFrame
 *
 * @return     The Rcpp::DataFrame
 */
Rcpp::DataFrame Modifications::getDF() {

  Rcpp::DataFrame df = Rcpp::DataFrame::create(
    Rcpp::Named("msg_type")        = type,
    Rcpp::Named("locate_code")     = locateCode,
    Rcpp::Named("tracking_number") = trackingNumber,
    Rcpp::Named("timestamp")       = timestamp,
    Rcpp::Named("order_ref")       = orderRef,
    Rcpp::Named("shares")          = shares,
    Rcpp::Named("match_number")    = matchNumber,
    Rcpp::Named("printable")       = printable,
    Rcpp::Named("price")           = price,
    Rcpp::Named("new_order_ref")   = newOrderRef
  );
  
  return df;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void Modifications::reserve(int64_t size) {
  type.reserve(size);
  locateCode.reserve(size);
  trackingNumber.reserve(size);
  timestamp.reserve(size);
  orderRef.reserve(size);
  shares.reserve(size);
  matchNumber.reserve(size);
  printable.reserve(size);
  price.reserve(size);
  newOrderRef.reserve(size);
}
