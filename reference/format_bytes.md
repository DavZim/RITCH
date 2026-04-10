# Formats a number of bytes

Formats a number of bytes

## Usage

``` r
format_bytes(x, digits = 2, unit_suffix = "B", base = 1000)
```

## Arguments

- x:

  the values

- digits:

  the number of digits to display, default value is 2

- unit_suffix:

  the unit suffix, default value is 'B' (for bytes), useful is also
  'B/s' if you have read/write speeds

- base:

  the base for kilo, mega, ... definition, default is 1000

## Value

the values as a character

## Examples

``` r
format_bytes(1234)
#> [1] "1.23KB"
format_bytes(1234567890)
#> [1] "1.23GB"
format_bytes(123456789012, unit_suffix = "iB", base = 1024)
#> [1] "114.98GiB"
```
