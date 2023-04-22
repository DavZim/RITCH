#include "write_functions.h"


/*
 * #############################################################################
 * Main Write Function
 * #############################################################################
 *
 * ll is a list of data.frames, where each df is sorted by timestamp
 * filename, the filename to write to
 * gz if the file should be a gz-compressed file
 * returns the number of bytes written
 */

// [[Rcpp::export]]
int64_t write_itch_impl(Rcpp::List ll, std::string filename,
                        bool append,
                        bool gz,
                        size_t max_buffer_size,
                        bool quiet) {

  if (max_buffer_size > 5e9) {
    Rcpp::warning("max_buffer_size set to > 5e9, capping it to 5e9\n");
    max_buffer_size = 5e9;
  }
  if (max_buffer_size < 52) {
    Rcpp::warning("max_buffer_size set to < 52, increasing to 52\n");
    max_buffer_size = 52;
  }

  const int list_length = ll.size();
  size_t msg_size = 0;
  int64_t total_msgs = 0;

  const int64_t max_val = std::numeric_limits<int64_t>::max();
  std::vector<int64_t> indices (list_length);
  std::vector<int64_t> timestamps (list_length);

  if (!quiet) Rprintf("[Counting]   ");
  // initiate the indices, timestamps as well as count the required buffer size
  for (int ii = 0; ii < list_length; ii++) {
    Rcpp::List df = ll.at(ii);
    Rcpp::CharacterVector mt = df["msg_type"];
    Rcpp::NumericVector ts = df["timestamp"];

    // populate the indices
    indices[ii] = 0;

    // populate the timestamps
    std::memcpy(&(timestamps[ii]), &(ts[0]), sizeof(int64_t));
    // Rprintf("ts[0] is %+lld; ts is now %+lld\n", ts[0], timestamps[ii]);

    for (int l = 0; l < mt.size(); l++) {
      const unsigned char msg = Rcpp::as<char>(mt[l]);
      msg_size += get_message_size(msg);
      total_msgs++;
    }
  }
  if (!quiet) Rprintf("%s messages (%s bytes) found\n",
      format_thousands(total_msgs).c_str(),
      format_thousands(msg_size).c_str());

  // min of max_buffer_size and msg_size, but only 32 bit for buffer - is large enough
  const size_t buff_size = max_buffer_size > msg_size ? msg_size : max_buffer_size;
  unsigned char * buf;
  buf = (unsigned char*) calloc(buff_size, sizeof(unsigned char));
  int64_t msg_ct = 0;

  // implement multi buffer....
  int64_t i = 0, total_bytes = 0;
  bool first_write = true;

  if (!quiet) Rprintf("[Converting] to binary .");
  while (msg_ct < total_msgs) {
    // find at which list position (lp) the lowest timestamp is
    const int lp = get_min_val_pos(timestamps);
    Rcpp::List df = ll[lp];
    int64_t lp_idx = indices[lp];

    Rcpp::NumericVector ts = df["timestamp"];
    Rcpp::CharacterVector mt = df["msg_type"];

    const unsigned char msg_type = Rcpp::as<char>(mt[lp_idx]);

    const int64_t msg_length = get_message_size(msg_type);
    if (i + msg_length > (int64_t) buff_size) {
      if (!quiet) Rprintf(".");
      // write_buffer
      // append can only be set on the first write... afterwards append by default
      write_buffer_to_file(buf, i, filename, first_write ? append : true, gz);
      first_write = false;

      // empty buffer
      buf = (unsigned char*) realloc(buf, buff_size * sizeof(unsigned char));
      if (!buf) Rcpp::stop("Out of Memory");
      total_bytes += i;
      // reset value in buffer to 0
      i = 0;
    }

    // load the message to the buffer
    i += load_message_to_buffer(&(buf[i]), lp_idx, df);

    // update the timestamps information
    // lp_idx was increased in load_message_to_buffer by one!
    if (lp_idx == ts.size()) {
      // take the max value if the end of this df is reached
      std::memcpy(&(timestamps[lp]), &max_val, sizeof(int64_t));
    } else {
      std::memcpy(&(timestamps[lp]), &(ts[lp_idx]), sizeof(int64_t));
    }
    // at position lp, the index is increased by one.
    // next loop, take the next row/observation
    indices[lp] += 1;
    msg_ct++;
  }

  if (!quiet) Rprintf("\n[Writing]    to file\n");
  total_bytes += i;
  // Rprintf("Write data to file '%s'\n", filename.c_str());
  write_buffer_to_file(buf, i, filename, first_write ? append : true, gz);
  free(buf);

  return total_bytes;
}

/*
 * #############################################################################
 * Parsing Functions
 * #############################################################################
 *
 * Each parsing function takes a buffer, an data.frame and a msg number (position)
 * and returns the number of bytes it has written to the buffer
 */

// orders
uint64_t parse_orders_at(unsigned char * buf, Rcpp::DataFrame df,
                         uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::NumericVector   order_ref       = df["order_ref"];
  Rcpp::LogicalVector   buy             = df["buy"];
  Rcpp::IntegerVector   shares          = df["shares"];
  Rcpp::CharacterVector stock           = df["stock"];
  Rcpp::NumericVector   price           = df["price"];
  Rcpp::CharacterVector mpid            = df["mpid"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  std::memcpy(&val64, &(order_ref[msg_num]), sizeof(int64_t));
  i += set8bytes(&buf[i], val64);

  buf[i++] = buy[msg_num] ? 'B' : 'S';
  i += set4bytes(&buf[i], shares[msg_num]);
  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  i += set4bytes(&buf[i], (int) round(price[msg_num] * 10000));

  if (msg == 'F') i += setCharBytes(&buf[i], std::string(mpid[msg_num]), 4);

  return i;
}

// trades
uint64_t parse_trades_at(unsigned char * buf, Rcpp::DataFrame df,
                         uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::NumericVector   order_ref       = df["order_ref"];
  Rcpp::LogicalVector   buy             = df["buy"];
  Rcpp::IntegerVector   shares          = df["shares"];
  Rcpp::CharacterVector stock           = df["stock"];
  Rcpp::NumericVector   price           = df["price"];
  Rcpp::NumericVector   match_number    = df["match_number"];
  Rcpp::CharacterVector cross_type      = df["cross_type"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  switch (msg) {
    case 'P':
      std::memcpy(&val64, &(order_ref[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);

      buf[i++] = buy[msg_num] ? 'B' : 'S';
      i += set4bytes(&buf[i], shares[msg_num]);
      i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
      i += set4bytes(&buf[i], (int) round(price[msg_num] * 10000.0));

      std::memcpy(&val64, &(match_number[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);
      // not used: cross_type
      break;
    case 'Q':
      // shares/cross-shares are usually 4 byte, but in Q its 8 byte
      val64 = (int64_t) shares[msg_num];
      i += set8bytes(&buf[i], val64);
      i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
      i += set4bytes(&buf[i], (int) round(price[msg_num] * 10000.0));
      std::memcpy(&val64, &(match_number[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);
      buf[i++] = Rcpp::as<char>(cross_type[msg_num]);
      // not used: order_ref, buy
      break;
    case 'B':
      std::memcpy(&val64, &(match_number[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);
      // not used: order_ref, buy, shares, stock, price, cross_type
      break;
    default:
      Rcpp::Rcout << "Unknown Type: " << buf[0] << "\n";
      break;
  }

  return i;
}

// modifications
uint64_t parse_modifications_at(unsigned char * buf, Rcpp::DataFrame df,
                                uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::NumericVector   order_ref       = df["order_ref"];
  Rcpp::IntegerVector   shares          = df["shares"];
  Rcpp::NumericVector   match_number    = df["match_number"];
  Rcpp::LogicalVector   printable       = df["printable"];
  Rcpp::NumericVector   price           = df["price"];
  Rcpp::NumericVector   new_order_ref   = df["new_order_ref"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  std::memcpy(&val64, &(order_ref[msg_num]), sizeof(int64_t));
  i += set8bytes(&buf[i], val64);

  switch (msg) {
    case 'E':
      i += set4bytes(&buf[i], shares[msg_num]);
      std::memcpy(&val64, &(match_number[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);
      break;
    case 'C':
      i += set4bytes(&buf[i], shares[msg_num]);
      std::memcpy(&val64, &(match_number[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);
      buf[i++] = printable[msg_num] ? 'P' : 'Y';
      i += set4bytes(&buf[i], (int) round(price[msg_num] * 10000.0));
      break;
    case 'X':
      i += set4bytes(&buf[i], shares[msg_num]);
      break;
    case 'D':
      // no other data needed...
      break;
    case 'U':
      std::memcpy(&val64, &(new_order_ref[msg_num]), sizeof(int64_t));
      i += set8bytes(&buf[i], val64);
      i += set4bytes(&buf[i], shares[msg_num]);
      i += set4bytes(&buf[i], (int) round(price[msg_num] * 10000.0));
      break;
    default:
      Rcpp::Rcout << "Unkown message type: " << msg << "\n";
      break;
  }
  return i;
}

// system_events
uint64_t parse_system_events_at(unsigned char * buf, Rcpp::DataFrame df,
                                uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::CharacterVector event_code      = df["event_code"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  buf[i++] = Rcpp::as<char>(event_code[msg_num]);
  return i;
}

// stock_directory
uint64_t parse_stock_directory_at(unsigned char * buf, Rcpp::DataFrame df,
                                  uint64_t msg_num) {

  Rcpp::CharacterVector msg_type             = df["msg_type"];
  Rcpp::IntegerVector   stock_locate         = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number      = df["tracking_number"];
  Rcpp::NumericVector   timestamp            = df["timestamp"];
  Rcpp::CharacterVector stock                = df["stock"];
  Rcpp::CharacterVector market_category      = df["market_category"];
  Rcpp::CharacterVector financial_status     = df["financial_status"];
  Rcpp::IntegerVector   lot_size             = df["lot_size"];
  Rcpp::LogicalVector   round_lots_only      = df["round_lots_only"];
  Rcpp::CharacterVector issue_classification = df["issue_classification"];
  Rcpp::CharacterVector issue_subtype        = df["issue_subtype"];
  Rcpp::LogicalVector   authentic            = df["authentic"];
  Rcpp::LogicalVector   short_sell_closeout  = df["short_sell_closeout"];
  Rcpp::LogicalVector   ipo_flag             = df["ipo_flag"];
  Rcpp::CharacterVector luld_price_tier      = df["luld_price_tier"];
  Rcpp::LogicalVector   etp_flag             = df["etp_flag"];
  Rcpp::IntegerVector   etp_leverage         = df["etp_leverage"];
  Rcpp::LogicalVector   inverse              = df["inverse"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  buf[i++] = Rcpp::as<char>(market_category[msg_num]);
  buf[i++] = Rcpp::as<char>(financial_status[msg_num]);
  i += set4bytes(&buf[i], lot_size[msg_num]);
  buf[i++] = round_lots_only[msg_num] ? 'Y' : 'N';
  buf[i++] = Rcpp::as<char>(issue_classification[msg_num]);
  i += setCharBytes(&buf[i], std::string(issue_subtype[msg_num]), 2);
  buf[i++] = authentic[msg_num] ? 'P' : 'T';
  buf[i++] = short_sell_closeout[msg_num] == true ? 'Y' : short_sell_closeout[msg_num] == false ? 'N' : ' ';
  buf[i++] = ipo_flag[msg_num] == true ? 'Y' : ipo_flag[msg_num] == false ? 'N' : ' ';
  buf[i++] = Rcpp::as<char>(luld_price_tier[msg_num]);
  buf[i++] = etp_flag[msg_num] == true ? 'Y' : etp_flag[msg_num] == false ? 'N' : ' ';
  i += set4bytes(&buf[i], etp_leverage[msg_num]);
  buf[i++] = inverse[msg_num] ? 'Y' : 'N';

  return i;
}

// trading_status
uint64_t parse_trading_status_at(unsigned char * buf, Rcpp::DataFrame df,
                                 uint64_t msg_num) {

  Rcpp::CharacterVector msg_type         = df["msg_type"];
  Rcpp::IntegerVector   stock_locate     = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number  = df["tracking_number"];
  Rcpp::NumericVector   timestamp        = df["timestamp"];
  Rcpp::CharacterVector stock            = df["stock"];
  Rcpp::CharacterVector trading_state    = df["trading_state"];
  Rcpp::CharacterVector reserved         = df["reserved"];
  Rcpp::CharacterVector reason           = df["reason"];
  Rcpp::CharacterVector market_code      = df["market_code"];
  Rcpp::LogicalVector   operation_halted = df["operation_halted"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);

  switch (msg) {
    case 'H':
      buf[i++] = Rcpp::as<char>(trading_state[msg_num]);
      buf[i++] = Rcpp::as<char>(reserved[msg_num]);
      i += setCharBytes(&buf[i], std::string(reason[msg_num]), 4);
      break;
    case 'h':
      buf[i++] = Rcpp::as<char>(market_code[msg_num]);
      buf[i++] = operation_halted[msg_num] ? 'H' : 'T';
      break;
    default:
      Rcpp::Rcout << "Unkown message type: " << msg << "\n";
      break;
  }

  return i;
}

// reg_sho
uint64_t parse_reg_sho_at(unsigned char * buf, Rcpp::DataFrame df,
                          uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::CharacterVector stock           = df["stock"];
  Rcpp::CharacterVector regsho_action   = df["regsho_action"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);
  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  buf[i++] = Rcpp::as<char>(regsho_action[msg_num]);

  return i;
}

// market_participants_states
uint64_t parse_market_participants_states_at(unsigned char * buf, Rcpp::DataFrame df,
                                             uint64_t msg_num) {

  Rcpp::CharacterVector msg_type          = df["msg_type"];
  Rcpp::IntegerVector   stock_locate      = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number   = df["tracking_number"];
  Rcpp::NumericVector   timestamp         = df["timestamp"];
  Rcpp::CharacterVector mpid              = df["mpid"];
  Rcpp::CharacterVector stock             = df["stock"];
  Rcpp::LogicalVector   primary_mm        = df["primary_mm"];
  Rcpp::CharacterVector mm_mode           = df["mm_mode"];
  Rcpp::CharacterVector participant_state = df["participant_state"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);
  i += setCharBytes(&buf[i], std::string(mpid[msg_num]), 4);
  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  buf[i++] = primary_mm[msg_num] ? 'Y' : 'N';
  buf[i++] = Rcpp::as<char>(mm_mode[msg_num]);
  buf[i++] = Rcpp::as<char>(participant_state[msg_num]);

  return i;
}

// mwcb
uint64_t parse_mwcb_at(unsigned char * buf, Rcpp::DataFrame df,
                       uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::NumericVector   level1          = df["level1"];
  Rcpp::NumericVector   level2          = df["level2"];
  Rcpp::NumericVector   level3          = df["level3"];
  Rcpp::IntegerVector   breached_level  = df["breached_level"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  switch (msg) {
    case 'V':
      i += set8bytes(&buf[i], (int64_t) round(level1[msg_num] * 100000000.0));
      i += set8bytes(&buf[i], (int64_t) round(level2[msg_num] * 100000000.0));
      i += set8bytes(&buf[i], (int64_t) round(level3[msg_num] * 100000000.0));
      break;
    case 'W':
      buf[i++] = '0' + breached_level[msg_num];
      break;
    default:
      Rcpp::Rcout << "Unkown message type: " << msg << "\n";
      break;
  }

  return i;
}

// ipo
uint64_t parse_ipo_at(unsigned char * buf, Rcpp::DataFrame df,
                      uint64_t msg_num) {

  Rcpp::CharacterVector msg_type          = df["msg_type"];
  Rcpp::IntegerVector   stock_locate      = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number   = df["tracking_number"];
  Rcpp::NumericVector   timestamp         = df["timestamp"];
  Rcpp::CharacterVector stock             = df["stock"];
  Rcpp::IntegerVector   release_time      = df["release_time"];
  Rcpp::CharacterVector release_qualifier = df["release_qualifier"];
  Rcpp::NumericVector   ipo_price         = df["ipo_price"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);
  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  i += set4bytes(&buf[i], release_time[msg_num]);
  buf[i++] = Rcpp::as<char>(release_qualifier[msg_num]);
  i += set4bytes(&buf[i], (int) round(ipo_price[msg_num] * 10000.0));

  return i;
}

// luld
uint64_t parse_luld_at(unsigned char * buf, Rcpp::DataFrame df,
                       uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::CharacterVector stock           = df["stock"];
  Rcpp::NumericVector   reference_price = df["reference_price"];
  Rcpp::NumericVector   upper_price     = df["upper_price"];
  Rcpp::NumericVector   lower_price     = df["lower_price"];
  Rcpp::IntegerVector   extension       = df["extension"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  i += set4bytes(&buf[i], (int) round(reference_price[msg_num] * 10000.0));
  i += set4bytes(&buf[i], (int) round(upper_price[msg_num] * 10000.0));
  i += set4bytes(&buf[i], (int) round(lower_price[msg_num] * 10000.0));
  i += set4bytes(&buf[i], extension[msg_num]);

  return i;
}

// noii
uint64_t parse_noii_at(unsigned char * buf, Rcpp::DataFrame df,
                       uint64_t msg_num) {

  Rcpp::CharacterVector msg_type            = df["msg_type"];
  Rcpp::IntegerVector   stock_locate        = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number     = df["tracking_number"];
  Rcpp::NumericVector   timestamp           = df["timestamp"];
  Rcpp::NumericVector   paired_shares       = df["paired_shares"];
  Rcpp::NumericVector   imbalance_shares    = df["imbalance_shares"];
  Rcpp::CharacterVector imbalance_direction = df["imbalance_direction"];
  Rcpp::CharacterVector stock               = df["stock"];
  Rcpp::NumericVector   far_price           = df["far_price"];
  Rcpp::NumericVector   near_price          = df["near_price"];
  Rcpp::NumericVector   reference_price     = df["reference_price"];
  Rcpp::CharacterVector cross_type          = df["cross_type"];
  Rcpp::CharacterVector variation_indicator = df["variation_indicator"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);

  std::memcpy(&val64, &(paired_shares[msg_num]), sizeof(int64_t));
  i += set8bytes(&buf[i], val64);
  std::memcpy(&val64, &(imbalance_shares[msg_num]), sizeof(int64_t));
  i += set8bytes(&buf[i], val64);
  buf[i++] = Rcpp::as<char>(imbalance_direction[msg_num]);
  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  i += set4bytes(&buf[i], (int) round(far_price[msg_num] * 10000.0));
  i += set4bytes(&buf[i], (int) round(near_price[msg_num] * 10000.0));
  i += set4bytes(&buf[i], (int) round(reference_price[msg_num] * 10000.0));
  buf[i++] = Rcpp::as<char>(cross_type[msg_num]);
  buf[i++] = Rcpp::as<char>(variation_indicator[msg_num]);

  return i;
}

// rpii
uint64_t parse_rpii_at(unsigned char * buf, Rcpp::DataFrame df,
                       uint64_t msg_num) {

  Rcpp::CharacterVector msg_type        = df["msg_type"];
  Rcpp::IntegerVector   stock_locate    = df["stock_locate"];
  Rcpp::IntegerVector   tracking_number = df["tracking_number"];
  Rcpp::NumericVector   timestamp       = df["timestamp"];
  Rcpp::CharacterVector stock           = df["stock"];
  Rcpp::CharacterVector interest_flag   = df["interest_flag"];

  uint64_t i = 2; // add two empty bytes at the beginning
  int64_t val64 = 0;
  const unsigned char msg = Rcpp::as<char>(msg_type[msg_num]);
  buf[i++] = msg;

  i += set2bytes(&buf[i], stock_locate[msg_num]);
  i += set2bytes(&buf[i], tracking_number[msg_num]);

  std::memcpy(&val64, &(timestamp[msg_num]), sizeof(int64_t));
  i += set6bytes(&buf[i], val64);
  i += setCharBytes(&buf[i], std::string(stock[msg_num]), 8);
  buf[i++] = Rcpp::as<char>(interest_flag[msg_num]);

  return i;
}

// loads from df at position msg_ct one message into the buffer
// returns the number of bytes written to the buffer
int64_t load_message_to_buffer(unsigned char * buf, int64_t &msg_ct,
                               Rcpp::DataFrame df) {
  int64_t i = 0;
  Rcpp::CharacterVector vec = df["msg_type"];
  const unsigned char msg = Rcpp::as<char>(vec[0]);
  switch (msg) {
    case 'A': case 'F':
      // orders
      i += parse_orders_at(&buf[i], df, msg_ct);
      break;
      // trades
    case 'P': case 'Q': case 'B':
      i += parse_trades_at(&buf[i], df, msg_ct);
      break;
      // modifications
    case 'E': case 'C': case 'X': case 'D': case 'U':
      i += parse_modifications_at(&buf[i], df, msg_ct);
      break;
      // system_events
    case 'S':
      i += parse_system_events_at(&buf[i], df, msg_ct);
      break;
      // stock_directory
    case 'R':
      i += parse_stock_directory_at(&buf[i], df, msg_ct);
      break;
      // trading stats
    case 'H': case 'h':
      i += parse_trading_status_at(&buf[i], df, msg_ct);
      break;
      // reg_sho
    case 'Y':
      i += parse_reg_sho_at(&buf[i], df, msg_ct);
      break;
      // market_participants_states
    case 'L':
      i += parse_market_participants_states_at(&buf[i], df, msg_ct);
      break;
      // mwcb
    case 'W': case 'V':
      i += parse_mwcb_at(&buf[i], df, msg_ct);
      break;
      // IPO
    case 'K':
      i += parse_ipo_at(&buf[i], df, msg_ct);
      break;
      // luld
    case 'J':
      i += parse_luld_at(&buf[i], df, msg_ct);
      break;
      // noii
    case 'I':
      i += parse_noii_at(&buf[i], df, msg_ct);
      break;
      // rpii
    case 'N':
      i += parse_rpii_at(&buf[i], df, msg_ct);
      break;
    default:
      Rprintf("Message type '%c' not implemented, skipping\n", msg);
      Rcpp::stop("Unkown Message Type\n");
      break;
  }

  msg_ct++;
  return i;
}

// returns the index at which the min value sits in the index
int get_min_val_pos(std::vector<int64_t> &x) {
  const auto min_el = std::min_element(x.begin(), x.end());
  return std::distance(x.begin(), min_el);
}

void write_buffer_to_file(unsigned char* buf, int64_t size,
                          std::string filename, bool append, bool gz) {
  char mode[] = "wb";// append ? "ab" : "wb";
  if (append) mode[0] = 'a';

  if (!gz) {
    FILE* outfile;
    outfile = fopen(filename.c_str(), mode);
    if (outfile == NULL) {
      char buffer [50];
      snprintf(buffer, sizeof(buffer), "File Error number %i!", errno);
      Rcpp::stop(buffer);
    }
    fwrite(&buf[0], 1, size, outfile);
    fclose(outfile);
  } else {
    gzFile gzfile = gzopen(filename.c_str(), mode);
    if (gzfile == NULL) {
      char buffer [50];
      snprintf(buffer, sizeof(buffer), "File Error number %i!", errno);
      Rcpp::stop(buffer);
    }
    gzwrite(gzfile, &buf[0], size);
    gzclose(gzfile);
  }
}
