library(magrittr)
dir_size <- function(dir) {
  files <- list.files(dir, full.names = T,
                      recursive = TRUE)
  v <- sapply(files, file.size)
  prettyunits::pretty_bytes(sum(v))
}
repl_names <- function(x) {
  names(x) <- gsub("^.*/(?=[^/]+$)", "", names(x), perl = TRUE)
  x
}

dir_size(file.path(.libPaths(), "RITCH"))
# before: [1] 13.11 MB

sapply(list.dirs(file.path(.libPaths(), "RITCH")),
                 dir_size) %>% 
  repl_names()
# OLD
#     RITCH     extdata        help     figures        html        libs        Meta           R    tinytest 
# "13.11 MB" "625.01 kB" "128.28 kB"  "89.34 kB"   "9.29 kB"  "12.22 MB"   "4.34 kB"  "99.88 kB"  "19.35 kB" 


# Speed
library(RITCH)
file <- "20191230.BX_ITCH_50"

funcs <- list(
  system_events = read_system_events,
  stock_directory = read_stock_directory,
  trading_status = read_trading_status,
  reg_sho = read_reg_sho,
  market_participant_states = read_market_participant_states,
  mwcb = read_mwcb,
  ipo = read_ipo,
  luld = read_luld,
  orders = read_orders,
  trades = read_trades,
  modifications = read_modifications,
  noii = read_noii,
  rpii = read_rpii
)

t0 <- Sys.time()
ll <- lapply(funcs, function(f) f(file, quiet = TRUE))
difftime(Sys.time(), t0)
# OLD: 16.50secs
