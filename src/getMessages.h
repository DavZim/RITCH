#ifndef GETMESSAGES_H
#define GETMESSAGES_H

#include "RITCH.h"
#include "countMessages.h"

/**
 * ########################################
 * getX loads the contents of the 
 * file int a Rcpp::DataFrame
 * 
 * These functions are supposed to be called from the get_* functions of the RITCH Package
 * ########################################
 */ 

Rcpp::DataFrame getMessagesTemplate(MessageType& msg,
                                    std::string filename, 
                                    int64_t startMsgCount = 0,
                                    int64_t endMsgCount = 0,
                                    int64_t bufferSize = 1e8,
                                    bool quiet = false);

Rcpp::DataFrame getOrders(std::string filename, 
                          int64_t startMsgCount = 0,
                          int64_t endMsgCount = 0,
                          int64_t bufferSize = 1e8,
                          bool quiet = false);

Rcpp::DataFrame getTrades(std::string filename, 
                          int64_t startMsgCount = 0,
                          int64_t endMsgCount = 0,
                          int64_t bufferSize = 1e8,
                          bool quiet = false);

Rcpp::DataFrame getModifications(std::string filename, 
                                 int64_t startMsgCount = 0,
                                 int64_t endMsgCount = 0,
                                 int64_t bufferSize = 1e8,
                                 bool quiet = false);

#endif //GETMESSAGES_H
