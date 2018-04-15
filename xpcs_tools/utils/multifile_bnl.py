import numpy as np

"""    Description:

    This is code that Mark wrote to open the multifile format
    in compressed mode, translated to python.
    This seems to work for DALSA, FCCD and EIGER in compressed mode.
    It should be included in the respective detector.i files
    Currently, this refers to the compression mode being '6'
    Each file is image descriptor files chunked together as follows:
            Header (1024 bytes)
    |--------------IMG N begin--------------|
    |                   Dlen
    |---------------------------------------|
    |       Pixel positions (dlen*4 bytes   |
    |      (0 based indexing in file)       |
    |---------------------------------------|
    |    Pixel data(dlen*bytes bytes)       |
    |    (bytes is found in header          |
    |    at position 116)                   |
    |--------------IMG N end----------------|
    |--------------IMG N+1 begin------------|
    |----------------etc.....---------------|


     Header contains 1024 bytes version name, 'beam_center_x', 'beam_center_y', 'count_time', 'detector_distance',
           'frame_time', 'incident_wavelength', 'x_pixel_size', 'y_pixel_size',
           bytes per pixel (either 2 or 4 (Default)),
           Nrows, Ncols, Rows_Begin, Rows_End, Cols_Begin, Cols_End,



"""


import struct
import time
# TODO : split into RO and RW classes
class MultifileBNL:
    '''
    Re-write multifile from scratch.

    '''
    HEADER_SIZE = 1024
    def __init__(self, filename, mode='rb', nbytes=4):
        '''
            Prepare a file for reading or writing.
            mode : either 'rb' or 'wb'
            numimgs: num images
        '''
        if mode == 'wb':
            raise ValueError("Write mode 'wb' not supported yet")

        if mode != 'rb' and mode != 'wb':
            raise ValueError("Error, mode must be 'rb' or 'wb'"
                             "got : {}".format(mode))

        self._filename = filename
        self._mode = mode

        # open the file descriptor
        # create a memmap
        if mode == 'rb':
            self._fd = np.memmap(filename, dtype='c')
        elif mode == 'wb':
            self._fd = open(filename, "wb")

        # these are only necessary for writing
        self.md = self._read_main_header()
        self._rows = self.md['ncols']
        self._cols = self.md['nrows']

        # some initialization stuff
        self.nbytes = self.md['bytes']
        if (self.nbytes==2):
            self.valtype = "<i2"#np.uint16
        elif (self.nbytes == 4):
            self.valtype = "<i4"#np.uint32
        elif (self.nbytes == 8):
            self.valtype = "<i8"#np.float64


        # frame number currently on
        self.index()

    def index(self):
        ''' Index the file by reading all frame_indexes.
            For faster later access.
        '''
        print('Indexing file...')
        t1 = time.time()
        cur = self.HEADER_SIZE
        file_bytes = len(self._fd)

        self.frame_indexes = list()
        while cur < file_bytes:
            self.frame_indexes.append(cur)
            # first get dlen, 4 bytes
            dlen = np.frombuffer(self._fd[cur:cur+4], dtype="<u4")[0]
            #print("found {} bytes".format(dlen))
            # self.nbytes is number of bytes per val
            cur += 4 + dlen*(4+self.nbytes)
            #break

        self.Nframes = len(self.frame_indexes)
        t2 = time.time()
        print("Done. Took {} secs for {} frames".format(t2-t1, self.Nframes))


    def _read_main_header(self):
        ''' Read header from current seek position.

            Extracting the header was written by Yugang Zhang. This is BNL's
            format.
            1024 byte header +
            4 byte dlen + (4 + nbytes)*dlen bytes
            etc...
        '''
        # read in bytes
        # header is always from zero
        cur = 0
        header_raw = self._fd[cur:cur + self.HEADER_SIZE]
        ms_keys = ['beam_center_x', 'beam_center_y', 'count_time',
                   'detector_distance', 'frame_time', 'incident_wavelength',
                   'x_pixel_size', 'y_pixel_size', 'bytes', 'nrows', 'ncols',
                   'rows_begin', 'rows_end', 'cols_begin', 'cols_end']
        magic = struct.unpack('@16s', header_raw[:16])
        md_temp =  struct.unpack('@8d7I916x', header_raw[16:])
        self.md = dict(zip(ms_keys, md_temp))
        return self.md

    def _read_raw(self, n):
        ''' Read from raw.
            Reads from current cursor in file.
        '''
        if n > self.Nframes:
            raise KeyError("Error, only {} frames, asked for {}".format(self.numimgs, n))
        # dlen is 4 bytes
        cur = self.frame_indexes[n]
        dlen = np.frombuffer(self._fd[cur:cur+4], dtype="<u4")[0]
        cur += 4

        pos = self._fd[cur: cur+dlen*4]
        cur += dlen*4
        pos = np.frombuffer(pos, dtype='<u4')

        # TODO: 2-> nbytes
        vals = self._fd[cur: cur+dlen*self.nbytes]
        vals = np.frombuffer(vals, dtype=self.valtype)

        return pos, vals

    def rdframe(self, n):
        # read header then image
        pos, vals = self._read_raw(n)
        img = np.zeros((self._rows*self._cols,))
        img[pos] = vals
        return img.reshape((self._rows, self._cols))

    def rdrawframe(self, n):
        # read header then image
        return self._read_raw(n)
