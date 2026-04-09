#pragma once
#ifndef LIDARSTREAMERBASE_HPP
#define LIDARSTREAMERBASE_HPP

#include"algos_pch.hpp"

namespace lapis {
    class LidarStreamer {
    public:
        ~LidarStreamer() = default;

        //The span returned is only guaranteed to be valid until getPoints is called again.
        //The number of points returned may be less than n, but is guaranteed to never be greater.
        //The implementation may reserve space for up to n points, but will never allocate more than that.
        virtual const std::span<const LasPoint> getPoints(size_t n) = 0;

        //equivalent to nPointsRemaining() > 0
        virtual bool hasMorePoints() const = 0;

        //This is guaranteed to be at least as large as the number of points that will be returned by getPoints() before reset() is called, but may be larger
        virtual size_t nPoints() const = 0;

        //This is guaranteed to be at least as large as the number of points that will be returned by getPoints() before reset() is called, but may be larger
        virtual size_t nPointsRemaining() const = 0;

        //All points returned by this streamer are guaranteed to be in this CRS, assuming the native resolution of points is specified correctly
        virtual const CoordRef& getCoordRef() const = 0;

        //This extent is guaranteed to contain all points returned by getPoints(), but is not guaranteed to be the minimal such extent
        virtual const Extent& getExtent() const = 0;

        //Resets the streamer to the beginning of the point stream
        virtual void reset() = 0;
    protected:
        //true if the std::span is a temporary object and derived objects should feel free to mess with it before returning
        //false if the std::span is a reference to permanent data that should be left alone
        virtual bool _canModifyInPlace() const = 0;
        static bool _canModifyInPlace(const LidarStreamer& streamer);
    };
}

#endif