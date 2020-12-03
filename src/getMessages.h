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

Rcpp::DataFrame getOrders_impl(std::string filename, 
                               int64_t startMsgCount = 0,
                               int64_t endMsgCount = 0,
                               int64_t bufferSize = 1e8,
                               bool quiet = false);

Rcpp::DataFrame getTrades_impl(std::string filename, 
                               int64_t startMsgCount = 0,
                               int64_t endMsgCount = 0,
                               int64_t bufferSize = 1e8,
                               bool quiet = false);

Rcpp::DataFrame getModifications_impl(std::string filename, 
                                      int64_t startMsgCount = 0,
                                      int64_t endMsgCount = 0,
                                      int64_t bufferSize = 1e8,
                                      bool quiet = false);

Rcpp::DataFrame getSystemEvents_impl(std::string filename, 
                                     int64_t startMsgCount = 0,
                                     int64_t endMsgCount = 0,
                                     int64_t bufferSize = 1e8,
                                     bool quiet = false);

Rcpp::DataFrame getStockDirectory_impl(std::string filename, 
                                       int64_t startMsgCount = 0,
                                       int64_t endMsgCount = 0,
                                       int64_t bufferSize = 1e8,
                                       bool quiet = false);

Rcpp::DataFrame getTradingStatus_impl(std::string filename, 
                                      int64_t startMsgCount = 0,
                                      int64_t endMsgCount = 0,
                                      int64_t bufferSize = 1e8,
                                      bool quiet = false);

Rcpp::DataFrame getRegSHO_impl(std::string filename, 
                               int64_t startMsgCount = 0,
                               int64_t endMsgCount = 0,
                               int64_t bufferSize = 1e8,
                               bool quiet = false);

Rcpp::DataFrame getParticipantStates_impl(std::string filename, 
                                          int64_t startMsgCount = 0,
                                          int64_t endMsgCount = 0,
                                          int64_t bufferSize = 1e8,
                                          bool quiet = false);

Rcpp::DataFrame getMWCB_impl(std::string filename, 
                             int64_t startMsgCount = 0,
                             int64_t endMsgCount = 0,
                             int64_t bufferSize = 1e8,
                             bool quiet = false);

#endif //GETMESSAGES_H
