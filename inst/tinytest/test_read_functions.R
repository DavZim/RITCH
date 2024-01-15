library(RITCH)
library(tinytest)
library(data.table)
setDTthreads(2)

as.int64 <- bit64::as.integer64
# overload table for better comparison
table <- function(x) {
  tb <- base::table(x)
  res <- as.numeric(tb)
  names(res) <- names(tb)
  res
}

# Begin Tests
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
gzfile <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
file_raw <- strsplit(file, "/")[[1]]
file_raw <- file_raw[length(file_raw)]

expect_true(file.exists(file))
expect_true(file.info(file)[["size"]] == 465048)
expect_true(file.exists(gzfile))
expect_true(file.info(gzfile)[["size"]] == 159965)

#### Count messages
ct <- count_messages(file, quiet = TRUE)
ct_exp <- data.table(
  msg_type = c("S", "R", "H", "Y", "L", "V", "W", "K", "J", "h", "A", "F", "E",
               "C", "X", "D", "U", "P", "Q", "B", "I", "N"),
  count = as.int64(c(6, 3, 3, 0, 0, 0, 0, 0, 0, 0, 4997, 3, 198, 0, 45, 1745,
                     12, 5000, 0, 0, 0, 0))
)

expect_equal(class(ct), c("data.table", "data.frame"))
expect_equal(nrow(ct), 22)
expect_equal(ct, ct_exp)

# force_cleanup = FALSE leaves the raw file behind
ct2 <- count_messages(gzfile, quiet = TRUE, force_gunzip = TRUE,
                      gz_dir = tempdir(), force_cleanup = FALSE)
expect_equal(ct, ct2)
expect_true(file.exists(file.path(tempdir(),
                                  gsub("\\.gz$", "", basename(gzfile)))))
unlink(file_raw)

# check that force_cleanup works
ct3 <- count_messages(gzfile, quiet = TRUE, force_gunzip = TRUE,
                      force_cleanup = TRUE)
expect_equal(ct, ct3)
expect_false(file.exists(file_raw))


#### Orders
od <- read_orders(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  stock_locate = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  order_ref = "integer64",
  buy = "logical",
  shares = "integer",
  stock = "character",
  price = "numeric",
  mpid = "character",
  date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(od), c("data.table", "data.frame"))
expect_equal(names(od), names(classes_exp))
expect_equal(lapply(od, class), classes_exp)
expect_equal(nrow(od), 5000)
expect_equal(table(od$msg_type), c("A" = 4997, "F" = 3))
expect_equal(table(od$buy), c("FALSE" = 2568, "TRUE" = 2432))
expect_equal(table(od$stock), c("ALC" = 950, "BOB" = 2482, "CHAR" = 1568))
expect_equal(unique(od$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(od$exchange), "TEST")

# test n_max = ct
od2 <- read_orders(file, quiet = TRUE, n_max = ct)
expect_equal(class(od2), c("data.table", "data.frame"))
expect_equal(names(od2), names(classes_exp))
expect_equal(lapply(od2, class), classes_exp)
expect_equal(nrow(od2), 5000)
expect_equal(od, od2)

# test skip and n_max
od3 <- read_orders(file, quiet = TRUE, skip = 3, n_max = 10)
expect_equal(od3, od[4:13])

od4 <- read_orders(file, quiet = TRUE, skip = 3)
expect_equal(od4, od[4:nrow(od)])

od5 <- read_orders(file, quiet = TRUE, n_max = 4)
expect_equal(od5, od[1:4])

#### Trades
tr <- read_trades(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  stock_locate = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  order_ref = "integer64",
  buy = "logical",
  shares = "integer",
  stock = "character",
  price = "numeric",
  match_number = "integer64",
  cross_type = "character",
  date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(tr), c("data.table", "data.frame"))
expect_equal(nrow(tr), 5000)
expect_equal(names(tr), names(classes_exp))
expect_equal(lapply(tr, class), classes_exp)
expect_equal(table(tr$msg_type), c("P" = 5000))
expect_equal(table(tr$buy), c("TRUE" = 5000))
expect_equal(table(tr$stock), c("ALC" = 1885, "BOB" = 1658, "CHAR" = 1457))
expect_equal(unique(tr$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(tr$exchange), "TEST")

#### Modifications
md <- read_modifications(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  stock_locate = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  order_ref = "integer64",
  shares = "integer",
  match_number = "integer64",
  printable = "logical",
  price = "numeric",
  new_order_ref = "integer64",
  date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(md), c("data.table", "data.frame"))
expect_equal(nrow(md), 2000)
expect_equal(names(md), names(classes_exp))
expect_equal(lapply(md, class), classes_exp)
expect_equal(table(md$msg_type), c("D" = 1745, "E" = 198, "U" = 12, "X" = 45))
expect_equal(table(is.na(md$shares)), c("FALSE" = 255, "TRUE" = 1745))
expect_equal(table(is.na(md$match_number)), c("FALSE" = 198, "TRUE" = 1802))
expect_equal(table(is.na(md$printable)), c("TRUE" = 2000))
expect_equal(table(is.na(md$price)), c("FALSE" = 12, "TRUE" = 1988))
expect_equal(table(is.na(md$new_order_ref)), c("FALSE" = 12, "TRUE" = 1988))
expect_equal(unique(md$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(md$exchange), "TEST")

#### System Events
sys <- read_system_events(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  stock_locate = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  event_code = "character",
  date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(sys), c("data.table", "data.frame"))
expect_equal(nrow(sys), 6)
expect_equal(names(sys), names(classes_exp))
expect_equal(lapply(sys, class), classes_exp)
expect_equal(table(sys$msg_type), c("S" = 6))
expect_equal(sys$event_code, c("O", "S", "Q", "M", "E", "C"))
expect_equal(unique(sys$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(sys$exchange), "TEST")

#### Stock Directory
sdir <- read_stock_directory(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  stock_locate = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  stock = "character",
  market_category = "character",
  financial_status = "character",
  lot_size = "integer",
  round_lots_only = "logical",
  issue_classification = "character",
  issue_subtype = "character",
  authentic = "logical",
  short_sell_closeout = "logical",
  ipo_flag = "logical",
  luld_price_tier = "character",
  etp_flag = "logical",
  etp_leverage = "integer",
  inverse = "logical",
  date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(sdir), c("data.table", "data.frame"))
expect_equal(nrow(sdir), 3)
expect_equal(names(sdir), names(classes_exp))
expect_equal(lapply(sdir, class), classes_exp)
expect_equal(table(sdir$msg_type), c("R" = 3))
expect_equal(sdir$stock_locate, 1:3)
expect_equal(unique(sdir$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(sdir$exchange), "TEST")

#### Trading Status
tstat <- read_trading_status(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  stock_locate = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  stock = "character",
  trading_state = "character",
  reserved = "character",
  reason = "character",
  market_code = "character",
  operation_halted = "logical",
  date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(tstat), c("data.table", "data.frame"))
expect_equal(nrow(tstat), 3)
expect_equal(names(tstat), names(classes_exp))
expect_equal(lapply(tstat, class), classes_exp)
expect_equal(table(tstat$msg_type), c("H" = 3))
expect_equal(tstat$stock_locate, 1:3)
expect_equal(unique(tstat$trading_state), "T")
expect_equal(unique(tstat$operation_halted), NA)
expect_equal(unique(tstat$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(tstat$exchange), "TEST")

######## Other empty message groups

## Reg Sho
rs <- read_reg_sho(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character", stock_locate = "integer", tracking_number = "integer",
  timestamp = "integer64", stock = "character", regsho_action = "character",
  date = c("POSIXct", "POSIXt"), datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(rs), c("data.table", "data.frame"))
expect_equal(nrow(rs), 0)
expect_equal(names(rs), names(classes_exp))
expect_equal(lapply(rs, class), classes_exp)


## Market Participant States
mps <- read_market_participant_states(file, quiet = TRUE)
classes_exp <- list(
  msg_type = "character", stock_locate = "integer", tracking_number = "integer",
  timestamp = "integer64", mpid = "character", stock = "character",
  primary_mm = "logical", mm_mode = "character", participant_state = "character",
  date = c("POSIXct", "POSIXt"), datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(mps), c("data.table", "data.frame"))
expect_equal(nrow(mps), 0)
expect_equal(names(mps), names(classes_exp))
expect_equal(lapply(mps, class), classes_exp)


## MWCB
mwcb <- read_mwcb(file, quiet = TRUE)
classes_exp <- list(
  msg_type = "character", stock_locate = "integer", tracking_number = "integer",
  timestamp = "integer64", level1 = "numeric", level2 = "numeric",
  level3 = "numeric", breached_level = "integer", date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"), exchange = "character"
)

expect_equal(class(mwcb), c("data.table", "data.frame"))
expect_equal(nrow(mwcb), 0)
expect_equal(names(mwcb), names(classes_exp))
expect_equal(lapply(mwcb, class), classes_exp)

## IPO
ipo <- read_ipo(file, quiet = TRUE)
classes_exp <- list(
  msg_type = "character", stock_locate = "integer", tracking_number = "integer",
  timestamp = "integer64", stock = "character", release_time = "integer",
  release_qualifier = "character", ipo_price = "numeric", date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"), exchange = "character"
)

expect_equal(class(ipo), c("data.table", "data.frame"))
expect_equal(nrow(ipo), 0)
expect_equal(names(ipo), names(classes_exp))
expect_equal(lapply(ipo, class), classes_exp)

## NOII
noii <- read_noii(file, quiet = TRUE)
classes_exp <- list(
  msg_type = "character", stock_locate = "integer", tracking_number = "integer",
  timestamp = "integer64", paired_shares = "integer64", imbalance_shares = "integer64",
  imbalance_direction = "character", stock = "character", far_price = "numeric",
  near_price = "numeric", reference_price = "numeric", cross_type = "character",
  variation_indicator = "character", date = c("POSIXct", "POSIXt"),
  datetime = structure("nanotime", package = "nanotime"), exchange = "character"
)

expect_equal(class(noii), c("data.table", "data.frame"))
expect_equal(nrow(noii), 0)
expect_equal(names(noii), names(classes_exp))
expect_equal(lapply(noii, class), classes_exp)

# RPII
rpii <- read_rpii(file, quiet = TRUE)
classes_exp <- list(
  msg_type = "character", stock_locate = "integer", tracking_number = "integer",
  timestamp = "integer64", stock = "character", interest_flag = "character",
  date = c("POSIXct", "POSIXt"), datetime = structure("nanotime", package = "nanotime"),
  exchange = "character"
)

expect_equal(class(rpii), c("data.table", "data.frame"))
expect_equal(nrow(rpii), 0)
expect_equal(names(rpii), names(classes_exp))
expect_equal(lapply(rpii, class), classes_exp)


##########################################################################
# Test Read with Filters

# Use Orders
orders <- read_orders(file, quiet = TRUE)

# test msg_type
expect_equal(orders[msg_type == "A"],
             read_orders(file, quiet = TRUE, filter_msg_type = "A"))
expect_equal(orders[msg_type == "F"],
             read_orders(file, quiet = TRUE, filter_msg_type = "F"))
expect_equal(orders[msg_type %in% c("A", "F")],
             read_orders(file, quiet = TRUE, filter_msg_type = c("A", "F")))
# Missing values are ommitted if possible
expect_equal(orders[msg_type == "A"],
             read_orders(file, quiet = TRUE, filter_msg_type = c("A", NA, NA)))
expect_equal(orders[msg_type == "A"],
             read_orders(file, quiet = TRUE, filter_msg_type = c(NA, NA, "A")))
# msg_types can also be a single character for ease of use
expect_equal(orders,
             read_orders(file, quiet = TRUE, filter_msg_type = "AF"),
             check.attributes = FALSE)

# test locate code
expect_equal(orders[stock_locate == 1],
             read_orders(file, quiet = TRUE, filter_stock_locate = 1))
expect_equal(orders[stock_locate == 2],
             read_orders(file, quiet = TRUE, filter_stock_locate = 2))
expect_equal(orders[stock_locate %in% 1:3],
             read_orders(file, quiet = TRUE, filter_stock_locate = 1:3))
expect_equal(orders[stock_locate %in% c(1, 3)],
             read_orders(file, quiet = TRUE, filter_stock_locate = c(1, NA, 3)))

# test timestamp
start_ts <- as.int64(40505246803501)  # Q1
end_ts <- as.int64(45475518278493)    # Mean

start_ts2 <- as.int64(49358420393946) # Q3
end_ts2 <- as.int64(57595326231183)   # Max


# different lengths are only allowed if one is NA and the other has only one entry
expect_error(
  read_orders(file, quiet = TRUE, min_timestamp = 1:2)
)
expect_error(
  read_orders(file, quiet = TRUE, min_timestamp = 1:2, max_timestamp = 1:3)
)

expect_equal(
  orders[timestamp >= start_ts & timestamp <= end_ts],
  read_orders(file, quiet = TRUE,
              min_timestamp = start_ts, max_timestamp = end_ts)
)
# can only specify one min/max timestamp
expect_equal(
  orders[timestamp >= start_ts],
  read_orders(file, quiet = TRUE,
              min_timestamp = start_ts)
)
# multiple timestamps
expect_equal(
  orders[timestamp >= start_ts & timestamp <= end_ts |
           timestamp >= start_ts2 & timestamp <= end_ts2],
  read_orders(file, quiet = TRUE,
              min_timestamp = c(start_ts, start_ts2),
              max_timestamp = c(end_ts, end_ts2))
)

# test stock
sdir <- read_stock_directory(file, quiet = TRUE)
# warning as no stock_directory is specified
expect_warning(read_orders(file, quiet = TRUE, filter_stock = "ALC"))
expect_equal(orders[stock == "ALC"],
             read_orders(file, quiet = TRUE, filter_stock = "ALC",
                         stock_directory = sdir))
expect_equal(orders[stock == "BOB"],
             read_orders(file, quiet = TRUE, filter_stock = "BOB",
                         stock_directory = sdir))
expect_equal(
  orders[stock %in% c("ALC", "BOB", "CHAR")],
  read_orders(file, quiet = TRUE, filter_stock = c("ALC", "BOB", "CHAR"),
              stock_directory = sdir)
)
expect_error(
  read_orders(file, quiet = TRUE, filter_stock = c("ALC", "NOSTOCK"),
              stock_directory = sdir)
)

# combine multiple filters act as an AND combination!
expect_equal(
  orders[stock == "ALC" &
           timestamp >= start_ts &
           timestamp <= end_ts &
           msg_type == "A"],
  read_orders(file, quiet = TRUE,
              filter_msg_type = "A",
              min_timestamp = start_ts,
              max_timestamp = end_ts,
              filter_stock = "ALC",
              stock_directory = sdir)
)

# combine multiple filters act as an AND combination!
# skip and n_max act as a filter within the filtered results

# skip = 10
expect_equal(
  orders[stock == "ALC" &
           timestamp >= start_ts &
           timestamp <= end_ts &
           msg_type == "A"][-c(1:10)],
  read_orders(file, quiet = TRUE,
              filter_msg_type = "A",
              min_timestamp = start_ts,
              max_timestamp = end_ts,
              filter_stock = "ALC",
              stock_directory = sdir,
              skip = 10)
)

# n_max
expect_equal(
  orders[stock == "ALC" &
           timestamp >= start_ts &
           timestamp <= end_ts &
           msg_type == "A"][1:3],
  read_orders(file, quiet = TRUE,
              filter_msg_type = "A",
              min_timestamp = start_ts,
              max_timestamp = end_ts,
              filter_stock = "ALC",
              stock_directory = sdir,
              n_max = 3)
)
