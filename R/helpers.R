#' Returns the date from an ITCH-filename
#'
#' @param file a filename
#'
#' @return the date as fastPOSIXct
#' @export
#' @keywords internal
#'
#' @examples
#' get_date_from_filename("03302017.NASDAQ_ITCH50")
#' get_date_from_filename("20170130.BX_ITCH_50.gz")
#' get_date_from_filename("S030220-v50-bx.txt.gz")
#' get_date_from_filename("unknown_file_format")
get_date_from_filename <- function(file) {
  date_ <- data.table::fifelse(
    grepl("S\\d{6}", file),
    sub(".*(\\d{6}).*", "\\1", file),
    sub(".*(\\d{8}).*", "\\1", file)
  )

  date_ <- data.table::fifelse(
    grepl("NASDAQ_ITCH50(\\.gz)?$", file),
    # format MMDDYYYY
    gsub("(\\d{2})(\\d{2})(\\d{4})", "\\3-\\1-\\2", date_),
    data.table::fifelse(grepl("S\\d{6}-", file),
                        # format MMDDYY
                        gsub("(\\d{2})(\\d{2})(\\d{2})", "20\\3-\\1-\\2", date_),
                        # format YYYYMMDD
                        gsub("(\\d{4})(\\d{2})(\\d{2})", "\\1-\\2-\\3", date_)
                        )
  )

  date_ <- try(as.POSIXct(date_, tz = "GMT"), silent = TRUE)
  if (inherits(date_, "try-error")) date_ <- NA
  return(date_)
}

#' Returns the exchange from an ITCH-filename
#'
#' @param file a filename
#'
#' @return The exchange
#' @export
#'
#' @examples
#' get_exchange_from_filename("03302017.NASDAQ_ITCH50")
#' get_exchange_from_filename("20170130.BX_ITCH_50.gz")
#' get_exchange_from_filename("S030220-v50-bx.txt.gz")
#' get_exchange_from_filename("Unknown_file_format")
get_exchange_from_filename <- function(file) {
  res <- regmatches(file, regexpr("(?<=\\.)[A-Z]+(?=_)", file, perl = TRUE))
  if (length(res) == 0)
    res <- regmatches(file, regexpr("(?<=-v50-)[a-z]+", file, perl = TRUE))
  res <- toupper(res)
  if (length(res) == 0) res <- NA
  return(res)
}

#' Adds meta information (date and exchange) to an itch filename
#'
#' Note that if date and exchange information are already present,
#' they are overwritten
#'
#' @param file the filename
#' @param date the date as a date-class or as a string that is understood by
#'   [base::as.Date()].
#' @param exchange the name of the exchange
#'
#' @return the filename with exchanged or added date and exchange information
#' @export
#'
#' @examples
#' add_meta_to_filename("03302017.NASDAQ_ITCH50", "2010-12-24", "TEST")
#' add_meta_to_filename("20170130.BX_ITCH_50.gz", "2010-12-24", "TEST")
#' add_meta_to_filename("S030220-v50-bx.txt.gz", "2010-12-24", "TEST")
#' add_meta_to_filename("unknown_file.ITCH_50", "2010-12-24", "TEST")
add_meta_to_filename <- function(file, date, exchange) {
  if (is.na(date) || is.na(exchange)) return(file)

  if (!"POSIXct" %in% class(date)) date <- as.Date(date)

  # First try to extract if the filename is in the standard formats.
  # if not use the "20101224.TEST_ITCH_50" format
  if (grepl("NASDAQ_ITCH", file)) { #03302017.NASDAQ_ITCH50

    file <- gsub("\\d{8}", format(date, "%m%d%Y"), file)
    file <- gsub("NASDAQ", exchange, file)

  } else if (grepl("S\\d{6}-", file)) { # S030220-v50-bx.txt.gz

    file <- gsub("\\d{6}", format(date, "%m%d%y"), file)
    file <- gsub("(?<=v50-)[^\\.]*(?=\\.)", exchange, file, perl = TRUE)

  } else if (grepl("(?<!NASDAQ)_ITCH", file, perl = TRUE)) { # 20170130.BX_ITCH_50.gz

    # replace the last 8 digits with the date
    file <- gsub("\\d{8}(?=[^0-9]+50.*)", format(date, "%Y%m%d"), file, perl = TRUE)
    file <- gsub("(?<=\\d{8}\\.)[^_]+", exchange, file, perl = TRUE)

  } else {

    # Unknown format... use 20101224.TEST_ITCH_50
    has_gz <- grepl("\\.gz$", file)
    if (has_gz) file <- gsub("\\.gz$", "", file)
    file <- gsub("\\.?_?ITCH_?50", "", file)

    file <- paste0(
      file,
      "_",
      format(date, "%Y%m%d"),
      ".",
      exchange,
      "_ITCH_50"
    )

    if (has_gz) file <- paste0(file, ".gz")
  }

  return(file)
}


#' Opens the ITCH Specification PDF
#'
#' The specifications can be found as a PDF <https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf>.
#'
#' @return the URL (invisible)
#' @export
#'
#' @examples
#' \dontrun{
#' open_itch_specification()
#' }
open_itch_specification <- function() {
  url <- "https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf"
  browseURL(url)
  return(invisible(url))
}

#' Opens the ITCH sample page
#'
#' The server can be found at <https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/>.
#'
#' @return the URL (invisible)
#' @export
#'
#' @examples
#' \dontrun{
#' open_itch_sample_server()
#' }
open_itch_sample_server <- function() {
  url <- "https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/"
  browseURL(url)
  return(invisible(url))
}

check_msg_types <- function(filter_msg_type, quiet) {
  # allow msg_classes: 'AF' (multiple values are split),
  # c('A', 'F'), c(NA, 'A') (NAs are ommited)
  filter_msg_type <- unique(filter_msg_type)

  if (any(nchar(filter_msg_type) > 1, na.rm = TRUE)) {
    x <- sapply(filter_msg_type, strsplit, split = "")
    filter_msg_type <- as.character(unlist(x))
  }

  filter_msg_type <- filter_msg_type[!is.na(filter_msg_type)]

  if (!quiet && length(filter_msg_type) > 0)
    cat(paste0("[Filter]     msg_type: '",
               paste(filter_msg_type, collapse = "', '"),
               "'\n"))

  return(filter_msg_type)
}

check_timestamps <- function(min_timestamp, max_timestamp, quiet) {
  min_timestamp <- min_timestamp[!is.na(min_timestamp)]
  max_timestamp <- max_timestamp[!is.na(max_timestamp)]

  lmin <- length(min_timestamp)
  lmax <- length(max_timestamp)

  txt <- "[Filter]     timestamp: "
  if (lmin != lmax) {
    # either vector has to have size 1 the other 0
    if ((lmin == 0 && lmax == 1) ||
        (lmin == 1 && lmax == 0)) {
      if (lmin == 0) {
        min_timestamp <- 0
        txt <- paste0(txt, "<= ", bit64::as.integer64(max_timestamp))
      } else { # lmax == 0
        max_timestamp <- -1
        txt <- paste0(txt, ">= ", bit64::as.integer64(min_timestamp))
      }
    } else {
      stop(paste("min_ and and max_timestamp have to have the same length",
                 "or only one has to have size 1!"))
    }
  } else { # lmin == lmax
    txt <- paste0(txt,
                  paste(bit64::as.integer64(min_timestamp),
                        bit64::as.integer64(max_timestamp),
                        sep = " - ", collapse = ", "))
  }
  if (length(min_timestamp) != 0 && !quiet) cat(txt, "\n")

  min_timestamp <- bit64::as.integer64(min_timestamp)
  max_timestamp <- bit64::as.integer64(max_timestamp)

  return(list(min = min_timestamp, max = max_timestamp))
}

check_stock_filters <- function(filter_stock, stock_directory,
                                filter_stock_locate, infile) {

  if (!(length(filter_stock) == 1 && is.na(filter_stock))) {
    if (length(stock_directory) == 1 && is.na(stock_directory)) {
      warning("filter_stock is given, but no stock_directory is specified. Trying to extract stock directory from file\n")
      stock_directory <- read_stock_directory(infile, quiet = TRUE)
    }

    if (!all(filter_stock %chin% stock_directory$stock)) {
      stop(paste0("Not all stocks found in stock_directory, missing: '",
                  paste(filter_stock[!filter_stock %chin% stock_directory$stock],
                        collapse = "', '"),
                  "'"))
    }
    # extend locate code by the stocks:
    filter_stock_locate <- c(filter_stock_locate,
                             stock_directory[stock %chin%filter_stock, stock_locate])
  }
  return(filter_stock_locate)
}

check_buffer_size <- function(buffer_size, file) {
  if (is.na(buffer_size) || buffer_size < 0)
    buffer_size <- ifelse(grepl("\\.gz$", file),
                          min(3 * file.size(file), 1e9),
                          1e8)

  if (!is.integer(buffer_size) || !is.numeric(buffer_size)) buffer_size <- 1e8

  if (buffer_size < 50)
    stop(paste("buffer_size has to be at least 50 bytes, otherwise the",
               "messages won't fit"))

  if (buffer_size > 5e9)
    warning(paste("You are trying to allocate a large array on the heap, if",
                  "the function crashes, try to use a smaller buffer_size"))
  return(buffer_size)
}

#' Formats a number of bytes
#'
#' @param x the values
#' @param digits the number of digits to display, default value is 2
#' @param unit_suffix the unit suffix, default value is 'B' (for bytes),
#' useful is also 'B/s' if you have read/write speeds
#' @param base the base for kilo, mega, ... definition, default is 1000
#'
#' @return the values as a character
#' @export
#'
#' @examples
#' format_bytes(1234)
#' format_bytes(1234567890)
#' format_bytes(123456789012, unit_suffix = "iB", base = 1024)
format_bytes <- function(x, digits = 2, unit_suffix = "B", base = 1000) {
  if (!all(is.finite(x))) return(rep(NA, length(x)))
  nr <- floor(log(x, base))
  # future proof it :)
  mtch <- c("", "K", "M", "G", "T", "P", "E", "Z", "Y")
  units <- paste0(mtch[nr + 1], unit_suffix)
  val <- x / base^nr

  res <- sprintf(sprintf("%%.%if%%s", digits), val, units)
  names(res) <- names(x)
  res
}

report_end <- function(t0, quiet, file = NA) {
  diff_secs <- as.numeric(difftime(Sys.time(), t0, units = "secs"))

  if (is.na(file)) {
    txt <- ""
  } else {
    if (file.exists(file)) size <- file.size(file) else size <- file
    speed_txt <- format_bytes(size / diff_secs, digits = 2,
                              unit_suffix = "B/s")
    txt <- sprintf(" at %s", speed_txt)
  }
  if (!quiet) cat(sprintf("[Done]       in %.2f secs%s\n", diff_secs, txt))
}
