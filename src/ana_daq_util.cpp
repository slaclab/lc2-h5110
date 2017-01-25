#include <cstdio>
#include <stdexcept>
#include "hdf5.h"

#include "ana_daq_util.h"


void check_nonneg(int err, const char *msg, int lineno, const char *fname) {
  if (err < 0) {
    fprintf(::stderr, "ERROR: %s line=%d  file=%s\n", msg, lineno, fname);
    throw std::runtime_error("FATAL: check_nonneg");
  } 
}


DsetInfo create_1d_dataset(hid_t parent, const char *dset, hid_t h5_type, hsize_t chunk_size, size_t type_size_bytes) {
  int rank = 1;
  static hsize_t current_dims = 0;
  static hsize_t maximum_dims = H5S_UNLIMITED;
  hid_t space_id = H5Screate_simple(rank, &current_dims, &maximum_dims);
  CHECK_NONNEG(space_id, "create_dataset H5Screate_simple");
  
  hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
  CHECK_NONNEG(plist_id, "H5Pcreate");
  int chunk_rank = 1;
  CHECK_NONNEG(H5Pset_chunk(plist_id, chunk_rank, &chunk_size),"set_chunk");
  
  hid_t access_id = H5Pcreate(H5P_DATASET_ACCESS);
  CHECK_NONNEG(access_id, "H5Pcreate");
  
  size_t rdcc_nslots = 101;
  size_t rdcc_nbytes = chunk_size * type_size_bytes * 3;
  double rdcc_w0 = 0.75;
  CHECK_NONNEG(H5Pset_chunk_cache(access_id, rdcc_nslots, rdcc_nbytes, rdcc_w0), "H5Pset_chunk_cache");

  hid_t h5_dset = H5Dcreate2(parent, dset, h5_type, space_id, H5P_DEFAULT, plist_id, access_id);
  CHECK_NONNEG(h5_dset, dset);

  CHECK_NONNEG(H5Sclose(space_id), "H5Sclose");
  CHECK_NONNEG(H5Pclose(plist_id), "H5Pclose");
  CHECK_NONNEG(H5Pclose(access_id), "H5Aclose");
  
  return DsetInfo(h5_dset, 0L);
}

DsetInfo create_3d_dataset(hid_t parent, const char *dset, hid_t h5_type,
                        int nrows, int ncols,
                        int chunk_size, size_t type_size_bytes) {
  int rank = 3;
  static hsize_t current_dims[3] = {0, hsize_t(nrows), hsize_t(ncols)};
  static hsize_t maximum_dims[3] = {H5S_UNLIMITED, hsize_t(nrows), hsize_t(ncols)};
  static hsize_t chunk_dims[3] = {hsize_t(chunk_size), hsize_t(nrows), hsize_t(ncols)};

  hid_t space_id = H5Screate_simple(rank, current_dims, maximum_dims);
  CHECK_NONNEG(space_id, "create_dataset H5Screate_simple");
  
  hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
  CHECK_NONNEG(plist_id, "H5Pcreate");  
  CHECK_NONNEG(H5Pset_chunk(plist_id, rank, chunk_dims), "set_chunk");
  
  //  hid_t access_id = H5Pcreate(H5P_DATASET_ACCESS);
  //  CHECK_NONNEG(access_id, "H5Pcreate");
  
  //  size_t rdcc_nslots = 101;
  //  size_t rdcc_nbytes = chunk_size * type_size_bytes * 3;
  //  double rdcc_w0 = 0.75;
  //  CHECK_NONNEG(H5Pset_chunk_cache(access_id, rdcc_nslots, rdcc_nbytes, rdcc_w0), "H5Pset_chunk_cache");

  //  hid_t h5_dset = H5Dcreate2(parent, dset, h5_type, space_id, H5P_DEFAULT, plist_id, access_id);
  hid_t h5_dset = H5Dcreate2(parent, dset, h5_type, space_id, H5P_DEFAULT, plist_id, H5P_DEFAULT);
  CHECK_NONNEG(h5_dset, dset);

  CHECK_NONNEG(H5Sclose(space_id), "H5Sclose");
  CHECK_NONNEG(H5Pclose(plist_id), "H5Pclose");
  //  CHECK_NONNEG(H5Pclose(access_id), "H5Aclose");
  
  return DsetInfo(h5_dset, 0);
}

hsize_t append_many_to_1d_dset(DsetInfo &dset_info, hsize_t count, long *data) {
  hsize_t start = dset_info.extent;
  dset_info.extent += count;
  CHECK_NONNEG(H5Dset_extent(dset_info.dset_id, &dset_info.extent), "increasing extent for vlen");
  hid_t dspace_id = H5Dget_space(dset_info.dset_id);
  CHECK_NONNEG(dspace_id, "H5Dget_space");
  CHECK_NONNEG( H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET,
                                    &start, NULL, &count, NULL),
                "H5Sselect_hyperslab - append many 1d");
  hid_t memory_dspace_id = H5Screate_simple(1, &count, &count);
  CHECK_NONNEG( H5Dwrite(dset_info.dset_id,
                         H5T_NATIVE_LONG,
                         memory_dspace_id,
                         dspace_id,
                         H5P_DEFAULT,
                         data), "H5Dwrite many 1d");
  CHECK_NONNEG( H5Sclose(dspace_id), "1d many close file dspace");    
  CHECK_NONNEG( H5Sclose(memory_dspace_id), "1d many close memory dspace");    
  
  return start;
}

void append_to_1d_dset(DsetInfo &dset_info, long value) {
  hsize_t current_size = dset_info.extent;
  dset_info.extent += 1;
  static hsize_t count = 1; 
  CHECK_NONNEG( H5Dset_extent(dset_info.dset_id, &dset_info.extent), "increasing extent for 1d");
  hid_t dspace_id = H5Dget_space(dset_info.dset_id);
  CHECK_NONNEG(dspace_id, "H5Dget_space");
  CHECK_NONNEG( H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET,
                                    &current_size, NULL, &count, NULL),
                "H5Sselect_hyperslab - append 1d");
  hid_t memory_dspace_id = H5Screate_simple(1, &count, &count);
  CHECK_NONNEG( H5Dwrite(dset_info.dset_id,
                         H5T_NATIVE_LONG,
                         memory_dspace_id,
                         dspace_id,
                         H5P_DEFAULT,
                         &value), "H5Dwrite 1d");
  CHECK_NONNEG( H5Sclose(dspace_id), "1d close");    
  CHECK_NONNEG( H5Sclose(memory_dspace_id), "1d close");    
}

void append_to_3d_dset(DsetInfo &dset_info, int nrows, int ncols, short *data) {
  hsize_t current = dset_info.extent;
  dset_info.extent += 1;
  hsize_t new_size[3] = {dset_info.extent, hsize_t(nrows), hsize_t(ncols)};
  CHECK_NONNEG(H5Dset_extent(dset_info.dset_id, new_size), "increasing extent for 3d");
  hid_t dspace_id = H5Dget_space(dset_info.dset_id);
  CHECK_NONNEG(dspace_id, "H5Dget_space");

  hsize_t offset[3] = {current, 0, 0};
  hsize_t count[3] = {1, hsize_t(nrows), hsize_t(ncols)};
  CHECK_NONNEG( H5Sselect_hyperslab(dspace_id,
                                    H5S_SELECT_SET,
                                    offset,
                                    NULL,
                                    count,
                                    NULL),
                "H5Sselect_hyperslab - append 1d");  
  hid_t memory_dspace_id = H5Screate_simple(3, count, NULL);
  CHECK_NONNEG( H5Dwrite(dset_info.dset_id,
                         H5T_NATIVE_SHORT,
                         memory_dspace_id,
                         dspace_id,
                         H5P_DEFAULT,
                         data), "H5Dwrite 3d");
  CHECK_NONNEG( H5Sclose(dspace_id), "3d close");    
  CHECK_NONNEG( H5Sclose(memory_dspace_id), "3d close");    
}

int foo() {
  return 3;
}


