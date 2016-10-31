/* Copyright (c) 2016, Julian Straub <jstraub@csail.mit.edu> Licensed
 * under the MIT license. See the license file LICENSE.
 */
#pragma once

#include <Eigen/Dense>
#include <tdp/data/image.h>
#include <tdp/data/managed_image.h>
#include <tdp/eigen/dense.h>

namespace tdp {

  int hammingDistance (uint64_t x, uint64_t y) {
    uint64_t res = x ^ y;
    return __builtin_popcountll (res);
  }
  int hammingDistance (uint32_t x, uint32_t y) {
    uint32_t res = x ^ y;
    return __builtin_popcountll (res);
  }

  int Distance(const Vector8uda& a, const Vector8uda& b) {
    return hammingDistance(a(0),b(0)) 
      + hammingDistance(a(1),b(1)) 
      + hammingDistance(a(2),b(2)) 
      + hammingDistance(a(3),b(3)) 
      + hammingDistance(a(4),b(4)) 
      + hammingDistance(a(5),b(5)) 
      + hammingDistance(a(6),b(6)) 
      + hammingDistance(a(7),b(7));
  }

  int Closest(const Vector8uda& a, const Image<Vector8uda>& bs, int* dist) {
    int minId = -1;
    int minDist = 257;
    for (int i=0; i<bs.Area(); ++i) {
      int dist = Distance(a, bs[i]);
      if (dist < minDist) {
        minDist = dist;
        minId = i;
      }
    }
    if (dist) *dist = minDist;
    return minId;
  }

  bool ExtractBrief(const Image<uint8_t>& patch, Vector8uda& desc) {
    desc(0) = ((patch(23,15) < patch(8,10) ? 1 : 0)
        | (patch(13,5) < patch(13,19) ? 2 : 0)
        | (patch(7,22) < patch(17,22) ? 4 : 0)
        | (patch(14,15) < patch(22,13) ? 8 : 0)
        | (patch(15,26) < patch(12,24) ? 16 : 0)
        | (patch(7,19) < patch(17,21) ? 32 : 0)
        | (patch(9,21) < patch(7,14) ? 64 : 0)
        | (patch(19,13) < patch(25,6) ? 128 : 0)
        | (patch(20,19) < patch(0,7) ? 256 : 0)
        | (patch(9,8) < patch(18,13) ? 512 : 0)
        | (patch(9,18) < patch(18,10) ? 1024 : 0)
        | (patch(29,14) < patch(22,8) ? 2048 : 0)
        | (patch(12,13) < patch(31,7) ? 4096 : 0)
        | (patch(10,4) < patch(12,22) ? 8192 : 0)
        | (patch(11,3) < patch(14,8) ? 16384 : 0)
        | (patch(22,14) < patch(15,11) ? 32768 : 0)
        | (patch(10,12) < patch(22,8) ? 65536 : 0)
        | (patch(16,17) < patch(13,6) ? 131072 : 0)
        | (patch(8,7) < patch(15,13) ? 262144 : 0)
        | (patch(16,24) < patch(10,13) ? 524288 : 0)
        | (patch(6,16) < patch(26,17) ? 1048576 : 0)
        | (patch(12,23) < patch(16,18) ? 2097152 : 0)
        | (patch(13,15) < patch(11,15) ? 4194304 : 0)
        | (patch(22,18) < patch(20,28) ? 8388608 : 0)
        | (patch(12,9) < patch(3,16) ? 16777216 : 0)
        | (patch(16,15) < patch(18,16) ? 33554432 : 0)
        | (patch(22,17) < patch(9,3) ? 67108864 : 0)
        | (patch(10,2) < patch(12,18) ? 134217728 : 0)
        | (patch(9,19) < patch(7,8) ? 268435456 : 0)
        | (patch(24,3) < patch(19,12) ? 536870912 : 0)
        | (patch(10,14) < patch(11,18) ? 1073741824 : 0)
        | (patch(14,6) < patch(16,10) ? 2147483648 : 0));
    desc(1) = ((patch(14,6) < patch(16,10) ? 1 : 0)
        | (patch(13,20) < patch(14,15) ? 2 : 0)
        | (patch(16,21) < patch(4,15) ? 4 : 0)
        | (patch(14,22) < patch(15,12) ? 8 : 0)
        | (patch(24,17) < patch(10,15) ? 16 : 0)
        | (patch(14,21) < patch(13,10) ? 32 : 0)
        | (patch(3,20) < patch(13,11) ? 64 : 0)
        | (patch(16,13) < patch(11,18) ? 128 : 0)
        | (patch(7,16) < patch(9,20) ? 256 : 0)
        | (patch(24,9) < patch(17,14) ? 512 : 0)
        | (patch(7,26) < patch(5,9) ? 1024 : 0)
        | (patch(15,16) < patch(9,28) ? 2048 : 0)
        | (patch(14,14) < patch(18,13) ? 4096 : 0)
        | (patch(8,15) < patch(16,25) ? 8192 : 0)
        | (patch(19,9) < patch(16,14) ? 16384 : 0)
        | (patch(9,12) < patch(15,8) ? 32768 : 0)
        | (patch(19,22) < patch(17,8) ? 65536 : 0)
        | (patch(22,14) < patch(17,16) ? 131072 : 0)
        | (patch(17,16) < patch(22,8) ? 262144 : 0)
        | (patch(13,19) < patch(18,15) ? 524288 : 0)
        | (patch(12,18) < patch(17,30) ? 1048576 : 0)
        | (patch(6,15) < patch(12,10) ? 2097152 : 0)
        | (patch(4,11) < patch(6,20) ? 4194304 : 0)
        | (patch(5,20) < patch(17,25) ? 8388608 : 0)
        | (patch(11,15) < patch(21,23) ? 16777216 : 0)
        | (patch(20,18) < patch(22,17) ? 33554432 : 0)
        | (patch(24,12) < patch(12,6) ? 67108864 : 0)
        | (patch(17,6) < patch(21,7) ? 134217728 : 0)
        | (patch(11,7) < patch(16,27) ? 268435456 : 0)
        | (patch(8,16) < patch(13,10) ? 536870912 : 0)
        | (patch(25,10) < patch(11,2) ? 1073741824 : 0)
        | (patch(17,17) < patch(12,15) ? 2147483648 : 0));
    desc(2) = ((patch(17,17) < patch(12,15) ? 1 : 0)
        | (patch(5,17) < patch(17,13) ? 2 : 0)
        | (patch(13,3) < patch(4,9) ? 4 : 0)
        | (patch(9,20) < patch(12,19) ? 8 : 0)
        | (patch(16,5) < patch(10,21) ? 16 : 0)
        | (patch(27,13) < patch(8,15) ? 32 : 0)
        | (patch(14,26) < patch(12,13) ? 64 : 0)
        | (patch(11,7) < patch(19,18) ? 128 : 0)
        | (patch(12,21) < patch(21,21) ? 256 : 0)
        | (patch(21,16) < patch(20,10) ? 512 : 0)
        | (patch(20,17) < patch(14,22) ? 1024 : 0)
        | (patch(16,10) < patch(9,16) ? 2048 : 0)
        | (patch(20,11) < patch(22,17) ? 4096 : 0)
        | (patch(15,30) < patch(7,14) ? 8192 : 0)
        | (patch(2,15) < patch(8,17) ? 16384 : 0)
        | (patch(14,17) < patch(25,22) ? 32768 : 0)
        | (patch(19,29) < patch(13,25) ? 65536 : 0)
        | (patch(19,20) < patch(14,15) ? 131072 : 0)
        | (patch(24,22) < patch(9,12) ? 262144 : 0)
        | (patch(11,19) < patch(12,12) ? 524288 : 0)
        | (patch(15,6) < patch(8,8) ? 1048576 : 0)
        | (patch(10,22) < patch(21,16) ? 2097152 : 0)
        | (patch(17,17) < patch(23,14) ? 4194304 : 0)
        | (patch(6,20) < patch(16,17) ? 8388608 : 0)
        | (patch(15,21) < patch(16,24) ? 16777216 : 0)
        | (patch(22,16) < patch(2,14) ? 33554432 : 0)
        | (patch(10,8) < patch(22,20) ? 67108864 : 0)
        | (patch(12,11) < patch(11,16) ? 134217728 : 0)
        | (patch(19,18) < patch(6,12) ? 268435456 : 0)
        | (patch(13,8) < patch(19,27) ? 536870912 : 0)
        | (patch(5,31) < patch(21,21) ? 1073741824 : 0)
        | (patch(29,9) < patch(15,20) ? 2147483648 : 0));
    desc(3) = ((patch(29,9) < patch(15,20) ? 1 : 0)
        | (patch(26,15) < patch(8,15) ? 2 : 0)
        | (patch(9,12) < patch(17,5) ? 4 : 0)
        | (patch(24,9) < patch(16,14) ? 8 : 0)
        | (patch(12,18) < patch(14,15) ? 16 : 0)
        | (patch(16,10) < patch(14,7) ? 32 : 0)
        | (patch(10,3) < patch(9,10) ? 64 : 0)
        | (patch(22,11) < patch(15,16) ? 128 : 0)
        | (patch(28,25) < patch(12,16) ? 256 : 0)
        | (patch(11,13) < patch(19,27) ? 512 : 0)
        | (patch(22,16) < patch(9,10) ? 1024 : 0)
        | (patch(24,21) < patch(9,23) ? 2048 : 0)
        | (patch(25,11) < patch(17,7) ? 4096 : 0)
        | (patch(18,13) < patch(14,10) ? 8192 : 0)
        | (patch(21,2) < patch(23,23) ? 16384 : 0)
        | (patch(25,12) < patch(9,21) ? 32768 : 0)
        | (patch(20,19) < patch(14,16) ? 65536 : 0)
        | (patch(10,17) < patch(9,17) ? 131072 : 0)
        | (patch(16,18) < patch(1,18) ? 262144 : 0)
        | (patch(11,31) < patch(17,18) ? 524288 : 0)
        | (patch(13,25) < patch(11,17) ? 1048576 : 0)
        | (patch(17,8) < patch(14,18) ? 2097152 : 0)
        | (patch(19,14) < patch(18,12) ? 4194304 : 0)
        | (patch(22,3) < patch(21,28) ? 8388608 : 0)
        | (patch(11,21) < patch(22,18) ? 16777216 : 0)
        | (patch(14,5) < patch(19,7) ? 33554432 : 0)
        | (patch(17,6) < patch(14,9) ? 67108864 : 0)
        | (patch(22,18) < patch(21,22) ? 134217728 : 0)
        | (patch(19,6) < patch(28,11) ? 268435456 : 0)
        | (patch(20,28) < patch(22,20) ? 536870912 : 0)
        | (patch(11,5) < patch(18,27) ? 1073741824 : 0)
        | (patch(11,3) < patch(11,9) ? 2147483648 : 0));
    desc(4) = ((patch(11,3) < patch(11,9) ? 1 : 0)
        | (patch(31,9) < patch(22,22) ? 2 : 0)
        | (patch(6,23) < patch(20,16) ? 4 : 0)
        | (patch(15,1) < patch(9,15) ? 8 : 0)
        | (patch(9,14) < patch(17,12) ? 16 : 0)
        | (patch(19,27) < patch(16,23) ? 32 : 0)
        | (patch(2,18) < patch(7,23) ? 64 : 0)
        | (patch(27,13) < patch(22,2) ? 128 : 0)
        | (patch(19,8) < patch(11,17) ? 256 : 0)
        | (patch(21,6) < patch(23,26) ? 512 : 0)
        | (patch(9,13) < patch(17,17) ? 1024 : 0)
        | (patch(23,19) < patch(14,18) ? 2048 : 0)
        | (patch(22,11) < patch(11,10) ? 4096 : 0)
        | (patch(14,21) < patch(13,17) ? 8192 : 0)
        | (patch(8,14) < patch(11,19) ? 16384 : 0)
        | (patch(23,21) < patch(25,9) ? 32768 : 0)
        | (patch(24,23) < patch(18,24) ? 65536 : 0)
        | (patch(6,24) < patch(13,14) ? 131072 : 0)
        | (patch(18,17) < patch(11,17) ? 262144 : 0)
        | (patch(18,27) < patch(19,20) ? 524288 : 0)
        | (patch(20,20) < patch(15,19) ? 1048576 : 0)
        | (patch(25,25) < patch(14,18) ? 2097152 : 0)
        | (patch(10,20) < patch(8,7) ? 4194304 : 0)
        | (patch(24,16) < patch(19,19) ? 8388608 : 0)
        | (patch(15,12) < patch(9,17) ? 16777216 : 0)
        | (patch(6,11) < patch(22,15) ? 33554432 : 0)
        | (patch(16,16) < patch(7,16) ? 67108864 : 0)
        | (patch(7,15) < patch(21,13) ? 134217728 : 0)
        | (patch(15,31) < patch(14,7) ? 268435456 : 0)
        | (patch(25,15) < patch(17,16) ? 536870912 : 0)
        | (patch(17,14) < patch(16,17) ? 1073741824 : 0)
        | (patch(23,15) < patch(5,15) ? 2147483648 : 0));
    desc(5) = ((patch(23,15) < patch(5,15) ? 1 : 0)
        | (patch(4,22) < patch(11,14) ? 2 : 0)
        | (patch(20,13) < patch(12,6) ? 4 : 0)
        | (patch(6,9) < patch(12,15) ? 8 : 0)
        | (patch(13,6) < patch(17,18) ? 16 : 0)
        | (patch(10,25) < patch(7,14) ? 32 : 0)
        | (patch(19,19) < patch(22,9) ? 64 : 0)
        | (patch(13,15) < patch(22,8) ? 128 : 0)
        | (patch(16,11) < patch(27,3) ? 256 : 0)
        | (patch(16,22) < patch(10,18) ? 512 : 0)
        | (patch(14,11) < patch(9,15) ? 1024 : 0)
        | (patch(12,15) < patch(14,2) ? 2048 : 0)
        | (patch(9,15) < patch(15,5) ? 4096 : 0)
        | (patch(7,12) < patch(27,10) ? 8192 : 0)
        | (patch(18,13) < patch(15,22) ? 16384 : 0)
        | (patch(8,21) < patch(13,9) ? 32768 : 0)
        | (patch(18,24) < patch(14,9) ? 65536 : 0)
        | (patch(3,12) < patch(14,11) ? 131072 : 0)
        | (patch(8,21) < patch(8,21) ? 262144 : 0)
        | (patch(20,7) < patch(17,18) ? 524288 : 0)
        | (patch(9,17) < patch(13,22) ? 1048576 : 0)
        | (patch(20,18) < patch(13,21) ? 2097152 : 0)
        | (patch(7,10) < patch(4,20) ? 4194304 : 0)
        | (patch(14,21) < patch(25,19) ? 8388608 : 0)
        | (patch(12,19) < patch(25,6) ? 16777216 : 0)
        | (patch(3,26) < patch(16,16) ? 33554432 : 0)
        | (patch(19,2) < patch(27,14) ? 67108864 : 0)
        | (patch(17,16) < patch(21,12) ? 134217728 : 0)
        | (patch(23,11) < patch(22,14) ? 268435456 : 0)
        | (patch(11,17) < patch(0,17) ? 536870912 : 0)
        | (patch(3,22) < patch(12,19) ? 1073741824 : 0)
        | (patch(11,15) < patch(11,14) ? 2147483648 : 0));
    desc(6) = ((patch(11,15) < patch(11,14) ? 1 : 0)
        | (patch(11,23) < patch(21,15) ? 2 : 0)
        | (patch(7,19) < patch(17,14) ? 4 : 0)
        | (patch(8,25) < patch(15,16) ? 8 : 0)
        | (patch(24,22) < patch(25,21) ? 16 : 0)
        | (patch(11,15) < patch(15,20) ? 32 : 0)
        | (patch(23,19) < patch(2,14) ? 64 : 0)
        | (patch(10,23) < patch(24,12) ? 128 : 0)
        | (patch(5,3) < patch(5,13) ? 256 : 0)
        | (patch(19,10) < patch(8,27) ? 512 : 0)
        | (patch(0,14) < patch(11,16) ? 1024 : 0)
        | (patch(18,23) < patch(25,17) ? 2048 : 0)
        | (patch(13,12) < patch(25,11) ? 4096 : 0)
        | (patch(22,22) < patch(9,6) ? 8192 : 0)
        | (patch(23,18) < patch(16,14) ? 16384 : 0)
        | (patch(11,13) < patch(16,10) ? 32768 : 0)
        | (patch(24,30) < patch(5,14) ? 65536 : 0)
        | (patch(9,11) < patch(22,14) ? 131072 : 0)
        | (patch(21,23) < patch(11,11) ? 262144 : 0)
        | (patch(22,7) < patch(19,15) ? 524288 : 0)
        | (patch(15,23) < patch(3,9) ? 1048576 : 0)
        | (patch(7,9) < patch(22,30) ? 2097152 : 0)
        | (patch(10,19) < patch(18,23) ? 4194304 : 0)
        | (patch(14,11) < patch(4,14) ? 8388608 : 0)
        | (patch(16,13) < patch(21,16) ? 16777216 : 0)
        | (patch(15,12) < patch(9,7) ? 33554432 : 0)
        | (patch(13,22) < patch(14,10) ? 67108864 : 0)
        | (patch(7,9) < patch(25,13) ? 134217728 : 0)
        | (patch(7,9) < patch(27,2) ? 268435456 : 0)
        | (patch(15,19) < patch(10,22) ? 536870912 : 0)
        | (patch(16,11) < patch(18,14) ? 1073741824 : 0)
        | (patch(14,6) < patch(12,14) ? 2147483648 : 0));
    desc(7) = ((patch(14,6) < patch(12,14) ? 1 : 0)
        | (patch(8,9) < patch(13,24) ? 2 : 0)
        | (patch(28,14) < patch(6,14) ? 4 : 0)
        | (patch(9,6) < patch(14,16) ? 8 : 0)
        | (patch(16,12) < patch(12,23) ? 16 : 0)
        | (patch(15,28) < patch(16,9) ? 32 : 0)
        | (patch(12,17) < patch(24,18) ? 64 : 0)
        | (patch(22,18) < patch(9,22) ? 128 : 0)
        | (patch(12,18) < patch(15,17) ? 256 : 0)
        | (patch(16,12) < patch(26,13) ? 512 : 0)
        | (patch(8,12) < patch(14,25) ? 1024 : 0)
        | (patch(14,8) < patch(14,21) ? 2048 : 0)
        | (patch(10,12) < patch(21,13) ? 4096 : 0)
        | (patch(10,13) < patch(17,12) ? 8192 : 0)
        | (patch(19,8) < patch(22,17) ? 16384 : 0)
        | (patch(19,27) < patch(27,25) ? 32768 : 0)
        | (patch(15,22) < patch(17,22) ? 65536 : 0)
        | (patch(4,18) < patch(20,20) ? 131072 : 0)
        | (patch(8,7) < patch(7,23) ? 262144 : 0)
        | (patch(20,13) < patch(16,17) ? 524288 : 0)
        | (patch(13,7) < patch(16,17) ? 1048576 : 0)
        | (patch(12,9) < patch(14,12) ? 2097152 : 0)
        | (patch(18,11) < patch(19,4) ? 4194304 : 0)
        | (patch(8,14) < patch(18,17) ? 8388608 : 0)
        | (patch(5,14) < patch(13,10) ? 16777216 : 0)
        | (patch(19,7) < patch(16,26) ? 33554432 : 0)
        | (patch(7,13) < patch(17,8) ? 67108864 : 0)
        | (patch(19,12) < patch(7,10) ? 134217728 : 0)
        | (patch(12,24) < patch(20,11) ? 268435456 : 0)
        | (patch(9,7) < patch(11,9) ? 536870912 : 0)
        | (patch(27,17) < patch(21,12) ? 1073741824 : 0)
        | (patch(19,25) < patch(22,17) ? 2147483648 : 0));

    return true;
  }

  bool ExtractBrief(const Image<uint8_t>& grey, int32_t x, int32_t y, 
      Vector8uda& desc) {
    if (!grey.Inside(x-16,y-16) || !grey.Inside(x+15, y+15)) {
      return false;
    }
    Image<uint8_t> patch = grey.GetRoi(x-16, y-16, 32,32);
    return ExtractBrief(patch, desc);
  }

  void ExtractBrief(const Image<uint8_t> grey, 
      const Image<Vector2ida>& pts,
      Image<Vector8uda>& descs) {
    for (size_t i=0; i<pts.Area(); ++i) {
      if(!tdp::ExtractBrief(grey, pts[i](0), pts[i](1), descs[i])) {
        std::cout << pts[i].transpose() << " could not be extracted" << std::endl;
      }
    }
  }

}
