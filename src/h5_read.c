/*
 * This file reads an EIGER file and compresses it.
 * The example was taken from
 * https://support.hdfgroup.org/ftp/HDF5/examples/introductory/C/h5_hyperslab.c
 * where one reads a hyperslab from a data set.
 */

// TODO : read in chunks, maybe 10 frames at a time, is there speedup?
// TODO : Use GPU acceleration for sparse format?
// TODO : approximate how sparse the array is?
// TODO : extract other simple statistics, how to add plugins?
//      This might be something to discuss with Faisal

#include "hdf5.h"
#include <stdlib.h>

#define H5FILE_NAME "/home/lhermitte/research/projects/xpcs-aps-chx-project/sample_data/flow10crlT0_EGhtd_011_66/new/flow10crlT0_EGhtd_011_66_master.h5"
#define DATASETNAME "entry/data_000001"
#define RANK         3
#define RANK_OUT     3

int
main (void)
{
    hid_t       file, dataset;         /* handles */
    hid_t       datatype, dataspace;
    hid_t       memspace;
    H5T_class_t t_class;                 /* data type class */
    H5T_order_t order;                 /* data order */
    size_t      size;                  /*
				        * size of the data element
				        * stored in file
				        */
    hsize_t     dimsm[RANK];              /* memory space dimensions */
    hsize_t     dims_out[RANK];           /* dataset dimensions */
    herr_t      status;

    // the output file
    FILE * fout;
    printf("opening file\n");
    fout = fopen("test.bin", "w");
    printf("done\n");

    char *header = (char *)malloc(1024);
    // write a dummy header
    fwrite(header, sizeof(char), 1024, fout);

    // 10 MB buffer for pos and val
    int *posbuffer = (int *)malloc(10000000*sizeof(unsigned int));
    int *valbuffer = (int *)malloc(10000000*sizeof(unsigned short int));
    int dlen;


    int          i, j, k, status_n, rank;

    /*
     * Open the file and the dataset.
     */
    file = H5Fopen(H5FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT);

    char *dataset_name = malloc(80);
    int dset_number;
    int image_number;
    image_number = 0;

    for (dset_number = 0; dset_number < 10; dset_number++){
        sprintf(dataset_name, "%s%06d", "entry/data_", dset_number);
        dataset = H5Dopen2(file, dataset_name, H5P_DEFAULT);
    
        /*
         * Get datatype and dataspace handles and then query
         * dataset class, order, size, rank and dimensions.
         */
        datatype  = H5Dget_type(dataset);     /* datatype handle */
        t_class     = H5Tget_class(datatype);
        /*
        if (t_class == H5T_INTEGER){
            printf("Data set has INTEGER type \n");
        }else{
            printf("Warning, not INTEGER type.\n");
        }
        order     = H5Tget_order(datatype);
        if (order == H5T_ORDER_LE){
            printf("Little endian order \n");
        }else{
            printf("Not Little endian order \n");
        }
        */
    
        size  = H5Tget_size(datatype);
        printf(" Data size is %d \n", (int)size);
    
        dataspace = H5Dget_space(dataset);    /* dataspace handle */
        rank      = H5Sget_simple_extent_ndims(dataspace);
        status_n  = H5Sget_simple_extent_dims(dataspace, dims_out, NULL);
        printf("rank %d, dimensions %lu x %lu x %lu \n", rank,
    	   (unsigned long)(dims_out[0]), (unsigned long)(dims_out[1]),
           (unsigned long)(dims_out[2]));
    
        // Now declare memory, everything for first array
        int         data_out[1][dims_out[1]][dims_out[2]]; /* output buffer */
    
        hsize_t * count = (hsize_t *) malloc(rank*sizeof(int));              /* size of the hyperslab in the file */
        hsize_t      offset[RANK];             /* hyperslab offset in the file */
        hsize_t      count_out[RANK];          /* size of the hyperslab in memory */
        hsize_t      offset_out[RANK];         /* hyperslab offset in memory */
        int nimg;
        int ind;
    
        /*
         * Define hyperslab in the dataset.
         */
        offset[0] = 0;
        offset[1] = 0;
        offset[2] = 0;
        count[0]  = 1;
        count[1]  = dims_out[1];
        count[2]  = dims_out[2];
        //status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL,
    				                 //count, NULL);
    
        /*
         * Define the memory dataspace.
         */
        dimsm[0] = 1;
        dimsm[1] = dims_out[1];
        dimsm[2] = dims_out[2];
        memspace = H5Screate_simple(rank, dimsm, NULL);
    
        /*
         * Define memory hyperslab.
         */
        offset_out[0] = 0;
        offset_out[1] = 0;
        offset_out[2] = 0;
        count_out[0]  = 1;
        count_out[1]  = dims_out[1];
        count_out[2]  = dims_out[2];
        status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL,
    				 count_out, NULL);
    
        //printf("begin loop\n");
        /*
         * Read data from hyperslab in the file into the hyperslab in
         * memory and display.
         */
        //status = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace,
    		     //H5P_DEFAULT, data_out);
        for (nimg = 0; nimg < dims_out[0]; nimg++){
            image_number++;
            // define hyperslab
            offset[0] = nimg;
            //printf("selecting\n");
            status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL,
    				    count, NULL);
            // now read
            //printf("reading\n");
            status = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace,
    		                 H5P_DEFAULT, data_out);
            //printf("done reading\n");
            // reset dlen
            dlen = 0;
            for (i = 0; i < dims_out[1]; i++) {
                for (j = 0; j < dims_out[2]; j++){
                    ind = i*dims_out[2] + j;
                    if(data_out[0][i][j] != 0 && data_out[0][i][j] < 10000){
                        //printf("dlen %d\n",dlen);
                        posbuffer[dlen] = ind;
                        valbuffer[dlen] = data_out[0][i][j];
                        dlen++;
                    }
                }
            }
            //printf("dlen : %d\n", dlen);
            fwrite(&dlen, sizeof(int), 1, fout);
            fwrite(posbuffer, sizeof(int), dlen, fout);
            fwrite(valbuffer, sizeof(int), dlen, fout);
        }
    }
    printf("Read %d images\n", image_number);

    /*
     * Close/release resources.
     */
    H5Tclose(datatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Fclose(file);
    fclose(fout);

    return 0;
}
