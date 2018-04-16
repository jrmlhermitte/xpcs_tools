import numpy as np
cimport numpy as np

cdef extern from "compress_file.c":
    int raw_compress_file(char * filename, char *dataset_prefix, char *out_filename);

def compress_file(char *filename, char* dataset_prefix, char* out_filename):
    raw_compress_file(filename, dataset_prefix, out_filename);
