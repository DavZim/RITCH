##############################
# This file tests the debug functionality.
# Note that it the data used can be handcrafted for testing purposes
##############################

library(RITCH)
library(data.table)
library(testthat) # tinytest does not throw errors on failed tests when used in functions...
as.int64 <- bit64::as.integer64

a <- capture.output(Rcpp::sourceCpp("debug_tools.cpp"))

# runs multiple tests on a hex and dt conversions
# hex is a string of hexadecimal values
# dt can be provided, is a target data.table
# hex_to_dt_func is the function to convert the hex values to the dt
test_hex_to_dt <- function(hex = NA, dt = NA, hex_to_dt_func) {

  if (is.na(hex) && !is.data.frame(dt) && is.na(dt))
    stop("Either hex or dt must be provided")

  if (is.na(hex)) hex <- dbg_messages_to_hex(dt)

  hex <- trimws(hex)

  if (is.data.frame(dt) && !is.na(dt)) {
    # Check Message Counts
    n_msgs <- as.int64(nrow(dt))
    msg_count <- dbg_hex_count_messages(hex)
    expect_equal(msg_count[msg_type %in% dt$msg_type, sum(count)],
                 n_msgs)
    expect_equal(msg_count[!msg_type %in% dt$msg_type, sum(count)],
                 as.int64(0))

    # Check Message Lengths
    xx <- strsplit(gsub(" ", "", hex), split = "")[[1]]
    nibbles <- paste0(xx[c(T, F)], xx[c(F, T)])
    expect_equal(length(nibbles),
                 sum(as.numeric(dbg_get_message_length(dt$msg_type))))
  }

  # Converting hex to DT messages
  dt_conv <- hex_to_dt_func(hex)
  if (is.data.frame(dt) && !is.na(dt)) expect_equal(dt, dt_conv)

  # Converting DT messages to hex
  hex_conv <- dbg_messages_to_hex(dt_conv)
  expect_equal(hex_conv, hex)
}

################################################################################
########### Order Messages
################################################################################

#' Messages 'A' and 'F'
#' Example Order 'A'
#' 41 20 2c 00 00 16 eb 55 2c 88 24 00 00 00 00 00 00 00 04 42 00 00 2e 7c 55 53 4f 20 20 20 20 20 00 01 fa 40
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 06 >> >> >> 07 >> >> >> >> >> >> >> 08 >> >> >> 09
#' id size name             hex value
#' 01   1  message type     41                       F
#' 02   2  stock locate     50 2c                    8236
#' 03   2  tracking number  00 00                    0
#' 04   6  timestamp        16 eb 55 2c 88 24        25200002107428
#' 05   8  order ref        00 00 00 00 00 00 00 04  4
#' 06   1  buysell          42                       B
#' 07   4  shares           00 00 2e 7c              11900
#' 08   8  stock            55 53 4f 20 20 20 20 20  USO
#' 09   4  price            00 01 fa 40              12.96
#' 10   4  attribution (ONLY in 'F', for the order below:) 56 49 52 54     VIRT
#' Order F
#' 46 10 6b 00 00 1d d7 8e aa af fb 00 00 00 00 00 02 b8 4e 42 00 00 00 64 49 50 20 20 20 20 20 20 00 06 aa a4 56 49 52 54
#'
#' Combined Order, for the test below (Take example Order 'A' set msg type to 'F' add MPID attribution)
#' 46 20 2c 00 00 16 eb 55 2c 88 24 00 00 00 00 00 00 00 04 42 00 00 2e 7c 55 53 4f 20 20 20 20 20 00 01 fa 40 56 49 52 54

# create an order
dt_f <- data.table(
  msg_type = "F", stock_locate = 8236L, tracking_number = 0L,
  timestamp = as.int64(25200002107428), order_ref = as.int64(4), buy = TRUE,
  shares = 11900L, stock = "USO", price = 12.96, mpid = "VIRT"
)

# the expected value
hex_f <- paste(
  "00 00", # first 2 empty bytes
  "46", # message type 'F'
  "20 2c", # stock locate 8263
  "00 00", # tracking number 0
  "16 eb 55 2c 88 24", # timestamp 25200002107428
  "00 00 00 00 00 00 00 04", # order ref 4
  "42", # buy == TRUE -> 'B'
  "00 00 2e 7c", # shares 11900
  "55 53 4f 20 20 20 20 20", # stock 'USO     ' (length 8)
  "00 01 fa 40", # price 129600 (12.96)
  "56 49 52 54" # mpid/attribution 'VIRT
)

test_hex_to_dt(hex_f, dt_f, dbg_hex_to_orders)

###############
## two messages
dt_f_2 <- rbindlist(list(dt_f, dt_f))
hx_f_2 <- paste(hex_f, hex_f)
test_hex_to_dt(hx_f_2, dt_f_2, dbg_hex_to_orders)

### More complicated Example

hex_mult_orders <- paste(
  "00 00 41 20 2c 00 00 16 eb 55 2c 88 24 00 00 00 00 00 00 00 04 42 00 00 2e",
  "7c 55 53 4f 20 20 20 20 20 00 01 fa 40 00 00 41 08 ac 00 00 16 eb 55 2c 94",
  "65 00 00 00 00 00 00 7c f5 42 00 00 3a 98 44 57 54 20 20 20 20 20 00 00 84",
  "08 00 00 41 1f a5 00 00 16 eb 55 2c e8 93 00 00 00 00 00 00 00 08 42 00 00",
  "05 dc 55 43 4f 20 20 20 20 20 00 03 35 7c 00 00 41 20 2c 00 00 16 eb 55 2d",
  "30 74 00 00 00 00 00 00 00 0c 53 00 00 2e 7c 55 53 4f 20 20 20 20 20 00 01",
  "fb 6c 00 00 41 08 ac 00 00 16 eb 55 2d 55 dd 00 00 00 00 00 00 7c f9 53 00",
  "00 3a 98 44 57 54 20 20 20 20 20 00 00 84 d0 00 00 41 1f a5 00 00 16 eb 55",
  "2d 7c 90 00 00 00 00 00 00 00 10 53 00 00 05 dc 55 43 4f 20 20 20 20 20 00",
  "03 38 38 00 00 41 1f e7 00 00 16 eb 55 2d b0 8f 00 00 00 00 00 00 00 14 42",
  "00 00 05 14 55 4e 47 20 20 20 20 20 00 02 9a cc 00 00 41 07 c3 00 00 16 eb",
  "55 2d ed 7e 00 00 00 00 00 00 7c fd 53 00 00 00 64 44 47 41 5a 20 20 20 20",
  "00 1a db 00 00 00 41 1f e7 00 00 16 eb 55 2e 15 b0 00 00 00 00 00 00 00 18",
  "53 00 00 05 14 55 4e 47 20 20 20 20 20 00 02 9d 24 00 00 41 20 2c 00 00 16",
  "eb 55 2e 9d 6a 00 00 00 00 00 00 00 1c 42 00 00 2e 7c 55 53 4f 20 20 20 20",
  "20 00 01 f9 dc")

test_hex_to_dt(hex_mult_orders, hex_to_dt_func = dbg_hex_to_orders)

################################################################################
########### Trade Messages
################################################################################

#' Messages 'P', 'Q', and 'B'
#' Example Order 'P'
#' 50 1f bc 00 02 17 fa ea ab e4 15 00 00 00 00 00 00 00 00 42 00 00 00 64 55 47 41 5a 20 20 20 20 00 0b 9a b4 00 00 00 00 00 00 45 8b
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 06 >> >> >> 07 >> >> >> >> >> >> >> 08 >> >> >> 09 >> >> >> >> >> >> >> 10
#' id size name             hex value
#' 01   1  message type     50                         P
#' 02   2  stock locate     1f bc                      8124
#' 03   2  tracking number  00 02                      2
#' 04   6  timestamp        17 fa ea ab e4 15          26366446396437
#' 05   8  order ref        00 00 00 00 00 00 00 00    0
#' 06   1  buysell          42                         B
#' 07   4  shares           00 00 00 64                100
#' 08   8  stock            55 47 41 5a 20 20 20 20    UGAZ
#' 09   4  price            00 0b 9a b4                76.05
#' 10   8  match number     00 00 00 00 00 00 45 8b    17803
#' 11   1  cross type (only 'Q') 43                    C
#' NOTE 05-09 and 11 are left out for type 'B'

#' TODO: NOT TESTED: Q AND B
#'

hex_p <- paste(
  "00 00 50 1f bc 00 02 17 fa ea ab e4 15 00 00 00 00 00 00 00 00 42 00 00 00",
  "64 55 47 41 5a 20 20 20 20 00 0b 9a b4 00 00 00 00 00 00 45 8b"
)
dt_p <- data.table(
  msg_type = "P",
  stock_locate = 8124,
  tracking_number = 2,
  timestamp = as.int64(26366446396437),
  order_ref = as.int64(0),
  buy = TRUE,
  shares = 100,
  stock = "UGAZ",
  price = 76.05,
  match_number = as.int64(17803),
  cross_type = NA_character_
)

test_hex_to_dt(hex_p, dt_p, dbg_hex_to_trades)


###############
## two messages
dt_p_2 <- rbindlist(list(dt_p, dt_p))
hx_p_2 <- paste(hex_p, hex_p)
test_hex_to_dt(hx_p_2, dt_p_2, dbg_hex_to_trades)

################################################################################
########### Modification Messages
################################################################################

#' Messages 'E', 'C', 'X', 'D', and 'U'
#' Example Order 'E'
#' 45 1f bc 00 02 16 eb 55 dc dc 34 00 00 00 00 00 00 00 38 00 00 00 64 00 00 00 00 00 00 45 83
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 >> >> >> 06 >> >> >> >> >> >> >> 07
#' id size name             hex value
#' 01   1  message type     45                         E
#' 02   2  stock locate     1f bc                      8124
#' 03   2  tracking number  00 02                      2
#' 04   6  timestamp        16 eb 55 dc dc 34          25200013663284
#' 05   8  order ref        00 00 00 00 00 00 00 38    56
#' 06   4  shares           00 00 00 64                100
#' 07   8  match number     00 00 00 00 00 00 45 83    17795
#' Example Order C:
#' 43 1f 5c 00 02 1f 1a d7 26 d9 cb 00 00 00 00 00 0e c9 70 00 00 00 64 00 00 00 00 00 00 48 5c 59 00 08 16 50
#' Example Order X:
#' 58 13 fa 00 00 1f 1a d7 85 88 9d 00 00 00 00 00 04 67 e3 00 00 00 64
#' Example Order D:
#' 44 22 33 00 00 1f 1a d7 8b a6 20 00 00 00 00 00 0e 9b 90
#' Example Order U
#' 55 1f bc 00 00 1f 1a d7 e5 01 66 00 00 00 00 00 0e a4 f0 00 00 00 00 00 0e cb b0 00 00 00 64 00 0b d8 6c

hex_e <- "00 00 45 1f bc 00 02 16 eb 55 dc dc 34 00 00 00 00 00 00 00 38 00 00 00 64 00 00 00 00 00 00 45 83"
hex_c <- "00 00 43 1f 5c 00 02 1f 1a d7 26 d9 cb 00 00 00 00 00 0e c9 70 00 00 00 64 00 00 00 00 00 00 48 5c 59 00 08 16 50"
hex_x <- "00 00 58 13 fa 00 00 1f 1a d7 85 88 9d 00 00 00 00 00 04 67 e3 00 00 00 64"
hex_d <- "00 00 44 22 33 00 00 1f 1a d7 8b a6 20 00 00 00 00 00 0e 9b 90"
hex_u <- "00 00 55 1f bc 00 00 1f 1a d7 e5 01 66 00 00 00 00 00 0e a4 f0 00 00 00 00 00 0e cb b0 00 00 00 64 00 0b d8 6c"

hex_mod_all <- paste(hex_e, hex_c, hex_x, hex_d, hex_u)

# Modification E
dt_e <- data.table(
  msg_type = "E", stock_locate = 8124, tracking_number = 2,
  timestamp = as.int64(25200013663284), order_ref = as.int64(56), shares = 100,
  match_number = as.int64(17795), printable = NA, price = NA_real_,
  new_order_ref = as.int64(NA)
)
test_hex_to_dt(hex_e, dt_e, dbg_hex_to_modifications)

# Modification C
dt_c <- data.table(
  msg_type = "C", stock_locate = 8028, tracking_number = 2,
  timestamp = as.int64(34200139258315), order_ref = as.int64(969072),
  shares = 100, match_number = as.int64(18524),
  printable = FALSE, price = 53.0, new_order_ref = as.int64(NA)
)
test_hex_to_dt(hex_c, dt_c, dbg_hex_to_modifications)

# Modification X
dt_x <- data.table(
  msg_type = "X", stock_locate = 5114, tracking_number = 0,
  timestamp = as.int64(34200145463453), order_ref = as.int64(288739),
  shares = 100, match_number = as.int64(NA),
  printable = NA, price = NA_real_, new_order_ref = as.int64(NA)
)
test_hex_to_dt(hex_x, dt_x, dbg_hex_to_modifications)

# Modification D
dt_d <- data.table(
  msg_type = "D", stock_locate = 8755, tracking_number = 0,
  timestamp = as.int64(34200145864224), order_ref = as.int64(957328),
  shares = NA_integer_, match_number = as.int64(NA),
  printable = NA, price = NA_real_, new_order_ref = as.int64(NA)
)
test_hex_to_dt(hex_d, dt_d, dbg_hex_to_modifications)

# Modification U
dt_u <- data.table(
  msg_type = "U", stock_locate = 8124, tracking_number = 0,
  timestamp = as.int64(34200151720294), order_ref = as.int64(959728),
  shares = 100, match_number = as.int64(NA),
  printable = NA, price = 77.63, new_order_ref = as.int64(969648)
)
test_hex_to_dt(hex_u, dt_u, dbg_hex_to_modifications)

# Test all Modifications
dt_mods_all <- rbindlist(list(dt_e, dt_c, dt_x, dt_d, dt_u))

test_hex_to_dt(dt = dt_mods_all, hex_to_dt_func = dbg_hex_to_modifications)

################################################################################
########### System Event Messages
################################################################################
#' Messages 'S'
#' 53 00 00 00 00 0a 2d f4 92 1d 67 4f
#' 01 >> 02 >> 03 >> >> >> >> >> 04 05
#' id size name             hex value
#' 01   1  message type     53                         S
#' 02   2  stock locate     00 00                      0
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 2d f4 92 1d 67          11192493022567
#' 05   1  event code       4f                         O

hex_s <- "00 00 53 00 00 00 00 0a 2d f4 92 1d 67 4f"
dt_s <- data.table(msg_type = "S", stock_locate = 0, tracking_number = 0,
                   timestamp = as.int64(11192493022567), event_code = "O")
test_hex_to_dt(hex_s, dt_s, dbg_hex_to_system_events)

################################################################################
########### Stock Directory Messages
################################################################################
#' Messages 'R'
#' 52 00 01 00 00 0a 66 a0 e0 dc 44 41 20 20 20 20 20 20 20 4e 20 00 00 00 64 4e 43 5a 20 50 4e 20 31 4e 00 00 00 00 4e
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 06 07 >> >> >> 08 09 10 >> 11 12 13 14 15 16 >> >> >> 17 18
#' id size name             hex value
#' 01   1  message type     52                         H
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e0 dc 44          11435902032964
#' 05   8  stock            41 20 20 20 20 20 20 20    A
#' 06   1  market_category  4e                         N
#' 07   1  financial status 20                         ' '
#' 08   4  lot_size         00 00 00 64                100
#' 09   1  round_lots_only  4e                         N
#' 10   1  issue_classifciation 43                     C
#' 11   2  issue subtype    5a 20                      'Z '
#' 12   1  authenticity     50                         P
#' 13   1  short_sell_closeout 4e                      N
#' 14   1  ipo_flag         20                         ' '
#' 15   1  luld_price_tier  31                         1
#' 16   1  etp_flag         4e                         N
#' 17   4  etp_leverage     00 00 00 00                0
#' 18   1  inverse          4e                         N

hex_r <- paste(
  "00 00 52 00 01 00 00 0a 66 a0 e0 dc 44 41 20 20 20 20 20 20 20 4e 20 00 00",
  "00 64 4e 43 5a 20 50 4e 20 31 4e 00 00 00 00 4e"
)
dt_r <- data.table(
  msg_type = "R", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902032964), stock = "A", market_category = "N",
  financial_status = " ", lot_size = 100L, round_lots_only = FALSE,
  issue_classification = "C", issue_subtype = "Z", authentic = TRUE,
  short_sell_closeout = FALSE, ipo_flag = NA, luld_price_tier = "1",
  etp_flag = FALSE, etp_leverage = 0L, inverse = FALSE
)
test_hex_to_dt(hex_r, dt_r, dbg_hex_to_stock_directory)


################################################################################
########### Trading Status
################################################################################
#' Messages 'H' and 'h'
#' 48 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20 54 20 20 20 20 20
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 06 07 >> >> >> 08
#' id size name             hex value
#' 01   1  message type     52                         H
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   8  stock            41 20 20 20 20 20 20 20    A
#' 06   1  trading state    54                         T
#' 07   1  reserved         20                         ' '
#' 08   4  reason           20 20 20 20                '    '

hex_h <- "00 00 48 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20 54 20 20 20 20 20"
dt_h <- data.table(
  msg_type = "H", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902304189), stock = "A", trading_state = "T",
  reserved = " ", reason = "", market_code = NA_character_, operation_halted = NA
)

test_hex_to_dt(hex_h, dt_h, dbg_hex_to_trading_status)


################################################################################
########### Reg SHO
################################################################################
#' Messages 'Y'
#' 59 00 01 00 00 0a 66 a0 e5 4a a2 41 20 20 20 20 20 20 20 30
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 06
#' id size name             hex value
#' 01   1  message type     52                         H
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   8  stock            41 20 20 20 20 20 20 20    A
#' 06   1  reg sho action   30                         0

hex_y <- "00 00 59 00 01 00 00 0a 66 a0 e5 4a a2 41 20 20 20 20 20 20 20 30"
dt_y <- data.table(
  msg_type = "Y", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902323362), stock = "A", regsho_action = "0"
)
test_hex_to_dt(hex_y, dt_y, dbg_hex_to_reg_sho)


################################################################################
########### Market Participants State
################################################################################
#' Messages 'L'
#' 4c 00 01 00 00 0a 67 e1 75 f8 65 43 44 52 47 41 20 20 20 20 20 20 20 59 4e 41
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> 05 >> >> >> >> >> >> >> 06
#' id size name             hex value
#' 01   1  message type     52                         L
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   4  mpid             43 44 52 47 41             CDRGA
#' 06   8  stock            41 20 20 20 20 20 20 20    A
#' 07   1  primary_mm       59                         Y
#' 08   1  mm_mode          4e                         N
#' 09   1  participant_state 41                        A

hex_l <- "00 00 4c 00 01 00 00 0a 67 e1 75 f8 65 43 44 52 47 41 20 20 20 20 20 20 20 59 4e 41"
dt_l <- data.table(
  msg_type = "L", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902304189), mpid = "CDRG", stock = "A",
  primary_mm = TRUE, mm_mode = "N", participant_state = "A"
)
test_hex_to_dt(hex_l, dt_l, dbg_hex_to_market_participant_states)

################################################################################
########### MWCB
################################################################################
#' Messages 'V' and 'W'
#' 56 00 00 00 00 16 ec bc 74 bd 18 00 00 00 46 28 21 94 40 00 00 00 41 a1 6a b8 40 00 00 00 3c 59 95 62 40
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 >> >> >> >> >> >> >> 06 >> >> >> >> >> >> >> 07
#' id size name             hex value
#' 01   1  message type     52                         H
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   8  level 1          00 00 00 46 28 21 94 40    3013.21
#' 06   8  level 2          00 00 00 41 a1 6a b8 40    2818.81
#' 07   8  level 3          00 00 00 3c 59 95 62 40    2592.01
#' or for 'W'
#' 05   1  breached level   31                         1

hex_v <- paste(
  "00 00 56 00 00 00 00 16 ec bc 74 bd 18 00 00 00 46 28 21 94 40 00 00 00 41",
  "a1 6a b8 40 00 00 00 3c 59 95 62 40"
)
dt_v <- data.table(
  msg_type = "V", stock_locate = 0L, tracking_number = 0L,
  timestamp = as.int64(11435902304189), level1 = 3013.21, level2 = 2818.81,
  level3 = 2592.01, breached_level = NA_integer_
)
test_hex_to_dt(hex_v, dt_v, dbg_hex_to_mwcb)

################################################################################
########### IPO
################################################################################
#' Messages 'K'
#' 4b 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20 00 ca 1c ee 41 00 01 fa 40
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 >> >> >> 06 07 >> >> >> 08
#' id size name             hex value
#' 01   1  message type     4B                         K
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   8  stock            41 20 20 20 20 20 20 20    A
#' 06   4  realease_time    00 ca 1c ee                1325678
#' 07   1  release_qualifier 41                        A
#' 08   4  ipo_price(4)     00 01 fa 40                12.96

hex_k <- "00 00 4b 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20 00 ca 1c ee 41 00 01 fa 40"
dt_k <- data.table(
  msg_type = "K", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902304189), stock = "A", release_time = 13245678L,
  release_qualifier = "A", ipo_price = 12.96
)
test_hex_to_dt(hex_k, dt_k, dbg_hex_to_ipo)

################################################################################
########### LULD
################################################################################
#' Messages 'J'
#' 4a 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20 00 0b 9a b4 00 0b 1e 5a 00 0b f2 1e  00 00 00 10
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 >> >> >> 06 >> >> >> 07 >> >> >> 08 >> >> >> 09
#' id size name             hex value
#' 01   1  message type     4a                         J
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   8  Stock            41 20 20 20 20 20 20 20    A
#' 06   4  reference_price  00 0b 9a b4                76.05
#' 07   4  upper_price      00 0b 1e 5a                72.8666
#' 08   4  lower_price      00 0b f2 1e                78.2878
#' 09   4  extension        00 00 00 10                16

hex_j <- paste("00 00 4a 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20",
               "00 0b 9a b4 00 0b 1e 5a 00 0b f2 1e 00 00 00 10")
dt_j <- data.table(
  msg_type = "J", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902304189), stock = "A", reference_price = 76.05,
  upper_price = 72.8666, lower_price = 78.2878, extension = 16L
)

test_hex_to_dt(hex_j, dt_j, dbg_hex_to_luld)

################################################################################
########### NOII
################################################################################
#' Messages 'I'
#' 49 00 01 00 00 0a 66 40 e4 ff bd 00 00 00 00 00 0f 1e 01 00 00 00 00 00 3a 71 50 42 41 20 20 20 20 20 20 20 00 0b f2 1e 00 0b 1e 5a 00 0b 9a b4 43 35
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 >> >> >> >> >> >> >> 06 07 >> >> >> >> >> >> >> 08 >> >> >> 09 >> >> >> 10 >> >> >> 11 12 13
#' id size name             hex value
#' 01   1  message type     49                         I
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        0a 66 a0 e4 ff bd          11435902304189
#' 05   8  paired shares    00 00 00 00 00 0f 1e 01    990721
#' 06   8  imbalance shares 00 00 00 00 00 3a 71 50    3830096
#' 07   1  imbalance direction 42                      B
#' 08   8  stock            41 20 20 20 20 20 20 20    A
#' 09   4  far_price (4)    00 0b f2 1e                78.2878
#' 10   4  near_price (4)   00 0b 1e 5a                72.8666
#' 11   4  ref_price (4)    00 0b 9a b4                76.05
#' 12   1  cross_type       43                         C
#' 13   1  variation_indicator 35                      "5"

hex_i <- paste(
  "00 00 49 00 01 00 00 0a 66 40 e4 ff bd 00 00 00 00 00 0f 1e 01 00 00 00 00",
  "00 3a 71 50 42 41 20 20 20 20 20 20 20 00 0b f2 1e 00 0b 1e 5a 00 0b 9a b4",
  "43 35"
)
dt_i <- data.table(
  msg_type = "I", stock_locate = 1L, tracking_number = 0L,
  timestamp = as.int64(11435902304189), paired_shares = as.int64(990721),
  imbalance_shares = as.int64(3830096), imbalance_direction = "B", stock = "A",
  far_price = 78.2878, near_price = 72.8666, reference_price = 76.05,
  cross_type = "C", variation_indicator = "5"
)

test_hex_to_dt(hex_i, dt_i, dbg_hex_to_noii)

################################################################################
########### RPII
################################################################################
#' Messages 'N'
#' 4e 02 9c 00 00 1a 31 a5 21 99 bc 41 58 53 4d 20 20 20 20 42
#' 01 >> 02 >> 03 >> >> >> >> >> 04 >> >> >> >> >> >> >> 05 06
#' id size name             hex value
#' 01   1  message type     4e                         N
#' 02   2  stock locate     00 01                      1
#' 03   2  tracking number  00 00                      0
#' 04   6  timestamp        1a 31 a5 21 99 bc          28800526162364
#' 05   8  stock            41 58 53 4d 20 20 20 20    'AXSM    '
#' 06   1  interest flag    42                         B

hex_n <- "00 00 4e 02 9c 00 00 1a 31 a5 21 99 bc 41 58 53 4d 20 20 20 20 42"
dt_n <- data.table(
  msg_type = "N", stock_locate = 668L, tracking_number = 0L,
  timestamp = as.int64(28800526162364), stock = "AXSM", interest_flag = "B"
)

test_hex_to_dt(hex_n, dt_n, dbg_hex_to_rpii)
