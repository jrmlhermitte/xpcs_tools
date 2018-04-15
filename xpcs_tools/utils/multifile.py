import numpy as np
import struct
import os

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

# TODO : split into RO and RW classes
class Multifile2:
    '''
    Re-write multifile from scratch.

    '''
    HEADER_SIZE = 1024
    def __init__(self, filename, numimgs, mode='rb', nbytes=2):
        '''
            Prepare a file for reading or writing.
            mode : either 'rb' or 'wb'
            numimgs: num images
        '''
        if mode != 'rb' and mode != 'wb':
            raise ValueError("Error, mode must be 'rb' or 'wb'"
                             "got : {}".format(mode))
        self.beg = 0
        self.end = numimgs-1
        self.numimgs = numimgs
        self._filename = filename
        self._mode = mode

        self._nbytes = nbytes
        if nbytes == 2:
            self._dtype = '<i2'
        elif nbytes == 4:
            self._dtype = '<i4'


        # open the file descriptor
        # create a memmap
        if mode == 'rb':
            self._fd = np.memmap(filename, dtype='c')
        elif mode == 'wb':
            self._fd = open(filename, "wb")
        # frame number currently on
        self.index()

        # these are only necessary for writing
        hdr = self._read_header(0)
        self._rows = hdr['rows']
        self._cols = hdr['cols']

    def rdframe(self, n):
        # read header then image
        hdr = self._read_header(n)
        pos, vals = self._read_raw(n)
        img = np.zeros((self._rows*self._cols,))
        img[pos] = vals
        return img.reshape((self._rows, self._cols))

    def rdrawframe(self, n):
        # read header then image
        hdr = self._read_header(n)
        return self._read_raw(n)

    def rdchunk(self):
        ''' read the next chunk'''
        header = self._fd.read(1024)

    def index(self):
        print('indexing file...')
        cur = 0
        self.frame_indexes = list()
        for i in range(self.numimgs):
            self.frame_indexes.append(cur)
            # first get dlen
            dlen = np.frombuffer(self._fd[cur+152:cur+156], dtype=self._dtype)[0]
            cur += 1024 + dlen*(4 + self._nbytes)
        print("done.")



    def _read_header(self, n):
        ''' Read header from current seek position.'''
        if n > self.numimgs:
            raise KeyError("Error, only {} frames, asked for {}".format(self.numimgs, n))
        # read in bytes
        cur = self.frame_indexes[n]
        header_raw = self._fd[cur:cur + self.HEADER_SIZE]
        header = dict()
        header['rows'] = np.frombuffer(header_raw[108:112], dtype=self._dtype)[0]
        header['cols'] = np.frombuffer(header_raw[112:116], dtype=self._dtype)[0]
        header['nbytes'] = np.frombuffer(header_raw[116:120], dtype=self._dtype)[0]
        header['dlen'] = np.frombuffer(header_raw[152:156], dtype=self._dtype)[0]
        #print("dlen: {}\trows: {}\tcols: {}\tnbytes: {}\n"\
              #.format(header['dlen'], header['rows'], header['cols'],
                      #header['nbytes']))

        self._dlen = header['dlen']
        self._nbytes = header['nbytes']

        return header

    def _read_raw(self, n):
        ''' Read from raw.
            Reads from current cursor in file.
        '''
        if n > self.numimgs:
            raise KeyError("Error, only {} frames, asked for {}".format(self.numimgs, n))
        cur = self.frame_indexes[n] + 1024
        dlen = self._read_header(n)['dlen']

        pos = self._fd[cur: cur+dlen*4]
        cur += dlen*4
        pos = np.frombuffer(pos, dtype='<i4')

        # TODO: 2-> nbytes
        vals = self._fd[cur: cur+dlen*2]
        # not necessary
        cur += dlen*2
        vals = np.frombuffer(vals, dtype=self._dtype)

        return pos, vals

    def seekimg(self, n):
        # not needed
        pass

    def _write_header(self, dlen, rows, cols):
        ''' Write header at current position.'''
        self._rows = rows
        self._cols = cols
        self._dlen = dlen
        # byte array
        header = np.zeros(self.HEADER_SIZE, dtype="c")
        # write the header dlen
        header[152:156] = np.array([dlen], dtype="<i4").tobytes()
        # rows
        header[108:112] = np.array([rows], dtype="<i4").tobytes()
        # colds
        header[112:116] = np.array([cols], dtype="<i4").tobytes()
        self._fd.write(header)


    def write_raw(self, pos, vals):
        ''' Write a raw set of values for the next chunk.'''
        rows = self._rows
        cols = self._cols
        dlen = len(pos)
        self._write_header(dlen, rows, cols)
        # now write the pos and vals in series
        pos = pos.astype(self._dtype)
        vals = vals.astype(self._dtype)
        self._fd.write(pos)
        self._fd.write(vals)




class Multifile:
    '''The class representing the multifile.
        The recno is in 1 based numbering scheme (first record is 1)
	This is efficient for reading in increasing order.
	Note: reading same image twice in a row is like reading an earlier
	numbered image and means the program starts for the beginning again.

    '''
    def __init__(self,filename,beg,end):
        '''Multifile initialization. Open the file.
            Here I use the read routine which returns byte objects
            (everything is an object in python). I use struct.unpack
            to convert the byte object to other data type (int object
            etc)
            NOTE: At each record n, the file cursor points to record n+1
        '''
        self.FID = open(filename,"rb")
#        self.FID.seek(0,os.SEEK_SET)
        self.filename = filename
        #br: bytes read
        br = self.FID.read(1024)
        self.beg=beg
        self.end=end
        ms_keys = ['beam_center_x', 'beam_center_y', 'count_time', 'detector_distance',
           'frame_time', 'incident_wavelength', 'x_pixel_size', 'y_pixel_size',
           'bytes',
            'nrows', 'ncols', 'rows_begin', 'rows_end', 'cols_begin', 'cols_end'
          ]

        magic = struct.unpack('@16s', br[:16])
        md_temp =  struct.unpack('@8d7I916x', br[16:])
        self.md = dict(zip(ms_keys, md_temp))

        self.imgread=0
        self.recno = 0
        # some initialization stuff
        self.byts = self.md['bytes']
        if (self.byts==2):
            self.valtype = np.uint16
        elif (self.byts == 4):
            self.valtype = np.uint32
        elif (self.byts == 8):
            self.valtype = np.float64
        #now convert pieces of these bytes to our data
        self.dlen =np.fromfile(self.FID,dtype=np.int32,count=1)[0]

        # now read first image
        #print "Opened file. Bytes per data is {0img.shape = (self.rows,self.cols)}".format(self.byts)

    def _readHeader(self):
        self.dlen =np.fromfile(self.FID,dtype=np.int32,count=1)[0]

    def _readImageRaw(self):
        p= np.fromfile(self.FID, dtype = np.int32, count= self.dlen)
        v= np.fromfile(self.FID, dtype = self.valtype, count= self.dlen)
        self.imgread=1
        return(p,v)

    def _readImage(self):
        (p,v)=self._readImageRaw()
        img = np.zeros( (  self.md['ncols'], self.md['nrows']   ) )
        np.put( np.ravel(img), p, v )
        return(img)

    def seekimg(self,n=None):

        '''Position file to read the nth image.
            For now only reads first image ignores n
        '''
        # the logic involving finding the cursor position
        if (n is None):
            n = self.recno
        if (n < self.beg or n > self.end):
            raise IndexError('Error, record out of range')
        #print (n, self.recno, self.FID.tell() )
        if ((n == self.recno)  and (self.imgread==0)):
            pass # do nothing

        else:
            if (n <= self.recno): #ensure cursor less than search pos
                self.FID.seek(1024,os.SEEK_SET)
                self.dlen =np.fromfile(self.FID,dtype=np.int32,count=1)[0]
                self.recno = 0
                self.imgread=0
                if n == 0:
                    return
            #have to iterate on seeking since dlen varies
            #remember for rec recno, cursor is always at recno+1
            if(self.imgread==0 ): #move to next header if need to
                self.FID.seek(self.dlen*(4+self.byts),os.SEEK_CUR)
            for i in range(self.recno+1,n):
                #the less seeks performed the faster
                #print (i)
                self.dlen =np.fromfile(self.FID,dtype=np.int32,count=1)[0]
                #print 's',self.dlen
                self.FID.seek(self.dlen*(4+self.byts),os.SEEK_CUR)

            # we are now at recno in file, read the header and data
            #self._clearImage()
            self._readHeader()
            self.imgread=0
            self.recno = n

    def rdframe(self,n):
        if self.seekimg(n)!=-1:
            return(self._readImage())

    def rdrawframe(self,n):
        if self.seekimg(n)!=-1:
             return(self._readImageRaw())
