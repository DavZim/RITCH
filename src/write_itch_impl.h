#include "RITCH.h"
#include <zlib.h>

// set functions, set X bytes in a buffer
uint64_t set2bytes(unsigned char* b, int32_t val);
uint64_t set4bytes(unsigned char* b, int32_t val);
uint64_t set6bytes(unsigned char* b, int64_t val);
uint64_t set8bytes(unsigned char* b, int64_t val);
uint64_t setCharBytes(unsigned char* b, std::string x, uint64_t n);

// parse specific messages into a buffer
uint64_t parse_orders_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_trades_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_modifications_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_system_events_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_stock_directory_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_trading_status_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_reg_sho_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_market_participants_states_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_mwcb_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_ipo_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_luld_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_noii_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);
uint64_t parse_rpii_at(unsigned char * buf, Rcpp::DataFrame df, uint64_t msg_num);

// loads a data.frame at a position into a buffer
int64_t load_message_to_buffer(unsigned char * buf, int64_t &msg_ct, Rcpp::DataFrame df);

// returns the index at which the values are minimum
int get_min_val_pos(std::vector<int64_t> &x);

// writes a buffer to file
void write_buffer(unsigned char* buf, int64_t size, std::string filename,
                  bool append = false, bool gz = false);

// Writes a list of data.frames (already sorted by timestamp)
// to a file, if specified, the file is a gz.file
int64_t write_buffer_to_file(Rcpp::List ll, std::string filename, 
                             bool append = false, bool gz = false,
                             size_t max_buffer_size = 1e9, bool quiet = false);
