#ifndef MESSAGES_H
#define MESSAGES_H

#include <Rcpp.h>
#include "Specifications.h"
// [[Rcpp::plugins("cpp11")]]

/**
 * #################################################################
 * Classes that are able to parse the buffer into multiple vectors
 *  containing the information and that can convert the vectors
 *  to an Rcpp::DataFrame
 * The classes are:
 *  - MessageType: A "template" class
 *  - Orders: For Messages 'A' and 'F' (addOrders + add Orders MPID)
 *  - Trades: For Trades 'P', 'Q', and 'B' (Trades, Cross-Trades, and Broken Trades)
 *  
 * Also some getXBytes functions, that get X bytes (big endian)
 *  and convert them to int32_t (or int64_t if needed)
 * #################################################################
 */

// __builtin_bswap requires gcc!
int32_t get2bytes(unsigned char* buf);
int32_t get4bytes(unsigned char* buf);
int64_t get6bytes(unsigned char* buf);
int64_t get8bytes(unsigned char* buf);

void copy64bit(Rcpp::NumericVector vec, int64_t idx, int64_t val);
// #################################################################

class MessageType {
public:
  MessageType() = default; // To still allow default construction as before

  // Functions
  int64_t countValidMessages(std::vector<int64_t> count);
  void setBoundaries(int64_t startMsgCount, int64_t endMsgCount);
  
  // Virtual Functions
  virtual bool loadMessage(unsigned char* buf);
  virtual Rcpp::DataFrame getDF();
  virtual void reserve(int64_t size);

  // Members
  int64_t current_idx = 0,
    messageCount  = 0,
    startMsgCount = 0, 
    endMsgCount   = std::numeric_limits<int64_t>::max();
  const std::vector<unsigned char> validTypes;
  const std::vector<int> typePositions;
  
  Rcpp::List data;
  Rcpp::CharacterVector colnames;
  
protected:
  explicit MessageType(std::vector<unsigned char> const& validTypes,
                       std::vector<int> const& typePositions,
                       Rcpp::CharacterVector colnames) : 
    validTypes(validTypes), typePositions(typePositions), colnames(colnames) {}
};

/**
 * @brief      A class that parses the orders (message type 'A' and 'F')
 */
class Orders : public MessageType {
public:
  Orders() : MessageType(
    {'A', 'F'}, 
    {ITCH::POS::A, ITCH::POS::F},
    {"msg_type", "locate_code", "tracking_number", "timestamp", "order_ref", "buy", "shares", "stock", "price", "mpid"}
  ) {}
  // Functions
  bool loadMessage(unsigned char* buf);
  void reserve(int64_t size);
  Rcpp::DataFrame getDF();
  
  // Members
  // The references to the data vectors
  Rcpp::CharacterVector msg_type;
  Rcpp::IntegerVector   locate_code;
  Rcpp::IntegerVector   tracking_number;
  Rcpp::NumericVector   timestamp;
  Rcpp::NumericVector   order_ref;
  Rcpp::LogicalVector   buy;
  Rcpp::IntegerVector   shares;
  Rcpp::CharacterVector stock;
  Rcpp::NumericVector   price;
  Rcpp::CharacterVector mpid;
};

/**
 * @brief      A class that parses the trades (message type 'P', 'Q', and 'B')
 */
class Trades : public MessageType {
public:
  Trades() : MessageType(
    {'P', 'Q', 'B'}, 
    {ITCH::POS::P, ITCH::POS::Q, ITCH::POS::B},
    {"msg_type", "locate_code", "tracking_number", "timestamp", "order_ref", "buy", "shares", "stock", "price", "match_number", "cross_type"}
) {}
  // Functions
  bool loadMessage(unsigned char* buf);
  void reserve(int64_t size);
  Rcpp::DataFrame getDF();
  
  // Members
  // The references to the data vectors
  Rcpp::CharacterVector msg_type;
  Rcpp::IntegerVector   locate_code;
  Rcpp::IntegerVector   tracking_number;
  Rcpp::NumericVector   timestamp;
  Rcpp::NumericVector   order_ref;
  Rcpp::LogicalVector   buy;
  Rcpp::IntegerVector   shares;
  Rcpp::CharacterVector stock;
  Rcpp::NumericVector   price;
  Rcpp::NumericVector   match_number;
  Rcpp::CharacterVector cross_type;
};


/**
 * @brief      A class that parses the modifications (message type 'E', 'C', 'X', 'D', and 'U')
 */
class Modifications : public MessageType {
public:
  Modifications() : MessageType(
    {'E', 'C', 'X', 'D', 'U'}, 
    {ITCH::POS::E, ITCH::POS::C, ITCH::POS::X, ITCH::POS::D, ITCH::POS::U},
    {"msg_type", "locate_code", "tracking_number", "timestamp", "order_ref", "shares", "match_number", "printable", "price", "new_order_ref"}
  ) {}

  // Functions
  bool loadMessage(unsigned char* buf);
  void reserve(int64_t size);
  Rcpp::DataFrame getDF();
  
  // Members
  Rcpp::CharacterVector msg_type;
  Rcpp::IntegerVector   locate_code;
  Rcpp::IntegerVector   tracking_number;
  Rcpp::NumericVector   timestamp;
  Rcpp::NumericVector   order_ref;
  Rcpp::IntegerVector   shares;
  Rcpp::CharacterVector stock;
  Rcpp::NumericVector   match_number;
  Rcpp::LogicalVector   printable;
  Rcpp::NumericVector   price;
  Rcpp::NumericVector   new_order_ref;
};

#endif //MESSAGES_H
