##############################
#' This script takes an existing dataset and samples and obfuscates the data
#' to create a smaller testing/example dataset.
#'
#' Messages that are sampled are:
#' - System Event Messages
#' - Stock Directory
#' - Trading Status
#' - Orders
#' - Modifications
#' - Trades
#'
##############################

library(RITCH)
library(data.table)

# take 3 most traded stocks in orders, trades
file <- "20191230.BX_ITCH_50"

loc_code <- read_stock_directory(file, add_meta = FALSE, quiet = TRUE)
trades   <- read_trades(file, add_meta = FALSE, quiet = TRUE)
orders   <- read_orders(file, add_meta = FALSE, quiet = TRUE)
mods     <- read_modifications(file, add_meta = FALSE, quiet = TRUE)

names_trades <- names(trades)
names_orders <- names(orders)
names_mods   <- names(mods)

# look at the most active stocks
orders[, .(n = .N), by = stock][order(-n)][1:3]
trades[, .(n = .N), by = stock][order(-n)][1:3]
merge(
  mods[, .(n = .N), by = stock_locate][order(-n)][1:3],
  loc_code[, .(stock_locate, stock)], by = "stock_locate", all.x = TRUE
)

# take the following stocks as a base
stock_select <- c("TSLA" = "ALC", "NIO" = "BOB", "BABA" = "CHAR")

loc_codes <- loc_code[
  stock %chin% names(stock_select)
][,
  .(stock_old = stock,
    old_loc_code = stock_locate,
    stock = stock_select[stock])
][order(stock)][, stock_locate := 1:.N][]

# removes price outliers outside of a given sigma range...
remove_price_outliers <- function(dt, sigma = 3) {
  dd <- dt[]
  setorder(dd, stock, timestamp)
  dd[, rmean := frollmean(price, 100, align = "left"), by = stock][, rmean := nafill(rmean, type = "locf"), by = stock]
  dd[, diff := (price - rmean), by = stock]
  dd[, diff := (diff - mean(diff, na.rm = TRUE)) / sd(diff, na.rm = TRUE), by = .(buy, stock)]
  dd <- dd[diff > -sigma & diff < sigma]

  dd[, -c("diff", "rmean")]
}

# obfuscates prices in a "standard" way
obfuscate_prices <- function(dt) {
  price_info <- data.table(stock = c("ALC", "BOB", "CHAR"),
                           tar_min_price = c(180, 45,  90),
                           tar_range     = c(20,  5,   15),
                           est_min_price = c(410, 2.5, 210),
                           est_range     = c(30,  6,   6))

  dd <- merge(dt, price_info, by = "stock", all.x = TRUE)
  # dd[, ':=' (
  #   min_price = min(price),
  #   price_range = max(price) - min(price)
  # ), by = stock]

  # scale the price by the base prices...
  dd[, price := (price - est_min_price) / est_range * (tar_range) + tar_range]
  dd[, price := round(price, 4)]
  return(dd[, -c("tar_min_price", "tar_range", "est_min_price", "est_range")])
}


######################
# Prepare System Event Messages
set.seed(65411235)

sys_ev <- read_system_events(file, add_meta = FALSE, quiet = TRUE)
sys_ev[, timestamp := timestamp + rnorm(.N, 0, 1e10)]


######################
# Prepare Stock Directory Messages
set.seed(76411948)

stock_dir <- read_stock_directory(file, add_meta = FALSE, quiet = TRUE)
names_dir <- names(stock_dir)
sdir <- stock_dir[stock %chin% names(stock_select)][, stock := stock_select[stock]][]

valid_market_cat <- c("Q", "G", "S", "N", "A", "P", "Z", "V", " ")
sdir[, ':='(
  market_category = sample(valid_market_cat, .N, replace = TRUE),
  financial_status = "N",
  issue_classification = "A",
  ipo_flag = FALSE,
  luld_price_tier = 2,
  etp_leverage = 0,
  stock_locate = NULL
)]
sdir <- sdir[loc_codes[, .(stock, stock_locate)], on = "stock"]
setorder(sdir, stock)
# rearrange timestamp to fit alphabetic stock names
sdir[, timestamp := sort(timestamp)]
setcolorder(sdir, names_dir)

######################
# Prepare Trading Status Messages
set.seed(198179841)

trad_stat <- read_trading_status(file, add_meta = FALSE, quiet = TRUE)
names_stat <- names(trad_stat)

# shuffle the timestamps and rename the stocks
trstat <- trad_stat[stock_locate %in% loc_codes$old_loc_code][
  , ':='(
    timestamp = timestamp + rnorm(.N, 0, 1e8),
    stock = stock_select[stock]
  )
][]

# add the new stock_locates
trstat <- merge(trstat[, -c("stock_locate")],
                loc_codes[, .(stock, stock_locate)],
                by = "stock", all.x = TRUE)

# order the timestamps by locate code...
trstat[, timestamp := timestamp[order(-stock_locate)]]

setcolorder(trstat, names_stat)

######################
# Prepare Orders Messages
set.seed(654918413)
N_ORDERS <- 5000

# rename the stock and stock_locates
or <- orders[stock %chin% names(stock_select)][, stock := stock_select[stock]]
or <- merge(or[, -c("stock_locate")], loc_codes[, .(stock, stock_locate)])

or <- remove_price_outliers(or, 2)

# Sample N orders
or <- or[sample.int(.N, N_ORDERS)]
# change timestamp
or <- or[, timestamp := timestamp + rnorm(.N, 0, 1e6)][order(timestamp)]

# treat order_ref
MIN_ORDER_REF <- min(or$order_ref)
or[, order_ref := order_ref - MIN_ORDER_REF]

# obfuscate prices
or <- obfuscate_prices(or)
setcolorder(or, names_orders)


######################
# Prepare Trades Messages
set.seed(7451984)
N_TRADES <- 1000

tr <- trades[stock %chin% names(stock_select)][, stock := stock_select[stock]]
tr <- merge(tr[, -c("stock_locate")], loc_codes[, .(stock, stock_locate)])

tr <- remove_price_outliers(tr, 2)

# Sample N orders
tr <- tr[sample.int(.N, N_ORDERS)]
# change timestamp
tr <- tr[, timestamp := timestamp + rnorm(.N, 0, 1e6)][order(timestamp)]

tr <- obfuscate_prices(tr)
setcolorder(tr, names_trades)


######################
# Prepare Modifications Messages
set.seed(78632176)
N_MODS <- 2000

md <- mods[stock_locate %in% loc_codes$old_loc_code][, old_loc_code := stock_locate]
md <- merge(md[, -c("stock_locate")],
            loc_codes[, .(stock, stock_locate, old_loc_code)],
            by = "old_loc_code")[, -c("old_loc_code")]

# subset only for stocks that are also in the orders
md[, order_ref := order_ref - MIN_ORDER_REF]
md <- md[order_ref %in% or$order_ref]

md <- md[sample.int(.N, N_MODS)]

md <- obfuscate_prices(md)
md[, stock := NULL]
setcolorder(md, names_mods)


########################################
# Combine datasets and write to file

ll <- list(
  sys_ev,
  sdir,
  trstat,
  or,
  tr,
  md
)

# write the dataset to file
if (!dir.exists("inst/extdata")) dir.create("inst/extdata")
outfile <- "inst/extdata/ex20101224.TEST_ITCH_50"

write_itch(ll, outfile, add_meta = FALSE, quiet = TRUE)
write_itch(ll, outfile, compress = TRUE, add_meta = FALSE, quiet = TRUE)

cat(sprintf("Wrote sample dataset to '%s' with size '%.2f'KB\n",
            outfile, file.info(outfile)[["size"]] / 1024))

#######################################
# Read in the dataset and compare results
funcs <- list(read_system_events, read_stock_directory, read_trading_status,
              read_orders, read_trades, read_modifications)

ll_read <- lapply(funcs, function(f) f(outfile, quiet = TRUE, add_meta = FALSE))
all.equal(ll, ll_read, check.attributes = FALSE)
