/*
 * HelperTypes.h
 *
 *
 *  Created on: May 29, 2012
 *      Author: Martin Uhrin
 */

#ifndef HELPER_TYPES_H
#define HELPER_TYPES_H

// INCLUDES /////////////////////////////////////////////


// DEFINES //////////////////////////////////////////////

namespace spl {
namespace yaml {

///////////////////////////////////////////////////////////
// CLASSES
///////////////////////////////////////////////////////////

template <typename T>
struct TypeWrapper
{
  typedef T ValueType;
  
  TypeWrapper() {}
  TypeWrapper(const T & value_): value(value_) {}

  T & operator *() { return value; }
  const T & operator *() const { return value; }
  T * operator ->() { return &value; }
  const T * operator ->() const { return &value; }

  TypeWrapper &
  operator =(const T & rhs)
  {
    value = rhs;
    return *this;
  }

  operator T() const
  {
    return value;
  }

  T value;
};

// An upper triangular arma matrix so we don't need
// to store all the repeated values
typedef TypeWrapper< ::arma::mat> ArmaTriangularMat;

// Wrap up a vector so that it can be read as a string
// rather than as a YAML sequence
template <typename T>
struct VecAsString
{
  typedef TypeWrapper< ::std::vector<T> > Type;
};


}
}

#endif /* HELPER_TYPES_H */

