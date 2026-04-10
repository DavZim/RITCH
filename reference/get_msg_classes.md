# Returns the message class data for the message types

All information is handled according to the official ITCH 5.0
documentation as found here:
<http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf>

## Usage

``` r
get_msg_classes()
```

## Value

a data.table with the information of the message-types

## Details

- `msg_type` the type of the message

- `msg_class` the group the message belongs to

- `msg_name` the official name of the message

- `doc_nr` the number of the message in the documentation

## See also

[`open_itch_specification()`](https://davzim.github.io/RITCH/reference/open_itch_specification.md)

## Examples

``` r
get_msg_classes()
#>     msg_type                 msg_class
#>       <char>                    <char>
#>  1:        S             system_events
#>  2:        R           stock_directory
#>  3:        H            trading_status
#>  4:        Y                   reg_sho
#>  5:        L market_participant_states
#>  6:        V                      mwcb
#>  7:        W                      mwcb
#>  8:        K                       ipo
#>  9:        J                      luld
#> 10:        h            trading_status
#> 11:        A                    orders
#> 12:        F                    orders
#> 13:        E             modifications
#> 14:        C             modifications
#> 15:        X             modifications
#> 16:        D             modifications
#> 17:        U             modifications
#> 18:        P                    trades
#> 19:        Q                    trades
#> 20:        B                    trades
#> 21:        I                      noii
#> 22:        N                      rpii
#>     msg_type                 msg_class
#>       <char>                    <char>
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
```
