// @HEADER
// ***********************************************************************
//
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#include "Stokhos.hpp"
#include "Teuchos_CommandLineProcessor.hpp"

// Growth policies
const int num_growth_types = 2;
const Stokhos::GrowthPolicy growth_type_values[] = {
  Stokhos::SLOW_GROWTH, Stokhos::MODERATE_GROWTH };
const char *growth_type_names[] = { "slow", "moderate" };

// Product Basis types
enum ProductBasisType { COMPLETE, TENSOR, TOTAL, SMOLYAK };
const int num_prod_basis_types = 4;
const ProductBasisType prod_basis_type_values[] = {
  COMPLETE, TENSOR, TOTAL, SMOLYAK };
const char *prod_basis_type_names[] = {
  "complete", "tensor", "total", "smolyak" };

// Ordering types
enum OrderingType { TOTAL_ORDERING, LEXICOGRAPHIC_ORDERING };
const int num_ordering_types = 2;
const OrderingType ordering_type_values[] = {
  TOTAL_ORDERING, LEXICOGRAPHIC_ORDERING };
const char *ordering_type_names[] = {
  "total", "lexicographic" };

using Teuchos::rcp;
using Teuchos::RCP;
using Teuchos::Array;

template <typename TupleType>
class RCB {
public:
  typedef typename TupleType::value_type coord_type;
  typedef typename ArrayView<TupleType>::size_type size_type;

  struct CoordCompare {
    size_type d;
    CoordCompare(const size_type& d_) : d(d_) {}
    bool operator(const TupleType& a, const TupleType& b) const {
      return a(d) < b(d);
    }
  };

  struct Box {
    coord_type xmin, ymin, zmin, xmax, ymax, zmax;
    coord_type delta_x, delta_y, delta_z;
    size_type split;
    RCP<Box> left, right;
    Array<TupleType> coords;

    Box(const ArrayView<TupleType>& c) : coords(c.begin(), c.end()) {
      computeBoundingBox();
      computeSplittingDimension();
    }

    // Compute bounding box around points
    void computeBoundingBox(){
      xmin = coords[0](0); xmax = coords[0](0);
      ymin = coords[0](1); ymax = coords[0](1);
      zmin = coords[0](2); zmax = coords[0](2);
      for (size_type i=0; i<coords.size(); ++i) {
        coord_type x = coords[i](0);
        coord_type y = coords[i](1);
        coord_type z = coords[i](2);
        if (x < xmin) xmin = x;
        if (y < ymin) ymin = y;
        if (z < zmin) zmin = z;
        if (x > xmax) xmax = x;
        if (y > ymax) ymax = y;
        if (z > zmax) zmax = z;
      }
      delta_x = xmax - xmin;
      delta_y = ymax - ymin;
      delta_z = zmax - zmin;
    }

    // Compute dimension to split
    void computeSplittingDimension() {
      split = 0;
      if (delta_y > delta_x && delta_y > delta_z) split = 1;
      if (delta_z > delta_x && delta_z > delta_y) split = 2;
    }

    // Split box into two pieces with roughly equal numbers of points
    void split() {
      // Sort points based on splitting dimension
      CoordCompare cmp(split);
      std::sort(coords.begin(), coords.end(), cmp);

      // Divide coords into two bins of roughly equal size, keeping
      // coords with equal values for split dimension together
      size_type n = coords.size();
      size_type s = n / 2;
      while (s < n-1 && coords[s](split) == coords[s+1](split)) ++s;
      if (s > 0)
        left = rcp(new Box(coords.view(0, s)));
      if (s < n)
        right = rcp(new Box(coords.view(s, n-s)));
    }

  };

  //! Constructor
  RCB(const size_type& max_length_, const size_type& max_parts_) :
    max_length(max_length_), max_parts(max_parts_) {}

  //! Destructor
  ~RCB() {}

  //! Partition
  void partition(const ArrayView<TupleType>& coords) {

    // Create root bounding box
    root = rcp(new Box(coords));
    size_type parts = 1;

    // Create array of boxes that are too big
    Array< RCP<Box> > boxes_to_split;
    if (root->delta_x > max_length ||
        root->delta_y > max_length ||
        root->delta_z > max_length)
      boxes_to_split.push_back(root);

    // Split each box until all boxes are less than tolerance
    while(boxes_to_split.size() > 0 && parts < max_parts) {
      RCP<Box> box = boxes_to_split.pop_back();
      box.split();
      ++parts;
      if (box->left != Teuchos::null)
        if (box->left->delta_x > max_length ||
            box->left->delta_y > max_length ||
            box->left->delta_z > max_length)
          boxes_to_split.push_back(box->left);
      if (box->right != Teuchos::null)
        if (box->right->delta_x > max_length ||
            box->right->delta_y > max_length ||
            box->right->delta_z > max_length)
          boxes_to_split.push_back(box->right);
    }
  }

private:

  size_type max_length, max_parts;
  RCP<Box> root;

};

struct CijkData {
  typedef int value_type;
  int gid;
  int i, j, k;

  int operator() (int d) const {
    if (d == 0) return i;
    if (d == 1) return j;
    if (d == 2) return k;
    return -1;
  }
};

// Functions implementing Cijk tensor for geometric partitioning
namespace CoordPart {

  // Return number of vertices
  int get_number_of_objects(void *data, int *ierr) {
    TensorData *td = static_cast<TensorData*>(data);
    *ierr = ZOLTAN_OK;

    return td->Cijk->num_entries();
  }

  // Compute IDs and weights of each vertex
  void get_object_list(void *data, int sizeGID, int sizeLID,
                       ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                       int wgt_dim, float *obj_wgts, int *ierr) {
    TensorData *td = static_cast<TensorData*>(data);
    *ierr = ZOLTAN_OK;

    int n = td->Cijk->num_entries();
    for (int i=0; i<n; ++i) {
      globalID[i] = i;
      localID[i] = i;
    }

    // Do not set weights so Zoltan assumes equally weighted vertices
  }

  // Geometry dimension -- here 3 for (i,j,k)
  int get_num_geometry(void *data, int *ierr)
  {
    *ierr = ZOLTAN_OK;
    return 3;
  }

  // Get list of (i,j,k) tuples
  void get_geometry_list(void *data, int sizeGID, int sizeLID,
                         int num_obj,
                         ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                         int num_dim, double *geom_vec, int *ierr)
  {
    TensorData *td = static_cast<TensorData*>(data);
    *ierr = ZOLTAN_OK;

    int idx = 0;
    TensorData::Cijk_type::k_iterator k_begin = td->Cijk->k_begin();
    TensorData::Cijk_type::k_iterator k_end =   td->Cijk->k_end();
    for (TensorData::Cijk_type::k_iterator k_it=k_begin; k_it!=k_end;
         ++k_it) {
      int k = index(k_it);
      TensorData::Cijk_type::kj_iterator j_begin = td->Cijk->j_begin(k_it);
      TensorData::Cijk_type::kj_iterator j_end =   td->Cijk->j_end(k_it);
      for (TensorData::Cijk_type::kj_iterator j_it = j_begin; j_it != j_end;
           ++j_it) {
        int j = index(j_it);
        TensorData::Cijk_type::kji_iterator i_begin = td->Cijk->i_begin(j_it);
        TensorData::Cijk_type::kji_iterator i_end =   td->Cijk->i_end(j_it);
        for (TensorData::Cijk_type::kji_iterator i_it = i_begin; i_it != i_end;
             ++i_it) {
          int i = index(i_it);
          geom_vec[3*idx  ] = i;
          geom_vec[3*idx+1] = j;
          geom_vec[3*idx+2] = k;
          ++idx;
        }
      }
    }
  }

}


int main(int argc, char **argv)
{
  try {

    // Initialize Zoltan
    float version;
    int rc = Zoltan_Initialize(argc,argv,&version);
    TEUCHOS_ASSERT(rc == 0);

    // Setup command line options
    Teuchos::CommandLineProcessor CLP;
    CLP.setDocString(
      "This example generates the sparsity pattern for the block stochastic Galerkin matrix.\n");
    int d = 5;
    CLP.setOption("dimension", &d, "Stochastic dimension");
    int p = 3;
    CLP.setOption("order", &p, "Polynomial order");
    double drop = 1.0e-12;
    CLP.setOption("drop", &drop, "Drop tolerance");
    bool symmetric = true;
    CLP.setOption("symmetric", "asymmetric", &symmetric, "Use basis polynomials with symmetric PDF");
    Stokhos::GrowthPolicy growth_type = Stokhos::SLOW_GROWTH;
    CLP.setOption("growth", &growth_type,
                  num_growth_types, growth_type_values, growth_type_names,
                  "Growth type");
    ProductBasisType prod_basis_type = TOTAL;
    CLP.setOption("product_basis", &prod_basis_type,
                  num_prod_basis_types, prod_basis_type_values,
                  prod_basis_type_names,
                  "Product basis type");
    OrderingType ordering_type = LEXICOGRAPHIC_ORDERING;
    CLP.setOption("ordering", &ordering_type,
                  num_ordering_types, ordering_type_values,
                  ordering_type_names,
                  "Product basis ordering");
    double imbalance_tol = 1.2;
    CLP.setOption("imbalance", &imbalance_tol, "Imbalance tolerance");
    bool full = true;
    CLP.setOption("full", "linear", &full, "Use full or linear expansion");
    int tile_size = 32;
    CLP.setOption("tile_size", &tile_size, "Tile size");
    bool save_3tensor = false;
    CLP.setOption("save_3tensor", "no-save_3tensor", &save_3tensor,
                  "Save full 3tensor to file");
    std::string file_3tensor = "Cijk.dat";
    CLP.setOption("filename_3tensor", &file_3tensor,
                  "Filename to store full 3-tensor");

    // Parse arguments
    CLP.parse( argc, argv );

    // Basis
    Array< RCP<const Stokhos::OneDOrthogPolyBasis<int,double> > > bases(d);
    const double alpha = 1.0;
    const double beta = symmetric ? 1.0 : 2.0 ;
    for (int i=0; i<d; i++) {
        bases[i] = rcp(new Stokhos::JacobiBasis<int,double>(
                                  p, alpha, beta, true, growth_type));
    }
    RCP<const Stokhos::ProductBasis<int,double> > basis;
    typedef Stokhos::TotalOrderLess< Stokhos::MultiIndex<int> > total_less;
    typedef Stokhos::LexographicLess< Stokhos::MultiIndex<int> > lexo_less;
    if (prod_basis_type == COMPLETE)
      basis =
        rcp(new Stokhos::CompletePolynomialBasis<int,double>(
                       bases, drop));
    else if (prod_basis_type == TENSOR) {
      if (ordering_type == TOTAL_ORDERING)
        basis =
          rcp(new Stokhos::TensorProductBasis<int,double,total_less>(
                         bases, drop));
      else if (ordering_type == LEXICOGRAPHIC_ORDERING)
        basis =
          rcp(new Stokhos::TensorProductBasis<int,double,lexo_less>(
                         bases, drop));
    }
    else if (prod_basis_type == TOTAL) {
      if (ordering_type == TOTAL_ORDERING)
        basis =
          rcp(new Stokhos::TotalOrderBasis<int,double,total_less>(
                         bases, drop));
      else if (ordering_type == LEXICOGRAPHIC_ORDERING)
        basis =
          rcp(new Stokhos::TotalOrderBasis<int,double,lexo_less>(
                         bases, drop));
    }
    else if (prod_basis_type == SMOLYAK) {
      Stokhos::TotalOrderIndexSet<int> index_set(d, p);
      if (ordering_type == TOTAL_ORDERING)
        basis =
          rcp(new Stokhos::SmolyakBasis<int,double,total_less>(
                         bases, index_set, drop));
      else if (ordering_type == LEXICOGRAPHIC_ORDERING)
        basis =
          rcp(new Stokhos::SmolyakBasis<int,double,lexo_less>(
                         bases, index_set, drop));
    }

    // Triple product tensor
    typedef Stokhos::Sparse3Tensor<int,double> Cijk_type;
    RCP<Cijk_type> Cijk;
    if (full)
      Cijk = basis->computeTripleProductTensor();
    else
      Cijk = basis->computeLinearTripleProductTensor();

    int basis_size = basis->size();
    std::cout << "basis size = " << basis_size
              << " num nonzero Cijk entries = " << Cijk->num_entries()
              << std::endl;

    // File for saving sparse Cijk tensor and parts
    std::ofstream cijk_file;
    if (save_3tensor) {
      cijk_file.open(file_3tensor.c_str());
      cijk_file.precision(14);
      cijk_file.setf(std::ios::scientific);
      cijk_file << "i, j, k, part" << std::endl;
    }

    // Create zoltan
    Zoltan_Struct *zz = Zoltan_Create(MPI_COMM_WORLD);

    // Setup Zoltan parameters
    Zoltan_Set_Param(zz, "DEBUG_LEVEL", "2");

    // partitioning method
    Zoltan_Set_Param(zz, "LB_METHOD", "RCB");
    //Zoltan_Set_Param(zz, "LB_METHOD", "HSFC");
    Zoltan_Set_Param(zz, "NUM_GID_ENTRIES", "1");// global IDs are integers
    Zoltan_Set_Param(zz, "NUM_LID_ENTRIES", "1");// local IDs are integers
    //Zoltan_Set_Param(zz, "RETURN_LISTS", "ALL"); // export AND import lists
    Zoltan_Set_Param(zz, "RETURN_LISTS", "PARTS");
    Zoltan_Set_Param(zz, "OBJ_WEIGHT_DIM", "0"); // use Zoltan default vertex weights
    int num_parts = basis_size / tile_size;
    if (num_parts * tile_size < basis_size) ++num_parts;
    Zoltan_Set_Param(zz, "NUM_GLOBAL_PARTS", toString(num_parts).c_str());
    Zoltan_Set_Param(zz, "NUM_LOCAL_PARTS", toString(num_parts).c_str());
    Zoltan_Set_Param(zz, "IMBALANCE_TOL", toString(imbalance_tol).c_str());

    Zoltan_Set_Param(zz, "KEEP_CUTS", "1");
    Zoltan_Set_Param(zz, "RCB_RECTILINEAR_BLOCKS", "1");
    Zoltan_Set_Param(zz, "RCB_RECOMPUTE_BOX", "1");
    Zoltan_Set_Param(zz, "RCB_MAX_ASPECT_RATIO", "3");

    // Set query functions
    TensorData td; td.basis = basis; td.Cijk = Cijk;

    Zoltan_Set_Num_Obj_Fn(zz, CoordPart::get_number_of_objects, &td);
    Zoltan_Set_Obj_List_Fn(zz, CoordPart::get_object_list, &td);
    Zoltan_Set_Num_Geom_Fn(zz, CoordPart::get_num_geometry, &td);
    Zoltan_Set_Geom_Multi_Fn(zz, CoordPart::get_geometry_list, &td);

    // Partition
    int changes, numGidEntries, numLidEntries, numImport, numExport;
    ZOLTAN_ID_PTR importGlobalGids, importLocalGids, exportGlobalGids, exportLocalGids;
    int *importProcs, *importToPart, *exportProcs, *exportToPart;
    rc =
      Zoltan_LB_Partition(
        zz, // input (all remaining fields are output)
        &changes,        // 1 if partitioning was changed, 0 otherwise
        &numGidEntries,  // Number of integers used for a global ID
        &numLidEntries,  // Number of integers used for a local ID
        &numImport,      // Number of vertices to be sent to me
        &importGlobalGids,  // Global IDs of vertices to be sent to me
        &importLocalGids,   // Local IDs of vertices to be sent to me
        &importProcs,    // Process rank for source of each incoming vertex
        &importToPart,   // New partition for each incoming vertex
        &numExport,      // Number of vertices I must send to other processes*/
        &exportGlobalGids,  // Global IDs of the vertices I must send
        &exportLocalGids,   // Local IDs of the vertices I must send
        &exportProcs,    // Process to which I send each of the vertices
        &exportToPart);  // Partition to which each vertex will belong
    TEUCHOS_ASSERT(rc == 0);

    std::cout << "num parts requested = " << num_parts
              << " changes= " << changes
              << " num import = " << numImport
              << " num export = " << numExport << std::endl;

    // Compute max/min bounding box dimensions
    // double dim_max = 0.0, dim_min = 0.0;
    // for (int part=0; part<num_parts; ++part) {
    //   double xmin, ymin, zmin, xmax, ymax, zmax;
    //   int ndim;
    //   rc = Zoltan_RCB_Box(zz, part, &ndim, &xmin, &ymin, &zmin, 
    //                       &xmax, &ymax, &zmax);
    //   TEUCHOS_ASSERT(rc == 0);
    //   double delta_x = xmax - xmin;
    //   double delta_y = ymax - ymin;
    //   double delta_z = zmax - zmin;
    //   std::cout << delta_x << " " << delta_y << " " << delta_z << std::endl;
    //   if (delta_x > dim_max) dim_max = delta_x;
    //   if (delta_y > dim_max) dim_max = delta_y;
    //   if (delta_z > dim_max) dim_max = delta_z;
    //   if (delta_x < dim_min) dim_min = delta_x;
    //   if (delta_y < dim_min) dim_min = delta_y;
    //   if (delta_z < dim_min) dim_min = delta_z;
    // }
    // std::cout << "max part dimension = " << dim_max
    //           << " min part dimension = " << dim_min << std::endl;

    // Build part list
    Teuchos::Array< Teuchos::Array<CijkData> > part_list(num_parts);
    int idx = 0;
    Cijk_type::k_iterator k_begin = Cijk->k_begin();
    Cijk_type::k_iterator k_end = Cijk->k_end();
    for (Cijk_type::k_iterator k_it=k_begin; k_it!=k_end; ++k_it) {
      int k = index(k_it);
      Cijk_type::kj_iterator j_begin = Cijk->j_begin(k_it);
      Cijk_type::kj_iterator j_end = Cijk->j_end(k_it);
      for (Cijk_type::kj_iterator j_it = j_begin; j_it != j_end; ++j_it) {
        int j = index(j_it);
        Cijk_type::kji_iterator i_begin = Cijk->i_begin(j_it);
        Cijk_type::kji_iterator i_end = Cijk->i_end(j_it);
        for (Cijk_type::kji_iterator i_it = i_begin; i_it != i_end; ++i_it) {
          int i = index(i_it);
          int part = exportToPart[idx];
          CijkData c;
          c.i = i; c.j = j; c.k = k; c.gid = idx;
          part_list[part].push_back(c);
          ++idx;
        }
      }
    }

    // Compute max/min bounding box dimensions
    int dim_max = -1, dim_min = basis_size;
    for (int part=0; part<num_parts; ++part) {
      int imin = basis_size, jmin = basis_size, kmin = basis_size;
      int imax = -1, jmax = -1, kmax = -1;
      for (int idx=0; idx<part_list[part].size(); ++idx) {
        if (part_list[part][idx].i < imin) imin = part_list[part][idx].i;
        if (part_list[part][idx].j < jmin) jmin = part_list[part][idx].j;
        if (part_list[part][idx].k < kmin) kmin = part_list[part][idx].k;
        if (part_list[part][idx].i > imax) imax = part_list[part][idx].i;
        if (part_list[part][idx].j > jmax) jmax = part_list[part][idx].j;
        if (part_list[part][idx].k > kmax) kmax = part_list[part][idx].k;
      }
      int delta_i = imax - imin;
      int delta_j = jmax - jmin;
      int delta_k = kmax - kmin;
      if (delta_i < dim_min) dim_min = delta_i;
      if (delta_j < dim_min) dim_min = delta_j;
      if (delta_k < dim_min) dim_min = delta_k;
      if (delta_i > dim_max) dim_max = delta_i;
      if (delta_j > dim_max) dim_max = delta_j;
      if (delta_k > dim_max) dim_max = delta_k;
    }
    std::cout << "max part dimension = " << dim_max
              << " min part dimension = " << dim_min << std::endl;

    if (save_3tensor) {
      int idx = 0;
      Cijk_type::k_iterator k_begin = Cijk->k_begin();
      Cijk_type::k_iterator k_end = Cijk->k_end();
      for (Cijk_type::k_iterator k_it=k_begin; k_it!=k_end; ++k_it) {
        int k = index(k_it);
        Cijk_type::kj_iterator j_begin = Cijk->j_begin(k_it);
        Cijk_type::kj_iterator j_end = Cijk->j_end(k_it);
        for (Cijk_type::kj_iterator j_it = j_begin; j_it != j_end; ++j_it) {
          int j = index(j_it);
          Cijk_type::kji_iterator i_begin = Cijk->i_begin(j_it);
          Cijk_type::kji_iterator i_end = Cijk->i_end(j_it);
          for (Cijk_type::kji_iterator i_it = i_begin; i_it != i_end; ++i_it) {
            int i = index(i_it);
            cijk_file << i << ", " << j << ", " << k << ", "
                      << exportToPart[idx++] << std::endl;
          }
        }
      }
      cijk_file.close();
    }

    // Clean-up
    Zoltan_LB_Free_Part(&importGlobalGids, &importLocalGids,
                        &importProcs, &importToPart);
    Zoltan_LB_Free_Part(&exportGlobalGids, &exportLocalGids,
                        &exportProcs, &exportToPart);
    Zoltan_Destroy(&zz);

    //Teuchos::TimeMonitor::summarize(std::cout);

  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }

  return 0;
}
