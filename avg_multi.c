#include <stdlib.h>
#include <stdio.h>

// TODO change to specific Ctypes (lib 64 or lin32 independent)
typedef struct multiheader {
   char beg[108]; 
   unsigned int rows; //108
   unsigned int cols; //112
   unsigned int nbytes; //116
   char middle[32];
   unsigned int dlen; //152
   char end[868];
} multiheader;

int read_header(FILE *f, multiheader* mhp){
    /* Read the header from a multifile
     * It is assumed you've already seeked to the header position.
     */
    return fread(mhp, sizeof(multiheader), 1, f); 
}

void read_raw(FILE *f, multiheader *mhp, unsigned int *posbuf, unsigned short int *valbuf){
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
    unsigned int posbuf[100000];
    unsigned short int valbuf[100000];
    unsigned int count;
    multiheader mh;
    FILE *f, *fimg;
    int i;
    //const char * filename = "sample_data/flow10crlT0_EGhtd_011_66/flow10crlT0_EGhtd_011_66_00001-10000.imm";
    f = fopen(filename, "r");
    count = 0;
    read_header(f, &mh);
    printf("Rows : %d; Cols : %d; Dlen : %d\n", mh.rows, mh.cols, mh.dlen);
    printf("foo\n");
    printf("Total size : %d\n", mh.rows*mh.cols);

    unsigned int *img = malloc(mh.rows*mh.cols*sizeof(unsigned int));

    read_raw(f, &mh, posbuf, valbuf);
    printf("size of struct %ld\n", sizeof(mh));
    for(i=0; i < mh.dlen; i++){
        //printf("position %d",posbuf[i]);
        img[posbuf[i]] += valbuf[i];
    }

    while(read_header(f, &mh) > 0){
        // FSEEK after first time
        //read_header(f, &mh);
        read_raw(f, &mh, posbuf, valbuf);
        for(i=0; i < mh.dlen; i++){
            //printf("position %d",posbuf[i]);
            img[posbuf[i]] += valbuf[i];
        }
        printf("Iteration %d\n", count);
        printf("Rows : %d; Cols : %d; Dlen : %d\n", mh.rows, mh.cols, mh.dlen);
        count +=1;
    }
    
    fclose(f);

    fimg = fopen("img.bin", "wb");
    fwrite(img, sizeof(unsigned int), mh.rows*mh.cols, fimg);
    fclose(fimg);


    return 1;
}
