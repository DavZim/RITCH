# Reads certain messages of an ITCH-file into a data.table

For faster file-reads (at the tradeoff of increased memory usages), you
can increase the `buffer_size` to 1GB (1e9) or more.

If you access the same file multiple times, you can provide the message
counts as outputted from
[`count_messages()`](https://davzim.github.io/RITCH/reference/count_functions.md)
to the `n_max` argument, this allows skipping one pass over the file per
read instruction.

If you need to read in multiple message classes, you can specify
multiple message classes to `read_itch`, which results in only a single
file pass.

If the file is too large to be loaded into the workspace at once, you
can specify different `skip` and `n_max` to load only a specific range
of messages. Alternatively, you can filter certain messages to another
file using
[`filter_itch()`](https://davzim.github.io/RITCH/reference/filter_itch.md),
which is substantially faster than parsing a file and filtering it.

Note that all read functions allow both plain ITCH files as well as
gzipped files. If a gzipped file is found, it will look for a plain ITCH
file with the same name and use that instead. If this file is not found,
it will be created by unzipping the archive. Note that the unzipped file
is NOT deleted by default (the file will be created in the current
working directory). It might result in increased disk usage but reduces
future read times for that specific file. To force RITCH to delete
"temporary" files after uncompressing, use `force_cleanup = TRUE` (only
deletes the files if they were extracted before, does not remove the
archive itself).

## Usage

``` r
read_itch(
  file,
  filter_msg_class = NA,
  skip = 0,
  n_max = -1,
  filter_msg_type = NA_character_,
  filter_stock_locate = NA_integer_,
  min_timestamp = bit64::as.integer64(NA),
  max_timestamp = bit64::as.integer64(NA),
  filter_stock = NA_character_,
  stock_directory = NA,
  buffer_size = -1,
  quiet = FALSE,
  add_meta = TRUE,
  force_gunzip = FALSE,
  gz_dir = tempdir(),
  force_cleanup = TRUE
)

read_system_events(file, ..., add_descriptions = FALSE)

read_stock_directory(file, ..., add_descriptions = FALSE)

read_trading_status(file, ..., add_descriptions = FALSE)

read_reg_sho(file, ..., add_descriptions = FALSE)

read_market_participant_states(file, ..., add_descriptions = FALSE)

read_mwcb(file, ...)

read_ipo(file, ..., add_descriptions = FALSE)

read_luld(file, ...)

read_orders(file, ...)

read_modifications(file, ...)

read_trades(file, ...)

read_noii(file, ..., add_descriptions = FALSE)

read_rpii(file, ..., add_descriptions = FALSE)

get_orders(file, ...)

get_trades(file, ...)

get_modifications(file, ...)
```

## Arguments

- file:

  the path to the input file, either a gz-archive or a plain ITCH file

- filter_msg_class:

  a vector of classes to load, can be "orders", "trades",
  "modifications", ... see also
  [`get_msg_classes()`](https://davzim.github.io/RITCH/reference/get_msg_classes.md).
  Default value is to take all message classes.

- skip:

  Number of messages to skip before starting parsing messages, note the
  skip parameter applies to the specific message class, i.e., it would
  skip the messages for each type (e.g., skip the first 10 messages for
  each class).

- n_max:

  Maximum number of messages to parse, default is to read all values.
  Can also be a data.frame of msg_types and counts, as returned by
  [`count_messages()`](https://davzim.github.io/RITCH/reference/count_functions.md).
  Note the n_max parameter applies to the specific message class not the
  whole file.

- filter_msg_type:

  a character vector, specifying a filter for message types. Note that
  this can be used to only return 'A' orders for instance.

- filter_stock_locate:

  an integer vector, specifying a filter for locate codes. The locate
  codes can be looked up by calling `read_stock_directory()` or by
  downloading from NASDAQ by using
  [`download_stock_directory()`](https://davzim.github.io/RITCH/reference/download_stock_directory.md).
  Note that some message types (e.g., system events, MWCB, and IPO) do
  not use a locate code.

- min_timestamp:

  an 64 bit integer vector (see also
  [`bit64::as.integer64()`](https://rdrr.io/pkg/bit64/man/as.integer64.character.html))
  of minimum timestamp (inclusive). Note: min and max timestamp must be
  supplied with the same length or left empty.

- max_timestamp:

  an 64 bit integer vector (see also
  [`bit64::as.integer64()`](https://rdrr.io/pkg/bit64/man/as.integer64.character.html))
  of maxium timestamp (inclusive). Note: min and max timestamp must be
  supplied with the same length or left empty.

- filter_stock:

  a character vector, specifying a filter for stocks. Note that this a
  shorthand for the `filter_stock_locate` argument, as it tries to find
  the stock_locate based on the `stock_directory` argument, if this is
  not found, it will try to extract the stock directory from the file,
  else an error is thrown.

- stock_directory:

  A data.frame containing the stock-locate code relationship. As
  outputted by `read_stock_directory()`. Only used if `filter_stock` is
  set. To download the stock directory from NASDAQs server, use
  [`download_stock_directory()`](https://davzim.github.io/RITCH/reference/download_stock_directory.md).

- buffer_size:

  the size of the buffer in bytes, defaults to 1e8 (100 MB), if you have
  a large amount of RAM, 1e9 (1GB) might be faster

- quiet:

  if TRUE, the status messages are suppressed, defaults to FALSE

- add_meta:

  if TRUE, the date and exchange information of the file are added,
  defaults to TRUE

- force_gunzip:

  only applies if the input file is a gz-archive and a file with the
  same (gunzipped) name already exists. If set to TRUE, the existing
  file is overwritten. Default value is FALSE

- gz_dir:

  a directory where the gz archive is extracted to. Only applies if file
  is a gz archive. Default is
  [`tempdir()`](https://rdrr.io/r/base/tempfile.html).

- force_cleanup:

  only applies if the input file is a gz-archive. If force_cleanup=TRUE,
  the gunzipped raw file will be deleted afterwards. Only applies when
  the gunzipped raw file did not exist before.

- ...:

  Additional arguments passed to `read_itch`

- add_descriptions:

  add longer descriptions to shortened variables. The added information
  is taken from the official ITCH documentation see also
  [`open_itch_specification()`](https://davzim.github.io/RITCH/reference/open_itch_specification.md)

## Value

a data.table containing the messages

## Details

The details of the different messages types can be found in the official
ITCH specification (see also
[`open_itch_specification()`](https://davzim.github.io/RITCH/reference/open_itch_specification.md))

- `read_itch`: Reads a message class message, can also read multiple
  classes in one file-pass.

&nbsp;

- `read_system_events`: Reads system event messages. Message type `S`

&nbsp;

- `read_stock_directory`: Reads stock trading messages. Message type `R`

&nbsp;

- `read_trading_status`: Reads trading status messages. Message type `H`
  and `h`

&nbsp;

- `read_reg_sho`: Reads messages regarding reg SHO. Message type `Y`

&nbsp;

- `read_market_participant_states`: Reads messages regarding the status
  of market participants. Message type `L`

&nbsp;

- `read_mwcb`: Reads messages regarding Market-Wide-Circuit-Breakers
  (MWCB). Message type `V` and `W`

&nbsp;

- `read_ipo`: Reads messages regarding IPOs. Message type `K`

&nbsp;

- `read_luld`: Reads messages regarding LULDs (limit up-limit down)
  auction collars. Message type `J`

&nbsp;

- `read_orders`: Reads order messages. Message type `A` and `F`

&nbsp;

- `read_modifications`: Reads order modification messages. Message type
  `E`, `C`, `X`, `D`, and `U`

&nbsp;

- `read_trades`: Reads trade messages. Message type `P`, `Q` and `B`

&nbsp;

- `read_noii`: Reads Net Order Imbalance Indicatio (NOII) messages.
  Message type `I`

&nbsp;

- `read_rpii`: Reads Retail Price Improvement Indicator (RPII) messages.
  Message type `N`

For backwards compatability reasons, the following functions are
provided as well:

- `get_orders`: Redirects to `read_orders`

&nbsp;

- `get_trades`: Redirects to `read_trades`

&nbsp;

- `get_modifications`: Redirects to `read_modifications`

## References

<https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf>

## Examples

``` r
file <- system.file("extdata", "ex20101224.TEST_ITCH_50", package = "RITCH")
od <- read_orders(file, quiet = FALSE) # note quiet = FALSE is the default
#> [Counting]   num messages 12,012
#> [Counting]   num 'orders' messages 5,000
#> [Converting] to data.table
#> [Done]       in 0.15 secs at 3.12MB/s
tr <- read_trades(file, quiet = TRUE)

## Alternatively
od <- read_itch(file, "orders", quiet = TRUE)

ll <- read_itch(file, c("orders", "trades"), quiet = TRUE)

od
#>       msg_type stock_locate tracking_number      timestamp order_ref    buy
#>         <char>        <int>           <int>          <i64>     <i64> <lgcl>
#>    1:        A            2               0 31139052372053         0   TRUE
#>    2:        A            2               0 31141354532167       100   TRUE
#>    3:        F            2               0 32813425752711     84836   TRUE
#>    4:        A            2               0 32826656500150     87020  FALSE
#>    5:        A            2               0 32827351405783     87040  FALSE
#>   ---                                                                      
#> 4996:        A            1               0 57586688692484  88478593  FALSE
#> 4997:        A            2               0 57586792358162  82185144  FALSE
#> 4998:        A            1               0 57590573033724  88557297   TRUE
#> 4999:        A            2               0 57595196456803  82354728   TRUE
#> 5000:        A            2               0 57595326231183  82357176  FALSE
#>       shares  stock   price   mpid       date
#>        <int> <char>   <num> <char>     <POSc>
#>    1:   1000    BOB  5.3167        2010-12-24
#>    2:   1000    BOB  5.3167        2010-12-24
#>    3:    100    BOB  5.2917   VIRT 2010-12-24
#>    4:   1220    BOB  5.4167        2010-12-24
#>    5:   2000    BOB  5.4167        2010-12-24
#>   ---                                        
#> 4996:    100    ALC 23.1000        2010-12-24
#> 4997:    900    BOB  6.0833        2010-12-24
#> 4998:    100    ALC 23.0200        2010-12-24
#> 4999:   1200    BOB  6.0583        2010-12-24
#> 5000:   2000    BOB  6.0750        2010-12-24
#>                                  datetime exchange
#>                                <nanotime>   <char>
#>    1: 2010-12-24T08:38:59.052372053+00:00     TEST
#>    2: 2010-12-24T08:39:01.354532167+00:00     TEST
#>    3: 2010-12-24T09:06:53.425752711+00:00     TEST
#>    4: 2010-12-24T09:07:06.656500150+00:00     TEST
#>    5: 2010-12-24T09:07:07.351405783+00:00     TEST
#>   ---                                             
#> 4996: 2010-12-24T15:59:46.688692484+00:00     TEST
#> 4997: 2010-12-24T15:59:46.792358162+00:00     TEST
#> 4998: 2010-12-24T15:59:50.573033724+00:00     TEST
#> 4999: 2010-12-24T15:59:55.196456803+00:00     TEST
#> 5000: 2010-12-24T15:59:55.326231183+00:00     TEST
tr
#>       msg_type stock_locate tracking_number      timestamp order_ref    buy
#>         <char>        <int>           <int>          <i64>     <i64> <lgcl>
#>    1:        P            2               2 34210128591201         0   TRUE
#>    2:        P            2               2 34210355475120         0   TRUE
#>    3:        P            2               2 34210767188977         0   TRUE
#>    4:        P            2               2 34211127433476         0   TRUE
#>    5:        P            2               2 34212046014088         0   TRUE
#>   ---                                                                      
#> 4996:        P            1               2 57594215201805         0   TRUE
#> 4997:        P            1               6 57595050849140         0   TRUE
#> 4998:        P            1               4 57595051019659         0   TRUE
#> 4999:        P            1               2 57595052006081         0   TRUE
#> 5000:        P            3               2 57597526823001         0   TRUE
#>       shares  stock   price match_number cross_type       date
#>        <int> <char>   <num>        <i64>     <char>     <POSc>
#>    1:    200    BOB  5.3333        19447       <NA> 2010-12-24
#>    2:    300    BOB  5.3333        19451       <NA> 2010-12-24
#>    3:    100    BOB  5.3250        19493       <NA> 2010-12-24
#>    4:     47    BOB  5.3333        19515       <NA> 2010-12-24
#>    5:    200    BOB  5.3333        19547       <NA> 2010-12-24
#>   ---                                                         
#> 4996:      1    ALC 23.0800       728935       <NA> 2010-12-24
#> 4997:     28    ALC 23.0200       730245       <NA> 2010-12-24
#> 4998:     50    ALC 23.0200       730244       <NA> 2010-12-24
#> 4999:     65    ALC 23.1000       730243       <NA> 2010-12-24
#> 5000:    100   CHAR 22.0250       731883       <NA> 2010-12-24
#>                                  datetime exchange
#>                                <nanotime>   <char>
#>    1: 2010-12-24T09:30:10.128591201+00:00     TEST
#>    2: 2010-12-24T09:30:10.355475120+00:00     TEST
#>    3: 2010-12-24T09:30:10.767188977+00:00     TEST
#>    4: 2010-12-24T09:30:11.127433476+00:00     TEST
#>    5: 2010-12-24T09:30:12.046014088+00:00     TEST
#>   ---                                             
#> 4996: 2010-12-24T15:59:54.215201805+00:00     TEST
#> 4997: 2010-12-24T15:59:55.050849140+00:00     TEST
#> 4998: 2010-12-24T15:59:55.051019659+00:00     TEST
#> 4999: 2010-12-24T15:59:55.052006081+00:00     TEST
#> 5000: 2010-12-24T15:59:57.526823001+00:00     TEST
str(ll, max.level = 1)
#> List of 2
#>  $ orders:Classes ‘data.table’ and 'data.frame': 5000 obs. of  13 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr> 
#>  $ trades:Classes ‘data.table’ and 'data.frame': 5000 obs. of  14 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr> 

## additional options:

# take only subset of messages
od <- read_orders(file, skip = 3, n_max = 10)
#> [Note]       n_max overrides counting the messages. Number of messages may be off
#> [Filter]     skip: 3 n_max: 10 (4 - 13)
#> [Counting]   num 'orders' messages 20
#> [Converting] to data.table
#> [Done]       in 0.13 secs at 3.53MB/s

# a message count can be provided for slightly faster reads
msg_count <- count_messages(file, quiet = TRUE)
od <- read_orders(file, n_max = msg_count)
#> [Filter]     skip: 0 n_max: 5000 (1 - 5000)
#> [Counting]   num 'orders' messages 10,000
#> [Converting] to data.table
#> [Done]       in 0.14 secs at 3.42MB/s

## .gz archive functionality
# .gz archives will be automatically unzipped
gz_file <- system.file("extdata", "ex20101224.TEST_ITCH_50.gz", package = "RITCH")
od <- read_orders(gz_file)
#> [Decompressing] '/home/runner/work/_temp/Library/RITCH/extdata/ex20101224.TEST_ITCH_50.gz' to '/tmp/Rtmp0LDw4D/ex20101224.TEST_ITCH_50'
#> [Counting]   num messages 12,012
#> [Counting]   num 'orders' messages 5,000
#> [Converting] to data.table
#> [Done]       in 0.14 secs at 1.15MB/s
# force a decompress and delete the decompressed file afterwards
od <- read_orders(gz_file, force_gunzip = TRUE, force_cleanup = TRUE)
#> [Decompressing] '/home/runner/work/_temp/Library/RITCH/extdata/ex20101224.TEST_ITCH_50.gz' to '/tmp/Rtmp0LDw4D/ex20101224.TEST_ITCH_50'
#> [Counting]   num messages 12,012
#> [Counting]   num 'orders' messages 5,000
#> [Converting] to data.table
#> [Done]       in 0.13 secs at 1.20MB/s

## read_itch()
otm <- read_itch(file, c("orders", "trades"), quiet = TRUE)
str(otm, max.level = 1)
#> List of 2
#>  $ orders:Classes ‘data.table’ and 'data.frame': 5000 obs. of  13 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr> 
#>  $ trades:Classes ‘data.table’ and 'data.frame': 5000 obs. of  14 variables:
#>   ..- attr(*, ".internal.selfref")=<externalptr> 

## read_system_events()
se <- read_system_events(file, add_descriptions = TRUE, quiet = TRUE)
se
#> Key: <event_code>
#>    msg_type stock_locate tracking_number      timestamp event_code       date
#>      <char>        <int>           <int>          <i64>     <char>     <POSc>
#> 1:        S            0               0 68698845099321          C 2010-12-24
#> 2:        S            0               0 68390401688921          E 2010-12-24
#> 3:        S            0               0 57607158187591          M 2010-12-24
#> 4:        S            0               0 11202475298710          O 2010-12-24
#> 5:        S            0               0 34199261999747          Q 2010-12-24
#> 6:        S            0               0 25209371478776          S 2010-12-24
#>                               datetime exchange            event_name
#>                             <nanotime>   <char>                <char>
#> 1: 2010-12-24T19:04:58.845099321+00:00     TEST       End of Messages
#> 2: 2010-12-24T18:59:50.401688921+00:00     TEST   End of System Hours
#> 3: 2010-12-24T16:00:07.158187591+00:00     TEST   End of Market Hours
#> 4: 2010-12-24T03:06:42.475298710+00:00     TEST     Start of Messages
#> 5: 2010-12-24T09:29:59.261999747+00:00     TEST Start of Market Hours
#> 6: 2010-12-24T07:00:09.371478776+00:00     TEST Start of System Hours
#>                                                                                                                                                                               event_note
#>                                                                                                                                                                                   <char>
#> 1:                                                                                                                              This is always the last message sent in any trading day.
#> 2: It indicates that Nasdaq is now closed and will not accept any new orders today. It is still possible to receive Broken Trade messages and Order Delete messages after the End of Day
#> 3:                                                                                   This message is intended to indicate that Market Hours orders are no longer available for execution
#> 4:                                                                                 Outside of time stamp messages, the start of day message is the first message sent in any trading day
#> 5:                                                                                             This message is intended to indicate that Market Hours orders are available for execution
#> 6:                                                                                                        This message indicates that NASDAQ is open and ready to start accepting orders

## read_stock_directory()
sd <- read_stock_directory(file, add_descriptions = TRUE, quiet = TRUE)
sd
#> Key: <luld_price_tier>
#>    msg_type stock_locate tracking_number      timestamp  stock market_category
#>      <char>        <int>           <int>          <i64> <char>          <char>
#> 1:        R            1               0 11435930564116    ALC               N
#> 2:        R            3               0 11436069862295   CHAR               P
#> 3:        R            2               0 11436025019799    BOB               S
#>    financial_status lot_size round_lots_only issue_classification issue_subtype
#>              <char>    <int>          <lgcl>               <char>        <char>
#> 1:                N      100           FALSE                    A             Z
#> 2:                N      100           FALSE                    A             Z
#> 3:                N      100           FALSE                    A             Z
#>    authentic short_sell_closeout ipo_flag luld_price_tier etp_flag etp_leverage
#>       <lgcl>              <lgcl>   <lgcl>          <char>   <lgcl>        <int>
#> 1:      TRUE               FALSE    FALSE               2    FALSE            0
#> 2:      TRUE               FALSE    FALSE               2    FALSE            0
#> 3:      TRUE               FALSE    FALSE               2    FALSE            0
#>    inverse       date                            datetime exchange
#>     <lgcl>     <POSc>                          <nanotime>   <char>
#> 1:   FALSE 2010-12-24 2010-12-24T03:10:35.930564116+00:00     TEST
#> 2:   FALSE 2010-12-24 2010-12-24T03:10:36.069862295+00:00     TEST
#> 3:   FALSE 2010-12-24 2010-12-24T03:10:36.025019799+00:00     TEST
#>       market_category_note financial_status_note luld_price_tier_note
#>                     <char>                <char>               <char>
#> 1: New York Stock Exchange                Normal    Tier 2 NMS Stocks
#> 2:               NYSE Arca                Normal    Tier 2 NMS Stocks
#> 3:   Nasdaq Capital Market                Normal    Tier 2 NMS Stocks

## read_trading_status()
ts <- read_trading_status(file, add_descriptions = TRUE, quiet = TRUE)
ts
#> Key: <market_code>
#>    msg_type stock_locate tracking_number      timestamp  stock trading_state
#>      <char>        <int>           <int>          <i64> <char>        <char>
#> 1:        H            1               0 11436094498153    ALC             T
#> 2:        H            2               0 11436235406277    BOB             T
#> 3:        H            3               0 11436375417237   CHAR             T
#>    reserved reason market_code operation_halted       date
#>      <char> <char>      <char>           <lgcl>     <POSc>
#> 1:                        <NA>               NA 2010-12-24
#> 2:                        <NA>               NA 2010-12-24
#> 3:                        <NA>               NA 2010-12-24
#>                               datetime exchange trading_state_note
#>                             <nanotime>   <char>             <char>
#> 1: 2010-12-24T03:10:36.094498153+00:00     TEST  Trading on Nasdaq
#> 2: 2010-12-24T03:10:36.235406277+00:00     TEST  Trading on Nasdaq
#> 3: 2010-12-24T03:10:36.375417237+00:00     TEST  Trading on Nasdaq
#>    market_code_note
#>              <char>
#> 1:             <NA>
#> 2:             <NA>
#> 3:             <NA>

## read_reg_sho()
# note the example file has no reg SHO messages
rs <- read_reg_sho(file, add_descriptions = TRUE, quiet = TRUE)
rs
#> Empty data.table (0 rows and 10 cols): msg_type,stock_locate,tracking_number,timestamp,stock,regsho_action...

## read_market_participant_states()
# note the example file has no market participant states
mps <- read_market_participant_states(file, add_descriptions = TRUE,
                                      quiet = TRUE)
mps
#> Empty data.table (0 rows and 14 cols): msg_type,stock_locate,tracking_number,timestamp,mpid,stock...

## read_mwcb()
# note the example file has no circuit breakers messages
mwcb <- read_mwcb(file, quiet = TRUE)
mwcb
#> Empty data.table (0 rows and 11 cols): msg_type,stock_locate,tracking_number,timestamp,level1,level2...

## read_ipo()
# note the example file has no IPOs
ipo <- read_ipo(file, add_descriptions = TRUE, quiet = TRUE)
ipo
#> Empty data.table (0 rows and 12 cols): msg_type,stock_locate,tracking_number,timestamp,stock,release_time...

## read_luld()
# note the example file has no LULD messages
luld <- read_luld(file, quiet = TRUE)
luld
#> Empty data.table (0 rows and 12 cols): msg_type,stock_locate,tracking_number,timestamp,stock,reference_price...

## read_orders()
od <- read_orders(file, quiet = TRUE)
od
#>       msg_type stock_locate tracking_number      timestamp order_ref    buy
#>         <char>        <int>           <int>          <i64>     <i64> <lgcl>
#>    1:        A            2               0 31139052372053         0   TRUE
#>    2:        A            2               0 31141354532167       100   TRUE
#>    3:        F            2               0 32813425752711     84836   TRUE
#>    4:        A            2               0 32826656500150     87020  FALSE
#>    5:        A            2               0 32827351405783     87040  FALSE
#>   ---                                                                      
#> 4996:        A            1               0 57586688692484  88478593  FALSE
#> 4997:        A            2               0 57586792358162  82185144  FALSE
#> 4998:        A            1               0 57590573033724  88557297   TRUE
#> 4999:        A            2               0 57595196456803  82354728   TRUE
#> 5000:        A            2               0 57595326231183  82357176  FALSE
#>       shares  stock   price   mpid       date
#>        <int> <char>   <num> <char>     <POSc>
#>    1:   1000    BOB  5.3167        2010-12-24
#>    2:   1000    BOB  5.3167        2010-12-24
#>    3:    100    BOB  5.2917   VIRT 2010-12-24
#>    4:   1220    BOB  5.4167        2010-12-24
#>    5:   2000    BOB  5.4167        2010-12-24
#>   ---                                        
#> 4996:    100    ALC 23.1000        2010-12-24
#> 4997:    900    BOB  6.0833        2010-12-24
#> 4998:    100    ALC 23.0200        2010-12-24
#> 4999:   1200    BOB  6.0583        2010-12-24
#> 5000:   2000    BOB  6.0750        2010-12-24
#>                                  datetime exchange
#>                                <nanotime>   <char>
#>    1: 2010-12-24T08:38:59.052372053+00:00     TEST
#>    2: 2010-12-24T08:39:01.354532167+00:00     TEST
#>    3: 2010-12-24T09:06:53.425752711+00:00     TEST
#>    4: 2010-12-24T09:07:06.656500150+00:00     TEST
#>    5: 2010-12-24T09:07:07.351405783+00:00     TEST
#>   ---                                             
#> 4996: 2010-12-24T15:59:46.688692484+00:00     TEST
#> 4997: 2010-12-24T15:59:46.792358162+00:00     TEST
#> 4998: 2010-12-24T15:59:50.573033724+00:00     TEST
#> 4999: 2010-12-24T15:59:55.196456803+00:00     TEST
#> 5000: 2010-12-24T15:59:55.326231183+00:00     TEST

## read_modifications()
mod <- read_modifications(file, quiet = TRUE)
mod
#>       msg_type stock_locate tracking_number      timestamp order_ref shares
#>         <char>        <int>           <int>          <i64>     <i64>  <int>
#>    1:        E            2               2 32857937604189     87020   1220
#>    2:        E            2               6 33415045933113    121012    200
#>    3:        E            2               6 33451454329367    130800   2738
#>    4:        E            2               2 33451456680919    130800    100
#>    5:        E            2               2 33452976359207    130800     62
#>   ---                                                                      
#> 1996:        D            1               0 57583565860801  88418673     NA
#> 1997:        U            1               0 57586687948197  88478593    100
#> 1998:        U            1               0 57590574066018  88557297    100
#> 1999:        D            2               0 57595204788097  82354728     NA
#> 2000:        D            2               0 57597497623835  58412056     NA
#>       match_number printable   price new_order_ref       date
#>              <i64>    <lgcl>   <num>         <i64>     <POSc>
#>    1:        18049        NA      NA          <NA> 2010-12-24
#>    2:        18225        NA      NA          <NA> 2010-12-24
#>    3:        18234        NA      NA          <NA> 2010-12-24
#>    4:        18235        NA      NA          <NA> 2010-12-24
#>    5:        18237        NA      NA          <NA> 2010-12-24
#>   ---                                                        
#> 1996:         <NA>        NA      NA          <NA> 2010-12-24
#> 1997:         <NA>        NA 23.0933      88575536 2010-12-24
#> 1998:         <NA>        NA 23.0267      88654608 2010-12-24
#> 1999:         <NA>        NA      NA          <NA> 2010-12-24
#> 2000:         <NA>        NA      NA          <NA> 2010-12-24
#>                                  datetime exchange
#>                                <nanotime>   <char>
#>    1: 2010-12-24T09:07:37.937604189+00:00     TEST
#>    2: 2010-12-24T09:16:55.045933113+00:00     TEST
#>    3: 2010-12-24T09:17:31.454329367+00:00     TEST
#>    4: 2010-12-24T09:17:31.456680919+00:00     TEST
#>    5: 2010-12-24T09:17:32.976359207+00:00     TEST
#>   ---                                             
#> 1996: 2010-12-24T15:59:43.565860801+00:00     TEST
#> 1997: 2010-12-24T15:59:46.687948197+00:00     TEST
#> 1998: 2010-12-24T15:59:50.574066018+00:00     TEST
#> 1999: 2010-12-24T15:59:55.204788097+00:00     TEST
#> 2000: 2010-12-24T15:59:57.497623835+00:00     TEST

## read_trades()
tr <- read_trades(file, quiet = TRUE)
tr
#>       msg_type stock_locate tracking_number      timestamp order_ref    buy
#>         <char>        <int>           <int>          <i64>     <i64> <lgcl>
#>    1:        P            2               2 34210128591201         0   TRUE
#>    2:        P            2               2 34210355475120         0   TRUE
#>    3:        P            2               2 34210767188977         0   TRUE
#>    4:        P            2               2 34211127433476         0   TRUE
#>    5:        P            2               2 34212046014088         0   TRUE
#>   ---                                                                      
#> 4996:        P            1               2 57594215201805         0   TRUE
#> 4997:        P            1               6 57595050849140         0   TRUE
#> 4998:        P            1               4 57595051019659         0   TRUE
#> 4999:        P            1               2 57595052006081         0   TRUE
#> 5000:        P            3               2 57597526823001         0   TRUE
#>       shares  stock   price match_number cross_type       date
#>        <int> <char>   <num>        <i64>     <char>     <POSc>
#>    1:    200    BOB  5.3333        19447       <NA> 2010-12-24
#>    2:    300    BOB  5.3333        19451       <NA> 2010-12-24
#>    3:    100    BOB  5.3250        19493       <NA> 2010-12-24
#>    4:     47    BOB  5.3333        19515       <NA> 2010-12-24
#>    5:    200    BOB  5.3333        19547       <NA> 2010-12-24
#>   ---                                                         
#> 4996:      1    ALC 23.0800       728935       <NA> 2010-12-24
#> 4997:     28    ALC 23.0200       730245       <NA> 2010-12-24
#> 4998:     50    ALC 23.0200       730244       <NA> 2010-12-24
#> 4999:     65    ALC 23.1000       730243       <NA> 2010-12-24
#> 5000:    100   CHAR 22.0250       731883       <NA> 2010-12-24
#>                                  datetime exchange
#>                                <nanotime>   <char>
#>    1: 2010-12-24T09:30:10.128591201+00:00     TEST
#>    2: 2010-12-24T09:30:10.355475120+00:00     TEST
#>    3: 2010-12-24T09:30:10.767188977+00:00     TEST
#>    4: 2010-12-24T09:30:11.127433476+00:00     TEST
#>    5: 2010-12-24T09:30:12.046014088+00:00     TEST
#>   ---                                             
#> 4996: 2010-12-24T15:59:54.215201805+00:00     TEST
#> 4997: 2010-12-24T15:59:55.050849140+00:00     TEST
#> 4998: 2010-12-24T15:59:55.051019659+00:00     TEST
#> 4999: 2010-12-24T15:59:55.052006081+00:00     TEST
#> 5000: 2010-12-24T15:59:57.526823001+00:00     TEST

## read_noii()
# note the example file has no NOII messages
noii <- read_noii(file, add_descriptions = TRUE, quiet = TRUE)
noii
#> Empty data.table (0 rows and 19 cols): msg_type,stock_locate,tracking_number,timestamp,paired_shares,imbalance_shares...

## read_rpii()
# note the example file has no RPII messages
rpii <- read_rpii(file, add_descriptions = TRUE, quiet = TRUE)
rpii
#> Empty data.table (0 rows and 10 cols): msg_type,stock_locate,tracking_number,timestamp,stock,interest_flag...
```
