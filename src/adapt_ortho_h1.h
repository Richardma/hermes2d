// This file is part of Hermes2D.
//
// Copyright 2005-2008 Jakub Cerveny <jakub.cerveny@gmail.com>
// Copyright 2005-2008 Lenka Dubcova <dubcova@gmail.com>
// Copyright 2005-2008 Pavel Solin <solin@unr.edu>
//
// Hermes2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __HERMES2D_ADAPT_ORTHO_H1_H
#define __HERMES2D_ADAPT_ORTHO_H1_H

#include "forms.h"
#include "weakform.h"
#include "integrals_h1.h"



/// \brief hp-adaptivity module for H1 spaces.
///
/// H1OrthoHP is a fast hp-adaptivity module for continuous elements.
/// Given a reference solution, it provides functions to calculate H1 or
/// energy error estimates, acts as a container for the calculated errors
/// and contains the "ortho" hp-adaptivty algorithm based on fast
/// projections to an orthonormal set of functions.
///
class H1OrthoHP
{
public:

  /// Initializes the class. 'num' is the number of mesh-space pairs to be adapted.
  /// After 'num', exactly that many space pointers must follow.
  H1OrthoHP(int num, ...);
  ~H1OrthoHP();


  typedef scalar (*biform_val_t) (int n, double *wt, Func<scalar> *u, Func<scalar> *v, Geom<double> *e, ExtData<scalar> *);
  typedef Ord (*biform_ord_t) (int n, double *wt, Func<Ord> *u, Func<Ord> *v, Geom<Ord> *e, ExtData<Ord> *);

  /// Sets user defined bilinear form to calculate error. Default forms are h1 error (on diagonal).
  /// Use this function only to change it (e.g. energy error).
  void set_biform(int i, int j, biform_val_t bi_form, biform_ord_t bi_ord);

  /// Type-safe version of calc_error_n() for one solution.
  double calc_error(MeshFunction* sln, MeshFunction* rsln);

  /// Type-safe version of calc_error_n() for two solutions.
  double calc_error_2(MeshFunction* sln1, MeshFunction* sln2, MeshFunction* rsln1, MeshFunction* rsln2);

  /// Calculates the error of the solution using given norms. 'n' must be the
  /// same as 'num' in the constructor. After that, n coarse solution
  /// pointers are passed, followed by n fine solution pointers.
  double calc_error_n(int n, ...);


  /// Selects elements to refine (based on results from calc_error() or calc_energy_error())
  /// and performs their optimal hp-refinement.
  bool adapt(double thr, int strat = 0, int adapt_type = 0, bool iso_only = false, int regularize = -1,
             int max_order = -1, bool same_orders = false, double to_be_processed = 0.0);

  /// Unrefines the elements with the smallest error
  void unrefine(double thr);

  /// Internal. Used by adapt(). Can be utilized in specialized adaptivity
  /// procedures, for which adapt() is not sufficient.
  static void get_optimal_refinement(Element* e, int order, Solution* rsln, int& split, int p[4], int q[4],
                                     bool h_only = false, bool iso_only = false, int max_order = -1);

  /// Internal. Functions to obtain errors of individual elements.
  double get_element_error(int component, int id) const { return errors[component][id]; }
  int2*  get_sorted_elements() const { return esort; }
  int    get_total_active_elements() const { return nact; }


protected:

  // spaces & solutions
  int num;
  Space* spaces[10];
  Solution* sln[10];
  Solution* rsln[10];

  // element error arrays
  double* errors[10];
  double  norms[10]; // ?
  bool    have_errors;
  double  total_err;
  int2* esort;
  int   nact;

  // bilinear forms to calculate error
  biform_val_t form[10][10];
  biform_ord_t ord[10][10];

  // evaluation of error and norm forms
  scalar eval_error(biform_val_t bi_fn, biform_ord_t bi_ord,
                    MeshFunction *sln1, MeshFunction *sln2, MeshFunction *rsln1, MeshFunction *rsln2,
                    RefMap *rv1,        RefMap *rv2,        RefMap *rrv1,        RefMap *rrv2);

  scalar eval_norm(biform_val_t bi_fn, biform_ord_t bi_ord,
                   MeshFunction *rsln1, MeshFunction *rsln2, RefMap *rrv1, RefMap *rrv2);

  // orthonormal basis tables
  static double3** obase[2][9];
  static int  basecnt[2][11];
  static bool obase_ready;

  static void calc_ortho_base();

  static void calc_projection_errors(Element* e, int order, Solution* rsln,
                                     double herr[4][11], double perr[11]);

public:

  /// Internal.
  static void free_ortho_base();

};



#endif
