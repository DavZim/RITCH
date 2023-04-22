#' @useDynLib RITCH
#' @importFrom Rcpp sourceCpp
#' @import data.table
#' @importFrom nanotime nanotime
#' @importFrom bit64 as.integer64
#' @importFrom utils browseURL download.file
NULL

#' @title ITCH 50 Example Testing Dataset
#' @name ex20101224.TEST_ITCH_50
#'
#' @section ex20101224.TEST_ITCH_50:
#'
#' The test dataset contains artificial trading data for three made up stocks:
#'  `ALC`, `BOB`, and `CHAR`.
#'
#' The dataset is used in the examples and unit tests of the package.
#'
#' The data contains the following count of messages:
#'
#' - 6 system event (message type `S`)
#' - 3 stock directory (message type `R`)
#' - 3 trading status (message type `H`)
#' - 5000 orders (4997 message type `A` and 3 `F`)
#' - 2000 modifications (198 `F`, 45 `X`, 1745 `D`, and 12 `U` message types)
#' - 5000 trades (message type `P`)
#'
#' The file is also available as `ex20101224.TEST_ITCH_50.gz`.
#'
#' To get real sample ITCH datasets, see the [download_sample_file()]
#' function.
#' @examples
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#'
#' sys <- read_system_events(file)
NULL
