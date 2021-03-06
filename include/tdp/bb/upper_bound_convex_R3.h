/* Copyright (c) 2015, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#pragma once

#include <vector>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <tdp/bb/node_R3.h>
#include <tdp/bb/numeric_helpers.h>
#include <tdp/distributions/normal.h>
#include <tdp/bb/bound.h>
#include <tdp/bb/lower_bound_R3.h>
#include <tdp/bb/upper_bound_indep_R3.h>

namespace tdp {

template <typename T>
class UpperBoundConvexR3 : public Bound<T,NodeR3<T>> {
 public:
  UpperBoundConvexR3(const std::vector<Normal<T,3>>& gmm_A, const
      std::vector<Normal<T,3>>& gmm_B, const Eigen::Quaternion<T>& q);
  virtual ~UpperBoundConvexR3() = default;
  virtual T Evaluate(const NodeR3<T>& node);
  virtual T EvaluateAndSet(NodeR3<T>& node);
 private:
  std::vector<Normal<T,3>> gmmT_;
};

typedef UpperBoundConvexR3<float>  UpperBoundConvexR3f;
typedef UpperBoundConvexR3<double> UpperBoundConvexR3d;

template <typename T>
Eigen::Matrix<T,3,1> FindMaxTranslationInNode(const Eigen::Matrix<T,3,3>& A, 
    const Eigen::Matrix<T,3,1>& b, const NodeR3<T>& node) {
  // Check corners of box.
  Eigen::Matrix<T,8,1> vals;
  for (uint32_t i=0; i<8; ++i) {
    Eigen::Matrix<T,3,1> t;
    node.GetBox().GetCorner(i, t);
    vals(i) = (t.transpose()*A*t - 2.*t.transpose()*b)(0);
  }
  uint32_t id_max = 0;
  vals.maxCoeff(&id_max);
  Eigen::Matrix<T,3,1> t;
  node.GetBox().GetCorner(id_max, t);
  return t;
}

}

