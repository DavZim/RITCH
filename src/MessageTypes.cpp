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
  price[current_idx] = ((double) get4bytes(&buf[32])) / 10000.0;
  
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
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
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
}


// ################################################################################
// ################################ Trades ########################################
// ################################################################################

/**
 * @brief      Loads the information from trades into the class, either of type 'P', 'Q', or 'B'
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
      price[current_idx]  = ((double) get4bytes(&buf[32])) / 10000.0;
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
      price[current_idx]      = ((double) get4bytes(&buf[27])) / 10000.0;
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
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
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
}


// ################################################################################
// ################################ Modifications #################################
// ################################################################################

/**
 * @brief      Loads the information from trades modifications into the class, either of type 'E', 'C', 'X', 'D', 'U', 'D'
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
    price[current_idx]        = ((double) get4bytes(&buf[32])) / 10000.0;
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
    price[current_idx]  = ((double) get4bytes(&buf[31])) / 10000.0;
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
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
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
}



// ################################################################################
// ################################ System Event Messages #########################
// ################################################################################

/**
 * @brief      Loads the information from system event messages into the class, type 'S'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool SystemEvents::loadMessage(unsigned char* buf) {
  
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
  event_code[current_idx]      = std::string(1, buf[11]);
  
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
Rcpp::DataFrame SystemEvents::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void SystemEvents::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type        = data["msg_type"]        = Rcpp::CharacterVector(size);
  locate_code     = data["locate_code"]     = Rcpp::IntegerVector(size);
  tracking_number = data["tracking_number"] = Rcpp::IntegerVector(size);
  timestamp       = data["timestamp"]       = Rcpp::NumericVector(size);
  event_code      = data["event_code"]      = Rcpp::CharacterVector(size);
}


// ################################################################################
// ################################ Stock Directory ###############################
// ################################################################################

/**
 * @brief      Loads the stock directory messages into the class, type 'R'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool StockDirectory::loadMessage(unsigned char* buf) {
  
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
  const unsigned char white = ' ';
  std::string stock_string, issue_string;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[11 + i] != white) stock_string += buf[11 + i];
  }
  stock[current_idx]            = stock_string;
  market_category[current_idx]  = std::string(1, buf[19]);
  financial_status[current_idx] = std::string(1, buf[20]);
  lot_size[current_idx]         = get4bytes(&buf[21]);
  round_lots_only[current_idx]  = buf[25] == 'Y';
  issue_classification[current_idx] = std::string(1, buf[26]);
  issue_string                  = std::string(1, buf[27]) + std::string(1, buf[28]);
  issue_subtype[current_idx]    = issue_string;
  authentic[current_idx]        = buf[29] == 'P'; // P is live/production, T is Test
  short_sell_closeout[current_idx] = buf[30] == 'Y' ? true : buf[30] == 'N' ? false : NA_LOGICAL;
  ipo_flag[current_idx]         = buf[31] == 'Y' ? true : buf[31] == 'N' ? false : NA_LOGICAL;
  luld_price_tier[current_idx]  = std::string(1, buf[32]);
  etp_flag[current_idx]         = buf[33] == 'Y' ? true : buf[33] == 'N' ? false : NA_LOGICAL;
  etp_leverage[current_idx]     = get4bytes(&buf[34]);;
  inverse[current_idx]          = buf[38] == 'Y';
  
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
Rcpp::DataFrame StockDirectory::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void StockDirectory::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type              = data["msg_type"]              = Rcpp::CharacterVector(size);
  locate_code           = data["locate_code"]           = Rcpp::IntegerVector(size);
  tracking_number       = data["tracking_number"]       = Rcpp::IntegerVector(size);
  timestamp             = data["timestamp"]             = Rcpp::NumericVector(size);
  stock                 = data["stock"]                 = Rcpp::CharacterVector(size);
  market_category       = data["market_category"]       = Rcpp::CharacterVector(size);
  financial_status      = data["financial_status"]      = Rcpp::CharacterVector(size);
  lot_size              = data["lot_size"]              = Rcpp::IntegerVector(size);
  round_lots_only       = data["round_lots_only"]       = Rcpp::LogicalVector(size);
  issue_classification  = data["issue_classification"]  = Rcpp::CharacterVector(size);
  issue_subtype         = data["issue_subtype"]         = Rcpp::CharacterVector(size);
  authentic             = data["authentic"]             = Rcpp::LogicalVector(size);
  short_sell_closeout   = data["short_sell_closeout"]   = Rcpp::LogicalVector(size);
  ipo_flag              = data["ipo_flag"]              = Rcpp::LogicalVector(size);
  luld_price_tier       = data["luld_price_tier"]       = Rcpp::CharacterVector(size);
  etp_flag              = data["etp_flag"]              = Rcpp::LogicalVector(size);
  etp_leverage          = data["etp_leverage"]          = Rcpp::IntegerVector(size);
  inverse               = data["inverse"]               = Rcpp::LogicalVector(size);
}


// ################################################################################
// ################################ Stock Trading Status ##########################
// ################################################################################

/**
 * @brief      Loads the information from stock trading status messages into the class, type 'S'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool TradingStatus::loadMessage(unsigned char* buf) {
  
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
  const unsigned char white = ' ';
  std::string stock_string, reas;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[11 + i] != white) stock_string += buf[11 + i];
  }
  stock[current_idx] = stock_string;
  
  switch (buf[0]) {
  case 'H':
    trading_state[current_idx]    = std::string(1, buf[19]);
    reserved[current_idx]         = std::string(1, buf[20]);
    for (unsigned int i = 0; i < 4U; ++i) {
      if (buf[21 + i] != white) reas += buf[21 + i];
    }
    reason[current_idx]           = reas;
    // fill NAs from h
    market_code[current_idx]      = NA_STRING;
    operation_halted[current_idx] = NA_LOGICAL;
    break;
    
  case 'h': 
    market_code[current_idx]      = std::string(1, buf[19]);
    operation_halted[current_idx] = buf[20] == 'H';
    // fill NAs from H
    trading_state[current_idx]    = NA_STRING;
    reserved[current_idx]         = NA_STRING;
    reason[current_idx]           = NA_STRING;
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
Rcpp::DataFrame TradingStatus::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void TradingStatus::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type         = data["msg_type"]         = Rcpp::CharacterVector(size);
  locate_code      = data["locate_code"]      = Rcpp::IntegerVector(size);
  tracking_number  = data["tracking_number"]  = Rcpp::IntegerVector(size);
  timestamp        = data["timestamp"]        = Rcpp::NumericVector(size);
  stock            = data["stock"]            = Rcpp::CharacterVector(size);
  trading_state    = data["trading_state"]    = Rcpp::CharacterVector(size);
  reserved         = data["reserved"]         = Rcpp::CharacterVector(size);
  reason           = data["reason"]           = Rcpp::CharacterVector(size);
  market_code      = data["market_code"]      = Rcpp::CharacterVector(size);
  operation_halted = data["operation_halted"] = Rcpp::LogicalVector(size);
}


// ################################################################################
// ################################ Reg SHO Messages ##############################
// ################################################################################

/**
 * @brief      Loads the information from Reg SHO messages into the class, type 'Y'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool RegSHO::loadMessage(unsigned char* buf) {
  
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
  const unsigned char white = ' ';
  std::string stock_string;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[11 + i] != white) stock_string += buf[11 + i];
  }
  stock[current_idx] = stock_string;
  regsho_action[current_idx] = std::string(1, buf[19]);
  
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
Rcpp::DataFrame RegSHO::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void RegSHO::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type         = data["msg_type"]         = Rcpp::CharacterVector(size);
  locate_code      = data["locate_code"]      = Rcpp::IntegerVector(size);
  tracking_number  = data["tracking_number"]  = Rcpp::IntegerVector(size);
  timestamp        = data["timestamp"]        = Rcpp::NumericVector(size);
  stock            = data["stock"]            = Rcpp::CharacterVector(size);
  regsho_action    = data["regsho_action"]    = Rcpp::CharacterVector(size);
}

// ################################################################################
// ################################ Market Participant Status #####################
// ################################################################################

/**
 * @brief      Loads the information from market participant status messages into the class, type 'L'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool ParticipantStates::loadMessage(unsigned char* buf) {
  
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
  const unsigned char white = ' ';
  std::string stock_string, mp;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  for (unsigned int i = 0; i < 4U; ++i) {
    if (buf[11 + i] != white) mp += buf[11 + i];
  }
  mpid[current_idx] = mp;
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[15 + i] != white) stock_string += buf[15 + i];
  }
  stock[current_idx]      = stock_string;
  primary_mm[current_idx] = buf[23] == 'Y';
  mm_mode[current_idx]    = std::string(1, buf[24]);
  participant_state[current_idx] = std::string(1, buf[25]);
  
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
Rcpp::DataFrame ParticipantStates::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void ParticipantStates::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type          = data["msg_type"]          = Rcpp::CharacterVector(size);
  locate_code       = data["locate_code"]       = Rcpp::IntegerVector(size);
  tracking_number   = data["tracking_number"]   = Rcpp::IntegerVector(size);
  timestamp         = data["timestamp"]         = Rcpp::NumericVector(size);
  mpid              = data["mpid"]              = Rcpp::CharacterVector(size);
  stock             = data["stock"]             = Rcpp::CharacterVector(size);
  primary_mm        = data["primary_mm"]        = Rcpp::LogicalVector(size);
  mm_mode           = data["mm_mode"]           = Rcpp::CharacterVector(size);
  participant_state = data["participant_state"] = Rcpp::CharacterVector(size);
}

// ################################################################################
// ################################ MWCB ##########################################
// ################################################################################

/**
 * @brief      Loads the information from MWCB messages into the class, type 'L'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool MWCB::loadMessage(unsigned char* buf) {
  
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
  
  switch (buf[0]) {
  case 'V':
    tmp = get8bytes(&buf[11]);
    std::memcpy(&(level1[current_idx]), &tmp, sizeof(double));
    tmp = get8bytes(&buf[19]);
    std::memcpy(&(level2[current_idx]), &tmp, sizeof(double));
    tmp = get8bytes(&buf[27]);
    std::memcpy(&(level3[current_idx]), &tmp, sizeof(double));
    // fill NAs from W
    breached_level[current_idx] = NA_INTEGER;
    break;
    
    case 'W':
      breached_level[current_idx] = buf[11] - '0';
      // fill NAs from V
      std::memcpy(&(level1[current_idx]), &NA_INT64, sizeof(double));
      std::memcpy(&(level2[current_idx]), &NA_INT64, sizeof(double));
      std::memcpy(&(level3[current_idx]), &NA_INT64, sizeof(double));
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
Rcpp::DataFrame MWCB::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  Rcpp::NumericVector lvl1 = data["level1"];
  Rcpp::NumericVector lvl2 = data["level2"];
  Rcpp::NumericVector lvl3 = data["level3"];
  ts.attr("class") = "integer64";
  lvl1.attr("class") = "integer64";
  lvl2.attr("class") = "integer64";
  lvl3.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void MWCB::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type          = data["msg_type"]          = Rcpp::CharacterVector(size);
  locate_code       = data["locate_code"]       = Rcpp::IntegerVector(size);
  tracking_number   = data["tracking_number"]   = Rcpp::IntegerVector(size);
  timestamp         = data["timestamp"]         = Rcpp::NumericVector(size);
  level1            = data["level1"]            = Rcpp::NumericVector(size);
  level2            = data["level2"]            = Rcpp::NumericVector(size);
  level3            = data["level3"]            = Rcpp::NumericVector(size);
  breached_level    = data["breached_level"]    = Rcpp::IntegerVector(size);
}


// ################################################################################
// ################################ IPO ###########################################
// ################################################################################

/**
 * @brief      Loads the information from IPO messages into the class, type 'K'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool IPO::loadMessage(unsigned char* buf) {
  
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
  const char white = ' ';
  std::string stock_string;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[11 + i] != white) stock_string += buf[11 + i];
  }
  stock[current_idx]        = stock_string;
  release_time[current_idx] = get4bytes(&buf[19]);
  release_qualifier[current_idx] = std::string(1, buf[23]);
  ipo_price = ((double) get4bytes(&buf[24])) / 10000.0;
  
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
Rcpp::DataFrame IPO::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void IPO::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type          = data["msg_type"]          = Rcpp::CharacterVector(size);
  locate_code       = data["locate_code"]       = Rcpp::IntegerVector(size);
  tracking_number   = data["tracking_number"]   = Rcpp::IntegerVector(size);
  timestamp         = data["timestamp"]         = Rcpp::NumericVector(size);
  stock             = data["stock"]             = Rcpp::CharacterVector(size);
  release_time      = data["release_time"]      = Rcpp::IntegerVector(size);
  release_qualifier = data["release_qualifier"] = Rcpp::CharacterVector(size);
  ipo_price         = data["ipo_price"]         = Rcpp::NumericVector(size);
}

// ################################################################################
// ################################ LULD ##########################################
// ################################################################################

/**
 * @brief      Loads the information from LULD messages into the class, type 'J'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool LULD::loadMessage(unsigned char* buf) {
  
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
  const char white = ' ';
  std::string stock_string;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[11 + i] != white) stock_string += buf[11 + i];
  }
  stock[current_idx]           = stock_string;
  reference_price[current_idx] = ((double) get4bytes(&buf[19])) / 10000.0;
  upper_price[current_idx]     = ((double) get4bytes(&buf[23])) / 10000.0;
  lower_price[current_idx]     = ((double) get4bytes(&buf[27])) / 10000.0;
  extension[current_idx]       = get4bytes(&buf[31]);
  
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
Rcpp::DataFrame LULD::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void LULD::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type          = data["msg_type"]          = Rcpp::CharacterVector(size);
  locate_code       = data["locate_code"]       = Rcpp::IntegerVector(size);
  tracking_number   = data["tracking_number"]   = Rcpp::IntegerVector(size);
  timestamp         = data["timestamp"]         = Rcpp::NumericVector(size);
  stock             = data["stock"]             = Rcpp::CharacterVector(size);
  reference_price   = data["reference_price"]   = Rcpp::NumericVector(size);
  upper_price       = data["upper_price"]       = Rcpp::NumericVector(size);
  lower_price       = data["lower_price"]       = Rcpp::NumericVector(size);
  extension         = data["extension"]         = Rcpp::IntegerVector(size);
}


// ################################################################################
// ################################ NOII ##########################################
// ################################################################################

/**
 * @brief      Loads the information from NOII messages into the class, type 'I'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool NOII::loadMessage(unsigned char* buf) {
  
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
  const char white = ' ';
  std::string stock_string;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  tmp = get8bytes(&buf[11]);
  std::memcpy(&(paired_shares[current_idx]), &tmp, sizeof(double));
  tmp = get8bytes(&buf[19]);
  std::memcpy(&(imbalance_shares[current_idx]), &tmp, sizeof(double));
  
  imbalance_direction[current_idx] = std::string(1, buf[27]);
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[28 + i] != white) stock_string += buf[28 + i];
  }
  stock[current_idx]           = stock_string;
  far_price[current_idx]       = ((double) get4bytes(&buf[36])) / 10000.0;
  near_price[current_idx]      = ((double) get4bytes(&buf[40])) / 10000.0;
  reference_price[current_idx] = ((double) get4bytes(&buf[44])) / 10000.0;
  cross_type[current_idx] = std::string(1, buf[48]);
  variation_indicator[current_idx] = std::string(1, buf[49]);
  
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
Rcpp::DataFrame NOII::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  Rcpp::NumericVector ps = data["paired_shares"];
  Rcpp::NumericVector is = data["imbalance_shares"];
  ts.attr("class") = "integer64";
  ps.attr("class") = "integer64";
  is.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void NOII::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type            = data["msg_type"]            = Rcpp::CharacterVector(size);
  locate_code         = data["locate_code"]         = Rcpp::IntegerVector(size);
  tracking_number     = data["tracking_number"]     = Rcpp::IntegerVector(size);
  timestamp           = data["timestamp"]           = Rcpp::NumericVector(size);
  paired_shares       = data["paired_shares"]       = Rcpp::NumericVector(size);
  imbalance_shares    = data["imbalance_shares"]    = Rcpp::NumericVector(size);
  imbalance_direction = data["imbalance_direction"] = Rcpp::CharacterVector(size);
  stock               = data["stock"]               = Rcpp::CharacterVector(size);
  far_price           = data["far_price"]           = Rcpp::NumericVector(size);
  near_price          = data["near_price"]          = Rcpp::NumericVector(size);
  reference_price     = data["reference_price"]     = Rcpp::NumericVector(size);
  cross_type          = data["cross_type"]          = Rcpp::CharacterVector(size);
  variation_indicator = data["variation_indicator"] = Rcpp::CharacterVector(size);
}

// ################################################################################
// ################################ RPII ##########################################
// ################################################################################

/**
 * @brief      Loads the information from RPII messages into the class, type 'N'
 *
 * @param      buf   The buffer
 *
 * @return     false if the boundaries are broken (all necessary messages are already loaded), 
 *              thus the loading process can be aborted, otherwise true
 */
bool RPII::loadMessage(unsigned char* buf) {
  
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
  const char white = ' ';
  std::string stock_string;
  
  msg_type[current_idx]        = std::string(1, buf[0]);
  locate_code[current_idx]     = get2bytes(&buf[1]);
  tracking_number[current_idx] = get2bytes(&buf[3]);
  tmp = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[current_idx]), &tmp, sizeof(double));
  
  for (unsigned int i = 0; i < 8U; ++i) {
    if (buf[11 + i] != white) stock_string += buf[11 + i];
  }
  stock[current_idx]           = stock_string;
  interest_flag[current_idx]   = std::string(1, buf[19]);
  
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
Rcpp::DataFrame RPII::getDF() {
  Rcpp::NumericVector ts = data["timestamp"];
  ts.attr("class") = "integer64";
  data.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  return data;
}

/**
 * @brief      Reserves the sizes of the content vectors (allows for faster code-execution)
 *
 * @param[in]  size  The size which should be reserved
 */
void RPII::reserve(int64_t size) {
  data = Rcpp::List(colnames.size());
  data.names() = colnames;
  msg_type        = data["msg_type"]        = Rcpp::CharacterVector(size);
  locate_code     = data["locate_code"]     = Rcpp::IntegerVector(size);
  tracking_number = data["tracking_number"] = Rcpp::IntegerVector(size);
  timestamp       = data["timestamp"]       = Rcpp::NumericVector(size);
  stock           = data["stock"]           = Rcpp::CharacterVector(size);
  interest_flag   = data["interest_flag"]   = Rcpp::CharacterVector(size);
}
