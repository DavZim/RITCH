
## a small price we pay for using data.table NSE with unquoted variables
utils::globalVariables(
  c("count", "datetime", "msg_type", "timestamp", "exchange", "file_size",
    "last_modified", ".", "size", "time", "stock", "stock_locate")
)
