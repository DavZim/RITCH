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
    // matters only in the verbosity and the msg.reserve
    nMessages = endMsgCount - startMsgCount + 1;
  }
  
  if (!quiet) Rcpp::Rcout << formatThousands(nMessages) << " messages found\n";

  // Reserve the space for the messages
  msg.reserve(nMessages);

  // load the file into the msg object
  if (!quiet) Rcpp::Rcout << "[Loading]    ";
  loadToMessages(filename, msg, startMsgCount, endMsgCount, bufferSize, quiet);

  // converting the messages to a data.frame
  return msg.getDF();
}


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
                               int64_t bufferSize,
                               bool quiet) {
  Orders orders;
  Rcpp::DataFrame df = getMessagesTemplate(orders, filename, startMsgCount, endMsgCount, bufferSize, quiet);
  return df;  
}


// @brief      Returns the Trades from a file as a dataframe
// 
// Trade Types considered are 'P', 'Q', and 'B' (Non Cross, Cross, and Broken)
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
// @return     The trades in a data.frame
//
// [[Rcpp::export]]
Rcpp::DataFrame getTrades_impl(std::string filename,
                               int64_t startMsgCount,
                               int64_t endMsgCount,
                               int64_t bufferSize,
                               bool quiet) {
  
  Trades trades;
  Rcpp::DataFrame df = getMessagesTemplate(trades, filename, startMsgCount, endMsgCount, bufferSize, quiet);
  return df;  
}

// @brief      Returns the Modifications from a file as a dataframe
// 
// Modification Types considered are 'E' (order executed), 'C' (order executed with price), 'X' (order cancelled), 'D' (order deleted), and 'U' (order replaced)
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
// @return     The modifications in a data.frame
// [[Rcpp::export]]
Rcpp::DataFrame getModifications_impl(std::string filename,
                                      int64_t startMsgCount,
                                      int64_t endMsgCount,
                                      int64_t bufferSize,
                                      bool quiet) {
  
  Modifications mods;
  Rcpp::DataFrame df = getMessagesTemplate(mods, filename, startMsgCount, endMsgCount, bufferSize, quiet);
  return df;  
}
