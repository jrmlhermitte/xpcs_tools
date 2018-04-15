/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *   This example reads hyperslab from the SDS.h5 file
 *   created by h5_write.c program into two-dimensional
 *   plane of the three-dimensional array.
 *   Information about dataset in the SDS.h5 file is obtained.
 */

#include "hdf5.h"
#include <stdlib.h>

#define H5FILE_NAME "/home/lhermitte/research/projects/xpcs-aps-chx-project/sample_data/flow10crlT0_EGhtd_011_66/new/flow10crlT0_EGhtd_011_66_master.h5"
#define DATASETNAME "entry/data_000001"
#define NX_SUB  3           /* hyperslab dimensions */
#define NY_SUB  4
#define NX 7           /* output buffer dimensions */
#define NY 7
#define NZ  3
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

    //int         data_out[NX][NY][NZ ]; /* output buffer */

    int          i, j, k, status_n, rank;

    /*
    for (j = 0; j < NX; j++) {
	for (i = 0; i < NY; i++) {
	    for (k = 0; k < NZ ; k++)
		data_out[j][i][k] = 0;
	}
    }
    */

    /*
     * Open the file and the dataset.
     */
    file = H5Fopen(H5FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT);

    char *dataset_name = malloc(80);
    int dset_number;
    for (dset_number = 0; dset_number < 10; dset_number++){
        sprintf(dataset_name, "%s%06d", "entry/data_", dset_number);
        dataset = H5Dopen2(file, dataset_name, H5P_DEFAULT);
    
        /*
         * Get datatype and dataspace handles and then query
         * dataset class, order, size, rank and dimensions.
         */
        datatype  = H5Dget_type(dataset);     /* datatype handle */
        t_class     = H5Tget_class(datatype);
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
    
        hsize_t      count[RANK];              /* size of the hyperslab in the file */
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
    
        printf("begin loop\n");
        /*
         * Read data from hyperslab in the file into the hyperslab in
         * memory and display.
         */
        //status = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace,
    		     //H5P_DEFAULT, data_out);
        for (nimg = 0; nimg < dims_out[0]; nimg++){
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
    /*
     * 0 0 0 0 0 0 0
     * 0 0 0 0 0 0 0
     * 0 0 0 0 0 0 0
     * 3 4 5 6 0 0 0
     * 4 5 6 7 0 0 0
     * 5 6 7 8 0 0 0
     * 0 0 0 0 0 0 0
     */

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
