#' Downloads the stock directory (stock locate codes) for a given date and exchange
#' 
#' The data is downloaded from NASDAQs FTP server, which can be found here 
#' \url{ftp://emi.nasdaq.com/ITCH/Stock_Locate_Codes/}
#' 
#' @param exchange The exchange, either NASDAQ (equivalent to NDQ), BX, or PSX
#' @param date The date, should be of class Date. If not the value is converted using \code{as.Date}.
#' @param quiet If the download function should be quiet, default is FALSE.
#'
#' @return a data.table of the tickers, the respective stock locate codes, and the exchange/date information
#' @export
#'
#' @examples
#' \dontrun{
#'   download_stock_directory("BX", "2019-07-02")
#'   download_stock_directory(c("BX", "NDQ"), c("2019-07-02", "2019-07-03"))
#' }
download_stock_directory <- function(exchange, date, quiet = FALSE) {
  
  exchange <- ifelse(tolower(exchange) == "nasdaq", "ndq", tolower(exchange))
  if (!all(exchange %in% c("ndq", "bx", "psx")))
    stop("Exchange must be 'NASDAQ' ('NDQ'), 'BX', or 'PSX'")
  
  if (is.character(date)) date <- as.Date(date)
  base_url <- "ftp://emi.nasdaq.com/ITCH/Stock_Locate_Codes/"
  
  # if multiple exchanges or dates were specified, take all possible combinations 
  # and call the function recursively
  if (length(exchange) > 1 || length(date) > 1) {
    vals <- expand.grid(ex = exchange, d = date, stringsAsFactors = FALSE)
    
    res <- lapply(1:nrow(vals), function(i) download_stock_directory(vals$ex[i], 
                                                                 vals$d[i]))
    d <- data.table::rbindlist(res)
    
  } else {
    url <- paste0(base_url, exchange, "_stocklocate_", format(date, "%Y%m%d"), ".txt")
    
    d <- data.table::fread(url, showProgress = !quiet)
    data.table::setnames(d, c("ticker", "stock_locate"))
    d[, ':=' (exchange = toupper(exchange), date = date)]
  }
  
  return(d[])
}

