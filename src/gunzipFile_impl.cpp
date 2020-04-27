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
void gunzipFile_impl(std::string infile, std::string outfile, int64_t bufferSize = 1e9) {
  gzFile gzfile = gzopen(infile.c_str(), "rb");
  
  unsigned char* bufferPtr;
  int64_t bufferCharSize = sizeof(char) * bufferSize > UINT_MAX ? 
    UINT_MAX : 
    sizeof(char) * bufferSize;
  bufferPtr = (unsigned char*) malloc(bufferCharSize);
  
  int64_t thisBufferSize;
  
  FILE* rawfile = fopen(outfile.c_str(), "wb");
  // iterate over the file until the all information is gathered

  while (1) {
    // fill the buffer
    thisBufferSize = gzread(gzfile, bufferPtr, bufferCharSize);
    // write the buffer
    fwrite(bufferPtr, thisBufferSize, 1, rawfile);
    
    // check if the read buffer is smaller than the asked size
    if (thisBufferSize < bufferCharSize || thisBufferSize == 0) {
      break;
    }
  }
  fclose(rawfile);
  gzclose(gzfile);
}
