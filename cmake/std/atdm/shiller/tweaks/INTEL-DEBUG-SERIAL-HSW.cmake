# Disable test that runs over 30 min currently (#2446)
ATDM_SET_ENABLE(PanzerAdaptersSTK_MixedPoissonExample-ConvTest-Hex-Order-3_DISABLE ON)

ATDM_SET_CACHE(KokkosKernels_sparse_serial_MPI_1_EXTRA_ARGS

# Disable some unit tests that run too slow in this DEBUG build (#2827)
"--gtest_filter=-serial.sparse_block_gauss_seidel_double_int_int_TestExecSpace:serial.sparse_trsv_mv_double_int_int_LayoutLeft_TestExecSpace"
  CACHE STRING )

# This test fails consistently with a major numerical error (#2474)
ATDM_SET_ENABLE(Piro_MatrixFreeDecorator_UnitTests_MPI_4_DISABLE ON)

# Disable perpetually failing KokkosCore test (#3233)
ATDM_SET_ENABLE(KokkosCore_UnitTest_Serial_MPI_1_DISABLE ON)
