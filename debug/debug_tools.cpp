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
#include "../src/specifications.h"
#include "../src/helper_functions.h"
#include "../src/read_functions.h"
#include "../src/write_functions.h"

// get_message_length(c("A", "B"))
// [[Rcpp::export]]
int dbg_get_message_length_impl(std::string m) {
  unsigned char msg = m[0];
  return get_message_size(msg);
}

/*** R
dbg_get_message_length <- function(x) {
  sapply(x, dbg_get_message_length_impl)
}
*/

// counts message types in a buffer
std::vector<int64_t> count_messages_buffer(unsigned char* buf,
                                           const uint64_t n_bytes) {
  std::vector<int64_t> count(N_TYPES, 0);
  uint64_t i = 0;
  while (i < n_bytes) {
    const unsigned char mt = buf[i + 2];

    count[mt - 'A']++;
    i += get_message_size(mt);
  }

  return take_needed_messages(count);
}
int64_t sum_messages(std::vector<int64_t>& count, unsigned char msg) {
  return count[msg - 'A'];
}

/*
 * Prints the bytes of each message of an ITCH file
 * Inputs are either
 *  - numeric which result in printing the next N values
 *  - a single character which corresponds to the message types and prints the next instance of the message
 */
// [[Rcpp::export]]
void dbg_itch_file(std::string filename = "inst/extdata/ex20101224.TEST_ITCH_50",
                   int64_t buffer_size = 1e9) {

  // to allow readline / user feedbakc
  Rcpp::Environment base = Rcpp::Environment("package:base");
  Rcpp::Function readline = base["readline"];
  Rcpp::Function as_character = base["as.character"];

  const bool is_gz = filename.substr(filename.size() - 3, filename.size()) == ".gz";

  // only one buffer is used...
  unsigned char* bufferPtr;
  int64_t bufferCharSize = sizeof(unsigned char) * buffer_size;
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

  std::vector<int64_t> counts_all = count_messages_buffer(bufferPtr, buf_size);
  std::vector<int64_t> counts = take_needed_messages(counts_all);

  Rprintf("Debugging File '%s' (.gz-file? %s)\n", filename.c_str(), is_gz ? "yes" : "no");
  Rprintf("Usage:\n");
  Rprintf("- Empty: next message\n");
  Rprintf("- Number: for next N messages\n");
  Rprintf("- Character: if valid message type, print the next message, e.g., 'A' for add order\n");
  Rprintf("- non valid Character: exits the debugging tool\n");
  Rprintf("Note: Bytes in parenthesis show the first two bytes, which are not used!\n");

  Rprintf("Number of Messages:\n");
  for (int j = 0; j < N_ACT_MSGS; j++) {
    Rprintf("- '%c': %ld\n", ACT_MSG_NAMES[j], counts[j]);
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
    const int l = get_message_size(num);
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

    Rprintf("'%c' (len 2 + %i) idx %4i at offset %5ld (0x%04lx) | ", num, l - 2, i, idx, idx);
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

          bool is_itch_message = false;
          for (const unsigned char c : ACT_MSG_NAMES) if (c == exit_msg) {
            is_itch_message = true;
            break;
          }

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
# dbg_hex_to_int("0a 2d f4 92 1d 67") == 11192493022567
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

// converts a std::string of hex values to a buffer
unsigned char * to_buffer(std::string x) {
  x.erase(remove_if(x.begin(), x.end(), isspace), x.end());
  const uint64_t n_bytes = x.size() / 2;
  unsigned char * buf;
  // Rprintf("Found %u bytes\n", x.size() / 2);
  buf = (unsigned char*) calloc(x.size() / 2, sizeof(unsigned char));

  for (uint64_t j = 0; j < n_bytes; j++)
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

  Rcpp::StringVector types;
  for (unsigned char c : ACT_MSG_NAMES) types.push_back(std::string(1, c));

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

Rcpp::DataFrame dbg_hex_to_df(std::string x, std::string msg_class) {
  // create buffer
  x.erase(remove_if(x.begin(), x.end(), isspace), x.end());
  const uint64_t n_bytes = x.size() / 2;
  unsigned char * buf = to_buffer(x);
  std::vector<int64_t> count = count_messages_buffer(buf, n_bytes);

  int64_t n_messages = 0;
  for (const int64_t p : count) n_messages += p;

  MessageParser mp(msg_class, 0, 100); // take max 100 messages...
  mp.activate();
  mp.init_vectors(n_messages + 100);
  uint64_t i = 2;

  while (i < n_bytes) {
    mp.parse_message(&buf[i]);
    i += get_message_size(buf[i]);
  }

  return mp.get_data_frame();
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_orders(std::string x) {
  return dbg_hex_to_df(x, "orders");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_trades(std::string x) {
  return dbg_hex_to_df(x, "trades");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_modifications(std::string x) {
  return dbg_hex_to_df(x, "modifications");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_system_events(std::string x) {
return dbg_hex_to_df(x, "system_events");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_stock_directory(std::string x) {
return dbg_hex_to_df(x, "stock_directory");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_trading_status(std::string x) {
  return dbg_hex_to_df(x, "trading_status");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_reg_sho(std::string x) {
  return dbg_hex_to_df(x, "reg_sho");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_market_participant_states(std::string x) {
  return dbg_hex_to_df(x, "market_participant_states");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_mwcb(std::string x) {
  return dbg_hex_to_df(x, "mwcb");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_ipo(std::string x) {
  return dbg_hex_to_df(x, "ipo");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_luld(std::string x) {
  return dbg_hex_to_df(x, "luld");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_noii(std::string x) {
  return dbg_hex_to_df(x, "noii");
}
//[[Rcpp::export]]
Rcpp::DataFrame dbg_hex_to_rpii(std::string x) {
  return dbg_hex_to_df(x, "rpii");
}


/*
 * ############################################################################
 * Messages to hex
 * The function takes one data.frame, deduces the type based on the message types
 * and converts it into binary (hex) data
 * ############################################################################
 */
//[[Rcpp::export]]
std::string dbg_messages_to_hex(Rcpp::DataFrame df,
                                size_t max_buffer_size = 1e8) {
  Rcpp::CharacterVector msgs = df["msg_type"];
  const int total_messages = msgs.length();
  // Rprintf("Found %i order messages\n", total_messages);
  unsigned char * buf;

  size_t req_size = 0;
  for (int i = 0; i < total_messages; i++) {
    const unsigned char msg = Rcpp::as<char>(msgs[i]);
    req_size += get_message_size(msg);
  }

  req_size = req_size > max_buffer_size ? max_buffer_size : req_size;
  // Rprintf("Need %u bytes for the messages\n", req_size);
  // allocate memory to the buffer and initialise it to 0
  buf = (unsigned char*) calloc(req_size, sizeof(unsigned char));

  int64_t i = 0;
  int64_t msg_ct = 0;
  while (msg_ct < total_messages) {
    // Rprintf("Parsing Message %i\n", msg_ct);
    i += load_message_to_buffer(&(buf[i]), msg_ct, df);
  }

  std::stringstream ss;
  for(int j = 0; j < i; ++j)
    ss <<
      std::setfill('0') <<
        std::setw(2) <<
          std::hex <<
           (int) (((int) buf[j] >> (8*0)) & 0xff) << // (int) buf[j]
              " ";
  std::string res = ss.str();

  return res.substr(0, res.size() - 1);
}
