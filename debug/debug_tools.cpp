/*
 * #######################################################
 * This file holds debug functions to look at and and write 
 * ITCH hex buffers
 * 
 * Functions include:
 * - dbg_get_message_length to get the length of a message
 * - dbg_itch_file to open an interactive mode in which the
 *   the file is shown as hex code, one message at a time, also
 *   includes modes to quickly see a certain message type
 * Hex related functions to convert hex codes into R types
 * - dbg_hex_to_char
 * - dbg_hex_to_int
 * - dbg_hex_to_dbl
 * - dbg_hex_count_messages to count the orders in a hex string
 * - dbg_hex_compare to compare two hex strings
 * 
 * Convert Hex Strings into data.tables and vice versa
 * - orders: dbg_hex_to_orders and dbg_messages_to_hex
 * - trades:
 * - modifications:
 * -
 * 
 * 
 * #######################################################
 */

// TODO: messages_to_bin: a function that takes in a list of dataframes
// for each list, have a running index (which messages have already been parsed in this element?)
// then find the next smallest timestamp, write one message, find next timestamp... until all are fully written
// write stream to file

#include <Rcpp.h>
#include <zlib.h>
#include "../src/RITCH.h"
#include "../src/MessageTypes.h"

// get_message_length(c("A", "B"))
// [[Rcpp::export]]
int dbg_get_message_length_impl(std::string m) {
  char msg = m[0];
  return getMessageLength(msg);
}

/*** R
dbg_get_message_length <- function(x) {
  sapply(x, dbg_get_message_length_impl)
}
*/

// counts message types in a buffer
std::vector<int64_t> count_messages_buffer(unsigned char* buf, const uint64_t n_bytes) {
  std::vector<int64_t> count(ITCH::TYPES.size(), 0); 
  uint64_t i = 2;
  while (i < n_bytes) {
    countMessageByType(count, buf[i]);
    i += getMessageLength(buf[i]) + 2;
  }
  return count;
}
int64_t sum_messages(std::vector<int64_t> count, char msg) {
  int64_t s = 0;
  switch(msg) {
    case 'S': 
      s += count[ITCH::POS::S];
      break;
    case 'R':
      s += count[ITCH::POS::R];
      break;
    case 'H':
      s += count[ITCH::POS::H];
      break;
    case 'Y':
      s += count[ITCH::POS::Y];
      break;
    case 'L':
      s += count[ITCH::POS::L];
      break;
    case 'V':
      s += count[ITCH::POS::V];
      break;
    case 'W':
      s += count[ITCH::POS::W];
      break;
    case 'K':
      s += count[ITCH::POS::K];
      break;
    case 'J':
      s += count[ITCH::POS::J];
      break;
    case 'h':
      s += count[ITCH::POS::h];
      break;
    case 'A':
      s += count[ITCH::POS::A];
      break;
    case 'F':
      s += count[ITCH::POS::F];
      break;
    case 'E':
      s += count[ITCH::POS::E];
      break;
    case 'C':
      s += count[ITCH::POS::C];
      break;
    case 'X':
      s += count[ITCH::POS::X];
      break;
    case 'D':
      s += count[ITCH::POS::D];
      break;
    case 'U':
      s += count[ITCH::POS::U];
      break;
    case 'P':
      s += count[ITCH::POS::P];
      break;
    case 'Q':
      s += count[ITCH::POS::Q];
      break;
    case 'B':
      s += count[ITCH::POS::B];
      break;
    case 'I':
      s += count[ITCH::POS::I];
      break;
    case 'N':
      s += count[ITCH::POS::N];
      break;
  }
  return s;
}

/*
 * Prints the bytes of each message of an ITCH file
 * Inputs are either 
 *  - numeric which result in printing the next N values
 *  - a single character which corresponds to the message types and prints the next instance of the message 
 */
// [[Rcpp::export]]
void dbg_itch_file(std::string filename = "20191230.BX_ITCH_50",
                   int64_t buffer_size = 1e9) {
  
  // to allow readline / user feedbakc
  Rcpp::Environment base = Rcpp::Environment("package:base");
  Rcpp::Function readline = base["readline"];
  Rcpp::Function as_character = base["as.character"];
  
  const bool is_gz = filename.substr(filename.size() - 3, filename.size()) == ".gz";
  
  // only one buffer is used...
  unsigned char* bufferPtr;
  int64_t bufferCharSize = sizeof(char) * buffer_size;
  bufferPtr = (unsigned char*) malloc(bufferCharSize);
  
  FILE* rawfile;
  gzFile gzfile;
  
  if (is_gz) {
    gzfile = gzopen(filename.c_str(), "rb");
  } else {
    rawfile = fopen(filename.c_str(), "rb");
  }
  
  int64_t buf_size;
  if (is_gz) {
    buf_size = gzread(gzfile, bufferPtr, bufferCharSize);
  } else {
    buf_size = fread(bufferPtr, 1, bufferCharSize, rawfile);
  }
  
  std::vector<int64_t> counts = count_messages_buffer(bufferPtr, buf_size);
  Rprintf("Debugging File '%s' (.gz-file? %s)\n", filename.c_str(), is_gz ? "yes" : "no");
  Rprintf("Usage:\n");
  Rprintf("- Empty: next message\n");
  Rprintf("- Number: for next N messages\n");
  Rprintf("- Character: if valid message type, print the next message, e.g., 'A' for add order\n");
  Rprintf("- non valid Character: exits the debugging tool\n");
  Rprintf("Note: Bytes in parenthesis show the first two bytes, which are not used!\n");

  Rprintf("Number of Messages:\n");
  for (int j = 0; j < ITCH::TYPES.size(); j++) {
    Rprintf("- '%c': %i\n", ITCH::TYPES[j], counts[j]);
  }
  Rprintf("=============================\n");
  // Use the Buffer
  int64_t idx;
  
  int i = 0;
  idx = 0;
  std::string exit_code = "";
  int skip_end = 0;
  bool skip_print = false;
  unsigned char msg_filter = ' ';

  // to enable multiple buffers: use this logic...
  // while ((thisBufferSize = fread(bufferPtr, 1, bufferCharSize, infile)) > 0) {
  //  while (true) {
  while (true) {
    if (idx > buf_size) {
      Rprintf("Reached end of buffer, increase buffer size to read more\n");
      return;
  }
    unsigned char num = bufferPtr[idx + 2];
    const int l = getMessageLength(num) + 2;
    // Rprintf("At offset '0x%04x' msg '%c' msg len %i (0x%04x)\n", idx, num, l, l);
    
    if (skip_print) {
      if (num != msg_filter) {
        // if the current message is not equal to the message filter, skip printing and advance
        idx += l;
        i++;
        continue;
      } else {
        skip_print = false;
      }
    }
    
    Rprintf("'%c' (len 2 + %i) idx %4i at offset %5i (0x%04x) | ", num, l - 2, i, idx, idx);
    Rprintf("(%02x %02x) ", bufferPtr[idx], bufferPtr[idx + 1]);
    for (int x = 2; x < l; x++) Rprintf("%02x ", bufferPtr[idx + x]);
    Rprintf("\n");
    
    // interactive element, allow numeric input (for N messages), 
    // Message Types for the next message type, or other non empty for quit
    if (i >= skip_end) {
      exit_code = Rcpp::as<std::string>(as_character(readline("#RITCH> ")));
      
      if (exit_code != "") {
        // check if all numeric, than skip N
        const bool only_numeric = exit_code.find_first_not_of("0123456789") == std::string::npos;
        if (only_numeric) {
          const int n = std::stoi(exit_code);
          skip_end = i + n;
          Rprintf("Showing next %i messages\n", n);
        } else {
          // check messages
          unsigned char exit_msg = exit_code.at(0);
          
          // check if the input is an itch message
          const bool is_itch_message = std::find(ITCH::TYPES.begin(), ITCH::TYPES.end(), exit_code[0]) != ITCH::TYPES.end();
          if (is_itch_message) {
            const bool has_message = sum_messages(counts, exit_msg) > 0;
            if (!has_message) {
              Rprintf("No messages found for type '%c' increase buffer size or use different message type.\n", exit_msg);
              continue;
            }
            skip_print = true;
            msg_filter = exit_code[0];
            
            Rcpp::Rcout << "Applied filter to message type '" << msg_filter << "'\n";
          } else {
            // else break
            Rprintf("Stopping Printing Messages\n");
            break;
          }
        }
      } // else: continue with next message
    }
    
    idx += l;
    i++;
  }
  
  free(bufferPtr);
  if (is_gz) {
    gzclose(gzfile); 
  } else {
    fclose(rawfile);
  }
}

/*** R
# Converts a hex string into char
# i.e., dbg_hex_to_char("4f") == "O"
dbg_hex_to_char <- function(h) {
  h <- gsub(" +", "", h)
  xx <- sapply(seq(1, nchar(h), by=2), function(x) substr(h, x, x+1))
  rawToChar(as.raw(strtoi(xx, 16L)))
}
# dbg_hex_to_int("01 23 45 67") == 19088743
# dbg_hex_to_int("0a 2d f4 92 1d 67")
dbg_hex_to_int <- function(h) {
  h <- gsub(" +", "", h)
  l <- nchar(h) %/% 2
  bit64::as.integer64(as.numeric(paste0("0x", h)))
}
# dbg_hex_to_dbl("00 01 fa 40") == 12.96
# dbg_hex_to_dbl("00 00 00 46 28 21 94 40", prec = 8) == 3013.21
dbg_hex_to_dbl <- function(h, prec = 4) {
  dbg_hex_to_int(h) / 10^prec
}
*/


// ############################################
/// HELPER FUNCTIONS
// ############################################

// sets inside a char buffer b, 2 bytes from the value val, returns number of bytes changed
// i.e., convert val = 8236 to 0x202c
uint64_t set2bytes(unsigned char* b, int32_t val) {
  b[1] = val         & 0xff;
  b[0] = (val >> 8)  & 0xff;
  // Rprintf("Converting: %15i -> 0x %02x %02x\n",
  //         val, b[0], b[1]);
  return 2;
}

// sets inside a char buffer b, 4 bytes from the value val, returns number of bytes changed
// i.e., convert val = 11900 to 0x00002e7c
uint64_t set4bytes(unsigned char* b, int32_t val) {
  b[3] = val         & 0xffff;
  b[2] = (val >> 8)  & 0xffff;
  b[1] = (val >> 16) & 0xffff;
  b[0] = (val >> 24) & 0xffff;
  // Rprintf("Converting: %15i -> 0x %02x %02x %02x %02x\n",
  //         val, b[0], b[1], b[2], b[3]);
  return 4;
}
// sets inside a char buffer b, 6 bytes from the value val, returns number of bytes changed
// i.e., 25200002107428 to 0x16eb552c8824
uint64_t set6bytes(unsigned char* b, int64_t val) {
  b[5] = val         & 0xffffff;
  b[4] = (val >> 8)  & 0xffffff;
  b[3] = (val >> 16) & 0xffffff;
  b[2] = (val >> 24) & 0xffffff;
  b[1] = (val >> 32) & 0xffffff;
  b[0] = (val >> 40) & 0xffffff;
  // Rprintf("Converting: %15lld -> 0x %02x %02x %02x %02x %02x %02x\n",
  //         (long long) val, b[0], b[1], b[2], b[3], b[4], b[5]);
  return 6;
}
// sets inside a char buffer b, 8 bytes from the value val, returns number of bytes changed
// i.e., 4 to 0x0000000000000004
uint64_t set8bytes(unsigned char* b, int64_t val) {
  b[7] = val         & 0xffffffff;
  b[6] = (val >> 8)  & 0xffffffff;
  b[5] = (val >> 16) & 0xffffffff;
  b[4] = (val >> 24) & 0xffffffff;
  b[3] = (val >> 32) & 0xffffffff;
  b[2] = (val >> 40) & 0xffffffff;
  b[1] = (val >> 48) & 0xffffffff;
  b[0] = (val >> 56) & 0xffffffff;
  // Rprintf("Converting: %15lld -> 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  //         (long long) val, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
  return 8;
}
// sets inside a char buffer b, n bytes from the string x, returns number of bytes changed
// i.e., "UFO" with 8 to 0x55534f2020202020 (filled with whitespaces)
uint64_t setCharBytes(unsigned char* b, std::string x, uint64_t n) {
  unsigned char st[n + 1];
  if (x.size() > n) Rprintf("ERROR: setChar Bytes for string '%s' larger than capacity %i\n", x.c_str(), n);
  for (uint64_t j = 0; j < n; j++) st[j] = ' '; // fill with n spaces
  for (uint64_t j = 0; j < x.size(); j++) st[j] = x[j]; // copy the string x
  memcpy(b, st, n);
  // Rprintf("Set %i char Bytes from '%s' -> 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  //         n, x.c_str(), b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
  return n;
}

// converts a std::string of hex values to a buffer
unsigned char * to_buffer(std::string x) {
  x.erase(remove_if(x.begin(), x.end(), isspace), x.end());
  const uint64_t n_bytes = x.size() / 2; 
  unsigned char * buf;
  // Rprintf("Found %u bytes\n", x.size() / 2);
  buf = (unsigned char*) calloc(x.size() / 2, sizeof(unsigned char));
  
  for (int j = 0; j < n_bytes; j++) 
    buf[j] = std::stoul(x.substr(j * 2, 2), nullptr, 16);
  return buf;
}


// ##############################
// User Functions...
// ##############################

//[[Rcpp::export]]
Rcpp::DataFrame hex_count_messages_impl(std::string x) {
  // remove whitespaces
  x.erase(remove_if(x.begin(), x.end(), isspace), x.end());
  const uint64_t n_bytes = x.size() / 2; 
  unsigned char * buf = to_buffer(x);
  
  std::vector<int64_t> count = count_messages_buffer(buf, n_bytes);
  
  Rcpp::StringVector types(ITCH::TYPES.size());
  types = ITCH::TYPESSTRING;
  Rcpp::List df(2);
  df.names() = Rcpp::CharacterVector::create("msg_type", "count");
  df["msg_type"] = types;
  const int len = types.size();
  Rcpp::NumericVector ct(len);
  std::memcpy(&(ct[0]), &(count[0]), len * sizeof(double));
  ct.attr("class") = "integer64";
  df["count"] = ct;
  
  df.attr("class") = Rcpp::CharacterVector::create("data.table", "data.frame");
  
  return df;
}
/***R
dbg_hex_compare <- function(x, y) {
  reset_whitespaces <- function(x) {
    xx <- strsplit(gsub(" ", "", x), split = "")[[1]]
    paste(paste0(xx[c(T, F)], xx[c(F, T)]), collapse = " ")
  }
  x <- reset_whitespaces(x)
  y <- reset_whitespaces(y)
  xx <- strsplit(x, " ")[[1]]
  yy <- strsplit(y, " ")[[1]]
  
  min_x <- min(length(xx), length(yy))
  cat(sprintf(" %3s | %4s | %4s | %4s\n%s\n", "idx", "x", "y", "diff",
              paste(rep("-", 25), collapse = "")))
  for (i in seq_len(min_x)) {
    cat(sprintf(" %3s | 0x%2s | 0x%2s | %4s\n", i, xx[i], yy[i],
                ifelse(xx[i] == yy[i], "", "XXX")))
  }
}
# count orders for a hex string
# dbg_hex_count_messages("00 00 41")
dbg_hex_count_messages <- function(x) {
  d <- hex_count_messages_impl(x)
  data.table::setalloccol(d)
}
*/

/* 
 * HEX to Ordertypes
 */
// loads a hex string to a buffer
Rcpp::DataFrame hex_to_message_type(std::string x, MessageType& msg_class, std::vector<int> pos) {
  // remove whitespaces
  x.erase(remove_if(x.begin(), x.end(), isspace), x.end());
  const uint64_t n_bytes = x.size() / 2; 
  unsigned char * buf = to_buffer(x);
  
  std::vector<int64_t> count = count_messages_buffer(buf, n_bytes);
  int n_messages = 0;
  for (const int p : pos) n_messages += count[p];
  uint64_t offset = 2;
  msg_class.reserve(n_messages);
  for (int i = 0; i < n_messages; i++) {
    msg_class.loadMessage(&buf[offset]);
    offset += getMessageLength(buf[offset]) + 2;
  }
  free(buf);
  return msg_class.getDF();
}

//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_orders(std::string x) {
  Orders od;
  return hex_to_message_type(x, od, {ITCH::POS::A, ITCH::POS::F});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_trades(std::string x) {
  Trades tr;
  return hex_to_message_type(x, tr, {ITCH::POS::P, ITCH::POS::Q, ITCH::POS::B});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_modifications(std::string x) {
  Modifications md;
  return hex_to_message_type(x, md, {ITCH::POS::E, ITCH::POS::C, ITCH::POS::X, ITCH::POS::D, ITCH::POS::U});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_system_events(std::string x) {
  SystemEvents se;
  return hex_to_message_type(x, se, {ITCH::POS::S});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_stock_directory(std::string x) {
  StockDirectory sd;
  return hex_to_message_type(x, sd, {ITCH::POS::R});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_trading_status(std::string x) {
  TradingStatus ts;
  return hex_to_message_type(x, ts, {ITCH::POS::H, ITCH::POS::h});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_reg_sho(std::string x) {
  RegSHO rs;
  return hex_to_message_type(x, rs, {ITCH::POS::Y});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_market_participant_states(std::string x) {
  ParticipantStates ps;
  return hex_to_message_type(x, ps, {ITCH::POS::L});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_mwcb(std::string x) {
  MWCB mwcb;
  return hex_to_message_type(x, mwcb, {ITCH::POS::V, ITCH::POS::W});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_ipo(std::string x) {
  IPO ipo;
  return hex_to_message_type(x, ipo, {ITCH::POS::K});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_luld(std::string x) {
  LULD luld;
  return hex_to_message_type(x, luld, {ITCH::POS::J});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_noii(std::string x) {
  NOII noii;
  return hex_to_message_type(x, noii, {ITCH::POS::I});
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_rpii(std::string x) {
  RPII rpii;
  return hex_to_message_type(x, rpii, {ITCH::POS::N});
}


/*
 * Parsing Messages at function
 * Parses the messages of data.frame df into the buffer as hex values
 * according to the ITCH rules. Note that only one message at number msg_num
 * is parsed
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
  const char msg = Rcpp::as<char>(msg_type[msg_num]);
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
int64_t load_message_to_buffer(unsigned char * buf, int64_t &msg_ct, Rcpp::DataFrame df) {
  int64_t i = 0;
  Rcpp::CharacterVector vec = df["msg_type"];
  const char msg = Rcpp::as<char>(vec[0]);
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

/*
 * ############################################################################
 * Messages to hex
 * The function takes one data.frame, deduces the type based on the message types
 * and converts it into binary (hex) data
 * ############################################################################
 */
//[[Rcpp::export]]
std::string dbg_messages_to_hex(Rcpp::DataFrame df) {
  Rcpp::CharacterVector msgs = df["msg_type"];
  const int total_messages = msgs.length();
  // Rprintf("Found %i order messages\n", total_messages);
  unsigned char * buf;
  
  int64_t req_size = 0;
  for (int i = 0; i < total_messages; i++) {
    const char msg = Rcpp::as<char>(msgs[i]);
    req_size += getMessageLength(msg) + 2;
    // two empty bytes at the start of each message..
  }
  // Rprintf("Need %u bytes for the messages\n", req_size);
  // allocate memory to the buffer and initialise it to 0
  buf = (unsigned char*) calloc(req_size, sizeof(char));
  
  int64_t i = 0;
  int64_t msg_ct = 0;
  while (msg_ct < total_messages) {
    // Rprintf("Parsing Message %i\n", msg_ct);
    i += load_message_to_buffer(&(buf[i]), msg_ct, df);
  }
  
  std::stringstream ss;
  for(int j = 0; j < i; ++j) 
    ss << std::setfill('0') << std::setw(2) << std::hex << (int) buf[j] << " ";
  std::string res = ss.str();
  
  return res.substr(0, res.size() - 1);
}

// returns the index at which the min value sits in the index
int get_min_val_pos(std::vector<int64_t> &x) {
  const auto min_el = std::min_element(x.begin(), x.end());
  return std::distance(x.begin(), min_el);  
}

// [[Rcpp::export]]
std::string dbg_write_itch_impl(Rcpp::List ll, std::string filename, bool gz = false) {
  
  const int list_length = ll.size();
  int64_t msg_length = 0, total_msgs = 0;

  const int64_t max_val = std::numeric_limits<int64_t>::max();
  std::vector<int64_t> indices (list_length);
  std::vector<int64_t> timestamps (list_length);
  
  // initiate the indices, timestamps as well as count the required buffer size
  for (int ii = 0; ii < list_length; ii++) {
    Rcpp::List df = ll.at(ii);
    Rcpp::CharacterVector mt = df["msg_type"];
    Rcpp::NumericVector ts = df["timestamp"];
    
    // populate the indices
    indices[ii] = 0;
    
    // populate the timestamps
    std::memcpy(&(timestamps[ii]), &(ts[0]), sizeof(int64_t));
    // Rprintf("ts[0] is %+" PRId64 "; ts is now +" PRId64 "\n", ts[0], timestamps[ii]);
    
    for (int l = 0; l < mt.size(); l++) {
      const char msg = Rcpp::as<char>(mt[l]);
      msg_length += getMessageLength(msg) + 2;
      total_msgs++;
    }
  }
  
  // Rprintf("Need a Buffer of size '%" PRId64 "' for '%" PRId64 "' messages\n", 
  //         msg_length, total_msgs);
  
  unsigned char * buf;
  buf = (unsigned char*) calloc(msg_length, sizeof(char));
  int64_t msg_ct = 0, i = 0;
  
  while (msg_ct < total_msgs) {
    // find at which list position (lp) the lowest timestamp is
    const int lp = get_min_val_pos(timestamps);
    Rcpp::List df = ll[lp];
    int64_t lp_idx = indices[lp];
    
    Rcpp::NumericVector ts = df["timestamp"];
    
    // load the message to the buffer
    i += load_message_to_buffer(&(buf[i]), lp_idx, df);
    
    // lp_idx was increased in load_message_to_buffer by one!
    if (lp_idx == ts.size()) {
      // take the max value if the end of this df is reached
      std::memcpy(&(timestamps[lp]), &max_val, sizeof(int64_t));
    } else {
      std::memcpy(&(timestamps[lp]), &(ts[lp_idx]), sizeof(int64_t));
    }
    // at position lp, the index is increased by one, next loop, take the next value
    indices[lp] += 1;
    msg_ct++;
  }
  
  // Rprintf("Write data to file '%s'\n", filename.c_str());
  if (!gz) {
    FILE* outfile;
    outfile = fopen(filename.c_str(), "wb");
    if (outfile == NULL) Rcpp::stop("File Error!\n");
    fwrite(&buf[0], 1, i, outfile);
    fclose(outfile);
  } else {
    filename += ".gz";
    gzFile gzfile = gzopen(filename.c_str(), "wb");
    if (gzfile == NULL) Rcpp::stop("File Error!\n");
    gzwrite(gzfile, &buf[0], i);
    gzclose(gzfile);
  }
  
  return filename;
}

/***R
dbg_write_itch <- function(ll, filename, gz = FALSE) {
  ll <- lapply(ll, data.table::setorder, timestamp)
  
  f <- dbg_write_itch_impl(ll, filename, gz)
  return(invisible(f))
}
*/