# makefile
# avg_multi : average a multifile
# read_hdf5 : read hdf5 (eventually compress
#


avg_multi:
	gcc -o avg_multi avg_multi.c -Wall

avg_multi_bnl:
	gcc -o avg_multi_bnl avg_multi_bnl.c -Wall

read_hdf5: read_eiger_groups.o read_eiger_hdf5.o
	h5cc read_eiger_groups.o read_eiger_hdf5.o -o read_eiger_hdf5

read_eiger_groups.o: read_eiger_groups.c
	h5cc -c read_eiger_groups.c

read_eiger_hdf5.o: read_eiger_hdf5.c
	h5cc -c read_eiger_hdf5.c

compress_file:
	#h5cc -o compress_file compress_file.c
	cythonize compress_file.pyx
	LD_LIBRARY_PATH=/home/lhermitte/miniconda3/envs/py3/lib h5cc -shared -o compress_file.so -I/home/lhermitte/miniconda3/include/python3.6m  compress_file.c

