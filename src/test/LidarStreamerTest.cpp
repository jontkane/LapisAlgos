#include"test_pch.hpp"
#include"..\AllLidarStreamers.hpp"

TEST(LidarStreamerTest, TestLidarStreamerHardDrive)
{
    using namespace lapis;

    std::string filename = std::string(LAPISALGOSTESTFILES) + "/testlaz14.laz";
    LidarStreamerHardDrive streamer(filename);
    constexpr size_t expectedNPoints = 93;
    constexpr coord_t expectedXMin = 299000;
    constexpr coord_t expectedXMax = 299001.3;
    constexpr coord_t expectedYMin = 4202000;
    constexpr coord_t expectedYMax = 4202005.5;
    constexpr coord_t expectedZMin = 3312.45;
    constexpr coord_t expectedZMax = 3314.11;
    const CoordRef expectedCrs{ "EPSG:6340+5703" };

    auto testOneCycle = [&]() {
        std::vector<LasPoint> points;

        EXPECT_TRUE(streamer.hasMorePoints());
        EXPECT_EQ(streamer.nPoints(), expectedNPoints);
        EXPECT_EQ(streamer.nPointsRemaining(), expectedNPoints);
        EXPECT_TRUE(streamer.getCoordRef().isConsistent(expectedCrs));

        Extent actualExtent = streamer.getExtent();
        EXPECT_NEAR(actualExtent.xmin(), expectedXMin, 0.1);
        EXPECT_NEAR(actualExtent.xmax(), expectedXMax, 0.1);
        EXPECT_NEAR(actualExtent.ymin(), expectedYMin, 0.1);
        EXPECT_NEAR(actualExtent.ymax(), expectedYMax, 0.1);
        EXPECT_TRUE(actualExtent.crs().isConsistent(expectedCrs));

        size_t prevNPointsRemaining = streamer.nPointsRemaining();
        constexpr size_t pointsAtATime = 10;
        while (streamer.hasMorePoints()) {
            std::span<const LasPoint> newPoints = streamer.getPoints(pointsAtATime);
            points.insert(points.end(), newPoints.begin(), newPoints.end());
            EXPECT_EQ(streamer.nPointsRemaining(), prevNPointsRemaining - newPoints.size());
            prevNPointsRemaining = streamer.nPointsRemaining();
        }
        EXPECT_EQ(points.size(), expectedNPoints);

        coord_t actualXMin = std::numeric_limits<coord_t>::max();
        coord_t actualXMax = std::numeric_limits<coord_t>::lowest();
        coord_t actualYMin = std::numeric_limits<coord_t>::max();
        coord_t actualYMax = std::numeric_limits<coord_t>::lowest();
        coord_t actualZMin = std::numeric_limits<coord_t>::max();
        coord_t actualZMax = std::numeric_limits<coord_t>::lowest();
        for (const LasPoint& p : points) {
            if (p.x < actualXMin) actualXMin = p.x;
            if (p.x > actualXMax) actualXMax = p.x;
            if (p.y < actualYMin) actualYMin = p.y;
            if (p.y > actualYMax) actualYMax = p.y;
            if (p.z < actualZMin) actualZMin = p.z;
            if (p.z > actualZMax) actualZMax = p.z;
        }
        EXPECT_NEAR(actualXMin, expectedXMin, 0.1);
        EXPECT_NEAR(actualXMax, expectedXMax, 0.1);
        EXPECT_NEAR(actualYMin, expectedYMin, 0.1);
        EXPECT_NEAR(actualYMax, expectedYMax, 0.1);
        EXPECT_NEAR(actualZMin, expectedZMin, 0.1);
        EXPECT_NEAR(actualZMax, expectedZMax, 0.1);


        points.clear();
        streamer.reset();
        };

    {
        SCOPED_TRACE("First Cycle");
        testOneCycle();
    }
    {
        SCOPED_TRACE("Second Cycle");
        testOneCycle();
    }
}

TEST(LidarStreamerTest, TestLidarStreamerMemory) {
    using namespace lapis;

    std::vector<LasPoint> origPoints = {
        { 0, 1, 2, 3, 4 },
        { 5, 6, 7, 8, 9  }
    };

    Extent extent(0, 5, 1, 6);
    CoordRef crs("EPSG:4326");

    LidarStreamerMemory streamer(origPoints, crs, extent);

    auto testOneCycle = [&]() {

        EXPECT_TRUE(streamer.hasMorePoints());
        EXPECT_EQ(streamer.nPoints(), origPoints.size());
        EXPECT_EQ(streamer.nPointsRemaining(), origPoints.size());
        EXPECT_TRUE(streamer.getCoordRef().isConsistent(crs));
        EXPECT_EQ(streamer.getExtent(), extent);

        std::vector<LasPoint> retrievedPoints;
        size_t prevNPointsRemaining = streamer.nPointsRemaining();
        while (streamer.hasMorePoints()) {
            std::span<const LasPoint> newPoints = streamer.getPoints(1);
            retrievedPoints.insert(retrievedPoints.end(), newPoints.begin(), newPoints.end());
            EXPECT_EQ(streamer.nPointsRemaining(), prevNPointsRemaining - newPoints.size());
            prevNPointsRemaining = streamer.nPointsRemaining();
        }
        for (size_t i = 0; i < origPoints.size(); i++) {
            EXPECT_EQ(retrievedPoints[i].x, origPoints[i].x);
            EXPECT_EQ(retrievedPoints[i].y, origPoints[i].y);
            EXPECT_EQ(retrievedPoints[i].z, origPoints[i].z);
            EXPECT_EQ(retrievedPoints[i].intensity, origPoints[i].intensity);
            EXPECT_EQ(retrievedPoints[i].returnNumber, origPoints[i].returnNumber);
        }

        streamer.reset();
        };

    {
        SCOPED_TRACE("First Cycle");
        testOneCycle();
    }
    {
        SCOPED_TRACE("Second Cycle");
        testOneCycle();
    }
    
}

TEST(LidarStreamerTest, TestLidarStreamerReproject) {
    using namespace lapis;

    CoordRef dstCrs("EPSG:26910", linearUnitPresets::internationalFoot);

    //need to test with both hard drive and memory streamers because it has logic that depends on whether the span
    //is a view into a temporary or permanent object (to avoid unneccesary copies)

    //the plan is to collect the points, reproject manually, then run it through the reproject deocrator and make sure it's the same
    //then run it through the decorator *again* to make sure it doesn't do a double transformation

    std::unique_ptr<LidarStreamer> hardDriveStreamer = std::make_unique<LidarStreamerHardDrive>(std::string(LAPISALGOSTESTFILES) + "/testlaz14.laz");
    CoordRef sourceCrs = hardDriveStreamer->getCoordRef();
    std::vector<LasPoint> hardDrivePoints;
    while (hardDriveStreamer->hasMorePoints()) {
        std::span<const LasPoint> newPoints = hardDriveStreamer->getPoints(10);
        hardDrivePoints.insert(hardDrivePoints.end(), newPoints.begin(), newPoints.end());
    }
    hardDriveStreamer->reset();

    const CoordTransform& transform = CoordTransformFactory::getTransform(hardDriveStreamer->getCoordRef(), dstCrs);
    transform.transformXYZ(hardDrivePoints);
    coord_t expectedXMin = std::numeric_limits<coord_t>::max();
    coord_t expectedXMax = std::numeric_limits<coord_t>::lowest();
    coord_t expectedYMin = std::numeric_limits<coord_t>::max();
    coord_t expectedYMax = std::numeric_limits<coord_t>::lowest();
    for (const LasPoint& p : hardDrivePoints) {
        if (p.x < expectedXMin) expectedXMin = p.x;
        if (p.x > expectedXMax) expectedXMax = p.x;
        if (p.y < expectedYMin) expectedYMin = p.y;
        if (p.y > expectedYMax) expectedYMax = p.y;
    }
    Extent expectedExtent(expectedXMin, expectedXMax, expectedYMin, expectedYMax, dstCrs);

    LidarStreamerReproject reprojectedHardDriveStreamer(std::move(hardDriveStreamer), dstCrs);

    auto testOneCycleHardDrive = [&]() {

        EXPECT_TRUE(reprojectedHardDriveStreamer.hasMorePoints());
        EXPECT_EQ(reprojectedHardDriveStreamer.nPoints(), hardDrivePoints.size());
        EXPECT_EQ(reprojectedHardDriveStreamer.nPointsRemaining(), hardDrivePoints.size());
        EXPECT_TRUE(reprojectedHardDriveStreamer.getCoordRef().isConsistent(dstCrs));

        Extent actualExtent = reprojectedHardDriveStreamer.getExtent();
        EXPECT_NEAR(expectedExtent.xmin(), actualExtent.xmin(), 0.1);
        EXPECT_NEAR(expectedExtent.xmax(), actualExtent.xmax(), 0.1);
        EXPECT_NEAR(expectedExtent.ymin(), actualExtent.ymin(), 0.1);
        EXPECT_NEAR(expectedExtent.ymax(), actualExtent.ymax(), 0.1);
        EXPECT_TRUE(expectedExtent.crs().isConsistent(actualExtent.crs()));

        std::vector<LasPoint> fromReprojectStreamer;
        size_t prevNPointsRemaining = reprojectedHardDriveStreamer.nPointsRemaining();
        while (reprojectedHardDriveStreamer.hasMorePoints()) {
            std::span<const LasPoint> newPoints = reprojectedHardDriveStreamer.getPoints(10);
            fromReprojectStreamer.insert(fromReprojectStreamer.end(), newPoints.begin(), newPoints.end());
            EXPECT_EQ(reprojectedHardDriveStreamer.nPointsRemaining(), prevNPointsRemaining - newPoints.size());
            prevNPointsRemaining = reprojectedHardDriveStreamer.nPointsRemaining();
        }

        for (size_t i = 0; i < hardDrivePoints.size(); i++) {
            EXPECT_NEAR(fromReprojectStreamer[i].x, hardDrivePoints[i].x, 0.1);
            EXPECT_NEAR(fromReprojectStreamer[i].y, hardDrivePoints[i].y, 0.1);
            EXPECT_NEAR(fromReprojectStreamer[i].z, hardDrivePoints[i].z, 0.1);
            EXPECT_EQ(fromReprojectStreamer[i].intensity, hardDrivePoints[i].intensity);
            EXPECT_EQ(fromReprojectStreamer[i].returnNumber, hardDrivePoints[i].returnNumber);
        }

        reprojectedHardDriveStreamer.reset();
        };

    {
        SCOPED_TRACE("First Hard Drive Cycle");
        testOneCycleHardDrive();
    }
    {
        SCOPED_TRACE("Second Hard Drive Cycle");
        testOneCycleHardDrive();
    }

    std::vector<LasPoint> memoryPoints = {
        { 300000, 4000000, 2, 3, 4 },
        { 300001, 4000001, 7, 8, 9 }
    };
    std::vector<LasPoint> memoryPointsReprojected = memoryPoints;
    transform.transformXYZ(memoryPointsReprojected);
    expectedXMin = std::numeric_limits<coord_t>::max();
    expectedXMax = std::numeric_limits<coord_t>::lowest();
    expectedYMin = std::numeric_limits<coord_t>::max();
    expectedYMax = std::numeric_limits<coord_t>::lowest();
    for (const LasPoint& p : memoryPointsReprojected) {
        if (p.x < expectedXMin) expectedXMin = p.x;
        if (p.x > expectedXMax) expectedXMax = p.x;
        if (p.y < expectedYMin) expectedYMin = p.y;
        if (p.y > expectedYMax) expectedYMax = p.y;
    }
    expectedExtent = Extent(expectedXMin, expectedXMax, expectedYMin, expectedYMax, dstCrs);

    std::unique_ptr<LidarStreamer> memoryStreamer = std::make_unique<LidarStreamerMemory>(
        memoryPoints,
        sourceCrs,
        Extent(300000, 300001, 4000000, 4000001, sourceCrs)
    );
    LidarStreamerReproject reprojectedMemoryStreamer(std::move(memoryStreamer), dstCrs);

    auto testOneCycleMemory = [&]() {
        EXPECT_TRUE(reprojectedMemoryStreamer.hasMorePoints());
        EXPECT_EQ(reprojectedMemoryStreamer.nPoints(), memoryPoints.size());
        EXPECT_EQ(reprojectedMemoryStreamer.nPointsRemaining(), memoryPoints.size());
        EXPECT_TRUE(reprojectedMemoryStreamer.getCoordRef().isConsistent(dstCrs));

        Extent actualExtent = reprojectedMemoryStreamer.getExtent();
        EXPECT_NEAR(expectedExtent.xmin(), actualExtent.xmin(), 0.1);
        EXPECT_NEAR(expectedExtent.xmax(), actualExtent.xmax(), 0.1);
        EXPECT_NEAR(expectedExtent.ymin(), actualExtent.ymin(), 0.1);
        EXPECT_NEAR(expectedExtent.ymax(), actualExtent.ymax(), 0.1);
        EXPECT_TRUE(expectedExtent.crs().isConsistent(actualExtent.crs()));

        std::vector<LasPoint> fromReprojectStreamer;
        size_t prevNPointsRemaining = reprojectedMemoryStreamer.nPointsRemaining();
        while (reprojectedMemoryStreamer.hasMorePoints()) {
            std::span<const LasPoint> newPoints = reprojectedMemoryStreamer.getPoints(1);
            fromReprojectStreamer.insert(fromReprojectStreamer.end(), newPoints.begin(), newPoints.end());
            EXPECT_EQ(reprojectedMemoryStreamer.nPointsRemaining(), prevNPointsRemaining - newPoints.size());
            prevNPointsRemaining = reprojectedMemoryStreamer.nPointsRemaining();
        }

        for (size_t i = 0; i < memoryPoints.size(); i++) {
            EXPECT_NEAR(fromReprojectStreamer[i].x, memoryPointsReprojected[i].x, 0.1);
            EXPECT_NEAR(fromReprojectStreamer[i].y, memoryPointsReprojected[i].y, 0.1);
            EXPECT_NEAR(fromReprojectStreamer[i].z, memoryPointsReprojected[i].z, 0.1);
            EXPECT_EQ(fromReprojectStreamer[i].intensity, memoryPointsReprojected[i].intensity);
            EXPECT_EQ(fromReprojectStreamer[i].returnNumber, memoryPointsReprojected[i].returnNumber);
        }

        reprojectedMemoryStreamer.reset();
        };

    {
        SCOPED_TRACE("First Memory Cycle");
        testOneCycleMemory();
    }
    {
        SCOPED_TRACE("Second Memory Cycle");
        testOneCycleMemory();
    }
}

TEST(LidarStreamerTest, TestLidarStreamerMinMaxFilter) {
    using namespace lapis;

    //this function has different branches based on _canModifyInPlace, so we need to test both a hard drive and memory streamer
    std::unique_ptr<LidarStreamer> hardDriveStreamer = std::make_unique<LidarStreamerHardDrive>(std::string(LAPISALGOSTESTFILES) + "/testlaz14.laz");

    constexpr coord_t minZ = 3313;
    constexpr coord_t maxZ = 3313.1;

    LidarStreamerMinMaxFilter minMaxFilteredHardDriveStreamer(std::move(hardDriveStreamer), minZ, maxZ);
    size_t origNPoints = minMaxFilteredHardDriveStreamer.nPoints();
    Extent origExtent = minMaxFilteredHardDriveStreamer.getExtent();
    size_t expectedNPoints = 14;
    const CoordRef expectedCrs{ "EPSG:6340+5703" };

    auto testOneCycle = [&](LidarStreamerMinMaxFilter& streamer) {
        EXPECT_TRUE(streamer.hasMorePoints());
        EXPECT_TRUE(streamer.nPoints() <= origNPoints);
        EXPECT_TRUE(streamer.nPointsRemaining() <= origNPoints);
        EXPECT_TRUE(streamer.getCoordRef().isConsistent(expectedCrs));

        Extent newExtent = streamer.getExtent();
        EXPECT_TRUE(newExtent.xmin() >= origExtent.xmin());
        EXPECT_TRUE(newExtent.xmax() <= origExtent.xmax());
        EXPECT_TRUE(newExtent.ymin() >= origExtent.ymin());
        EXPECT_TRUE(newExtent.ymax() <= origExtent.ymax());
        EXPECT_TRUE(newExtent.crs().isConsistent(origExtent.crs()));

        size_t prevNPointsRemaining = streamer.nPointsRemaining();
        size_t actualPointCount = 0;
        constexpr size_t pointsAtATime = 5;
        while (streamer.hasMorePoints()) {
            std::span<const LasPoint> newPoints = streamer.getPoints(pointsAtATime);
            actualPointCount += newPoints.size();
            for (const LasPoint& p : newPoints) {
                EXPECT_TRUE(p.z >= minZ && p.z <= maxZ);
                EXPECT_TRUE(newExtent.contains(p.x, p.y));
            }
            size_t expectedRemaining = prevNPointsRemaining > pointsAtATime ? prevNPointsRemaining - pointsAtATime : 0;
            EXPECT_EQ(streamer.nPointsRemaining(), expectedRemaining);
            prevNPointsRemaining = streamer.nPointsRemaining();
        }
        EXPECT_EQ(actualPointCount, expectedNPoints);

        streamer.reset();
        };

    {
        SCOPED_TRACE("First Hard Drive Cycle");
        testOneCycle(minMaxFilteredHardDriveStreamer);
    }
    {
        SCOPED_TRACE("Second Hard Drive Cycle");
        testOneCycle(minMaxFilteredHardDriveStreamer);
    }


    std::vector<LasPoint> memoryPoints = {
        { 0, 0, 3312, 0, 0 },
        { 0, 1, 3313, 0, 0 },
        { 1, 0, 3313.05, 0, 0 },
        { 1, 1, 3313.1, 0, 0 },
        { 1, 0.5, 3313.15, 0, 0 },
        { 0.5, 1, 3314, 0, 0 }
    };
    origNPoints = memoryPoints.size();
    expectedNPoints = 3;
    std::unique_ptr<LidarStreamer> memoryStreamer = std::make_unique<LidarStreamerMemory>(
        memoryPoints,
        expectedCrs,
        Extent(0, 1, 0, 1, expectedCrs)
    );
    origExtent = memoryStreamer->getExtent();
    LidarStreamerMinMaxFilter minMaxFilteredMemoryStreamer(std::move(memoryStreamer), minZ, maxZ);
    {
        SCOPED_TRACE("First Memory Cycle");
        testOneCycle(minMaxFilteredMemoryStreamer);
    }
    {
        SCOPED_TRACE("Second Memory Cycle");
        testOneCycle(minMaxFilteredMemoryStreamer);
    }

}

TEST(LidarStreamerTest, TestLidarStreamerComposite) {
    using namespace lapis;

    // Test 1: Combine two memory streamers
    std::vector<LasPoint> points1 = {
        { 0, 0, 1, 10, 1 },
        { 1, 1, 2, 20, 2 }
    };
    std::vector<LasPoint> points2 = {
        { 2, 2, 3, 30, 3 },
        { 3, 3, 4, 40, 4 },
        { 4, 4, 5, 50, 5 }
    };

    CoordRef crs("EPSG:4326");
    Extent extent1(0, 1, 0, 1, crs);
    Extent extent2(2, 4, 2, 4, crs);

    std::unique_ptr<LidarStreamer> streamer1 = std::make_unique<LidarStreamerMemory>(points1, crs, extent1);
    std::unique_ptr<LidarStreamer> streamer2 = std::make_unique<LidarStreamerMemory>(points2, crs, extent2);

    LidarStreamerComposite composite;
    composite.addStream(std::move(streamer1));
    composite.addStream(std::move(streamer2));

    auto testOneCycle = [&]() {
        EXPECT_TRUE(composite.hasMorePoints());
        EXPECT_EQ(composite.nPoints(), points1.size() + points2.size());
        EXPECT_EQ(composite.nPointsRemaining(), points1.size() + points2.size());
        EXPECT_TRUE(composite.getCoordRef().isConsistent(crs));

        Extent compositeExtent = composite.getExtent();
        EXPECT_EQ(compositeExtent.xmin(), 0);
        EXPECT_EQ(compositeExtent.xmax(), 4);
        EXPECT_EQ(compositeExtent.ymin(), 0);
        EXPECT_EQ(compositeExtent.ymax(), 4);

        std::vector<LasPoint> allPoints;
        size_t batchSize = 3;
        size_t prevRemaining = composite.nPointsRemaining();
        while (composite.hasMorePoints()) {
            std::span<const LasPoint> batch = composite.getPoints(batchSize);
            allPoints.insert(allPoints.end(), batch.begin(), batch.end());
            EXPECT_EQ(composite.nPointsRemaining(), prevRemaining - batch.size());
            prevRemaining = composite.nPointsRemaining();
        }

        EXPECT_EQ(allPoints.size(), points1.size() + points2.size());

        for (size_t i = 0; i < points1.size(); i++) {
            EXPECT_EQ(allPoints[i].x, points1[i].x);
            EXPECT_EQ(allPoints[i].z, points1[i].z);
            EXPECT_EQ(allPoints[i].intensity, points1[i].intensity);
        }
        for (size_t i = 0; i < points2.size(); i++) {
            EXPECT_EQ(allPoints[points1.size() + i].x, points2[i].x);
            EXPECT_EQ(allPoints[points1.size() + i].z, points2[i].z);
            EXPECT_EQ(allPoints[points1.size() + i].intensity, points2[i].intensity);
        }

        composite.reset();
        };

    {
        SCOPED_TRACE("First Cycle");
        testOneCycle();
    }
    {
        SCOPED_TRACE("Second Cycle");
        testOneCycle();
    }
}

TEST(LidarStreamerTest, TestLidarStreamerCompositeCRSMismatch) {
    using namespace lapis;

    std::vector<LasPoint> points1 = {
        { 299000, 4202000, 3313, 10, 1 },
        { 299001, 4202001, 3313.5, 20, 2 }
    };
    std::vector<LasPoint> points2 = {
        { 299002, 4202002, 3314, 30, 3 },
        { 299003, 4202003, 3314.5, 40, 4 }
    };

    CoordRef crs1("EPSG:6340+5703");
    CoordRef crs2("EPSG:26910", linearUnitPresets::internationalFoot);
    Extent extent1(299000, 299001, 4202000, 4202001, crs1);
    Extent extent2(299002, 299003, 4202002, 4202003, crs1);

    std::unique_ptr<LidarStreamer> streamer1 = std::make_unique<LidarStreamerMemory>(points1, crs1, extent1);
    std::unique_ptr<LidarStreamer> streamer2 = std::make_unique<LidarStreamerMemory>(points2, crs1, extent2);

    const CoordTransform& transform = CoordTransformFactory::getTransform(crs1, crs2);
    std::vector<LasPoint> points2Reprojected = points2;
    transform.transformXYZ(points2Reprojected);

    LidarStreamerComposite composite;
    composite.addStream(std::move(streamer1));

    std::unique_ptr<LidarStreamer> streamer2DifferentCRS = std::make_unique<LidarStreamerMemory>(
        points2Reprojected, crs2,
        Extent(points2Reprojected[0].x, points2Reprojected[1].x,
            points2Reprojected[0].y, points2Reprojected[1].y, crs2)
    );
    composite.addStream(std::move(streamer2DifferentCRS));

    auto testOneCycle = [&]() {
        EXPECT_TRUE(composite.getCoordRef().isConsistent(crs1));
        EXPECT_EQ(composite.nPoints(), points1.size() + points2.size());

        std::vector<LasPoint> allPoints;
        while (composite.hasMorePoints()) {
            std::span<const LasPoint> batch = composite.getPoints(2);
            allPoints.insert(allPoints.end(), batch.begin(), batch.end());
        }

        EXPECT_EQ(allPoints.size(), points1.size() + points2.size());

        for (size_t i = 0; i < points1.size(); i++) {
            EXPECT_NEAR(allPoints[i].x, points1[i].x, 0.1);
            EXPECT_NEAR(allPoints[i].y, points1[i].y, 0.1);
            EXPECT_NEAR(allPoints[i].z, points1[i].z, 0.1);
        }

        for (size_t i = 0; i < points2.size(); i++) {
            EXPECT_NEAR(allPoints[points1.size() + i].x, points2[i].x, 0.1);
            EXPECT_NEAR(allPoints[points1.size() + i].y, points2[i].y, 0.1);
            EXPECT_NEAR(allPoints[points1.size() + i].z, points2[i].z, 0.1);
        }

        composite.reset();
        };

    {
        SCOPED_TRACE("First Cycle");
        testOneCycle();
    }
    {
        SCOPED_TRACE("Second Cycle");
        testOneCycle();
    }

    std::vector<LasPoint> points3 = {
        { 100, 200, 300, 5, 1 }
    };
    CoordRef crs3("EPSG:4326");
    CoordRef targetCrs("EPSG:32610");

    LidarStreamerComposite compositeWithCrs(targetCrs);

    std::unique_ptr<LidarStreamer> streamer3 = std::make_unique<LidarStreamerMemory>(
        points3, crs3, Extent(100, 100, 200, 200, crs3)
    );
    compositeWithCrs.addStream(std::move(streamer3));

    EXPECT_TRUE(compositeWithCrs.getCoordRef().isConsistent(targetCrs));
}

TEST(LidarStreamerTest, TestLidarStreamerCompositeWithFilters) {
    using namespace lapis;

    std::vector<LasPoint> memoryPoints1 = {
        { 0, 0, 3312, 0, 0 },    // filtered out
        { 0, 1, 3313, 10, 1 },   // kept
        { 1, 0, 3313.05, 20, 2 },// kept
        { 1, 1, 3313.15, 30, 3 } // filtered out
    };
    std::vector<LasPoint> memoryPoints2 = {
        { 2, 2, 3313, 40, 4 },   // kept
        { 3, 3, 3313.1, 50, 5 }, // kept
        { 4, 4, 3314, 60, 6 }    // filtered out
    };

    constexpr coord_t minZ = 3313;
    constexpr coord_t maxZ = 3313.1;
    const CoordRef crs("EPSG:4326");

    std::unique_ptr<LidarStreamer> memoryStreamer1 = std::make_unique<LidarStreamerMemory>(
        memoryPoints1, crs, Extent(0, 1, 0, 1, crs)
    );
    std::unique_ptr<LidarStreamer> memoryStreamer2 = std::make_unique<LidarStreamerMemory>(
        memoryPoints2, crs, Extent(2, 4, 2, 4, crs)
    );

    std::unique_ptr<LidarStreamer> filteredStreamer1 = std::make_unique<LidarStreamerMinMaxFilter>(
        std::move(memoryStreamer1), minZ, maxZ
    );
    std::unique_ptr<LidarStreamer> filteredStreamer2 = std::make_unique<LidarStreamerMinMaxFilter>(
        std::move(memoryStreamer2), minZ, maxZ
    );

    LidarStreamerComposite composite;
    composite.addStream(std::move(filteredStreamer1));
    composite.addStream(std::move(filteredStreamer2));

    constexpr size_t expectedTotalPoints = 4;

    auto testOneCycle = [&]() {
        EXPECT_TRUE(composite.hasMorePoints());
        EXPECT_GE(composite.nPoints(), expectedTotalPoints);
        EXPECT_GE(composite.nPointsRemaining(), expectedTotalPoints);

        std::vector<LasPoint> allPoints;
        size_t prevRemaining = composite.nPointsRemaining();
        constexpr size_t batchSize = 10;

        while (composite.hasMorePoints()) {
            std::span<const LasPoint> batch = composite.getPoints(batchSize);
            EXPECT_GT(batch.size(), 0);

            for (const LasPoint& p : batch) {
                EXPECT_GE(p.z, minZ);
                EXPECT_LE(p.z, maxZ);
            }

            allPoints.insert(allPoints.end(), batch.begin(), batch.end());
            size_t expectedRemaining = prevRemaining > batchSize ? prevRemaining - batchSize : 0;
            EXPECT_EQ(composite.nPointsRemaining(), expectedRemaining);
            prevRemaining = composite.nPointsRemaining();
        }

        EXPECT_EQ(allPoints.size(), expectedTotalPoints);

        EXPECT_EQ(allPoints[0].intensity, 10);
        EXPECT_EQ(allPoints[1].intensity, 20);
        EXPECT_EQ(allPoints[2].intensity, 40);
        EXPECT_EQ(allPoints[3].intensity, 50);

        composite.reset();
        };

    {
        SCOPED_TRACE("First Cycle");
        testOneCycle();
    }
    {
        SCOPED_TRACE("Second Cycle");
        testOneCycle();
    }

    std::vector<LasPoint> unfilteredPoints = {
        { 5, 5, 100, 70, 7 },
        { 6, 6, 200, 80, 8 }
    };

    LidarStreamerComposite mixedComposite;

    std::unique_ptr<LidarStreamer> filteredStreamer = std::make_unique<LidarStreamerMinMaxFilter>(
        std::make_unique<LidarStreamerMemory>(memoryPoints1, crs, Extent(0, 1, 0, 1, crs)),
        minZ, maxZ
    );
    std::unique_ptr<LidarStreamer> unfilteredStreamer = std::make_unique<LidarStreamerMemory>(
        unfilteredPoints, crs, Extent(5, 6, 5, 6, crs)
    );

    mixedComposite.addStream(std::move(filteredStreamer));
    mixedComposite.addStream(std::move(unfilteredStreamer));

    EXPECT_TRUE(mixedComposite.hasMorePoints());
    EXPECT_GE(mixedComposite.nPoints(), 4);

    std::vector<LasPoint> mixedPoints;
    while (mixedComposite.hasMorePoints()) {
        std::span<const LasPoint> batch = mixedComposite.getPoints(3);
        mixedPoints.insert(mixedPoints.end(), batch.begin(), batch.end());
    }

    EXPECT_EQ(mixedPoints.size(), 4);
    EXPECT_EQ(mixedPoints[0].intensity, 10);
    EXPECT_EQ(mixedPoints[1].intensity, 20);
    EXPECT_EQ(mixedPoints[2].intensity, 70);
    EXPECT_EQ(mixedPoints[3].intensity, 80);
}

TEST(LidarStreamerTest, TestLidarStreamerBuilder) {
    using namespace lapis;

    LidarStreamerBuilder builder;
    std::string lasfilename = std::string(LAPISALGOSTESTFILES) + "/testlaz14.laz";
    constexpr size_t lasNPoints = 93;
    std::string demfilename = std::string(LAPISALGOSTESTFILES) + "/testlazground.img";

    auto lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);

    //first just testing the basics
    {
        std::unique_ptr<LidarStreamer> streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_EQ(streamer->nPoints(), lasNPoints);
        EXPECT_EQ(streamer->nPointsRemaining(), lasNPoints);
        EXPECT_TRUE(streamer->getCoordRef().isConsistent(CoordRef{ "EPSG:6340+5703" }));
        Extent actualExtent = streamer->getExtent();
        Extent expectedExtent{ lasfilename };
        EXPECT_TRUE(actualExtent == expectedExtent);

        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), 93);
    }
    {
        auto streamer = lasSpec.buildHardDrive();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_EQ(streamer->nPoints(), lasNPoints);
        EXPECT_EQ(streamer->nPointsRemaining(), lasNPoints);
        EXPECT_TRUE(streamer->getCoordRef().isConsistent(CoordRef{ "EPSG:6340+5703" }));
        Extent actualExtent = streamer->getExtent();
        Extent expectedExtent{ lasfilename };
        EXPECT_TRUE(actualExtent == expectedExtent);
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), lasNPoints);
    }

    //filters should do something
    builder.reset();
    builder.addFilter(std::make_shared<LasFilterFirstReturns>());
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        size_t expectedNPoints = 91; //2 points fail the filter
        auto streamer = lasSpec.buildInMemory(); //buildInMemory causes filters to be applied before nPoint is calculated
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_GE(streamer->nPoints(), expectedNPoints);
        EXPECT_GE(streamer->nPointsRemaining(), expectedNPoints);
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), expectedNPoints);
        for (const LasPoint& p : points) {
            EXPECT_EQ(p.returnNumber, 1);
        }
    }

    //manul crs and unit specification
    builder.reset();
    builder.setLasCrsOverride(CoordRef("EPSG:26910"));
    builder.setLasZUnitsOverride(linearUnitPresets::internationalFoot);
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_EQ(streamer->nPoints(), lasNPoints);
        EXPECT_EQ(streamer->nPointsRemaining(), lasNPoints);
        EXPECT_TRUE(streamer->getCoordRef().isConsistent(CoordRef{ "EPSG:26910", linearUnitPresets::internationalFoot }));
        EXPECT_DOUBLE_EQ(streamer->getCoordRef().getZUnits().convertOneToSI(1), linearUnitPresets::internationalFoot.convertOneToSI(1));
    }
    builder.reset();
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename).setCrs(CoordRef("EPSG:26910")).setZUnits(linearUnitPresets::internationalFoot);
    {
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_EQ(streamer->nPoints(), lasNPoints);
        EXPECT_EQ(streamer->nPointsRemaining(), lasNPoints);
        EXPECT_TRUE(streamer->getCoordRef().isConsistent(CoordRef{ "EPSG:26910", linearUnitPresets::internationalFoot }));
        EXPECT_DOUBLE_EQ(streamer->getCoordRef().getZUnits().convertOneToSI(1), linearUnitPresets::internationalFoot.convertOneToSI(1));
    }

    //multiple las files
    builder.reset();
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    lasSpec.addLas(lasfilename);
    {
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_EQ(streamer->nPoints(), lasNPoints * 2);
        EXPECT_EQ(streamer->nPointsRemaining(), lasNPoints * 2);
        EXPECT_TRUE(streamer->getCoordRef().isConsistent(CoordRef{ "EPSG:6340+5703" }));
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), lasNPoints * 2);
    }

    //apply decorator
    builder.reset();
    builder.apply<LidarStreamerMinMaxFilter>(3313, 9000);
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        size_t expectedNPoints = 58;
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_GE(streamer->nPoints(), expectedNPoints);
        EXPECT_GE(streamer->nPointsRemaining(), expectedNPoints);
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), expectedNPoints);
    }

    //normalize by dem
    builder.reset();
    builder.addDem(demfilename);
    builder.normalizeByDem();
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        //all the Z values should be between -1 and +2 now
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_EQ(streamer->nPoints(), lasNPoints);
        EXPECT_EQ(streamer->nPointsRemaining(), lasNPoints);
        auto points = streamer->getPoints(streamer->nPoints());
        for (const LasPoint& p : points) {
            EXPECT_GE(p.z, -1);
            EXPECT_LE(p.z, 2);
        }
    }

    //decorators should be applied in order
    builder.reset();
    builder.addDem(demfilename);
    builder.normalizeByDem().apply<LidarStreamerMinMaxFilter>(0, 1);
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        //all the Z values should be between 0 and 1 now because the filter is applied after the normalization
        size_t expectedNPoints = 90;
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_GE(streamer->nPoints(), expectedNPoints);
        EXPECT_GE(streamer->nPointsRemaining(), expectedNPoints);
        auto points = streamer->getPoints(streamer->nPoints());
        for (const LasPoint& p : points) {
            EXPECT_GE(p.z, 0);
            EXPECT_LE(p.z, 1);
        }
    }
    //doing the same decorators in the other order
    builder.reset();
    builder.addDem(demfilename);
    builder.apply<LidarStreamerMinMaxFilter>(0,1).normalizeByDem();
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    { 
        //no points should pass filter this time
        size_t expectedNPoints = 0;
        auto streamer = lasSpec.buildInMemory();
        //hasmorepoints is a little ambiguous here, so not testing it
        
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), expectedNPoints);
    }

    //normalizing by dem without any dems should throw
    builder.reset();
    builder.normalizeByDem();
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    EXPECT_THROW(lasSpec.buildInMemory(), std::runtime_error);

    //specifying an extent should cause non-overlapping las files to be ignored
    builder.reset();
    builder.setExtent(Extent(0, 1, 0, 1));
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        //no las files should pass the filter
        auto streamer = lasSpec.buildInMemory();
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), 0);
    }

    //specifying an extent should also apply a filter to the points of files that do pass
    builder.reset();
    builder.setExtent(Extent(299001, 299050, 4201090, 4202050));
    lasSpec = builder.createLasSpecifier();
    lasSpec.addLas(lasfilename);
    {
        size_t expectedNPoints = 8;
        auto streamer = lasSpec.buildInMemory();
        EXPECT_TRUE(streamer->hasMorePoints());
        EXPECT_GE(streamer->nPoints(), expectedNPoints);
        EXPECT_GE(streamer->nPointsRemaining(), expectedNPoints);
        auto points = streamer->getPoints(streamer->nPoints());
        EXPECT_EQ(points.size(), expectedNPoints);
    }
}