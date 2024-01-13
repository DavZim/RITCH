#include <Rcpp.h>
#include <zlib.h>

/**
 * @brief Inflates (uncompresses) a gz file of binary data
 *
 * @param infile The name of the compressed gz archive
 * @param outfile The name of the uncompressed target file (make sure it does not exist before for faster speeds!)
 * @param buffer_size the size of the buffer, default is 1e9 bytes.
 */
// [[Rcpp::export]]
void gunzip_file_impl(std::string infile,
                      std::string outfile,
                      int64_t buffer_size = 1e9) {

  gzFile gzfile = gzopen(infile.c_str(), "rb");
  if (gzfile == NULL) {
    Rcpp::stop("Could not open file '%s' for gunzip", infile.c_str());
  }

  unsigned char* buf;
  int64_t buffer_char_size = sizeof(unsigned char) * buffer_size > UINT_MAX ?
    UINT_MAX :
    sizeof(unsigned char) * buffer_size;
  buf = (unsigned char*) malloc(buffer_char_size);

  int64_t this_buffer_size;

  FILE* ofile = fopen(outfile.c_str(), "wb");
  if (ofile == NULL) {
    Rcpp::stop("Could not open file '%s' for gunzip", outfile.c_str());
  }
  // iterate over the file until the all information is gathered

  while (1) {
    // fill the buffer
    this_buffer_size = gzread(gzfile, buf, buffer_char_size);
    // write the buffer
    fwrite(&buf[0], 1, this_buffer_size, ofile);

    // check if the read buffer is smaller than the asked size
    if (this_buffer_size < buffer_char_size || this_buffer_size == 0) {
      break;
    }
  }

  free(buf);
  fclose(ofile);
  gzclose(gzfile);
}


/**
 * @brief Deflates (compresses) a gz file of binary data
 *
 * @param infile The name of the raw uncompressed file
 * @param outfile The name of the compressed target file (make sure it does not exist before for faster speeds!)
 * @param buffer_size the size of the buffer, default is 1e9 bytes.
 */
// [[Rcpp::export]]
void gzip_file_impl(std::string infile,
                    std::string outfile,
                    int64_t buffer_size = 1e9) {

  FILE* file = fopen(infile.c_str(), "rb");
  if (file == NULL) {
    Rcpp::stop("Could not open file %s for gzip", infile.c_str());
  }

  unsigned char* buf;
  int64_t buffer_char_size = sizeof(unsigned char) * buffer_size > UINT_MAX ?
    UINT_MAX :
    sizeof(unsigned char) * buffer_size;
  buf = (unsigned char*) malloc(buffer_char_size);

  int64_t this_buffer_size;

  gzFile ofile = gzopen(outfile.c_str(), "wb");
  if (ofile == NULL) {
    Rcpp::stop("Could not open file %s for gzip", outfile.c_str());
  }
  // iterate over the file until the all information is gathered

  while (1) {
    // fill the buffer
    this_buffer_size = fread(buf, 1, buffer_char_size, file);
    // write the buffer
    gzwrite(ofile, &buf[0], this_buffer_size);

    // check if the read buffer is smaller than the asked size
    if (this_buffer_size < buffer_char_size || this_buffer_size == 0) {
      break;
    }
  }

  free(buf);
  fclose(file);
  gzclose(ofile);
}
