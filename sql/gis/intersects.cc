// Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, 51 Franklin
// Street, Suite 500, Boston, MA 02110-1335 USA.

/// @file
///
/// This file implements the intersects functor and function.

#include <boost/geometry.hpp>

#include "sql/dd/types/spatial_reference_system.h" // dd::Spatial_reference_system
#include "sql/gis/box.h"
#include "sql/gis/box_traits.h"
#include "sql/gis/disjoint_functor.h"
#include "sql/gis/geometries.h"
#include "sql/gis/geometries_traits.h"
#include "sql/gis/intersects_functor.h"
#include "sql/gis/mbr_utils.h"
#include "sql/gis/relops.h"
#include "sql/sql_exception_handler.h" // handle_gis_exception

namespace bg = boost::geometry;

namespace gis {

/// Apply an Intersects functor to two geometries, which both may be geometry
/// collections, and return the booelan result of the functor applied on each
/// combination of elements in the collections.
///
/// @tparam GC Coordinate specific gometry collection type.
///
/// @param f Functor to apply.
/// @param g1 First geometry.
/// @param g2 Second geometry.
///
/// @retval true g1 intersects g2.
/// @retval false g1 doesn't intersect g2.
template <typename GC>
static bool geometry_collection_apply_intersects(const Intersects &f,
                                                 const Geometry *g1,
                                                 const Geometry *g2) {
  if (g1->type() == Geometry_type::kGeometrycollection) {
    const auto gc1 = down_cast<const GC *>(g1);
    for (const auto g1_i : *gc1)
      if (geometry_collection_apply_intersects<GC>(f, g1_i, g2)) return true;
  } else if (g2->type() == Geometry_type::kGeometrycollection) {
    const auto gc2 = down_cast<const GC *>(g2);
    for (const auto g2_j : *gc2)
      if (geometry_collection_apply_intersects<GC>(f, g1, g2_j)) return true;
  } else {
    return f(g1, g2);
  }

  return false;
}

Intersects::Intersects(double semi_major, double semi_minor)
    : m_semi_major(semi_major),
      m_semi_minor(semi_minor),
      m_geographic_pl_pa_strategy(bg::strategy::side::geographic<>(
          bg::srs::spheroid<double>(semi_major, semi_minor))),
      m_geographic_ll_la_aa_strategy(
          bg::srs::spheroid<double>(semi_major, semi_minor)) {}

bool Intersects::operator()(const Geometry *g1, const Geometry *g2) const {
  return apply(*this, g1, g2);
}

bool Intersects::operator()(const Box *b1, const Box *b2) const {
  DBUG_ASSERT(b1->coordinate_system() == b2->coordinate_system());
  switch (b1->coordinate_system()) {
    case Coordinate_system::kCartesian:
      return eval(down_cast<const Cartesian_box *>(b1),
                  down_cast<const Cartesian_box *>(b2));
    case Coordinate_system::kGeographic:
      return eval(down_cast<const Geographic_box *>(b1),
                  down_cast<const Geographic_box *>(b2));
  }

  DBUG_ASSERT(false);
  return false;
}

bool Intersects::eval(const Geometry *g1, const Geometry *g2) const {
  // All parameter type combinations have been implemented.
  DBUG_ASSERT(false);
  throw not_implemented_exception(g1->coordinate_system(), g1->type(),
                                  g2->type());
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_point, *)

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_point *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_linestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_polygon *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_multipoint *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_multilinestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_point *g1,
                      const Cartesian_multipolygon *g2) const {
  return bg::intersects(*g1, *g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_linestring, *)

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_point *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_linestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_polygon *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_multipoint *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_multilinestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_linestring *g1,
                      const Cartesian_multipolygon *g2) const {
  return bg::intersects(*g1, *g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_polygon, *)

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_point *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_linestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_polygon *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_multipoint *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_multilinestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_polygon *g1,
                      const Cartesian_multipolygon *g2) const {
  return bg::intersects(*g1, *g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_geometrycollection, *)

bool Intersects::eval(const Cartesian_geometrycollection *g1,
                      const Geometry *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_multipoint, *)

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_point *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_linestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_polygon *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_multipoint *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_multilinestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipoint *g1,
                      const Cartesian_multipolygon *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_multilinestring, *)

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_point *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_linestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_polygon *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_multipoint *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_multilinestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multilinestring *g1,
                      const Cartesian_multipolygon *g2) const {
  return bg::intersects(*g1, *g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Cartesian_multipolygon, *)

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_point *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_linestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_polygon *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Cartesian_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_multipoint *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_multilinestring *g2) const {
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Cartesian_multipolygon *g1,
                      const Cartesian_multipolygon *g2) const {
  return bg::intersects(*g1, *g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_point, *)

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_point *g2) const {
  // Default strategy is OK. P/P computations do not depend on shape of
  // ellipsoid.
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_linestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_polygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_multipoint *g2) const {
  // Default strategy is OK. P/P computations do not depend on shape of
  // ellipsoid.
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_multilinestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_point *g1,
                      const Geographic_multipolygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_linestring, *)

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_point *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_linestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_polygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_multipoint *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_multilinestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_linestring *g1,
                      const Geographic_multipolygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_polygon, *)

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_point *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_linestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_polygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_multipoint *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_multilinestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_polygon *g1,
                      const Geographic_multipolygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_geometrycollection, *)

bool Intersects::eval(const Geographic_geometrycollection *g1,
                      const Geometry *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_multipoint, *)

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_point *g2) const {
  // Default strategy is OK. P/P computations do not depend on shape of
  // ellipsoid.
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_linestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_polygon *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_multipoint *g2) const {
  // Default strategy is OK. P/P computations do not depend on shape of
  // ellipsoid.
  return bg::intersects(*g1, *g2);
}

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_multilinestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_multipoint *g1,
                      const Geographic_multipolygon *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_multilinestring, *)

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_point *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_linestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_polygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_multipoint *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_multilinestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_multilinestring *g1,
                      const Geographic_multipolygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Geographic_multipolygon, *)

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_point *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_pl_pa_strategy);
}

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_linestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_polygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_geometrycollection *g2) const {
  return geometry_collection_apply_intersects<Geographic_geometrycollection>(
      *this, g1, g2);
}

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_multipoint *g2) const {
  Disjoint disjoint(m_semi_major, m_semi_minor);
  return !disjoint(g1, g2);
}

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_multilinestring *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

bool Intersects::eval(const Geographic_multipolygon *g1,
                      const Geographic_multipolygon *g2) const {
  return bg::intersects(*g1, *g2, m_geographic_ll_la_aa_strategy);
}

//////////////////////////////////////////////////////////////////////////////

// intersects(Box, Box)

bool Intersects::eval(const Cartesian_box *b1, const Cartesian_box *b2) const {
  return bg::intersects(*b1, *b2);
}

bool Intersects::eval(const Geographic_box *b1,
                      const Geographic_box *b2) const {
  return bg::intersects(*b1, *b2);
}

//////////////////////////////////////////////////////////////////////////////

bool intersects(const dd::Spatial_reference_system *srs, const Geometry *g1,
                const Geometry *g2, const char *func_name, bool *intersects,
                bool *null) noexcept {
  try {
    DBUG_ASSERT(g1->coordinate_system() == g2->coordinate_system());
    DBUG_ASSERT(srs == nullptr ||
                ((srs->is_cartesian() &&
                  g1->coordinate_system() == Coordinate_system::kCartesian) ||
                 (srs->is_geographic() &&
                  g1->coordinate_system() == Coordinate_system::kGeographic)));

    if ((*null = (g1->is_empty() || g2->is_empty()))) return false;

    Intersects intersects_func(srs ? srs->semi_major_axis() : 0.0,
                               srs ? srs->semi_minor_axis() : 0.0);
    *intersects = intersects_func(g1, g2);
  } catch (...) {
    handle_gis_exception(func_name);
    return true;
  }

  return false;
}

bool mbr_intersects(const dd::Spatial_reference_system *srs, const Geometry *g1,
                    const Geometry *g2, const char *func_name, bool *intersects,
                    bool *null) noexcept {
  try {
    DBUG_ASSERT(g1->coordinate_system() == g2->coordinate_system());
    DBUG_ASSERT(srs == nullptr ||
                ((srs->is_cartesian() &&
                  g1->coordinate_system() == Coordinate_system::kCartesian) ||
                 (srs->is_geographic() &&
                  g1->coordinate_system() == Coordinate_system::kGeographic)));

    if ((*null = (g1->is_empty() || g2->is_empty()))) return false;

    Intersects intersects_func(srs ? srs->semi_major_axis() : 0.0,
                               srs ? srs->semi_minor_axis() : 0.0);

    switch (g1->coordinate_system()) {
      case Coordinate_system::kCartesian: {
        Cartesian_box mbr1;
        box_envelope(g1, srs, &mbr1);
        Cartesian_box mbr2;
        box_envelope(g2, srs, &mbr2);
        *intersects = intersects_func(&mbr1, &mbr2);
        break;
      }
      case Coordinate_system::kGeographic: {
        Geographic_box mbr1;
        box_envelope(g1, srs, &mbr1);
        Geographic_box mbr2;
        box_envelope(g2, srs, &mbr2);
        *intersects = intersects_func(&mbr1, &mbr2);
        break;
      }
    }
  } catch (...) {
    handle_gis_exception(func_name);
    return true;
  }

  return false;
}

}  // namespace gis
