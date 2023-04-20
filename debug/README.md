
# Debug Tools for `RITCH`

This document quickly outlines the debugging tools of the `RITCH`
library.

## Building

These tools are used for debugging and understanding the data format.
They are not shipped with the package itself but need to be sourced
independently.

If you want to play around with the tools, clone the git repository and
source the `debug/debug_tools.cpp` script:

``` r
Sys.setenv("PKG_LIBS" = "-lz")
Rcpp::sourceCpp("debug/debug_tools.cpp")
```

Note that `debug_tools.cpp` includes `../src/RITCH.h` as well as
`../src/MessageTypes.h` (relative from the `debug_tools.cpp` script), if
you have cloned the repository as is, it should work out of the box,
otherwise, make sure that the two header files are found.

## Debug Tools

- `dbg_get_message_length(msgs)` returns the size of Messages in bytes.
  Note that each message adds 2 bytes that are not used

``` r
dbg_get_message_length(c("A", "F"))
```

    ##  A  F 
    ## 38 42

- `dbg_itch_file(filename)` allows you to interactively list messages in
  a file You are asked for input inside the function, which can be:

  - msg type, e.g., `A`, `H`, `h` to see the next instance of that
    message type
  - numeric value, e.g., `3` to see the next N values

  For example:

``` r
file <- "20191230.BX_ITCH_50"
dbg_itch_file(file)
## Debugging File '20191230.BX_ITCH_50' (.gz-file? no)
## Usage:
## - Empty: next message
## - Number: for next N messages
## - Character: if valid message type, print the next message, e.g., 'A' for add order
## - non valid Character: exits the debugging tool
## Note: Bytes in parenthesis show the first two bytes, which are not used!
## Number of Messages:
## - 'S': 6
## - 'R': 8906
## - 'H': 8961
## - 'Y': 9013
## - 'L': 6171
## - 'V': 1
## - 'W': 0
## - 'K': 0
## - 'J': 0
## - 'h': 0
## - 'A': 12210139
## - 'F': 45058
## - 'E': 578839
## - 'C': 2686
## - 'X': 348198
## - 'D': 11821540
## - 'U': 1741672
## - 'P': 134385
## - 'Q': 0
## - 'B': 0
## - 'I': 0
## - 'N': 2241182
## =============================
## 'S' (len 2 + 12) idx    0 at offset     0 (0x0000) | (00 0c) 53 00 00 00 00 0a 2d f4 92 1d 67 4f
#RITCH> 3
## Showing next 3 messages
## 'R' (len 2 + 39) idx    1 at offset    14 (0x000e) | (00 27) 52 00 01 00 00 0a 66 a0 e0 dc 44 41 20 20 20 20 20 20 20 4e 20 00 00 00 64 4e 43 5a 20 50 4e 20 31 4e 00 00 00 00 4e
## 'R' (len 2 + 39) idx    2 at offset    55 (0x0037) | (00 27) 52 00 02 00 00 0a 66 a0 e2 c8 6c 41 41 20 20 20 20 20 20 4e 20 00 00 00 64 4e 43 5a 20 50 4e 20 31 4e 00 00 00 01 4e
## 'H' (len 2 + 25) idx    3 at offset    96 (0x0060) | (00 19) 48 00 01 00 00 0a 66 a0 e4 ff bd 41 20 20 20 20 20 20 20 54 20 20 20 20 20
#RITCH> A
## Applied filter to message type 'A'
## 'A' (len 2 + 36) idx 32873 at offset 973915 (0xedc5b) | (00 24) 41 20 2c 00 00 16 eb 55 2c 88 24 00 00 00 00 00 00 00 04 42 00 00 2e 7c 55 53 4f 20 20 20 20 20 00 01 fa 40
#RITCH> q
## Stopping Printing Messages
```

- `dbg_hex_to_char(hex_string)` converts a hex value to character
- `dbg_hex_to_int(hex_string)` converts a hex value to integer
- `dbg_hex_to_dbl(hex_string)` converts a hex value to dbl

``` r
dbg_hex_to_char("52 49 54 43 48 20 20 20") # 'RITCH   '
```

    ## [1] "RITCH   "

``` r
dbg_hex_to_int("01 23 45 67") # 19088743
```

    ## integer64
    ## [1] 19088743

``` r
dbg_hex_to_dbl("00 01 fa 40") # 12.96
```

    ## [1] 12.96

- `dbg_hex_compare(x, y)` to get a quick comparison of two hex strings

``` r
x <- "00 01 02 03 04"
y <- "00 01 00 03 0a"
dbg_hex_compare(x, y)
```

    ##  idx |    x |    y | diff
    ## -------------------------
    ##    1 | 0x00 | 0x00 |     
    ##    2 | 0x01 | 0x01 |     
    ##    3 | 0x02 | 0x00 |  XXX
    ##    4 | 0x03 | 0x03 |     
    ##    5 | 0x04 | 0x0a |  XXX

- `dbg_hex_count_messages(hex_string)` counts the number of messages by
  type in a hex string

``` r
incomplete_hex_string <- "00 00 53" # . . S
dbg_hex_count_messages(incomplete_hex_string)
```

    ##     msg_type count
    ##  1:        S     1
    ##  2:        R     0
    ##  3:        H     0
    ##  4:        Y     0
    ##  5:        L     0
    ##  6:        V     0
    ##  7:        W     0
    ##  8:        K     0
    ##  9:        J     0
    ## 10:        h     0
    ## 11:        A     0
    ## 12:        F     0
    ## 13:        E     0
    ## 14:        C     0
    ## 15:        X     0
    ## 16:        D     0
    ## 17:        U     0
    ## 18:        P     0
    ## 19:        Q     0
    ## 20:        B     0
    ## 21:        I     0
    ## 22:        N     0
    ##     msg_type count

- `dbg_hex_to_*()` to convert hexadecimal strings to message
  `data.table`s (\* can be `orders`, `trades`, `modifications`,
  `system_events`, `stock_directory`, `trading_status`, `reg_sho`,
  `market_participant_states`, `mwcb`, `ipo`, `luld`, `noii`, or `rpii`)

``` r
hex_string <- paste(
  "00 00", # first 2 empty nibbles
  "46", # message type 'F'
  "20 2c", # stock locate 8236
  "00 00", # tracking number 0
  "16 eb 55 2c 88 24", # timestamp 25200002107428
  "00 00 00 00 00 00 00 04", # order ref 4
  "42", # buy == TRUE -> 'B'
  "00 00 2e 7c", # shares 11900
  "55 53 4f 20 20 20 20 20", # stock 'USO     ' (length 8)
  "00 01 fa 40", # price 129600 (12.96)
  "56 49 52 54" # mpid/attribution 'VIRT
)

dbg_hex_to_orders(hex_string)
```

    ##    msg_type stock_locate tracking_number      timestamp order_ref  buy shares
    ## 1:        F         8236               0 25200002107428         4 TRUE  11900
    ##    stock price mpid
    ## 1:   USO 12.96 VIRT

- `dbg_messages_to_hex()` to convert the message `data.table`s to a
  hexadecimal string

``` r
od <- data.table::data.table(
  msg_type = "F",
  stock_locate = 8236L,
  tracking_number = 0L,
  timestamp = bit64::as.integer64(25200002107428),
  order_ref = bit64::as.integer64(4),
  buy = TRUE,
  shares = 11900L,
  stock = "USO",
  price = 12.96,
  mpid = "VIRT"
)
hex_order <- dbg_messages_to_hex(od)
hex_order
```

    ## [1] "00 00 46 20 2c 00 00 16 eb 55 2c 88 24 00 00 00 00 00 00 00 04 42 00 00 2e 7c 55 53 4f 20 20 20 20 20 00 01 fa 40 56 49 52 54"

``` r
# convert back to a data.table and see if they are identical
od2 <- dbg_hex_to_orders(hex_order)
all.equal(od, od2)
```

    ## [1] TRUE
