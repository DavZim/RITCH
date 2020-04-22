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
 *  and convert them to unsigned int (or long long int if needed)
 * #################################################################
 */

// __builtin_bswap requires gcc!
unsigned int get2bytes(unsigned char* buf);
unsigned int get4bytes(unsigned char* buf);
int64_t get6bytes(unsigned char* buf);
int64_t get8bytes(unsigned char* buf);

// #################################################################

class MessageType {
public:
  MessageType() = default; // To still allow default construction as before

  // Functions
  int64_t countValidMessages(std::vector<int64_t> count);
  void setBoundaries(int64_t startMsgCount, int64_t endMsgCount);
  
  // Virtual Functions
  virtual bool loadMessages(unsigned char* buf);
  virtual Rcpp::DataFrame getDF();
  virtual void reserve(int64_t size);

  // Members
  int64_t messageCount  = 0,
                     startMsgCount = 0, 
                     endMsgCount   = std::numeric_limits<int64_t>::max();
  const std::vector<unsigned char> validTypes;
  const std::vector<int> typePositions;

protected:
  explicit MessageType(std::vector<unsigned char> const& validTypes,
                       std::vector<int> const& typePositions) : 
    validTypes(validTypes), typePositions(typePositions) {}
};

/**
 * @brief      A class that parses the orders (message type 'A' and 'F')
 */
class Orders : public MessageType {
public:
  Orders() : MessageType({'A', 'F'}, {ITCH::POS::A, ITCH::POS::F}) {}
  // Functions
  bool loadMessages(unsigned char* buf);
  void reserve(int64_t size);
  Rcpp::DataFrame getDF();
  
  // Members
  std::vector<char> type;
  std::vector<int64_t> locateCode;
  std::vector<int64_t> trackingNumber;
  std::vector<int64_t> timestamp;
  std::vector<int64_t> orderRef;
  std::vector<bool>               buy;
  std::vector<int64_t> shares;
  std::vector<std::string>        stock;
  std::vector<double>             price;
  std::vector<std::string>        mpid;
};

/**
 * @brief      A class that parses the trades (message type 'P', 'Q', and 'B')
 */
class Trades : public MessageType {
public:
  Trades() : MessageType({'P', 'Q', 'B'}, {ITCH::POS::P, ITCH::POS::Q, ITCH::POS::B}) {}
  // Functions
  bool loadMessages(unsigned char* buf);
  void reserve(int64_t size);
  Rcpp::DataFrame getDF();
  
  // Members
  std::vector<char> type;
  std::vector<int64_t> locateCode;
  std::vector<int64_t> trackingNumber;
  std::vector<int64_t> timestamp;
  std::vector<int64_t> orderRef;
  std::vector<bool>               buy;
  std::vector<int64_t> shares;
  std::vector<std::string>        stock;
  std::vector<double>             price;
  std::vector<int64_t> matchNumber;
  std::vector<char>               crossType;
};


/**
 * @brief      A class that parses the modifications (message type 'E', 'C', 'X', 'D', and 'U')
 */
class Modifications : public MessageType {
public:
  Modifications() : MessageType({'E', 'C', 'X', 'D', 'U'}, 
    {ITCH::POS::E, ITCH::POS::C, ITCH::POS::X, ITCH::POS::D, ITCH::POS::U}) {}
  // Functions
  bool loadMessages(unsigned char* buf);
  void reserve(int64_t size);
  Rcpp::DataFrame getDF();
  
  // Members
  std::vector<char> type;
  std::vector<int64_t> locateCode;
  std::vector<int64_t> trackingNumber;
  std::vector<int64_t> timestamp;
  std::vector<int64_t> orderRef;
  std::vector<int64_t> shares;
  std::vector<int64_t> matchNumber;
  std::vector<bool>               printable;
  std::vector<double>             price;
  std::vector<int64_t> newOrderRef;
};

#endif //MESSAGES_H
