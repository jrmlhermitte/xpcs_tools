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
#define DATASETPREFIX "entry/data_"
#define RANK         3
#define RANK_OUT     3

// Data set strings for the EIGER
#define WAVELENGTH "entry/instrument/monochromator/wavelength"
#define BEAM_CENTER_X "entry/instrument/detector/beam_center_x"
#define BEAM_CENTER_Y "entry/instrument/detector/beam_center_x"
#define COUNT_TIME "entry/instrument/detector/count_time"
#define X_PIXEL_SIZE "entry/instrument/detector/x_pixel_size"
#define Y_PIXEL_SIZE "entry/instrument/detector/y_pixel_size"


/*
 *
 * In [6]: list(f['entry/instrument/detector'].keys())
 * Out[6]: 
 * ["beam_center_x",
 * "beam_center_y",
 * "bit_depth_readout",
 * "count_time",
 * "countrate_correction_applied",
 * "description",
 * "detectorSpecific",
 * "detector_distance",
 * "detector_number",
 * "detector_readout_time",
 * "efficiency_correction_applied",
 * "flatfield_correction_applied",
 * "frame_time",
 * "pixel_mask_applied",
 * "sensor_material",
 * "sensor_thickness",
 * "threshold_energy",
 * "virtual_pixel_correction_applied",
 * "x_pixel_size",
 * "y_pixel_size"]
 *
 */


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


int read_multifile_header(hid_t file, multifile_header_t * mheader){
    /* read multifile header
     *
     * file : the file id
     * mheader : the multifile header
     *
     */
    double wavelength, beam_center_x, beam_center_y;
    hid_t       dataset;         /* handles */
    herr_t      status;

    printf("Reading wavelength\n");
    dataset = H5Dopen2(file, WAVELENGTH, H5P_DEFAULT);
    status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                     &(mheader->incident_wavelength));
    printf("got %lf\n", mheader->incident_wavelength);
    printf("Reading beam_center_x\n");
    dataset = H5Dopen2(file, BEAM_CENTER_X, H5P_DEFAULT);
    status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                     &(mheader->beam_center_x));
    printf("got %lf\n", mheader->beam_center_x);
    printf("Reading beam_center_y\n");
    dataset = H5Dopen2(file, BEAM_CENTER_X, H5P_DEFAULT);
    status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                     &(mheader->beam_center_x));
    printf("got %lf\n", mheader->beam_center_y);
    printf("Reading x pixel size\n");
    dataset = H5Dopen2(file, X_PIXEL_SIZE, H5P_DEFAULT);
    status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                     &(mheader->x_pixel_size));
    printf("got %lf\n", mheader->x_pixel_size);
    printf("Reading y pixel size\n");
    dataset = H5Dopen2(file, Y_PIXEL_SIZE, H5P_DEFAULT);
    status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                     &(mheader->y_pixel_size));
    printf("got %lf\n", mheader->y_pixel_size);
}

int raw_compress_file(char * filename, char *dataset_prefix, char *out_filename){
    multifile_header_t multifile_header;

    hid_t       file, dataset;         /* handles */
    hid_t       datatype, dataspace;
    hid_t       memspace;
    H5T_class_t t_class;                 /* data type class */
    H5T_order_t order;                 /* data order */
    size_t      size;                  /*
				        * size of the data element
				        * stored in file
				        */
    hsize_t     dims[3];    /* dataset dimensions */
    herr_t      status;

    // the output file
    FILE * fout;
    printf("opening file : %s\n", filename);
    fout = fopen(out_filename, "w");
    printf("done\n");


    // 10 MB buffer for pos and val
    int *posbuffer = (int *)malloc(10000000*sizeof(unsigned int));
    int *valbuffer = (int *)malloc(10000000*sizeof(unsigned short int));
    int dlen;
    int     i, j, k, status_n, rank;

    char *dataset_name = malloc(80);
    int dset_number;
    int image_number;

    /*
     * Open the file and the dataset.
     */
    file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);

    // read header
    printf("reading multifile header\n");
    read_multifile_header(file, &multifile_header);
    printf("done\n");

    // write the header
    printf("writing header\n");
    fwrite(&multifile_header, sizeof(multifile_header_t), 1, fout);
    printf("done\n");

    // TODO : make a function
    // just try first number to get dimensions of data set
    sprintf(dataset_name, "%s%06d", dataset_prefix, 0);
    printf("grabbing dataset %s\n", dataset_name);
    dataset = H5Dopen2(file, dataset_name, H5P_DEFAULT);

    /*
     * Get datatype and dataspace handles and then query
     * dataset class, order, size, rank and dimensions.
     */
    datatype  = H5Dget_type(dataset);     /* datatype handle */
    t_class     = H5Tget_class(datatype);

    size  = H5Tget_size(datatype);
    printf(" Data size is %d \n", (int)size);

    dataspace = H5Dget_space(dataset);    /* dataspace handle */
    rank      = H5Sget_simple_extent_ndims(dataspace);
    status_n  = H5Sget_simple_extent_dims(dataspace, dims, NULL);
    printf("rank %d, dimensions %lu x %lu x %lu \n", rank,
	   (unsigned long)(dims[0]), (unsigned long)(dims[1]),
       (unsigned long)(dims[2]));

    
    // Now declare memory, everything for first array
    printf("allocating memory for data\n");
    // interesting. if i made a 1D array this is slower
    //int data_out[1][dims[1]][dims[2]]; /* output buffer */
    int data_out[dims[1]*dims[2]]; /* output buffer */
    //int *data_out = (int *)malloc(dims[1]*dims[2]*sizeof(int)); /* output buffer */
    printf("done\n");

    hsize_t *count = (hsize_t *)malloc(rank*sizeof(int));              /* size of the hyperslab in the file */
    hsize_t  offset[rank];             /* hyperslab offset in the file */

    hsize_t     dimsm[1];   /* memory space dimensions */
    hsize_t  count_out[1];          /* size of the hyperslab in memory */
    hsize_t  offset_out[1];         /* hyperslab offset in memory */


    int nimg; // to iterate over image number
    int ind;
    printf("iterating\n");
    image_number = 0;
    /*
     * Define hyperslab in the dataset.
     */
    offset[0] = 0;
    offset[1] = 0;
    offset[2] = 0;
    count[0]  = 1;
    count[1]  = dims[1];
    count[2]  = dims[2];
    //status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL,
				                 //count, NULL);

    /*
     * Define the memory dataspace.
     */
    // flatten
    dimsm[0] = dims[1]*dims[2];
    //dimsm[1] = dims[1];
    //dimsm[2] = dims[2];
    // rank, dims, ...
    memspace = H5Screate_simple(1, dimsm, NULL);

    /*
     * Define memory hyperslab.
     */
    offset_out[0] = 0;
    //offset_out[1] = 0;
    //offset_out[2] = 0;
    count_out[0]  = dimsm[0];
    //count_out[0]  = 1;
    //count_out[1]  = dimsm[1];
    //count_out[2]  = dimsm[2];
    status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL,
				                 count_out, NULL);
    
    for (dset_number = 0; dset_number < 10; dset_number++){
        sprintf(dataset_name, "%s%06d", dataset_prefix, dset_number);
        //printf("grabbing dataset %s\n", dataset_name);
        dataset = H5Dopen2(file, dataset_name, H5P_DEFAULT);
    
        dataspace = H5Dget_space(dataset);    /* dataspace handle */
    
    
        //printf("begin loop\n");
        /*
         * Read data from hyperslab in the file into the hyperslab in
         * memory and display.
         */
        //status = H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace,
    		     //H5P_DEFAULT, data_out);
        printf("begin image read\n");
        for (nimg = 0; nimg < dims[0]; nimg++){
            //printf("reading image # %d\n",nimg);
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
            for (i = 0; i < dimsm[0]; i++) {
            //for (i = 0; i < dimsm[1]; i++) {
                //for (j = 0; j < dimsm[2]; j++) {
                    ind = i*dimsm[2] + j;
                    // the second statement is just checking the data
                    // is not a bad pixel
                    // TODO : use the mask
                    //if(data_out[0][i][j] != 0 && data_out[0][i][j] < 10000){
                        ////printf("dlen %d\n",dlen);
                        //posbuffer[dlen] = ind;
                        //valbuffer[dlen] = data_out[0][i][j];
                        //dlen++;
                    //}
                    if(data_out[i] != 0 && data_out[i] < 10000){
                        //printf("dlen %d\n",dlen);
                        posbuffer[dlen] = i;
                        valbuffer[dlen] = data_out[i];
                        dlen++;
                    }
            }
                //}
            //}
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
    free(posbuffer);
    free(valbuffer);
    free(dataset_name);
    free(count);
    H5Tclose(datatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Fclose(file);
    fclose(fout);

    return 0;
}

int main(int argc, char ** argv){
    char * dataset_prefix;
    char * out_filename;
    char *filename;
    if (argc < 2 || argc > 4){
        printf("Usage : compress_file filename <dataset_prefix> <outfilename>\n");
        printf("(data_setname defaults to %s\n)", DATASETPREFIX);
        return -1;
    }
    filename = argv[1];
    if(argc == 3){
        dataset_prefix = argv[2];
    }else{
        dataset_prefix = DATASETPREFIX;
    }
    printf("Using dataset prefix %s\n", dataset_prefix);
    if(argc == 4){
        out_filename = argv[3];
    }else{
        out_filename = "test.bin";
    }
    printf("Writing output to filename %s\n", out_filename);

    raw_compress_file(filename, dataset_prefix, out_filename);
}

