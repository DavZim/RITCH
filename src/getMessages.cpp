#include "getMessages.h"

/**
 * @brief      Loads the messages from a file into the given messagetype (i.e., Trades, Orders, etc)
 *
 * @param      msg            The given messagetype (i.e., Trades, Orders, etc)
 * @param[in]  filename       The filename to a plain-text-file
 * @param[in]  startMsgCount  The start message count, the message (order) count at which we 
 *                              start to save the messages, the defaults to 0 (first message)
 * @param[in]  endMsgCount    The end message count, the message count at which we stop to 
 *                              stop to save the messages, defaults to 0, which will be 
 *                              substituted to all messages
 * @param[in]  bufferSize     The buffer size in bytes, defaults to 100MB
 * @param[in]  quiet          If true, no status message is printed, defaults to false
 *
 * @return     A Rcpp::DataFrame containing the data 
 */
Rcpp::DataFrame getMessagesTemplate(MessageType& msg,
                                    std::string filename, 
                                    int64_t startMsgCount,
                                    int64_t endMsgCount,
                                    Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                    int64_t bufferSize, 
                                    bool quiet) {

  int64_t nMessages;

  // check that the order is correct
  if (startMsgCount > endMsgCount && endMsgCount >= 0) {
    int64_t t = startMsgCount;
    startMsgCount = endMsgCount;
    endMsgCount = t;
  }
  
  // if no max num given, count valid messages!
  if (!quiet) Rcpp::Rcout << "[Counting]   ";
  if (endMsgCount < 0) {
    std::vector<int64_t> count = countMessages(filename, bufferSize);
    endMsgCount = msg.countValidMessages(count);
    nMessages = endMsgCount - startMsgCount;
  } else {
    // endMsgCount was specified, thus zero-index miscount
    // matters only in the verbosity and the msg.reserve()
    nMessages = endMsgCount - startMsgCount + 1;
  }
  
  if (!quiet) Rcpp::Rcout << formatThousands(nMessages) << " messages found\n";
  
  // Reserve the space for the messages
  msg.reserve(nMessages);
  
  if (nMessages == 0) return msg.getDF();

  // convert the Filter to STL types

  // msg_type
  std::vector<char> msgFilter;
  if (filterMsgType.size() > 0) {
    msgFilter.resize(filterMsgType.size());
    for (int i = 0; i < filterMsgType.size(); i++) 
      msgFilter[i] = Rcpp::as<char>(filterMsgType[i]);
  }

  // Locate Code
  std::vector<int> locFilter;
  if (filterLocCode.size() > 0) {
    locFilter.resize(filterLocCode.size());
    for (int i = 0; i < filterLocCode.size(); i++) 
      locFilter[i] = filterLocCode[i];
  }

  // Min and Max Timestamp
  std::vector<int64_t> minTS, maxTS;
  
  const size_t lenMin = minTimestamp.size();
  const size_t lenMax = maxTimestamp.size();
  const size_t len = lenMin > lenMax ? lenMin : lenMax;

  minTS.resize(len);
  maxTS.resize(len);
  const int64_t max_int64_t = std::numeric_limits<int64_t>::max();
  const int64_t min_int64_t = std::numeric_limits<int64_t>::min();

  if (lenMax == lenMin) {
    // max and min TS have the same lengths...
    std::memcpy(&(minTS[0]), &(minTimestamp[0]), len * sizeof(double));
    std::memcpy(&(maxTS[0]), &(maxTimestamp[0]), len * sizeof(double));
  } else if (lenMax > lenMin) {
    // min TS has size 0, max TS is filled
    for (size_t t = 0; t < len; t++) minTS[t] = min_int64_t;
    std::memcpy(&(maxTS[0]), &(maxTimestamp[0]), len * sizeof(double));
  } else {// (lenMin < lenMax)
    // max TS has size 0, min TS is filled
    std::memcpy(&(minTS[0]), &(minTimestamp[0]), len * sizeof(double));
    for (size_t t = 0; t < len; t++) maxTS[t] = max_int64_t;
  }
  
  // load the file into the msg object
  if (!quiet) Rcpp::Rcout << "[Loading]    ";
  loadToMessages(filename, msg, startMsgCount, endMsgCount, msgFilter, locFilter, minTS, maxTS, bufferSize, quiet);

  // converting the messages to a data.frame
  return msg.getDF();
}

/*
 * Implicit Functions for all parts
 */

// @brief      Returns the Orders from a file as a dataframe
// 
// Order Types considered are 'A' (add order) and 'F' (add order with MPID)
//
// @param[in]  filename       The filename to a plain-text-file
// @param[in]  startMsgCount  The start message count, the message (order) count at which we 
//                              start to save the messages, the defaults to 0 (first message)
// @param[in]  endMsgCount    The end message count, the message count at which we stop to 
//                              stop to save the messages, defaults to 0, which will be 
//                              substituted to all messages
// @param[in]  bufferSize     The buffer size in bytes, defaults to 100MB
// @param[in]  quiet          If true, no status message is printed, defaults to false
//
// @return     The orders in a data.frame
//
// [[Rcpp::export]]
Rcpp::DataFrame getOrders_impl(std::string filename,
                               int64_t startMsgCount,
                               int64_t endMsgCount,
                               Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                               int64_t bufferSize,
                               bool quiet) {
  Orders orders;
  Rcpp::DataFrame df = getMessagesTemplate(orders, filename, startMsgCount, endMsgCount,
   filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Trades from a file as a dataframe
// Trade Types considered are 'P', 'Q', and 'B' (Non Cross, Cross, and Broken)
// [[Rcpp::export]]
Rcpp::DataFrame getTrades_impl(std::string filename,
                               int64_t startMsgCount,
                               int64_t endMsgCount,
                               Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                               int64_t bufferSize,
                               bool quiet) {
  
  Trades trades;
  Rcpp::DataFrame df = getMessagesTemplate(trades, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Modifications from a file as a dataframe
// Modification Types considered are 'E' (order executed), 'C' (order executed with price), 'X' (order cancelled), 'D' (order deleted), and 'U' (order replaced)
// [[Rcpp::export]]
Rcpp::DataFrame getModifications_impl(std::string filename,
                                      int64_t startMsgCount,
                                      int64_t endMsgCount,
                                      Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                      int64_t bufferSize,
                                      bool quiet) {
  
  Modifications mods;
  Rcpp::DataFrame df = getMessagesTemplate(mods, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the System Events from a file as a dataframe
// System Event Information Types considered are 'S'
// [[Rcpp::export]]
Rcpp::DataFrame getSystemEvents_impl(std::string filename,
                                     int64_t startMsgCount,
                                     int64_t endMsgCount,
                                     Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                     int64_t bufferSize,
                                     bool quiet) {
  
  SystemEvents sys;
  Rcpp::DataFrame df = getMessagesTemplate(sys, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Stock Directory from a file as a dataframe
// Message Types considered are 'R'
// [[Rcpp::export]]
Rcpp::DataFrame getStockDirectory_impl(std::string filename,
                                       int64_t startMsgCount,
                                       int64_t endMsgCount,
                                       Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                       int64_t bufferSize,
                                       bool quiet) {
  
  StockDirectory sys;
  Rcpp::DataFrame df = getMessagesTemplate(sys, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Trading Status from a file as a dataframe
// Message Types considered are 'H' and 'h'
// [[Rcpp::export]]
Rcpp::DataFrame getTradingStatus_impl(std::string filename,
                                      int64_t startMsgCount,
                                      int64_t endMsgCount,
                                      Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                      int64_t bufferSize,
                                      bool quiet) {
  
  TradingStatus trad;
  Rcpp::DataFrame df = getMessagesTemplate(trad, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Reg SHO from a file as a dataframe
// Message Types considered are 'Y'
// [[Rcpp::export]]
Rcpp::DataFrame getRegSHO_impl(std::string filename,
                                      int64_t startMsgCount,
                                      int64_t endMsgCount,
                                      Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                      int64_t bufferSize,
                                      bool quiet) {
  
  RegSHO reg;
  Rcpp::DataFrame df = getMessagesTemplate(reg, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Market Participants Status from a file as a dataframe
// Message Types considered are 'L'
// [[Rcpp::export]]
Rcpp::DataFrame getParticipantStates_impl(std::string filename,
                                          int64_t startMsgCount,
                                          int64_t endMsgCount,
                                          Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                                          int64_t bufferSize,
                                          bool quiet) {
  
  ParticipantStates ps;
  Rcpp::DataFrame df = getMessagesTemplate(ps, filename, startMsgCount, endMsgCount,
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the MWCB from a file as a dataframe
// Message Types considered are 'V' and 'W'
// [[Rcpp::export]]
Rcpp::DataFrame getMWCB_impl(std::string filename,
                             int64_t startMsgCount,
                             int64_t endMsgCount,
                             Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                             int64_t bufferSize,
                             bool quiet) {
  
  MWCB cb;
  Rcpp::DataFrame df = getMessagesTemplate(cb, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the IPO from a file as a dataframe
// Message Types considered are 'K'
// [[Rcpp::export]]
Rcpp::DataFrame getIPO_impl(std::string filename,
                             int64_t startMsgCount,
                             int64_t endMsgCount,
                             Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                             int64_t bufferSize,
                             bool quiet) {
  
  IPO ipo;
  Rcpp::DataFrame df = getMessagesTemplate(ipo, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the LULD from a file as a dataframe
// Message Types considered are 'J'
// [[Rcpp::export]]
Rcpp::DataFrame getLULD_impl(std::string filename,
                             int64_t startMsgCount,
                             int64_t endMsgCount,
                             Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                             int64_t bufferSize,
                             bool quiet) {
  
  LULD luld;
  Rcpp::DataFrame df = getMessagesTemplate(luld, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the NOII from a file as a dataframe
// Message Types considered are 'I'
// [[Rcpp::export]]
Rcpp::DataFrame getNOII_impl(std::string filename,
                             int64_t startMsgCount,
                             int64_t endMsgCount,
                             Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                             int64_t bufferSize,
                             bool quiet) {
  
  NOII noii;
  Rcpp::DataFrame df = getMessagesTemplate(noii, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}

// @brief      Returns the RPII from a file as a dataframe
// Message Types considered are 'N'
// [[Rcpp::export]]
Rcpp::DataFrame getRPII_impl(std::string filename,
                             int64_t startMsgCount,
                             int64_t endMsgCount,
                             Rcpp::CharacterVector filterMsgType,
                                    Rcpp::IntegerVector filterLocCode,
                                    Rcpp::NumericVector minTimestamp,
                                    Rcpp::NumericVector maxTimestamp,
                             int64_t bufferSize,
                             bool quiet) {
  
  RPII rpii;
  Rcpp::DataFrame df = getMessagesTemplate(rpii, filename, startMsgCount, endMsgCount, 
    filterMsgType, filterLocCode, minTimestamp, maxTimestamp, bufferSize, quiet);
  return df;  
}
