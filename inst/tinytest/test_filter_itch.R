library(RITCH)
library(tinytest)
library(data.table)
suppressPackageStartupMessages(library(bit64))
setDTthreads(2)

infile <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
outfile <- file.path(tempdir(), "testfile_20101224.TEST_ITCH_50")


################################################################################
# Test that filtering for all trades returns all data entries
orig <- read_itch(infile, quiet = TRUE)
trades <- read_trades(infile, quiet = TRUE)
expect_equal(orig$trades, trades)

res <- read_itch(infile, quiet = TRUE, filter_msg_class = "trades",
                 filter_stock_locate = c(1, 2, 3), min_timestamp = 1,
                 max_timestamp = 6e14, filter_msg_type = "P")
expect_equal(res, trades)

filter_itch(infile, outfile, filter_msg_class = "trades", quiet = TRUE)
res <- read_itch(outfile, quiet = TRUE)
expect_equal(res$trades, trades)
unlink(outfile)


################################################################################
# Test that the first and last messages are parsed
of <- filter_itch(infile, outfile, filter_msg_class = "system_events", quiet = TRUE)

# the filename is not changed
expect_equal(of, outfile)

unlink(of)
tmpfile <- tempfile("testfile")
of <- filter_itch(infile, tmpfile, filter_msg_class = "system_events", quiet = TRUE)

# the outfile name is correctly constructed!
expect_equal(of, paste0(tmpfile, "_20101224.TEST_ITCH_50"))

# test file contents
expect_equal(file.size(of), 84)
df <- read_system_events(of, quiet = TRUE)
expect_equal(nrow(df), 6)
unlink(of)

################################################################################
################################################################################
# Test Message Class
filter_itch(infile, outfile, filter_msg_class = "orders", quiet = TRUE)

# calling it on the file again causes error unless overwrite = TRUE
expect_error(
  filter_itch(infile, outfile, filter_msg_class = "orders", quiet = TRUE)
)

# test overwrite = TRUE
filter_itch(infile, outfile, filter_msg_class = "orders", overwrite = TRUE,
            quiet = TRUE)

expect_equal(file.size(outfile), 190012)

# check that the output file contains only orders
df  <- read_orders(outfile, quiet = TRUE)
expect_equal(nrow(df), 5000)

df2 <- read_orders(infile, quiet = TRUE)
expect_equal(df, df2)

# writing again to the same fail results in error
expect_error(
  filter_itch(infile, outfile, filter_msg_class = "orders", quiet = TRUE)
)
unlink(outfile)

################################################################################
# Test Append
filter_itch(infile, outfile, filter_msg_class = "orders", quiet = TRUE)
filter_itch(infile, outfile, filter_msg_class = "orders", append = TRUE,
            quiet = TRUE)

df <- read_orders(outfile, quiet = TRUE)
dforig <- read_orders(infile, quiet = TRUE)

expect_equal(
  df,
  rbindlist(list(dforig, dforig))
)
unlink(outfile)

################################################################################
# Test smaller buffer_size

filter_itch(infile, outfile, filter_msg_class = "orders",
            buffer_size = 50,
            quiet = TRUE)

expect_equal(file.size(outfile), 190012)

# check that the output file contains only orders
df  <- read_orders(outfile, quiet = TRUE)
expect_equal(nrow(df), 5000)

df2 <- read_orders(infile, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)

################################################################################
################################################################################
# Test Msg Type
filter_itch(infile, outfile, filter_msg_type = "S", quiet = TRUE)

expect_equal(file.size(outfile), 84)
# check that the output file contains only orders
df <- read_system_events(outfile, quiet = TRUE)
expect_equal(nrow(df), 6)

df2 <- read_system_events(infile, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
################################################################################
# Test Stock Locate
filter_itch(infile, outfile, filter_stock_locate = c(2, 3), quiet = TRUE)

expect_equal(file.size(outfile), 333876)
# check that the output file contains only orders
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(
  stock_directory = 2L, trading_status = 2L,
  orders = 4050L, modifications = 1626L, trades = 3115L
)
expect_equal(sapply(df, nrow), exp_count)

df2 <- read_itch(infile, filter_stock_locate = c(2, 3), quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
################################################################################
# Test filter_stock
stock_sel <- c("BOB", "CHAR")
sdir <- data.table(stock = stock_sel,
                   stock_locate = c(2, 3))
filter_itch(infile, outfile, filter_stock = stock_sel, stock_directory = sdir,
            quiet = TRUE)

expect_equal(file.size(outfile), 333876)
# check that the output file contains only orders
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(
  stock_directory = 2L, trading_status = 2L,
  orders = 4050L, modifications = 1626L, trades = 3115L
)
expect_equal(sapply(df, nrow), exp_count)

df2 <- read_itch(infile, filter_stock = stock_sel, stock_directory = sdir,
                 quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
################################################################################
# Test Timestamps

get_func_of_ts <- function(ll, func = min) {
  ll <- ll[sapply(ll, nrow) != 0]
  mm <- lapply(ll, function(d) list(func(d$timestamp)))
  x <- unlist(mm)
  class(x) <- "integer64"
  func(x)
}

# check errors
# either min & max timestamp have the same size or 0 and 1
expect_error(
  filter_itch(infile, outfile, min_timestamp = 1:2, quiet = TRUE)
)
expect_error(
  filter_itch(infile, outfile, min_timestamp = 1:2, max_timestamp = 1:3,
              quiet = TRUE)
)
expect_error(
  filter_itch(infile, outfile, min_timestamp = 1, max_timestamp = 1:3,
              quiet = TRUE)
)


################################################################################
## Min only
ms <- as.integer64(45463537089764)
filter_itch(infile, outfile, min_timestamp = ms, quiet = TRUE)

expect_equal(file.size(outfile), 236547)
# check that the output file contains only orders
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(
  system_events = 3L, orders = 2501L, modifications = 979L, trades = 2598L
)
expect_equal(sapply(df, nrow), exp_count)

# read-in all data and filter the data manually
df_all <- read_itch(infile, quiet = TRUE)
df_all_f <- lapply(df, function(d) d[timestamp >= ms, ])
expect_equal(df_all_f, df)

# check that for all classes the min timestamp is larger than the expected value
expect_true(get_func_of_ts(df, min) >= ms)

df2 <- read_itch(infile, min_timestamp = ms, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
## Max only
ms <- as.integer64(45463537089764)
filter_itch(infile, outfile, max_timestamp = ms, quiet = TRUE)

expect_equal(file.size(outfile), 228539)
# check that the output file contains only orders
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(
  system_events = 3L, stock_directory = 3L, trading_status = 3L,
  orders = 2500L, modifications = 1021L, trades = 2402L
)
expect_equal(sapply(df, nrow), exp_count)

# read-in all data and filter the data manually
df_all <- read_itch(infile, quiet = TRUE)
df_all_f <- lapply(df, function(d) d[timestamp <= ms, ])
expect_equal(df_all_f, df)

# check that for all classes the max timestamp is smaller than the expected value
expect_true(get_func_of_ts(df, max) <= ms)

df2 <- read_itch(infile, max_timestamp = ms, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
## min and max
min_ts <- as.integer64(45463537089764)
max_ts <- as.integer64(51233773867238)
filter_itch(infile, outfile, min_timestamp = min_ts, max_timestamp = max_ts,
            quiet = TRUE)

expect_equal(file.size(outfile), 138558)

# check that the output file contains only orders
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(orders = 1501L, modifications = 598L, trades = 1477L)
expect_equal(sapply(df, nrow), exp_count)

# read-in all data and filter the data manually
df_all <- read_itch(infile, quiet = TRUE)
df_all_f <- lapply(df, function(d) d[timestamp >= min_ts & timestamp <= max_ts, ])
expect_equal(df_all_f, df)


# check that for all classes the max timestamp is smaller than the expected value
dd <- df[sapply(df, nrow) != 0]
expect_true(get_func_of_ts(df, min) >= min_ts)
expect_true(get_func_of_ts(df, max) <= max_ts)

df2 <- read_itch(infile, min_timestamp = min_ts, max_timestamp = max_ts,
                 quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
################################################################################
# Test n_max

# max number of messages is 5000, taking all messages results in the same file
filter_itch(infile, outfile, n_max = 5000, quiet = TRUE)
expect_equal(file.size(infile), file.size(outfile))
unlink(outfile)

# take the first 100 messages for each message class
filter_itch(infile, outfile, n_max = 100, quiet = TRUE)
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(system_events = 6, stock_directory = 3, trading_status = 3,
               orders = 100, modifications = 100, trades = 100)
expect_equal(sapply(df, nrow), exp_count)

df2 <- read_itch(infile, n_max = 100, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)

################################################################################
# Test skip

# skipping 0 messages results in the same file
filter_itch(infile, outfile, skip = 0, quiet = TRUE)
expect_equal(file.size(infile), file.size(outfile))
unlink(outfile)


filter_itch(infile, outfile, skip = 1000, quiet = TRUE)
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(orders = 4000, modifications = 1000, trades = 4000)
expect_equal(sapply(df, nrow), exp_count)

df2 <- read_itch(infile, skip = 1000, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


# skip the first 4000 messages for each message class
# expect to see 5000-4000 trades and 5000-4000 orders
filter_itch(
  infile, outfile,
  skip = 4000,
  quiet = TRUE
)
df <- read_itch(outfile, quiet = TRUE)
exp_count <- c(orders = 1000, trades = 1000)
expect_equal(sapply(df, nrow), exp_count)

df2 <- read_itch(infile, skip = 4000, quiet = TRUE)
expect_equal(df, df2)
unlink(outfile)


################################################################################
################################################################################
# Test more complex filter
min_ts <- 40505246803501 # Q1 of all orders
max_ts <- 49358420393946 # Q3 of all orders

filter_itch(
  infile, outfile,
  filter_msg_class = c("orders", "trades"),
  filter_stock_locate = c(1, 3),
  filter_msg_type = "D",
  skip = 0, n_max = 100,
  min_timestamp = min_ts,
  max_timestamp = max_ts,
  quiet = TRUE
)
expect_equal(file.size(outfile), 10500)

# check that the output file contains the same
filtered_res  <- read_itch(outfile, c("orders", "trades", "modifications"),
                           quiet = TRUE)
expect_equal(sapply(filtered_res, nrow),
             c(orders = 100, trades = 100, modifications = 100))

# read in the original file, and apply the same filters to each class
df_orig <- read_itch(infile,  c("orders", "trades", "modifications"),
                     quiet = TRUE)
# apply the filters
msg_types <- c('D', 'A', 'F', 'P', 'Q', 'B')
df_orig_res <- lapply(df_orig, function(d)
  d[msg_type %in% msg_types &
      stock_locate %in% c(1, 3) &
      timestamp > min_ts & timestamp < max_ts][1:100,]
)

expect_equal(filtered_res, df_orig_res)
unlink(outfile)


################################################################################
# filter_itch works on gz input files
gzinfile <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
tmpoutfile <- file.path(tempdir(), "gz_testfile_20101224.TEST_ITCH_50")

rawoutfile <- filter_itch(gzinfile, tmpoutfile, filter_msg_class = "orders",
                          quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
expect_equal(rawoutfile, tmpoutfile)
expect_equal(file.size(rawoutfile), 190012)

odf <- read_orders(rawoutfile, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
idf <- read_orders(gzinfile, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
expect_equal(odf, idf)
unlink(rawoutfile)


################################################################################
# works also on gz-output files
rawoutfile <- filter_itch(gzinfile, tmpoutfile, filter_msg_class = "orders", gz = TRUE,
                          quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)

expect_equal(rawoutfile, paste0(tmpoutfile, ".gz"))
expect_true(file.exists(rawoutfile))
expect_equal(file.size(rawoutfile), 72619)

odf <- read_orders(rawoutfile, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)
idf <- read_orders(gzinfile, quiet = TRUE, force_gunzip = TRUE, force_cleanup = TRUE)

expect_equal(odf, idf)
unlink(rawoutfile)
unlink(tmpoutfile)
