/************************************************************

  Test read of the EIGER hdf5 data.
  This file is intended for use with HDF5 Library version 1.8

  Note : need the z4 plugin setup with the HDF5PLUGINPATH env variable
  pointing to it.

 ************************************************************/

#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#include "read_eiger_groups.h"

#define FILENAME        "/home/lhermitte/research/projects/xpcs-aps-chx-project/sample_data/flow10crlT0_EGhtd_011_66/flow10crlT0_EGhtd_011_66_master.h5"
// TODO : add all data sets
#define DATASET         "entry/data_000001"
#define DIM0            1000
#define ADIM0           1065
#define ADIM1           1030

int main (void){
    hid_t       file, filetype, memtype, space, dset;
                                                /* Handles */
    herr_t      status;
    hsize_t     dims[1] = {DIM0},
                adims[2] = {ADIM0, ADIM1};
    int         ***rdata,                       /* Read buffer */
                ndims,
                i, j, k;
    int         eiger_groups = 0;
    int         ind = 0;

    /*
     * Now we begin the read section of this example.  Here we assume
     * the dataset and array have the same name and rank, but can have
     * any size.  Therefore we must allocate a new array to read in
     * data using malloc().
     */

    /*
     * Open file and dataset.
     */
    //printf("Reading EIGER groups\n");
    eiger_groups = read_eiger_groups(FILENAME);
    //printf("done \n");
    file = H5Fopen (FILENAME, H5F_ACC_RDONLY, H5P_DEFAULT);
    for(ind=0; ind < eiger_groups; ind++){
        printf("opening dataset: %d\n", ind);
        dset = H5Dopen(file, DATASET, H5P_DEFAULT);

        /*
         * Get the datatype and its dimensions.
         */
        printf("getting file type\n");
        filetype = H5Dget_type (dset);
        printf("getting dims\n");
        ndims = H5Tget_array_dims (filetype, adims);
        printf("dims are %d", ndims);
    
        /*
         * Get dataspace and allocate memory for read buffer.  This is a
         * three dimensional dataset when the array datatype is included so
         * the dynamic allocation must be done in steps.
         */
        space = H5Dget_space (dset);
        //ndims = H5Sget_simple_extent_dims (space, dims, NULL);
    
        /*
         * Allocate array of pointers to two-dimensional arrays (the
         * elements of the dataset.
         */
        rdata = (int ***) malloc (dims[0] * sizeof (int **));
    
        /*
         * Allocate two dimensional array of pointers to rows in the data
         * elements.
         */
        rdata[0] = (int **) malloc (dims[0] * adims[0] * sizeof (int *));
    
        /*
         * Allocate space for integer data.
         */
        rdata[0][0] = (int *) malloc (dims[0] * adims[0] * adims[1] * sizeof (int));
    
        /*
         * Set the members of the pointer arrays allocated above to point
         * to the correct locations in their respective arrays.
         */
        for (i=0; i<dims[0]; i++) {
            rdata[i] = rdata[0] + i * adims[0];
            for (j=0; j<adims[0]; j++)
                rdata[i][j] = rdata[0][0] + (adims[0] * adims[1] * i) +
                            (adims[1] * j);
        }
    
        /*
         * Create the memory datatype.
         */
        memtype = H5Tarray_create (H5T_NATIVE_INT, 2, adims);
    
        /*
         * Read the data.
         */
        status = H5Dread (dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                    rdata[0][0]);

    }
    /*
     * Output the data to the screen.
    for (i=0; i<dims[0]; i++) {
        printf ("%s[%d]:\n", DATASET, i);
        for (j=0; j<adims[0]; j++) {
            printf (" [");
            for (k=0; k<adims[1]; k++)
                printf (" %3d", rdata[i][j][k]);
            printf ("]\n");
        }
        printf("\n");
    }
     */

    /*
     * Close and release resources.
     */
    free (rdata[0][0]);
    free (rdata[0]);
    free (rdata);
    status = H5Dclose (dset);
    status = H5Sclose (space);
    status = H5Tclose (filetype);
    status = H5Tclose (memtype);
    status = H5Fclose (file);

    return 0;
}
