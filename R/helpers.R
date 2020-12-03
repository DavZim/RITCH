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
  
  date_ <- as.POSIXct(date_, tz = "GMT")
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
get_exchange_from_filename <- function(file) {
  res <- regmatches(file, regexpr("(?<=\\.)[A-Z]+(?=_)", file, perl = TRUE))
  if (length(res) == 0)
    res <- regmatches(file, regexpr("(?<=-v50-)[a-z]+", file, perl = TRUE))
  toupper(res)
}


#' Opens the ITCH Specification URL
#'
#' The specifications can be found as a PDF \url{https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf}.
#' 
#' @return
#' @export
#'
#' @examples
#' \dontrun{
#' open_itch_specification()
#' }
open_itch_specification <- function() {
  url <- "https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf"
  browseURL(url)
}
