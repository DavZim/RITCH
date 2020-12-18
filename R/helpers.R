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
#'   \code{\link[base]{as.Date}}.
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
    
    file <- gsub("\\d{8}", format(date, "%Y%m%d"), file)
    file <- gsub("(?<=\\d{8}\\.)[^_]+", exchange, file, perl = TRUE)
    
  } else {
    
    # Unkown format... use 20101224.TEST_ITCH_50
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
#' The specifications can be found as a PDF \url{https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf}.
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

#' Opens the ITCH sample FTP page
#'
#' The FTP server can be found at \url{ftp://emi.nasdaq.com/ITCH/}.
#' 
#' @return the URL (invisible)
#' @export
#'
#' @examples
#' \dontrun{
#' open_itch_ftp()
#' }
open_itch_ftp <- function() {
  url <- "ftp://emi.nasdaq.com/ITCH/"
  browseURL(url)
  return(invisible(url))
}
