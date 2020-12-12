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

#' @name count_functions
#' @rdname count_functions
#'
#' @title Counts the number of messages in a group from a data.table of message counts
#' 
#' @description
#' 
#' @param x a file or a data.frame containing the message types and the counts
#' 
#' @details
#' The count functions apply to all message groups.
#' 
#' @return a numeric value of number of orders in x
#'
#' @examples
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' msg_count <- count_messages(file, quiet = TRUE)
NULL

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_orders(msg_count)
count_orders <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("A", "F")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_trades(msg_count)
count_trades <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("P", "Q", "B")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_modifications(msg_count)
count_modifications <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("E", "C", "X", "D", "U")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_system_events(msg_count)
count_system_events <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("S")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_stock_directory(msg_count)
count_stock_directory <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("R")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_trading_status(msg_count)
count_trading_status <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("H", "h")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_reg_sho(msg_count)
count_reg_sho <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("Y")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_market_participant_states(msg_count)
count_market_participant_states <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("L")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_mwcb(msg_count)
count_mwcb <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("V", "W")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_ipo(msg_count)
count_ipo <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("K")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_luld(msg_count)
count_luld <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("J")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_noii(msg_count)
count_noii <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("I")
  count_internal(x, types)
}

#' @rdname count_functions
#' @export
#' @examples
#' 
#' count_rpii(msg_count)
count_rpii <- function(x) {
  if (is.character(x)) x <- count_messages(x, quiet = TRUE)
  types <- c("N")
  count_internal(x, types)
}