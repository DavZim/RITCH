#' returns the date from an ITCH-filename
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
get_date_from_filename <- function(file) {
  date_ <- sub(".*(\\d{8}).*", "\\1", file)
  
  if (grepl("NASDAQ_ITCH50$", file)) {
    # format MMDDYYYY
    date_ <- gsub("(\\d{2})(\\d{2})(\\d{4})", "\\3-\\1-\\2", date_)
  } else {
    # format YYYYMMDD
    date_ <- gsub("(\\d{4})(\\d{2})(\\d{2})", "\\1-\\2-\\3", date_)
  }
  
  date_ <- fasttime::fastPOSIXct(date_, tz = "GMT")
  return(date_)
}
