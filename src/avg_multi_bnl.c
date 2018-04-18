#include <stdlib.h>
#include <stdio.h>

// TODO change to specific Ctypes (lib 64 or lin32 independent)
typedef struct multifile_header_t{
    /* The multifile header (for BNL) is a 1024 byte header
     * 
     * with the data in the locations specified below
     *
     */
    // this full struct totals 1024 bytes
    // the magic is 32 bytes long?
    int magic[4];
    double beam_center_x;
    double beam_center_y;
    double count_time;
    double detector_distance;
    double frame_time;
    double incident_wavelength;
    double x_pixel_size;
    double y_pixel_size;
    int bytes;
    int nrows;
    int ncols;
    int rows_begin;
    int rows_end;
    int cols_begin;
    int cols_end;
    char footer[916];
} multifile_header_t;

typedef struct multiheader2 {
    unsigned int dlen;
} multiheader2;

int read_header(FILE *f, multifile_header_t* mhp){
    /* Read the header from a multifile
     * It is assumed you've already seeked to the header position.
     */
    return fread(mhp, sizeof(multifile_header_t), 1, f); 
}

int read_dlen(FILE *f, multiheader2* mhp){
    /* Read the header from a multifile
     * It is assumed you've already seeked to the header position.
     */
    return fread(mhp, sizeof(multiheader2), 1, f); 
}

void read_raw(FILE *f, multiheader2 *mhp, unsigned int *posbuf, unsigned short int *valbuf){
    /* Assumes 2 byte files here.
     */
    fread(posbuf, 4, mhp->dlen, f); 
    fread(valbuf, 2, mhp->dlen, f); 
}

int main(int argc, char **argv){
    /* Test the reading of multifile here.
     *
     */
    if (argc != 2){
        printf("Usage : avg_multi filename");
        return -1;
    }
    char *filename = argv[1];
    unsigned int posbuf[1000000];
    unsigned short int valbuf[1000000];
    unsigned int count;
    multifile_header_t mh;
    multiheader2 mh2;
    FILE *f, *fimg;
    int i;
    //const char * filename = "sample_data/flow10crlT0_EGhtd_011_66/flow10crlT0_EGhtd_011_66_00001-10000.imm";
    f = fopen(filename, "r");
    count = 0;
    read_header(f, &mh);
    read_dlen(f, &mh2);
    printf("Rows : %d; Cols : %d; Dlen : %d\n", mh.nrows, mh.ncols, mh2.dlen);
    printf("foo\n");
    printf("Total size : %d\n", mh.nrows*mh.ncols);

    unsigned int *img = (unsigned int *)malloc(mh.nrows*mh.ncols*sizeof(unsigned int));

    read_raw(f, &mh2, posbuf, valbuf);
    printf("size of struct %ld\n", sizeof(mh));
    printf("reading...\n");
    for(i=0; i < mh2.dlen; i++){
        //printf("position %d\n",posbuf[i]);
        img[posbuf[i]] += valbuf[i];
    }
    printf("done\n");

    while(read_dlen(f, &mh2) > 0){
        // FSEEK after first time
        //read_header(f, &mh);
        read_raw(f, &mh2, posbuf, valbuf);
        for(i=0; i < mh2.dlen; i++){
            //printf("position %d",posbuf[i]);
            img[posbuf[i]] += valbuf[i];
        }
        //printf("Iteration %d\n", count);
        //printf("Rows : %d; Cols : %d; Dlen : %d\n", mh.nrows, mh.ncols, mh2.dlen);
        count +=1;
    }
    
    fclose(f);

    fimg = fopen("img.bin", "wb");
    fwrite(img, sizeof(unsigned int), mh.nrows*mh.ncols, fimg);
    fclose(fimg);


    return 1;
}
