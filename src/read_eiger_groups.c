/************************************************************
 * 
 * This code will read from an hdf5 file and return the EIGER groups
 ************************************************************/
#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"

typedef struct hdf5EigerDatasetInfo {
    int cnt;
    int max;
} hdf5EigerDatasetInfo;

herr_t count_groups(hid_t loc_id, const char *name, const H5L_info_t *info,
                         void *operator_data);

/*
 * Operator function to be called by H5Literate.
 */

int read_eiger_groups(char * filename){
    hid_t           file;           /* Handle */
    herr_t          status;
    hdf5EigerDatasetInfo h5ed; // hdf5 Eiger dataset info function
    h5ed.cnt=0;
    h5ed.max=0;

    /*
     * Open file.
     */
    //printf("opening\n");
    file = H5Fopen (filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    //printf("done\n");

    /*
     * Begin iteration.
     */
    //printf ("Objects in root group:\n");
    status = H5Literate_by_name (file, "/entry", H5_INDEX_NAME, H5_ITER_NATIVE, NULL, count_groups, &h5ed, H5P_DEFAULT);

    /*
     * Close and release resources.
     */
    status = H5Fclose (file);
    printf("Final\nMax num: %d\n", h5ed.max);
    printf("tot num: %d\n", h5ed.cnt);

    // Valid only if the max and count are equal
    if(h5ed.max == h5ed.cnt - 1){
        return h5ed.cnt;
    }
    else{
        return -1;
    }
}


/************************************************************

  Operator function.  Prints the name and type of the object
  being examined.

 ************************************************************/
herr_t count_groups(hid_t loc_id, const char *name, const H5L_info_t *info,
            void *op_data)
{
    herr_t          status;
    H5O_info_t      infobuf;
    int dset_num;
    hdf5EigerDatasetInfo *h5edp = (hdf5EigerDatasetInfo *)op_data;

    if(strncmp(name, "data_", 5) == 0){
        h5edp->cnt += 1;
        dset_num = atoi(name+5);
        if(dset_num > h5edp->max){
            h5edp->max = dset_num;
        }
        //printf("Dataset: %s\n",name);
        //printf("max num: %d\n",h5edp->max);
        //printf("tot num: %d\n",h5edp->cnt);
    }
    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library.
     */
    /* Just an example:
    status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    switch (infobuf.type) {
        case H5O_TYPE_GROUP:
            printf ("  Group: %s\n", name);
            break;
        case H5O_TYPE_DATASET:
            printf ("  Dataset: %s\n", name);
            break;
        case H5O_TYPE_NAMED_DATATYPE:
            printf ("  Datatype: %s\n", name);
            break;
        default:
            printf ( "  Unknown: %s\n", name);
    }
    */

    return 0;
}
