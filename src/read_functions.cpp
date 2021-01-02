#include "read_functions.h"

// [[Rcpp::export]]
Rcpp::List read_itch_impl(std::vector<std::string> classes,
                          std::string filename,
                          int64_t start, int64_t end,
                          Rcpp::CharacterVector filter_msg_type,
                          Rcpp::IntegerVector filter_stock_locate,
                          Rcpp::NumericVector min_timestamp,
                          Rcpp::NumericVector max_timestamp,
                          int64_t max_buffer_size,
                          bool quiet) {
  
  std::vector<int64_t> count = count_messages_internal(filename, max_buffer_size);
  
  int64_t total_msgs = 0;
  for (int64_t v : count) total_msgs += v;
  if (!quiet) Rprintf("[Counting]   num messages %s\n",
                      format_thousands(total_msgs).c_str());
  
  std::vector<int64_t> sizes(classes.size());
  
  // initiate the MessageParsers and resize the vectors...
  
  // for each message class, hold a pointer to a message parser
  // message classes: 13
  
  // N_TYPES = 40, for each message in MSG_SIZES one
  std::vector<MessageParser*> msg_parsers(N_TYPES);
  std::map<std::string, MessageParser*> class_to_parsers;
  
  MessageParser empty("");
  for (int i = 0; i < N_TYPES; i++) msg_parsers[i] = &empty;
  
  // treat filters
  std::vector<char> filter_msgs;
  std::vector<int>  filter_sloc;
  
  for (auto f : filter_msg_type) filter_msgs.push_back(Rcpp::as<char>(f));
  for (int s : filter_stock_locate) filter_sloc.push_back(s);

  const size_t ts_size = min_timestamp.size();
  std::vector<int64_t> min_ts(ts_size);
  std::vector<int64_t> max_ts(ts_size);
  
  std::memcpy(&(min_ts[0]), &(min_timestamp[0]), ts_size * sizeof(int64_t));
  std::memcpy(&(max_ts[0]), &(max_timestamp[0]), ts_size * sizeof(int64_t));
  
  // Rprintf("Loading Message Parsers\n");
  for (std::string cls : MSG_CLASSES) {
    // Rprintf("Looking at message class '%s'\n", cls.c_str());
    MessageParser* msgp_ptr = new MessageParser(cls, start, end);
    
    // check if this class needs to be activated?!
      for (const std::string c : classes) if (c == cls) msgp_ptr->activate();
    
    int64_t num_msg_this_type = 0;
    std::vector<char> this_msg_types = msgp_ptr->msg_types;
    
    for (const char mt : this_msg_types) num_msg_this_type += count[mt - 'A'];
    
    if (msgp_ptr->active) {
      if (!quiet) Rprintf("[Counting]   num '%s' messages %s\n",
                          cls.c_str(), format_thousands(num_msg_this_type).c_str());
      
      // Rprintf("Active and resized to '%i'\n", num_msg_this_type);
      msgp_ptr->init_vectors(num_msg_this_type);
    }
    
    class_to_parsers[cls] = msgp_ptr;
    for (const char mt : msgp_ptr->msg_types) msg_parsers[mt - 'A'] = msgp_ptr;
  }
  
  // parse the messages
  // redirect to the correct msg types only
  FILE* infile;
  infile = fopen(filename.c_str(), "rb");
  if (infile == NULL) Rcpp::stop("File Error!\n");
  
  // get size of the file
  fseek(infile, 0L, SEEK_END);
  int64_t filesize = ftell(infile);
  fseek(infile, 0L, SEEK_SET);
  
  // create buffer
  int64_t buf_size = max_buffer_size > filesize ? filesize : max_buffer_size;
  char * buf;
  buf = (char*) malloc(buf_size);
  // Rprintf("Allocating buffer to size %" PRId64 "\n", buf_size);
  
  int64_t bytes_read = 0, this_buffer_size = 0;
  
  while (bytes_read < filesize) {
    Rcpp::checkUserInterrupt();
    
    // read in buffer buffers
    this_buffer_size = fread(buf, 1, buf_size, infile);
    int64_t i = 0;
    
    int msg_size = 0;
    do {
      // Rprintf("offset %" PRId64 ":%" PRId64 " (size size %" PRId64 ") '%c'\n",
      //         bytes_read + i, bytes_read + i + get_message_size(buf[i + 2]),
      //         get_message_size(buf[i + 2]), buf[i + 2]);
      
      const char mt = buf[i + 2];
      // Check Filter Messages
      bool parse_message = true;
      // only check the filter if previous tests are all OK
      if (parse_message) 
        parse_message = passes_filter(&buf[i + 2], filter_msgs);
      if (parse_message) 
        parse_message = passes_filter(&buf[i + 2 + 1], filter_sloc);
      if (parse_message) 
        parse_message = passes_filter_in(&buf[i + 2 + 5], min_ts, max_ts);
      
      msg_size = get_message_size(mt);
      
      if (parse_message) msg_parsers[mt - 'A']->parse_message(&buf[i + 2]);
      
      // Rprintf("  take ? %i\n", msg_parsers[mt - 'A']->active ? 1 : 0);
      // Rprintf("msg size %" PRId64 "\n", msg_size);
      i += msg_size;
      // Rprintf("  i %" PRId64 "\n", i);
      
      // Rprintf("i + msg_size <= this_buffer_size %" PRId64 " <= %" PRId64 "\n",
                 // i + msg_size, this_buffer_size);
      // Rprintf("bytes_read + i <= filesize %" PRId64 " <= %" PRId64 "\n",
                 // bytes_read + i, filesize);
      
    } while (i + msg_size <= this_buffer_size && bytes_read + i <= filesize);
    
    // offset file pointer to fit the next message into the buffer
    const int64_t offset = i - this_buffer_size;
    fseek(infile, offset, SEEK_CUR);
    bytes_read += i;
  }
  
  free(buf);
  fclose(infile);
  
  // gather the data.frames into a list
  Rcpp::List res;
  Rcpp::CharacterVector res_names;
  
  for (std::string cls : classes) 
    res.push_back(class_to_parsers[cls]->get_data_frame());
  
  res.attr("names") = classes;
  
  return res;
}

// #############################################################################
// small internal helper function to convert bytes etc
// #############################################################################

/**
 * @brief      Converts 2 bytes from a buffer in big endian to an int32_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted integer
 */
inline int32_t get2bytes(char* buf) {
  return __builtin_bswap16(*reinterpret_cast<uint16_t*>(&buf[0]));
}

/**
 * @brief      Converts 4 bytes from a buffer in big endian to an int32_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted integer
 */
inline int32_t get4bytes(char* buf) {
  return __builtin_bswap32(*reinterpret_cast<uint32_t*>(&buf[0]));
}

/**
 * @brief      Converts 6 bytes from a buffer in big endian to an int64_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted int64_t
 */
inline int64_t get6bytes(char* buf) {
  return (__builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0])) & 0xFFFFFFFFFFFF0000) >> 16;
}

/**
 * @brief      Converts 8 bytes from a buffer in big endian to an int64_t
 *
 * @param      buf   The buffer as a pointer to an array of chars
 *
 * @return     The converted int64_t
 */
inline int64_t get8bytes(char* buf) {
  return __builtin_bswap64(*reinterpret_cast<uint64_t*>(&buf[0]));
}

// return N bytes of a buffer as a string
std::string getNBytes(char* buf, const int n, const char empty) {
  std::string res;
  for (int i = 0; i < n; ++i) if (buf[i] != empty) res += buf[i];
  return res;
}

// converts a Numeric Vector to int64
inline Rcpp::NumericVector to_int64(Rcpp::NumericVector v) {
  v.attr("class") = "integer64";
  return v;
}

// helper functions that check if a buffer value is in a vector of filters 
// equivalent of R buf_val %in% filter
inline bool passes_filter(char* buf, std::vector<char> &filter) {
  if (filter.size() == 0) return true;
  for (char cc : filter) if (cc == *buf) return true;
  return false;
}
// same helper function as before but for int vector
inline bool passes_filter(char* buf, std::vector<int> &filter) {
  if (filter.size() == 0) return true;
  const int val = get2bytes(&buf[0]);
  for (int cc : filter) if (cc == val) return true;
  return false;
}
// check larger/smaller inclusive for 8 byte numbers (timestamp)
// equivalent to R (buf_val >= lower & buf_val <= upper)
inline bool passes_filter_in(char* buf,
                             std::vector<int64_t> &lower, 
                             std::vector<int64_t> &upper) {
  // lower and upper have the same size!
  if (lower.size() == 0) return true;
  const int64_t val = get6bytes(&buf[0]);
  for (int i = 0; i < lower.size(); i++) 
    if (val >= lower[i] && val <= upper[i]) 
      return true;
  return false;
}

// #############################################################################
// Message Parser Functions
// #############################################################################

MessageParser::MessageParser(std::string type, int64_t start_count, int64_t end_count) {
  std::vector<std::string> base_colnames = {
    "msg_type", "stock_locate", "tracking_number", "timestamp"
  };
  
  this->start_count = start_count;
  this->end_count = end_count == -1 ? std::numeric_limits<int64_t>::max() : end_count;
  this->type = type;
  
  if (type == "system_events") {
    msg_types = {'S'};
    colnames = {"event_code"};
  } else if (type == "stock_directory") {
    msg_types = {'R'};
    colnames = {"stock", "market_category", "financial_status", "lot_size",
                "round_lots_only", "issue_classification", "issue_subtype",
                "authentic", "short_sell_closeout", "ipo_flag",
                "luld_price_tier", "etp_flag", "etp_leverage", "inverse"};
  } else if (type == "trading_status") {
    msg_types = {'H', 'h'};
    colnames = {"stock", "trading_state", "reserved", "reason", "market_code",
                "operation_halted"};
  } else if (type == "reg_sho") {
    msg_types = {'Y'};
    colnames = {"stock", "regsho_action"};
  } else if (type == "market_participant_states") {
    msg_types = {'L'};
    colnames = {"mpid", "stock", "primary_mm", "mm_mode", "participant_state"};
  } else if (type == "mwcb") {
    msg_types = {'V', 'W'};
    colnames = {"level1", "level2", "level3", "breached_level"};
  } else if (type == "ipo") {
    msg_types = {'K'};
    colnames = {"stock", "release_time", "release_qualifier", "ipo_price"};
  } else if (type == "luld") {
    msg_types = {'J'};
    colnames = {"stock", "reference_price", "upper_price", "lower_price",
                "extension"};
  } else if (type == "orders") {
    msg_types = {'A', 'F'};
    colnames = {"order_ref", "buy", "shares", "stock", "price", "mpid"};
  } else if (type == "modifications") {
    msg_types = {'E', 'C', 'X', 'D', 'U'};
    colnames = {"order_ref", "shares", "match_number", "printable", "price",
                "new_order_ref"};
  } else if (type == "trades") {
    msg_types = {'P', 'Q', 'B'};
    colnames = {"order_ref", "buy", "shares", "stock", "price",
                "match_number", "cross_type"};
  } else if (type == "noii") {
    msg_types = {'I'};
    colnames = {"paired_shares", "imbalance_shares", "imbalance_direction",
                "stock", "far_price", "near_price", "reference_price",
                "cross_type", "variation_indicator"};
  } else if (type == "rpii") {
    msg_types = {'N'};
    colnames = {"stock", "interest_flag"};
  } else if (type == "") {
    msg_types = {};
    colnames = {};
  } else {
    Rprintf("Unkown type of type '%s'\n", type.c_str());
    Rcpp::stop("Unknown message type\n");
  }
  colnames.insert(colnames.begin(), base_colnames.begin(), base_colnames.end());
}

// activates the class so that it actually parses messages when asked!
// this is done to make it easier to parse multiple classes.
void MessageParser::activate() {
  active = true;
}


// creates and resizes all needed vectors to a given size (n) 
void MessageParser::init_vectors(int64_t n) {
  if (!active) return;
  size = n;
  // Rprintf("Resize %s to %" PRId64 "\n", type.c_str(), n);
  
  msg_type        = Rcpp::CharacterVector(n);
  stock_locate    = Rcpp::IntegerVector(n);
  tracking_number = Rcpp::IntegerVector(n);
  timestamp       = Rcpp::NumericVector(n);
  
  if (type == "system_events") {
    
    event_code = Rcpp::CharacterVector(n);
    
  } else if (type == "stock_directory") {
    
    stock                = Rcpp::CharacterVector(n);
    market_category      = Rcpp::CharacterVector(n);
    financial_status     = Rcpp::CharacterVector(n);
    lot_size             = Rcpp::IntegerVector(n);
    round_lots_only      = Rcpp::LogicalVector(n);
    issue_classification = Rcpp::CharacterVector(n);
    issue_subtype        = Rcpp::CharacterVector(n);
    authentic            = Rcpp::LogicalVector(n);
    short_sell_closeout  = Rcpp::LogicalVector(n);
    ipo_flag             = Rcpp::LogicalVector(n);
    luld_price_tier      = Rcpp::CharacterVector(n);
    etp_flag             = Rcpp::LogicalVector(n);
    etp_leverage         = Rcpp::IntegerVector(n);
    inverse              = Rcpp::LogicalVector(n);
    
  } else if (type == "trading_status") {
    
    stock            = Rcpp::CharacterVector(n);
    trading_state    = Rcpp::CharacterVector(n);
    reserved         = Rcpp::CharacterVector(n);
    reason           = Rcpp::CharacterVector(n);
    market_code      = Rcpp::CharacterVector(n);
    operation_halted = Rcpp::LogicalVector(n);
    
  } else if (type == "reg_sho") {
    
    stock         = Rcpp::CharacterVector(n);
    regsho_action = Rcpp::CharacterVector(n);
    
  } else if (type == "market_participant_states") {
    
    mpid              = Rcpp::CharacterVector(n);
    stock             = Rcpp::CharacterVector(n);
    primary_mm        = Rcpp::LogicalVector(n);
    mm_mode           = Rcpp::CharacterVector(n);
    participant_state = Rcpp::CharacterVector(n);
    
  } else if (type == "mwcb") {
    
    level1         = Rcpp::NumericVector(n);
    level2         = Rcpp::NumericVector(n);
    level3         = Rcpp::NumericVector(n);
    breached_level = Rcpp::IntegerVector(n);
    
  } else if (type == "ipo") {
    
    stock             = Rcpp::CharacterVector(n);
    release_time      = Rcpp::IntegerVector(n);
    release_qualifier = Rcpp::CharacterVector(n);
    ipo_price         = Rcpp::NumericVector(n);
    
  } else if (type == "luld") {
    
    stock           = Rcpp::CharacterVector(n);
    reference_price = Rcpp::NumericVector(n);
    upper_price     = Rcpp::NumericVector(n);
    lower_price     = Rcpp::NumericVector(n);
    extension       = Rcpp::IntegerVector(n);
    
  } else if (type == "orders") {
    
    order_ref = Rcpp::NumericVector(n);
    buy       = Rcpp::LogicalVector(n);
    shares    = Rcpp::IntegerVector(n);
    stock     = Rcpp::CharacterVector(n);
    price     = Rcpp::NumericVector(n);
    mpid      = Rcpp::CharacterVector(n);
    
  } else if (type == "modifications") {
    
    order_ref     = Rcpp::NumericVector(n);
    shares        = Rcpp::IntegerVector(n);
    match_number  = Rcpp::NumericVector(n);
    printable     = Rcpp::LogicalVector(n);
    price         = Rcpp::NumericVector(n);
    new_order_ref = Rcpp::NumericVector(n);
    
  } else if (type == "trades") {
    
    order_ref    = Rcpp::NumericVector(n);
    buy          = Rcpp::LogicalVector(n);
    shares       = Rcpp::IntegerVector(n);
    stock        = Rcpp::CharacterVector(n);
    price        = Rcpp::NumericVector(n);
    match_number = Rcpp::NumericVector(n);
    cross_type   = Rcpp::CharacterVector(n);
    
  } else if (type == "noii") {
    
    paired_shares       = Rcpp::NumericVector(n);
    imbalance_shares    = Rcpp::NumericVector(n);
    imbalance_direction = Rcpp::CharacterVector(n);
    stock               = Rcpp::CharacterVector(n);
    far_price           = Rcpp::NumericVector(n);
    near_price          = Rcpp::NumericVector(n);
    reference_price     = Rcpp::NumericVector(n);
    cross_type          = Rcpp::CharacterVector(n);
    variation_indicator = Rcpp::CharacterVector(n);
    
  } else if (type == "rpii") {
    
    stock         = Rcpp::CharacterVector(n);
    interest_flag = Rcpp::CharacterVector(n);
    
  }
}

// prunes all used vectors to its index (= actual used size)
void MessageParser::prune_vectors() {
  if (!active) return;
  
  msg_type.erase(        msg_type.begin() + index,         msg_type.end());
  stock_locate.erase(    stock_locate.begin() + index,     stock_locate.end());
  tracking_number.erase( tracking_number.begin() + index,  tracking_number.end());
  timestamp.erase(       timestamp.begin() + index,        timestamp.end());
  
  if (type == "system_events") {
    
    event_code.erase( event_code.begin() + index,  event_code.end());
    
  } else if (type == "stock_directory") {
    
    stock.erase(                stock.begin() + index,                 stock.end());
    market_category.erase(      market_category.begin() + index,       market_category.end());
    financial_status.erase(     financial_status.begin() + index,      financial_status.end());
    lot_size.erase(             lot_size.begin() + index,              lot_size.end());
    round_lots_only.erase(      round_lots_only.begin() + index,       round_lots_only.end());
    issue_classification.erase( issue_classification.begin() + index,  issue_classification.end());
    issue_subtype.erase(        issue_subtype.begin() + index,         issue_subtype.end());
    authentic.erase(            authentic.begin() + index,             authentic.end());
    short_sell_closeout.erase(  short_sell_closeout.begin() + index,   short_sell_closeout.end());
    ipo_flag.erase(             ipo_flag.begin() + index,              ipo_flag.end());
    luld_price_tier.erase(      luld_price_tier.begin() + index,       luld_price_tier.end());
    etp_flag.erase(             etp_flag.begin() + index,              etp_flag.end());
    etp_leverage.erase(         etp_leverage.begin() + index,          etp_leverage.end());
    inverse.erase(              inverse.begin() + index,               inverse.end());
    
  } else if (type == "trading_status") {
    
    stock.erase(            stock.begin() + index,             stock.end());
    trading_state.erase(    trading_state.begin() + index,     trading_state.end());
    reserved.erase(         reserved.begin() + index,          reserved.end());
    reason.erase(           reason.begin() + index,            reason.end());
    market_code.erase(      market_code.begin() + index,       market_code.end());
    operation_halted.erase( operation_halted.begin() + index,  operation_halted.end());
    
  } else if (type == "reg_sho") {
    
    stock.erase(         stock.begin() + index,          stock.end());
    regsho_action.erase( regsho_action.begin() + index,  regsho_action.end());
    
  } else if (type == "market_participant_states") {
    
    mpid.erase(              mpid.begin() + index,               mpid.end());
    stock.erase(             stock.begin() + index,              stock.end());
    primary_mm.erase(        primary_mm.begin() + index,         primary_mm.end());
    mm_mode.erase(           mm_mode.begin() + index,            mm_mode.end());
    participant_state.erase( participant_state.begin() + index,  participant_state.end());
    
  } else if (type == "mwcb") {
    
    level1.erase(         level1.begin() + index,          level1.end());
    level2.erase(         level2.begin() + index,          level2.end());
    level3.erase(         level3.begin() + index,          level3.end());
    breached_level.erase( breached_level.begin() + index,  breached_level.end());
    
  } else if (type == "ipo") {
    
    stock.erase(             stock.begin() + index,              stock.end());
    release_time.erase(      release_time.begin() + index,       release_time.end());
    release_qualifier.erase( release_qualifier.begin() + index,  release_qualifier.end());
    ipo_price.erase(         ipo_price.begin() + index,          ipo_price.end());
    
  } else if (type == "luld") {
    
    stock.erase(           stock.begin() + index,            stock.end());
    reference_price.erase( reference_price.begin() + index,  reference_price.end());
    upper_price.erase(     upper_price.begin() + index,      upper_price.end());
    lower_price.erase(     lower_price.begin() + index,      lower_price.end());
    extension.erase(       extension.begin() + index,        extension.end());
    
  } else if (type == "orders") {
    
    order_ref.erase( order_ref.begin() + index,  order_ref.end());
    buy.erase(       buy.begin() + index,        buy.end());
    shares.erase(    shares.begin() + index,     shares.end());
    stock.erase(     stock.begin() + index,      stock.end());
    price.erase(     price.begin() + index,      price.end());
    mpid.erase(      mpid.begin() + index,       mpid.end());
    
  } else if (type == "modifications") {
    
    order_ref.erase(     order_ref.begin() + index,      order_ref.end());
    shares.erase(        shares.begin() + index,         shares.end());
    match_number.erase(  match_number.begin() + index,   match_number.end());
    printable.erase(     printable.begin() + index,      printable.end());
    price.erase(         price.begin() + index,          price.end());
    new_order_ref.erase( new_order_ref.begin() + index,  new_order_ref.end());
    
  } else if (type == "trades") {
    
    order_ref.erase(    order_ref.begin() + index,     order_ref.end());
    buy.erase(          buy.begin() + index,           buy.end());
    shares.erase(       shares.begin() + index,        shares.end());
    stock.erase(        stock.begin() + index,         stock.end());
    price.erase(        price.begin() + index,         price.end());
    match_number.erase( match_number.begin() + index,  match_number.end());
    cross_type.erase(   cross_type.begin() + index,    cross_type.end());
    
  } else if (type == "noii") {
    
    paired_shares.erase(       paired_shares.begin() + index,        paired_shares.end());
    imbalance_shares.erase(    imbalance_shares.begin() + index,     imbalance_shares.end());
    imbalance_direction.erase( imbalance_direction.begin() + index,  imbalance_direction.end());
    stock.erase(               stock.begin() + index,                stock.end());
    far_price.erase(           far_price.begin() + index,            far_price.end());
    near_price.erase(          near_price.begin() + index,           near_price.end());
    reference_price.erase(     reference_price.begin() + index,      reference_price.end());
    cross_type.erase(          cross_type.begin() + index,           cross_type.end());
    variation_indicator.erase( variation_indicator.begin() + index,  variation_indicator.end());
    
  } else if (type == "rpii") {
    
    stock.erase(         stock.begin() + index,          stock.end());
    interest_flag.erase( interest_flag.begin() + index,  interest_flag.end());
    
  }
}

// Parses a message if the object is active and the message type belongs to this
// class! 
void MessageParser::parse_message(char * buf) {

  if (!active) return; 
  
  // -> if !any(msg_types == buf[2]) return...
  bool cont = false;
  for (char type : msg_types) if (type == buf[0]) cont = true;
  if (!cont) return;
  msg_buf_idx++;
  
  // check indices
  if (msg_buf_idx < start_count) return;
  if (msg_buf_idx > end_count) {
    // stop parsing future messages
    active = false;
    return;
  }
  
  // for all parse:
  // msg_type, stock_locate, tracking_number and timestamp
  msg_type[index]        = std::string(1, buf[0]);
  stock_locate[index]    = get2bytes(&buf[1]);
  tracking_number[index] = get2bytes(&buf[3]);
  int64_t ts = get6bytes(&buf[5]);
  std::memcpy(&(timestamp[index]), &ts, sizeof(double));
  
  // parse specific values for each message
  if (type == "system_events") {
    event_code[index] = std::string(1, buf[11]);
  } else if (type == "stock_directory") {
    
    stock[index]                = getNBytes(&buf[11], 8);
    market_category[index]      = std::string(1, buf[19]);
    financial_status[index]     = std::string(1, buf[20]);
    lot_size[index]             = get4bytes(&buf[21]);
    round_lots_only[index]      = buf[25] == 'Y';
    issue_classification[index] = std::string(1, buf[26]);
    issue_subtype[index]        = getNBytes(&buf[27], 2);
    authentic[index]            = buf[29] == 'P'; // P is live/production, T is Test
    short_sell_closeout[index]  = buf[30] == 'Y' ? true : buf[30] == 'N' ? false : NA_LOGICAL;
    ipo_flag[index]             = buf[31] == 'Y' ? true : buf[31] == 'N' ? false : NA_LOGICAL;
    luld_price_tier[index]      = std::string(1, buf[32]);
    etp_flag[index]             = buf[33] == 'Y' ? true : buf[33] == 'N' ? false : NA_LOGICAL;
    etp_leverage[index]         = get4bytes(&buf[34]);
    inverse[index]              = buf[38] == 'Y';
    
  } else if (type == "trading_status") {
    
    stock[index] = getNBytes(&buf[11], 8);
    
    if (buf[0] == 'H') {
      trading_state[index]    = std::string(1, buf[19]);
      reserved[index]         = std::string(1, buf[20]);
      reason[index]           = getNBytes(&buf[21], 4);
      // fill NAs from h
      market_code[index]      = NA_STRING;
      operation_halted[index] = NA_LOGICAL;
    } else { // buf[0] == 'h'
      market_code[index]      = std::string(1, buf[19]);
      operation_halted[index] = buf[20] == 'H';
      // fill NAs from H
      trading_state[index]    = NA_STRING;
      reserved[index]         = NA_STRING;
      reason[index]           = NA_STRING;
    }
    
  } else if (type == "reg_sho") {
    
    stock[index] = getNBytes(&buf[11], 8);
    regsho_action[index] = std::string(1, buf[19]);
    
  } else if (type == "market_participant_states") {
    
    mpid[index]              = getNBytes(&buf[11], 4);
    stock[index]             = getNBytes(&buf[15], 8);
    primary_mm[index]        = buf[23] == 'Y';
    mm_mode[index]           = std::string(1, buf[24]);
    participant_state[index] = std::string(1, buf[25]);
    
  } else if (type == "mwcb") {
    
    if (buf[0] == 'V') {
      level1[index]         = ((double) get8bytes(&buf[11])) / 100000000.0;
      level2[index]         = ((double) get8bytes(&buf[19])) / 100000000.0;
      level3[index]         = ((double) get8bytes(&buf[27])) / 100000000.0;
      breached_level[index] = NA_INTEGER;
    } else { // buf[0] == 'W'
      breached_level[index] = buf[11] - '0';
      level1[index]         = NA_REAL;
      level1[index]         = NA_REAL;
      level1[index]         = NA_REAL;
    }
    
  } else if (type == "ipo") {
    
    stock[index]             = getNBytes(&buf[11], 8);
    release_time[index]      = get4bytes(&buf[19]);
    release_qualifier[index] = std::string(1, buf[23]);
    ipo_price[index]         = ((double) get4bytes(&buf[24])) / 10000.0;
    
  } else if (type == "luld") {
    
    stock[index]           = getNBytes(&buf[11], 8);
    reference_price[index] = ((double) get4bytes(&buf[19])) / 10000.0;
    upper_price[index]     = ((double) get4bytes(&buf[23])) / 10000.0;
    lower_price[index]     = ((double) get4bytes(&buf[27])) / 10000.0;
    extension[index]       = get4bytes(&buf[31]);
    
  } else if (type == "orders") {
    
    const int64_t tmp = get8bytes(&buf[11]);
    std::memcpy(&(order_ref[index]), &tmp, sizeof(double));
    
    buy[index]    = buf[19] == 'B';
    shares[index] = get4bytes(&buf[20]);
    stock[index]  = getNBytes(&buf[24], 8);
    price[index]  = ((double) get4bytes(&buf[32])) / 10000.0;
    
    if (buf[0] == 'F') {
      mpid[index] = getNBytes(&buf[36], 4);
    } else {
      mpid[index] = "";
    }
    
  } else if (type == "modifications") {
    
    const int64_t tmp = get8bytes(&buf[11]);
    std::memcpy(&(order_ref[index]), &tmp, sizeof(double));
    
    if (buf[0] == 'E') {
      shares[index]       = get4bytes(&buf[19]);// executed shares
      
      const int64_t tt = get8bytes(&buf[23]);
      std::memcpy(&(match_number[index]), &tt, sizeof(double));
      
      // empty assigns
      printable[index]    = NA_LOGICAL;
      price[index]        = NA_REAL;
      std::memcpy(&(new_order_ref[index]), &NA_INT64, sizeof(double));
      
    } else if (buf[0] == 'C') {
      shares[index]       = get4bytes(&buf[19]);// executed shares
      
      const int64_t tt = get8bytes(&buf[23]);
      std::memcpy(&(match_number[index]), &tt, sizeof(double));
      
      printable[index]    = buf[31] == 'P';
      price[index]        = ((double) get4bytes(&buf[32])) / 10000.0;
      // empty assigns
      std::memcpy(&(new_order_ref[index]), &NA_INT64, sizeof(double));
      
    } else if (buf[0] == 'X') {
      shares[index] = get4bytes(&buf[19]); // canceled shares
      // empty assigns
      std::memcpy(&(match_number[index]), &NA_INT64, sizeof(double));
      printable[index] = NA_LOGICAL;
      price[index]     = NA_REAL;
      std::memcpy(&(new_order_ref[index]), &NA_INT64, sizeof(double));
      
    } else if (buf[0] == 'D') {
      shares[index]    = NA_INTEGER;
      std::memcpy(&(match_number[index]), &NA_INT64, sizeof(double));
      printable[index] = NA_LOGICAL;
      price[index]     = NA_REAL;
      std::memcpy(&(new_order_ref[index]), &NA_INT64, sizeof(double));
      
    } else if (buf[0] == 'U') {
      
      const int64_t tt = get8bytes(&buf[19]);
      std::memcpy(&(order_ref[index]), &tt, sizeof(double));
      
      shares[index] = get4bytes(&buf[27]);
      price[index]  = ((double) get4bytes(&buf[31])) / 10000.0;
      // empty assigns
      std::memcpy(&(match_number[index]), &NA_INT64, sizeof(double));
      printable[index] = NA_LOGICAL;
    }
    
  } else if (type == "trades") {
    
    int64_t tmp;
    
    if (buf[0] == 'P') {
      tmp = get8bytes(&buf[11]);
      std::memcpy(&(order_ref[index]), &tmp, sizeof(double));
      
      buy[index] = buf[19] == 'B';
      shares[index] = get4bytes(&buf[20]);
      
      stock[index]  = getNBytes(&buf[24], 8);
      price[index]  = ((double) get4bytes(&buf[32])) / 10000.0;
      
      const int64_t tt = get8bytes(&buf[36]);
      std::memcpy(&(match_number[index]), &tt, sizeof(double));
      
      // empty assigns
      cross_type[index] = NA_STRING;
    } else if (buf[0] == 'Q') {
      // only Q has 8 byte shares... otherwise 4 bytes for shares...
      tmp = get8bytes(&buf[11]);
      if (tmp >= INT32_MAX)
        Rcpp::Rcout <<
          "Warning, overflow for shares on message 'Q' at position " <<
            index << "\n";
      shares[index] = (int32_t) tmp;
      
      stock[index] = getNBytes(&buf[19], 8);
      price[index] = ((double) get4bytes(&buf[27])) / 10000.0;
      
      
      const int64_t tt = get8bytes(&buf[31]);
      std::memcpy(&(match_number[index]), &tt, sizeof(double));
      
      cross_type[index] = std::string(1, buf[39]);
      //empty assigns
      std::memcpy(&(order_ref[index]), &NA_INT64, sizeof(double));
      buy[index] = false; // NA_LOGICAL;
      // WARNING: Message Q: bool has no NA... default is TRUE
    } else if (buf[0] == 'B') {
      
      const int64_t tt = get8bytes(&buf[11]);
      std::memcpy(&(match_number[index]), &tt, sizeof(double));
      // empty assigns
      std::memcpy(&(order_ref[index]), &NA_INT64, sizeof(double));
      buy[index]        = NA_LOGICAL;
      shares[index]     = NA_INTEGER;
      stock[index]      = " ";
      price[index]      = NA_REAL;
      cross_type[index] = ' ';
    }
    
  } else if (type == "noii") {
    
    const int64_t tmp = get8bytes(&buf[11]);
    std::memcpy(&(paired_shares[index]), &tmp, sizeof(double));
    
    const int64_t tt = get8bytes(&buf[19]);
    std::memcpy(&(imbalance_shares[index]), &tt, sizeof(double));
    
    imbalance_direction[index] = std::string(1, buf[27]);
    stock[index]               = getNBytes(&buf[28], 8);
    far_price[index]           = ((double) get4bytes(&buf[36])) / 10000.0;
    near_price[index]          = ((double) get4bytes(&buf[40])) / 10000.0;
    reference_price[index]     = ((double) get4bytes(&buf[44])) / 10000.0;
    cross_type[index]          = std::string(1, buf[48]);
    variation_indicator[index] = std::string(1, buf[49]);
    
  } else if (type == "rpii") {
    
    stock[index]         = getNBytes(&buf[11], 8);
    interest_flag[index] = std::string(1, buf[19]);
    
  }
  
  index++;
}

// Converts the messages parsed to a data frame
Rcpp::List MessageParser::get_data_frame() {
  // if (index == 0) {
  //   Rcpp::List res(colnames.size());
  //   res.attr("names") = colnames;
  //   res.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");
  //   return res;
  // }
  
  // prune vector
  if (index != msg_type.size()) {
    // Rprintf("Pruning found index '%" PRId64 "' msg_type_size '%" PRId64 "'!\n",
    //         index, msg_type.size());
    prune_vectors();
  }
  // create a dataframe
  Rcpp::List res(colnames.size());
  res[0] = msg_type;
  res[1] = stock_locate;
  res[2] = tracking_number;
  res[3] = to_int64(timestamp);
  
  if (type == "system_events") {
    
    res[4] = event_code;
    
  } else if (type == "stock_directory") {
    
    res[4]  = stock;
    res[5]  = market_category;
    res[6]  = financial_status;
    res[7]  = lot_size;
    res[8]  = round_lots_only;
    res[9]  = issue_classification;
    res[10] = issue_subtype;
    res[11] = authentic;
    res[12] = short_sell_closeout;
    res[13] = ipo_flag;
    res[14] = luld_price_tier;
    res[15] = etp_flag;
    res[16] = etp_leverage;
    res[17] = inverse;
    
  } else if (type == "trading_status") {
    
    res[4]  = stock;
    res[5]  = trading_state;
    res[6]  = reserved;
    res[7]  = reason;
    res[8]  = market_code;
    res[9]  = operation_halted;
    
  } else if (type == "reg_sho") {
    
    res[4]  = stock;
    res[5]  = regsho_action;
    
  } else if (type == "market_participant_states") {
    
    res[4]  = mpid;
    res[5]  = stock;
    res[6]  = primary_mm;
    res[7]  = mm_mode;
    res[8]  = participant_state;
    
  } else if (type == "mwcb") {
    
    res[4] = level1;
    res[5] = level2;
    res[6] = level3;
    res[7] = breached_level;
    
  } else if (type == "ipo") {
    
    res[4] = stock;
    res[5] = release_time;
    res[6] = release_qualifier;
    res[7] = ipo_price;
    
  } else if (type == "luld") {
    
    res[4] = stock;
    res[5] = reference_price;
    res[6] = lower_price;
    res[7] = upper_price;
    res[8] = extension;
    
  } else if (type == "orders") {
    
    res[4] = to_int64(order_ref);
    res[5] = buy;
    res[6] = shares;
    res[7] = stock;
    res[8] = price;
    res[9] = mpid;
    
  } else if (type == "modifications") {
    
    res[4] = to_int64(order_ref);
    res[5] = shares;
    res[6] = to_int64(match_number);
    res[7] = printable;
    res[8] = price;
    res[9] = to_int64(new_order_ref);
    
  } else if (type == "trades") {
    
    res[4]  = to_int64(order_ref);
    res[5]  = buy;
    res[6]  = shares;
    res[7]  = stock;
    res[8]  = price;
    res[9]  = to_int64(match_number);
    res[10] = cross_type;
    
  } else if (type == "noii") {
    
    res[4]  = to_int64(paired_shares);
    res[5]  = to_int64(imbalance_shares);
    res[6]  = imbalance_direction;
    res[7]  = stock;
    res[8]  = far_price;
    res[9]  = near_price;
    res[10] = reference_price;
    res[11] = cross_type;
    res[12] = variation_indicator;
    
  } else if (type == "rpii") {
    
    res[4] = stock;
    res[5] = interest_flag;
    
  }
  
  // need to call data.table::setalloccol() on data in R!
  res.names() = colnames;
  res.attr("class") = Rcpp::StringVector::create("data.table", "data.frame");

  return res;
}
