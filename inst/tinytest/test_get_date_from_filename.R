library(RITCH)
library(tinytest)

expect_equal(
  get_date_from_filename("03302017.NASDAQ_ITCH50"),
  as.POSIXct("2017-03-30", "GMT")
)
expect_equal(
  get_date_from_filename("20170130.BX_ITCH_50.gz"),
  as.POSIXct("2017-01-30", "GMT")
)
expect_equal(
  get_date_from_filename("S030220-v50-bx.txt.gz"),
  as.POSIXct("2020-03-02", "GMT")
)

