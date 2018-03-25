# makefile
# avg_multi : average a multifile
# read_hdf5 : read hdf5 (eventually compress
#


avg_multi:
	gcc -o avg_multi avg_multi.c -Wall

read_hdf5: read_eiger_groups.o read_eiger_hdf5.o
	h5cc read_eiger_groups.o read_eiger_hdf5.o -o read_eiger_hdf5

read_eiger_groups.o: read_eiger_groups.c
	h5cc -c read_eiger_groups.c

read_eiger_hdf5.o: read_eiger_hdf5.c
	h5cc -c read_eiger_hdf5.c
