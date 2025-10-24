/**
 * @file vector.h
 * @brief 3-dimensional vector data structure to encapsulate the ICM42670P data.
 */

#ifndef VECTOR_H
#define VECTOR_H

#include <cstdint>

/**
 * @brief A measurement observed by the IMU with 3-dimensions.
 */
struct IMUVector {
    double x;
    double y;
    double z;
};

#endif /* VECTOR_H */