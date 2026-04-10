# Returns the date from an ITCH-filename

Returns the date from an ITCH-filename

## Usage

``` r
get_date_from_filename(file)
```

## Arguments

- file:

  a filename

## Value

the date as fastPOSIXct

## Examples

``` r
get_date_from_filename("03302017.NASDAQ_ITCH50")
#> [1] "2017-03-30 GMT"
get_date_from_filename("20170130.BX_ITCH_50.gz")
#> [1] "2017-01-30 GMT"
get_date_from_filename("S030220-v50-bx.txt.gz")
#> [1] "2020-03-02 GMT"
get_date_from_filename("unknown_file_format")
#> [1] NA
```
