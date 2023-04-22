#ifndef READFUNCTIONS_H
#define READFUNCTIONS_H

#include "specifications.h"
#include "count_messages.h"

// Entry Function for the reading function
Rcpp::List read_itch_impl(std::vector<std::string> classes,
                          std::string filename,
                          int64_t start, int64_t end,
                          Rcpp::CharacterVector filter_msg_type,
                          Rcpp::IntegerVector filter_stock_locate,
                          Rcpp::NumericVector min_timestamp,
                          Rcpp::NumericVector max_timestamp,
                          int64_t max_buffer_size = 1e8,
                          bool quiet = false);

/*
 * Message Parser class, each class holds one "class" (stock_directory,
 *   sytem_events, trades, ...) and is able to parse them.
 *
 * The main usage is
 *
 * - create a MessageParser with its type (can be empty for no class)
 * - activate the object if messages need to be parsed later on
 * - init the vectors to appropriate sizes
 * - loop over a buffer and call parse_message on the respective messages
 * - convert the parsed messages to a data.frame with get_data_frame
 *
 * Note that the class holds vectors for all possible classes but only fills
 * and uses needed classes.
 *
 */
class MessageParser{
public:
  MessageParser(std::string type,
                int64_t skip = 0,
                int64_t n_max = std::numeric_limits<int64_t>::max());

  void activate();
  void init_vectors(int64_t n);
  void parse_message(unsigned char * buf);
  Rcpp::List get_data_frame();

  std::vector<char> msg_types;
  bool active = false;

private:
  void prune_vectors();

  std::string type;
  // msg_buf_idx is only used when the skip/n_max is used.
  // index counts the number of messages in the Parser, msg_buf_idx counts the
  // running number of messages of this type it has seen (but not necessarily parsed!)
  int64_t size = 0, index = 0, msg_buf_idx = 0, start_count, end_count;
  std::vector<std::string> colnames;

  // general data vectors
  // NOTE: later classes may use earlier vectors as well,
  // e.g., noii also uses cross_type, defined under trades...

  Rcpp::CharacterVector msg_type;
  Rcpp::IntegerVector stock_locate, tracking_number;
  Rcpp::NumericVector timestamp;

  // system_events
  Rcpp::CharacterVector    event_code;

  // stock_directory
  Rcpp::CharacterVector stock;
  Rcpp::CharacterVector market_category, financial_status;
  Rcpp::IntegerVector lot_size;
  Rcpp::LogicalVector round_lots_only;
  Rcpp::CharacterVector issue_classification;
  Rcpp::CharacterVector issue_subtype;
  Rcpp::LogicalVector authentic;
  Rcpp::LogicalVector short_sell_closeout;
  Rcpp::LogicalVector ipo_flag;
  Rcpp::CharacterVector luld_price_tier;
  Rcpp::LogicalVector etp_flag;
  Rcpp::IntegerVector etp_leverage;
  Rcpp::LogicalVector inverse;

  // trading_status
  Rcpp::CharacterVector trading_state, reserved;
  Rcpp::CharacterVector reason;
  Rcpp::CharacterVector market_code;
  Rcpp::LogicalVector operation_halted;

  // reg_sho
  Rcpp::CharacterVector regsho_action;

  // Market Participant States
  Rcpp::LogicalVector primary_mm;
  Rcpp::CharacterVector mm_mode, participant_state;

  // mwcb
  Rcpp::NumericVector level1, level2, level3;
  Rcpp::IntegerVector breached_level;

  // ipo
  Rcpp::IntegerVector release_time;
  Rcpp::CharacterVector release_qualifier;
  Rcpp::NumericVector ipo_price;

  // luld
  Rcpp::NumericVector reference_price, lower_price, upper_price;
  Rcpp::IntegerVector extension;

  // orders
  Rcpp::NumericVector order_ref;
  Rcpp::LogicalVector buy;
  Rcpp::IntegerVector shares;
  Rcpp::NumericVector price;
  Rcpp::CharacterVector mpid;

  // modifications
  Rcpp::NumericVector new_order_ref;
  Rcpp::LogicalVector printable;

  // trades
  Rcpp::NumericVector match_number;
  Rcpp::CharacterVector cross_type;

  // noii
  Rcpp::NumericVector paired_shares, imbalance_shares;
  Rcpp::CharacterVector imbalance_direction;
  Rcpp::NumericVector far_price, near_price;
  Rcpp::CharacterVector variation_indicator;

  // rpii
  Rcpp::CharacterVector interest_flag;
};

#endif // READFUNCTIONS_H
