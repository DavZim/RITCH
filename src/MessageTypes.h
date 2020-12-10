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
  Rcpp::NumericVector   match_number;
  Rcpp::LogicalVector   printable;
  Rcpp::NumericVector   price;
  Rcpp::NumericVector   new_order_ref;
};

/*
 * #############################################################################
 * # Other classes, i.e., system information, circuit breakers etc...
 * #############################################################################
 * 
 */

/**
 * @brief      A class that parses the System Event (message type 'S')
 */
class SystemEvents : public MessageType {
public:
  SystemEvents() : MessageType(
    {'S'}, 
    {ITCH::POS::S},
    {"msg_type", "locate_code", "tracking_number", "timestamp", "event_code"}
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
  Rcpp::CharacterVector event_code;
};


/**
 * @brief      A class that parses the Stock Directory (message type 'R')
 */
class StockDirectory : public MessageType {
public:
  StockDirectory() : MessageType(
  {'R'}, 
  {ITCH::POS::R},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "stock", 
   "market_category", "financial_status", "lot_size", "round_lots_only",
   "issue_classification", "issue_subtype", "authentic", "short_sell_closeout",
   "ipo_flag", "luld_price_tier", "etp_flag", "etp_leverage", "inverse"}
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
  Rcpp::CharacterVector stock;
  Rcpp::CharacterVector market_category;
  Rcpp::CharacterVector financial_status;
  Rcpp::IntegerVector   lot_size;
  Rcpp::LogicalVector   round_lots_only;
  Rcpp::CharacterVector issue_classification;
  Rcpp::CharacterVector issue_subtype;
  Rcpp::CharacterVector authentic; // authentic true = P (Live/Production) false = T (Test)
  Rcpp::LogicalVector   short_sell_closeout;
  Rcpp::LogicalVector   ipo_flag;
  Rcpp::CharacterVector luld_price_tier;
  Rcpp::LogicalVector   etp_flag;
  Rcpp::IntegerVector   etp_leverage;
  Rcpp::LogicalVector   inverse;
};


/**
 * @brief      A class that parses the Stock Directory (message type 'H' and 'h')
 */
class TradingStatus : public MessageType {
public:
  TradingStatus() : MessageType(
  {'H', 'h'}, 
  {ITCH::POS::H, ITCH::POS::h},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "stock",
   // type H
   "trading_state", "reserved", "reason",
   // type h"
   "market_code", "operation_halted"
  }
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
  Rcpp::CharacterVector stock;
  Rcpp::CharacterVector trading_state;
  Rcpp::CharacterVector reserved;
  Rcpp::CharacterVector reason;
  Rcpp::CharacterVector market_code;
  Rcpp::LogicalVector   operation_halted;
};

/**
 * @brief      A class that parses Reg SHO messages (message type 'Y')
 */
class RegSHO : public MessageType {
public:
  RegSHO() : MessageType(
  {'Y'}, 
  {ITCH::POS::Y},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "stock", "regsho_action"
  }
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
  Rcpp::CharacterVector stock;
  Rcpp::CharacterVector regsho_action;
};

/**
 * @brief      A class that parses Market Participant Status messages (message type 'L')
 */
class ParticipantStates : public MessageType {
public:
  ParticipantStates() : MessageType(
  {'L'}, 
  {ITCH::POS::L},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "mpid", "stock",
   "primary_mm", "mm_mode", "participant_state"
  }
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
  Rcpp::CharacterVector mpid;
  Rcpp::CharacterVector stock;
  Rcpp::LogicalVector   primary_mm;
  Rcpp::CharacterVector mm_mode;
  Rcpp::CharacterVector participant_state;
};
/**
 * @brief      A class that parses MWCB information (message type 'V' and 'W')
 */
class MWCB : public MessageType {
public:
  MWCB() : MessageType(
  {'V', 'W'}, 
  {ITCH::POS::V, ITCH::POS::W},
  {"msg_type", "locate_code", "tracking_number", "timestamp", 
   // message type 'V'
   "level1", "level2", "level3",
   // message type 'W'
   "breached_level"
  }
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
  Rcpp::NumericVector   level1;
  Rcpp::NumericVector   level2;
  Rcpp::NumericVector   level3;
  Rcpp::IntegerVector   breached_level;
};

/**
 * @brief      A class that parses IPO information (message type 'V' and 'W')
 */
class IPO : public MessageType {
public:
  IPO() : MessageType(
  {'K'}, 
  {ITCH::POS::K},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "stock",
   "release_time", "release_qualifier", "ipo_price"
  }
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
  Rcpp::CharacterVector stock;
  Rcpp::IntegerVector   release_time;
  Rcpp::CharacterVector release_qualifier;
  Rcpp::NumericVector   ipo_price;
};

/**
 * @brief      A class that parses LULD information (message type 'J')
 */
class LULD : public MessageType {
public:
  LULD() : MessageType(
  {'J'}, 
  {ITCH::POS::J},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "stock",
   "reference_price", "upper_price", "lower_price", "extension"
  }
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
  Rcpp::CharacterVector stock;
  Rcpp::NumericVector   reference_price;
  Rcpp::NumericVector   upper_price;
  Rcpp::NumericVector   lower_price;
  Rcpp::IntegerVector   extension;
};


/**
 * @brief      A class that parses NOII information (message type 'I')
 */
class NOII : public MessageType {
public:
  NOII() : MessageType(
  {'I'}, 
  {ITCH::POS::I},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "paired_shares",
   "imbalance_shares", "imbalance_direction", "stock", "far_price", 
   "near_price", "reference_price", "cross_type", "variation_indicator"
  }
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
  Rcpp::NumericVector   paired_shares;
  Rcpp::NumericVector   imbalance_shares;
  Rcpp::CharacterVector imbalance_direction;
  Rcpp::CharacterVector stock;
  Rcpp::NumericVector   far_price;
  Rcpp::NumericVector   near_price;
  Rcpp::NumericVector   reference_price;
  Rcpp::CharacterVector cross_type;
  Rcpp::CharacterVector variation_indicator;
};

/**
 * @brief      A class that parses RPII information (message type 'N')
 */
class RPII : public MessageType {
public:
  RPII() : MessageType(
  {'N'}, 
  {ITCH::POS::N},
  {"msg_type", "locate_code", "tracking_number", "timestamp", "stock", "interest_flag"
  }
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
  Rcpp::CharacterVector stock;
  Rcpp::CharacterVector interest_flag;
};


#endif //MESSAGES_H
