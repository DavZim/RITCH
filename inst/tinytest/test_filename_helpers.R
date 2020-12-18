library(RITCH)
library(tinytest)

# Get date from filename
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
expect_equal(
  get_date_from_filename("unknown_file_format"),
  NA
)

## Get exchange from filename
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
expect_equal(
  get_exchange_from_filename("unknown_file_format"),
  NA
)

## Add meta to filename
expect_equal(
  add_meta_to_filename("03302017.NASDAQ_ITCH50", "2010-12-24", "TEST"),
  "12242010.TEST_ITCH50"
)

expect_equal(
  add_meta_to_filename("20170130.BX_ITCH_50.gz", "2010-12-24", "TEST"),
  "20101224.TEST_ITCH_50.gz"
)
expect_equal(
  add_meta_to_filename("S030220-v50-bx.txt.gz", "2010-12-24", "TEST"),
  "S122410-v50-TEST.txt.gz"
)
expect_equal(
  add_meta_to_filename("unknown_file.ITCH_50", "2010-12-24", "TEST"),
  "unknown_file_20101224.TEST_ITCH_50"
)
expect_equal(
  add_meta_to_filename("some_folder/unknown_file.ITCH_50", "2010-12-24", "TEST"),
  "some_folder/unknown_file_20101224.TEST_ITCH_50"
)
