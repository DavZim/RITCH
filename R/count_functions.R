#' Internal function to count the messages
#'
#' @param x a data.frame containing the message types and the counts
#' @param types a vector containing the types
#'
#' @keywords internal
#' @return a numeric value of number of orders in x
#'
#' @examples
#' # Only used internally
count_internal <- function(x, types) {
  if (!is.data.frame(x)) stop("x has to be a data.table")
  if (!all(c("msg_type", "count") %in% names(x))) stop("x has to have the variables 'msg_type' and 'count'")
  
  as.integer(x[msg_type %in% types][, sum(count)])
}

#' Counts the number of orders from a data.table of message counts
#'
#' @param x a file or a data.frame containing the message types and the counts
#'
#' @return a numeric value of number of orders in x
#' @export
#' 
#' @seealso \code{\link{count_messages}}
#'
#' @examples
#' \dontrun{
#'   raw_file <- "20170130.PSX_ITCH_50"
#' 
#'   msg_count <- count_messages(raw_file)
#' 
#'   count_orders(msg_count)
#' }
count_orders <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = T)
  types <- c("A", "F")
  count_internal(x, types)
}


#' Counts the number of trades from a data.table of message counts
#'
#' @param x a file or a data.frame containing the message types and the counts
#'
#' @return a numeric value of number of trades in x
#' @export
#' 
#' @seealso \code{\link{count_messages}}
#'
#' @examples
#' \dontrun{
#'   raw_file <- "20170130.PSX_ITCH_50"
#' 
#'   msg_count <- count_messages(raw_file)
#' 
#'   count_trades(msg_count)
#' }
count_trades <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = T)
  types <- c("P", "Q", "B")
  count_internal(x, types)
}

#' Counts the number of order modifications from a data.table of message counts
#'
#' @param x a file or a data.frame containing the message types and the counts
#'
#' @return a numeric value of number of order modifications in x
#' @export
#' 
#' @seealso \code{\link{count_messages}}
#'
#' @examples
#' \dontrun{
#'   raw_file <- "20170130.PSX_ITCH_50"
#' 
#'   msg_count <- count_messages(raw_file)
#' 
#'   count_modifications(msg_count)
#' }
count_modifications <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = T)
  types <- c("E", "C", "X", "D", "U")
  count_internal(x, types)
}
