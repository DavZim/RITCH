#' Downloads the stock directory (stock locate codes) for a given date and exchange
#'
#' The data is downloaded from NASDAQs server, which can be found here
#' <https://emi.nasdaq.com/ITCH/Stock_Locate_Codes/>
#'
#' @param exchange The exchange, either NASDAQ (equivalent to NDQ), BX, or PSX
#' @param date The date, should be of class Date. If not the value is converted
#' using `as.Date`.
#' @param cache If the stock directory should be cached, can be set to TRUE
#' to save the stock directories in the working directory or a character for a
#' target directory.
#' @param quiet If the download function should be quiet, default is FALSE.
#'
#' @return a data.table of the tickers, the respective stock locate codes, and
#' the exchange/date information
#' @export
#'
#' @examples
#' \dontrun{
#'   download_stock_directory("BX", "2019-07-02")
#'   download_stock_directory(c("BX", "NDQ"), c("2019-07-02", "2019-07-03"))
#'   download_stock_directory("BX", "2019-07-02", cache = TRUE)
#'
#'   download_stock_directory("BX", "2019-07-02", cache = "stock_directory")
#'   dir.exists("stock_directory")
#'   list.files("stock_directory")
#' }
download_stock_directory <- function(exchange, date, cache = FALSE,
                                     quiet = FALSE) {

  exchange <- ifelse(tolower(exchange) == "nasdaq", "ndq", tolower(exchange))
  if (!all(exchange %in% c("ndq", "bx", "psx")))
    stop("Exchange must be 'NASDAQ' ('NDQ'), 'BX', or 'PSX'")
  if (length(cache) != 1) stop("cache must be of size 1")

  if (is.character(date)) date <- as.Date(date)
  base_url <- "https://emi.nasdaq.com/ITCH/Stock_Locate_Codes/"

  # if multiple exchanges or dates were specified, take all possible combinations
  # and call the function recursively
  if (length(exchange) > 1 || length(date) > 1) {
    vals <- expand.grid(ex = exchange, d = date, stringsAsFactors = FALSE)

    res <- lapply(1:nrow(vals),
                  function(i) download_stock_directory(vals$ex[i], vals$d[i]))

    d <- data.table::rbindlist(res)

  } else {
    filename <- paste0(exchange, "_stocklocate_", format(date, "%Y%m%d"), ".txt")
    url <- paste0(base_url, filename)
    file <- url

    if (is.character(cache) || is.logical(cache) && cache) {

      destfile <- filename
      if (is.character(cache)) {
        if (!dir.exists(cache)) dir.create(cache)
        destfile <- file.path(cache, filename)
      }

      txt <- sprintf("for exchange '%s' and date '%s'",
                     exchange, format(date, "%Y-%m-%d"))
      # download or use cache
      if (!file.exists(destfile)) {
        if (!quiet) cat(sprintf("[Stock Locate] Downloading %s\n", txt))
        download.file(url, destfile, quiet = quiet)
      } else {
        if (!quiet)
          cat(sprintf("[Stock Locate] File %s already exists, using cache\n",
                      txt))
      }
      file <- destfile
    }

    d <- data.table::fread(file, showProgress = !quiet)

    data.table::setnames(d, c("ticker", "stock_locate"))
    d[, ':=' (exchange = toupper(exchange), date = date)]
  }

  return(d[])
}

