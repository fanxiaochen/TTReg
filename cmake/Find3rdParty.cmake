set(ThirdParty_DIR ${PROJECT_SOURCE_DIR}/3rd-party)

set(CLAPACK_ROOT "/usr/local")
set(CLAPACK_INCLUDE_DIR ${CLAPACK_ROOT}/include)
set(CLAPACK_LIBRARY_DIR ${CLAPACK_ROOT}/lib)

set(BLAS_LIBRARY optimized libblas.a debug libblas.a)
set(LIBF2C_LIBRARY optimized libf2c.a debug libf2c.a)
set(LAPACK_LIBRARY optimized liblapack.a debug liblapack.a)
# set(BLAS_LIBRARY optimized blas.lib debug blasd.lib)
# set(LIBF2C_LIBRARY optimized libf2c.lib debug libf2cd.lib)
# set(LAPACK_LIBRARY optimized lapack.lib debug lapackd.lib)

# set(BLAS_LIBRARY blas_LINUX.a)
# set(LIBF2C_LIBRARY libf2c.a)
# set(LAPACK_LIBRARY lapack_LINUX.a)

include_directories(${CLAPACK_INCLUDE_DIR})
link_directories(${CLAPACK_LIBRARY_DIR})
LINK_LIBRARIES(blas lapack f2c)
set(ThirdParty_LIBS ${ThirdParty_LIBS} ${BLAS_LIBRARY} ${LAPACK_LIBRARY} ${LIBF2C_LIBRARY})