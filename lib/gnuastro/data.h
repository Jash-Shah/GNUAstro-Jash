/*********************************************************************
data -- Structure and functions to represent/work with data
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2015, Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#ifndef __GAL_DATA_H__
#define __GAL_DATA_H__

/* Include other headers if necessary here. Note that other header files
   must be included before the C++ preparations below */
#include <math.h>
#include <limits.h>
#include <stdint.h>

#include <wcslib/wcs.h>
#include <gsl/gsl_complex.h>

#include <gnuastro/linkedlist.h>

/* When we are within Gnuastro's building process, `IN_GNUASTRO_BUILD' is
   defined. In the build process, installation information (in particular
   `GAL_CONFIG_SIZEOF_SIZE_T' that we need below) is kept in
   `config.h'. When building a user's programs, this information is kept in
   `gnuastro/config.h'. Note that all `.c' files in Gnuastro's source must
   start with the inclusion of `config.h' and that `gnuastro/config.h' is
   only created at installation time (not present during the building of
   Gnuastro).*/
#ifndef IN_GNUASTRO_BUILD
#include <gnuastro/config.h>
#endif

/* C++ Preparations */
#undef __BEGIN_C_DECLS
#undef __END_C_DECLS
#ifdef __cplusplus
# define __BEGIN_C_DECLS extern "C" {
# define __END_C_DECLS }
#else
# define __BEGIN_C_DECLS                /* empty */
# define __END_C_DECLS                  /* empty */
#endif
/* End of C++ preparations */



/* Actual header contants (the above were for the Pre-processor). */
__BEGIN_C_DECLS  /* From C++ preparations */





/* Macros to identify the type of data. */
enum gal_data_types
{
  GAL_DATA_TYPE_INVALID,     /* Invalid (=0 by C standard).             */

  GAL_DATA_TYPE_BIT,         /* 1 bit                                   */
  GAL_DATA_TYPE_UINT8,       /* 8-bit  unsigned integer.                */
  GAL_DATA_TYPE_INT8,        /* 8-bit  signed   integer.                */
  GAL_DATA_TYPE_UINT16,      /* 16-bit unsigned integer.                */
  GAL_DATA_TYPE_INT16,       /* 16-bit signed   integer.                */
  GAL_DATA_TYPE_UINT32,      /* 32-bit unsigned integer.                */
  GAL_DATA_TYPE_INT32,       /* 32-bit signed   integer.                */
  GAL_DATA_TYPE_UINT64,      /* 64-bit unsigned integer.                */
  GAL_DATA_TYPE_INT64,       /* 64-bit signed   integer.                */
  GAL_DATA_TYPE_FLOAT32,     /* 32-bit single precision floating point. */
  GAL_DATA_TYPE_FLOAT64,     /* 64-bit double precision floating point. */
  GAL_DATA_TYPE_COMPLEX32,   /* Complex 32-bit floating point.          */
  GAL_DATA_TYPE_COMPLEX64,   /* Complex 64-bit floating point.          */
  GAL_DATA_TYPE_STRING,      /* String of characters.                   */
  GAL_DATA_TYPE_STRLL,       /* Linked list of strings.                 */
};

/* `size_t' is 4 and 8 bytes on 32 and 64 bit systems respectively. In both
   cases, the standard defines `size_t' to be unsigned. During
   `./configure' the sizeof size_t was found and is stored in
   `GAL_CONFIG_SIZEOF_SIZE_T'. */
#if GAL_CONFIG_SIZEOF_SIZE_T == 4
#define GAL_DATA_TYPE_SIZE_T GAL_DATA_TYPE_UINT32
#else
#define GAL_DATA_TYPE_SIZE_T GAL_DATA_TYPE_UINT64
#endif





/* Main data structure.

   mmap (keep data outside of RAM)
   -------------------------------

     `mmap' is C's facility to keep the data on the HDD/SSD instead of
     inside the RAM. This can be very useful for large data sets which can
     be very memory intensive. Ofcourse, the speed of operation greatly
     decreases when defining not using RAM, but that is worth it because
     increasing RAM might not be possible. So in `gal_data_t' when the size
     of the requested array (in bytes) is larger than a certain minimum
     size (in bytes), Gnuastro won't write the array in memory but on
     non-volatile memory (like HDDs and SSDs) as a file in the
     `./.gnuastro' directory of the directory it was run from.

         - If mmapname==NULL, then the array is allocated (using malloc, in
           the RAM), otherwise its is mmap'd (is actually a file on the
           ssd/hdd).

         - minmapsize is stored in the data structure to allow any
           derivative data structures to follow the same number and decide
           if they should be mmap'd or allocated.

         - `minmapsize' ==  0: array is definitely mmap'd.

         - `minmapsize' == -1: array is definitely in RAM.


   block (work with only a subset of the data)
   -------------------------------------------

     In many contexts, it is desirable to slice the data set into subsets
     or tiles, not necessarily overlapping. In such a way that you can work
     on each independently. One option is to copy that region to a separate
     allocated space, but in many contexts this isn't necessary and infact
     can be a big burden on CPU/Memory usage. The `block' pointer in
     `gal_data_t' is defined for situations where allocation is not
     necessary: you just want to read the data or write to it
     independently, or in coordination with, other regions of the
     dataset. Added with parallel processing, this can greatly improve the
     time/memory consumption.

     See the figure below for example: assume the larger box is a
     contiguous block of memory that you are interpretting as a 2D
     array. But you only want to work on the region marked by the smaller
     box.

                    tile->block = larger
                ---------------------------
                |                         |
                |           tile          |
                |        ----------       |
                |        |        |       |
                |        |_       |       |
                |        |*|      |       |
                |        ----------       |
                |_                        |
                |*|                       |
                ---------------------------

     To use `gal_data_t's block concept, you allocate a `gal_data_t *tile'
     which is initialized with the pointer to the first element in the
     sub-array (as its `array' argument). Note that this is not necessarily
     the first element in the larger array. You can set the size of the
     tile along with the initialization as you please. Recall that, when
     given a non-NULL pointer as `array', `gal_data_initialize' (and thus
     `gal_data_alloc') do not allocate any space and just uses the given
     pointer for the new `array' element of the `gal_data_t'. So your
     `tile' data structure will not be pointing to a separately allocated
     space.

     After the allocation is done, you just point `tile->block' to the
     `gal_data_t' which hosts the larger array. Afterwards, the programs
     that take in the `sub' array can check the `block' pointer to see how
     to deal with dimensions and increments (strides) while working on the
     `sub' datastructure. For example to increment along the vertical
     dimension, the program must look at index i+W (where `W' is the width
     of the larger array, not the tile).

     Since the block structure is defined as a pointer, arbitrary levels of
     tesselation/griding are possible. Therefore, just like a linked list,
     it is important to have the `block' pointer of the largest dataset set
     to NULL. Normally, you won't have to worry about this, because
     `gal_data_initialize' (and thus `gal_data_alloc') will set it to NULL
     by default (just remember not to change it). You can then only change
     the `block' element for the tiles you define over the allocated space.

     In Gnuastro, there is a separate library for tiling operations called
     `tile.h', see the functions there for tools to effectively use this
     feature. This approach to dealing with parts of a larger block was
     inspired from the way the GNU Scientific Library does it in the
     "Vectors and Matrices" chapter.
*/
typedef struct gal_data_t
{
  /* Basic information on array of data. */
  void              *array;  /* Array keeping data elements.               */
  uint8_t             type;  /* Type of data (from `gal_data_alltypes').   */
  size_t              ndim;  /* Number of dimensions in the array.         */
  size_t            *dsize;  /* Size of array along each dimension.        */
  size_t              size;  /* Total number of data-elements.             */
  char           *mmapname;  /* File name of the mmap.                     */
  size_t        minmapsize;  /* Minimum number of bytes to mmap the array. */

  /* WCS information. */
  int                 nwcs;  /* for WCSLIB: no. coord. representations.    */
  struct wcsprm       *wcs;  /* WCS information for this dataset.          */

  /* Content descriptions. */
  int               status;  /* Any context-specific status value.         */
  char               *name;  /* e.g., EXTNAME, or column, or keyword.      */
  char               *unit;  /* Units of the data.                         */
  char            *comment;  /* A more detailed description of the data.   */

  /* For printing */
  int             disp_fmt;  /* See `gal_table_diplay_formats'.            */
  int           disp_width;  /* Width of space to print in ASCII.          */
  int       disp_precision;  /* Precision to print in ASCII.               */

  /* Pointers to other data structures. */
  struct gal_data_t  *next;  /* To use it as a linked list if necessary.   */
  struct gal_data_t *block;  /* `gal_data_t' of hosting block, see above.  */
} gal_data_t;




/*************************************************************
 **************        Type information        ***************
 *************************************************************/
char *
gal_data_type_as_string(uint8_t type, int long_name);

uint8_t
gal_data_string_as_type(char *str);

void
gal_data_type_min(uint8_t type, void *in);

void
gal_data_type_max(uint8_t type, void *in);

int
gal_data_is_linked_list(uint8_t type);

int
gal_data_out_type(gal_data_t *first, gal_data_t *second);



/*********************************************************************/
/*************         Size and allocation         *******************/
/*********************************************************************/
int
gal_data_dsize_is_different(gal_data_t *first, gal_data_t *second);

size_t
gal_data_sizeof(uint8_t type);

void *
gal_data_ptr_increment(void *pointer, size_t increment, uint8_t type);

size_t
gal_data_ptr_dist(void *earlier, void *later, uint8_t type);

void *
gal_data_malloc_array(uint8_t type, size_t size);

void *
gal_data_calloc_array(uint8_t type, size_t size);

void *
gal_data_alloc_number(uint8_t type, void *number);

void
gal_data_initialize(gal_data_t *data, void *array, uint8_t type, size_t ndim,
                    size_t *dsize, struct wcsprm *wcs, int clear,
                    size_t minmapsize, char *name, char *unit, char *comment);

gal_data_t *
gal_data_alloc(void *array, uint8_t type, size_t ndim, size_t *dsize,
               struct wcsprm *wcs, int clear, size_t minmapsize,
               char *name, char *unit, char *comment);

size_t
gal_data_string_fixed_alloc_size(gal_data_t *data);

void
gal_data_free_contents(gal_data_t *data);

void
gal_data_free(gal_data_t *data);



/*********************************************************************/
/*************        Array of data structures      ******************/
/*********************************************************************/
gal_data_t *
gal_data_array_calloc(size_t size);

void
gal_data_array_free(gal_data_t *data, size_t num, int free_array);



/*********************************************************************/
/*************    Data structure as a linked list   ******************/
/*********************************************************************/
void
gal_data_add_existing_to_ll(gal_data_t **list, gal_data_t *newnode);

void
gal_data_add_to_ll(gal_data_t **list, void *array, uint8_t type, size_t ndim,
                   size_t *dsize, struct wcsprm *wcs, int clear,
                   size_t minmapsize, char *name, char *unit, char *comment);

gal_data_t *
gal_data_pop_from_ll(struct gal_data_t **list);

void
gal_data_reverse_ll(gal_data_t **list);

size_t
gal_data_num_in_ll(struct gal_data_t *list);

gal_data_t **
gal_data_ll_to_array_of_ptrs(gal_data_t *list, size_t *num);

void
gal_data_free_ll(gal_data_t *list);



/*************************************************************
 **************            Copying             ***************
 *************************************************************/
gal_data_t *
gal_data_copy_to_new_type(gal_data_t *in, uint8_t newtype);

gal_data_t *
gal_data_copy_to_new_type_free(gal_data_t *in, uint8_t type);

gal_data_t *
gal_data_copy(gal_data_t *in);

void
gal_data_copy_element_same_type(gal_data_t *input, size_t index, void *ptr);



/*************************************************************
 **************              Write             ***************
 *************************************************************/
char *
gal_data_write_to_string(void *ptr, uint8_t type, int quote_if_str_has_space);



/*************************************************************
 **************              Read              ***************
 *************************************************************/
gal_data_t *
gal_data_string_to_number(char *string);

int
gal_data_string_to_type(void **out, char *string, uint8_t type);



__END_C_DECLS    /* From C++ preparations */

#endif           /* __GAL_DATA_H__ */
