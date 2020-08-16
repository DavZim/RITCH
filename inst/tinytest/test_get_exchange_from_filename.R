library(RITCH)
library(tinytest)

expect_equal(
  get_exchange_from_filename("03302017.NASDAQ_ITCH50"),
  "NASDAQ"
)
expect_equal(
  get_exchange_from_filename("20170130.BX_ITCH_50.gz"),
  "BX"
)
expect_equal(
  get_exchange_from_filename("S030220-v50-bx.txt.gz"),
  "BX"
)

