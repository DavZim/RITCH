#' @useDynLib RITCH
#' @importFrom Rcpp sourceCpp
#' @import data.table
#' @importFrom nanotime nanotime
#' @importFrom bit64 as.integer64
NULL

#' @title ITCH 50 Example Testing Dataset 
#' @name ex20101224.TEST_ITCH_50
#' 
#' @section ex20101224.TEST_ITCH_50:
#' 
#' The test dataset contains artificial trading data for three made up stocks: 
#'  "ALC", "BOB", and "CHAR"
#' 
#' The dataset is used in the examples and unit tests of the package.
#'
#' The data contains the following count of messages:
#'  \itemize{
#'  \item{6 system event (message type \code{S})}
#'  \item{3 stock directory (message type \code{R})}
#'  \item{3 trading status (message type \code{H})}
#'  \item{5000 orders (4997 message type \code{A} and 3 \code{F})}
#'  \item{2000 modifications (198 \code{F}, 45 \code{X}, 1745 \code{D}, and 12 \code{U} message types)}
#' }
#'  
#' To get real sample ITCH datasets, see the \code{\link{download_sample_file}}
#' function.
#' @examples 
#' file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
#' 
#' sys <- read_system_events(file)
NULL
