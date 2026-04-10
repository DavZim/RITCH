# Counts the messages of an ITCH-file

Counts the messages of an ITCH-file

## Usage

``` r
count_messages(
  file,
  add_meta_data = FALSE,
  buffer_size = -1,
  quiet = FALSE,
  force_gunzip = FALSE,
  gz_dir = tempdir(),
  force_cleanup = TRUE
)

count_orders(x)

count_trades(x)

count_modifications(x)

count_system_events(x)

count_stock_directory(x)

count_trading_status(x)

count_reg_sho(x)

count_market_participant_states(x)

count_mwcb(x)

count_ipo(x)

count_luld(x)

count_noii(x)

count_rpii(x)
```

## Arguments

- file:

  the path to the input file, either a gz-file or a plain-text file

- add_meta_data:

  if the meta-data of the messages should be added, defaults to FALSE

- buffer_size:

  the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have
  a large amount of RAM, 1e9 (1GB) might be faster

- quiet:

  if TRUE, the status messages are supressed, defaults to FALSE

- force_gunzip:

  only applies if file is a gz-file and a file with the same (gunzipped)
  name already exists. if set to TRUE, the existing file is overwritten.
  Default value is FALSE

- gz_dir:

  a directory where the gz archive is extracted to. Only applies if file
  is a gz archive. Default is
  [`tempdir()`](https://rdrr.io/r/base/tempfile.html).

- force_cleanup:

  only applies if file is a gz-file. If force_cleanup=TRUE, the
  gunzipped raw file will be deleted afterwards.

- x:

  a file or a data.table containing the message types and the counts, as
  outputted by `count_messages`

## Value

a data.table containing the message-type and their counts for
`count_messages` or an integer value for the other functions.

## Details

- `count_orders`: Counts order messages. Message type `A` and `F`

&nbsp;

- `count_trades`: Counts trade messages. Message type `P`, `Q` and `B`

&nbsp;

- `count_modifications`: Counts order modification messages. Message
  type `E`, `C`, `X`, `D`, and `U`

&nbsp;

- `count_system_events`: Counts system event messages. Message type `S`

&nbsp;

- `count_stock_directory`: Counts stock trading messages. Message type
  `R`

&nbsp;

- `count_trading_status`: Counts trading status messages. Message type
  `H` and `h`

&nbsp;

- `count_reg_sho`: Counts messages regarding reg SHO. Message type `Y`

&nbsp;

- `count_market_participant_states`: Counts messages regarding the
  status of market participants. Message type `L`

&nbsp;

- `count_mwcb`: Counts messages regarding Market-Wide-Circuit-Breakers
  (MWCB). Message type `V` and `W`

&nbsp;

- `count_ipo`: Counts messages regarding IPOs. Message type `K`

&nbsp;

- `count_luld`: Counts messages regarding LULDs (limit up-limit down)
  auction collars. Message type `J`

&nbsp;

- `count_noii`: Counts Net Order Imbalance Indicatio (NOII) messages.
  Message type `I`

&nbsp;

- `count_rpii`: Counts Retail Price Improvement Indicator (RPII)
  messages. Message type `N`

## Examples

``` r
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
count_messages(file)
#> [Counting]   12,012 total messages found
#> [Converting] to data.table
#> [Done]       in 0.00 secs at 708.00MB/s
#>     msg_type count
#>       <char> <i64>
#>  1:        S     6
#>  2:        R     3
#>  3:        H     3
#>  4:        Y     0
#>  5:        L     0
#>  6:        V     0
#>  7:        W     0
#>  8:        K     0
#>  9:        J     0
#> 10:        h     0
#> 11:        A  4997
#> 12:        F     3
#> 13:        E   198
#> 14:        C     0
#> 15:        X    45
#> 16:        D  1745
#> 17:        U    12
#> 18:        P  5000
#> 19:        Q     0
#> 20:        B     0
#> 21:        I     0
#> 22:        N     0
#>     msg_type count
#>       <char> <i64>
count_messages(file, add_meta_data = TRUE, quiet = TRUE)
#>     msg_type count                 msg_class
#>       <char> <i64>                    <char>
#>  1:        S     6             system_events
#>  2:        R     3           stock_directory
#>  3:        H     3            trading_status
#>  4:        Y     0                   reg_sho
#>  5:        L     0 market_participant_states
#>  6:        V     0                      mwcb
#>  7:        W     0                      mwcb
#>  8:        K     0                       ipo
#>  9:        J     0                      luld
#> 10:        h     0            trading_status
#> 11:        A  4997                    orders
#> 12:        F     3                    orders
#> 13:        E   198             modifications
#> 14:        C     0             modifications
#> 15:        X    45             modifications
#> 16:        D  1745             modifications
#> 17:        U    12             modifications
#> 18:        P  5000                    trades
#> 19:        Q     0                    trades
#> 20:        B     0                    trades
#> 21:        I     0                      noii
#> 22:        N     0                      rpii
#>     msg_type count                 msg_class
#>       <char> <i64>                    <char>
#>                                      msg_name  doc_nr
#>                                        <char>  <char>
#>  1:                      System Event Message     4.1
#>  2:                           Stock Directory   4.2.1
#>  3:                      Stock Trading Action   4.2.2
#>  4:                       Reg SHO Restriction   4.2.3
#>  5:               Market Participant Position   4.2.4
#>  6:                MWCB Decline Level Message 4.2.5.1
#>  7:                       MWCB Status Message 4.2.5.2
#>  8:                 IPO Quoting Period Update   4.2.6
#>  9:                       LULD Auction Collar   4.2.7
#> 10:                          Operational Halt   4.2.8
#> 11:                         Add Order Message   4.3.1
#> 12:      Add Order - MPID Attribution Message   4.3.2
#> 13:                    Order Executed Message   4.4.1
#> 14: Order Executed Message With Price Message   4.4.2
#> 15:                      Order Cancel Message   4.4.3
#> 16:                      Order Delete Message   4.4.4
#> 17:                     Order Replace Message   4.4.5
#> 18:                 Trade Message (Non-Cross)   4.5.1
#> 19:                       Cross Trade Message   4.5.2
#> 20:                      Broken Trade Message   4.5.3
#> 21:                              NOII Message     4.6
#> 22:                   Retail Interest Message     4.7
#>                                      msg_name  doc_nr
#>                                        <char>  <char>

# file can also be a .gz file
gz_file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
count_messages(gz_file, quiet = TRUE)
#>     msg_type count
#>       <char> <i64>
#>  1:        S     6
#>  2:        R     3
#>  3:        H     3
#>  4:        Y     0
#>  5:        L     0
#>  6:        V     0
#>  7:        W     0
#>  8:        K     0
#>  9:        J     0
#> 10:        h     0
#> 11:        A  4997
#> 12:        F     3
#> 13:        E   198
#> 14:        C     0
#> 15:        X    45
#> 16:        D  1745
#> 17:        U    12
#> 18:        P  5000
#> 19:        Q     0
#> 20:        B     0
#> 21:        I     0
#> 22:        N     0
#>     msg_type count
#>       <char> <i64>

# count only a specific class
msg_count <- count_messages(file, quiet = TRUE)

# either count based on a given data.table outputted by count_messages
count_orders(msg_count)
#> [1] 5000

# or count orders from a file and not from a msg_count
count_orders(file)
#> [1] 5000

### Specific class count functions are:
count_orders(msg_count)
#> [1] 5000
count_trades(msg_count)
#> [1] 5000
count_modifications(msg_count)
#> [1] 2000
count_system_events(msg_count)
#> [1] 6
count_stock_directory(msg_count)
#> [1] 3
count_trading_status(msg_count)
#> [1] 3
count_reg_sho(msg_count)
#> [1] 0
count_market_participant_states(msg_count)
#> [1] 0
count_mwcb(msg_count)
#> [1] 0
count_ipo(msg_count)
#> [1] 0
count_luld(msg_count)
#> [1] 0
count_noii(msg_count)
#> [1] 0
count_rpii(msg_count)
#> [1] 0
```
