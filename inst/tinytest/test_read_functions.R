library(RITCH)
library(tinytest)

nanotime_class <- "nanotime"
attr(nanotime_class, "package") <- "nanotime"
as.int64 <- bit64::as.integer64

# Begin Tests
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")

expect_true(file.exists(file))
expect_true(file.info(file)[["size"]] > 0)

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

#### Orders
od <- read_orders(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  locate_code = "integer",
  tracking_number = "integer",
  timestamp = "integer64",
  order_ref = "integer64",
  buy = "logical",
  shares = "integer",
  stock = "character",
  price = "numeric",
  mpid = "character",
  date = c("POSIXct", "POSIXt"),
  datetime = nanotime_class,
  exchange = "character"
)

expect_equal(class(od), c("data.table", "data.frame"))
expect_equal(nrow(od), 5000)
expect_equal(names(od), names(classes_exp))
expect_equal(lapply(od, class), classes_exp)
expect_equal(table(od$msg_type), c("A" = 4997, "F" = 3))
expect_equal(table(od$buy), c("FALSE" = 2568, "TRUE" = 2432))
expect_equal(table(od$stock), c("ALC" = 950, "BOB" = 2482, "CHAR" = 1568))
expect_equal(unique(od$date), as.POSIXct("2010-12-24", "GMT"))
expect_equal(unique(od$exchange), "TEST")


#### Trades
tr <- read_trades(file, quiet = TRUE)

classes_exp <- list(
  msg_type = "character",
  locate_code = "integer",
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
  datetime = nanotime_class,
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

#### System Events

#### Stock Directory

#### Trading Status
