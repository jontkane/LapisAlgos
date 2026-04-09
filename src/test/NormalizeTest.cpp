#include"test_pch.hpp"
#include"..\AllNormalizers.hpp"
#include"..\AllLidarStreamers.hpp"

TEST(NormalizeTests, NormalizeDoNothingTest) {
    using namespace lapis;

    std::vector<LasPoint> points = {
        LasPoint(1, 2, 3, 100, 1),
        LasPoint(4, 5, 6, 200, 2),
        LasPoint(7, 8, 9, 300, 3)
    };
    CoordRef crs{ "EPSG:26910" };
    Extent expectedExtent{ 1, 7, 2, 8, crs };

    std::unique_ptr<LidarStreamer> streamer = std::make_unique<LidarStreamerMemory>(points, crs, expectedExtent);

    NormalizeDoNothing normalizer{ std::move(streamer) };

    auto testOneCycle = [&]() {

        EXPECT_TRUE(normalizer.hasMorePoints());
        EXPECT_EQ(normalizer.nPoints(), points.size());
        EXPECT_EQ(normalizer.nPointsRemaining(), points.size());
        EXPECT_TRUE(normalizer.getCoordRef().isConsistent(crs));

        EXPECT_EQ(normalizer.getExtent().xmin(), expectedExtent.xmin());
        EXPECT_EQ(normalizer.getExtent().xmax(), expectedExtent.xmax());
        EXPECT_EQ(normalizer.getExtent().ymin(), expectedExtent.ymin());
        EXPECT_EQ(normalizer.getExtent().ymax(), expectedExtent.ymax());
        EXPECT_TRUE(normalizer.getExtent().crs().isConsistent(crs));

        std::vector<LasPoint> actualPoints;

        size_t batchSize = 2;
        size_t prevNPointsRemaining = normalizer.nPointsRemaining();
        while (normalizer.hasMorePoints()) {
            auto batch = normalizer.getPoints(batchSize);
            EXPECT_EQ(batch.size(), std::min(batchSize, prevNPointsRemaining));
            actualPoints.insert(actualPoints.end(), batch.begin(), batch.end());
            prevNPointsRemaining -= batch.size();
        }

        EXPECT_EQ(actualPoints.size(), points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            EXPECT_EQ(actualPoints[i].x, points[i].x);
            EXPECT_EQ(actualPoints[i].y, points[i].y);
            EXPECT_EQ(actualPoints[i].z, points[i].z);
            EXPECT_EQ(actualPoints[i].intensity, points[i].intensity);
            EXPECT_EQ(actualPoints[i].returnNumber, points[i].returnNumber);
        }

        normalizer.reset();
        };

    {
        SCOPED_TRACE("First cycle");
        testOneCycle();
    }
    {
        SCOPED_TRACE("Second cycle");
        testOneCycle();
    }
}

TEST(NormalizeTests, NormalizeByRasterTest) {
    using namespace lapis;
    
    CoordRef utm10{ "EPSG:26910" };
    CoordRef utm11{ "EPSG:26911" };

    Raster<coord_t> singleDem{
        Alignment{Extent{0,4,0,4,utm10},4,4}
    };

    //2x2 hole of noData in the upper left
    //this means points falling in the absolute upper left cell will extract noData, even when extracting bilinearly
    for (cell_t cell : CellIterator(singleDem)) {
        singleDem[cell] = cell;
        rowcol_t row = singleDem.rowFromCell(cell);
        rowcol_t col = singleDem.colFromCell(cell);
        if (row > 1 || col > 1) {
            singleDem[cell].has_value() = true;
        }
        else {
            singleDem[cell].has_value() = false;
        }
    }

    std::vector<LasPoint> inPoints = {
        LasPoint{1.5,1.5,10,0,0}, //valid
        LasPoint{3.5,0.5,10,0,0}, //valid
        LasPoint{2,0.9,6,0,0} , //valid
        LasPoint{-1,-1,-1,0,0}, //outside the DEM
        LasPoint{0.5,3.5,10,0,0}, //should extract as noData
    };
    Extent expectedExtent{ -1,3.5,-1,3.5,utm10 };

    std::vector<xtl::xoptional<coord_t>> extractedDemValues;
    for (const auto& pt : inPoints) {
        extractedDemValues.push_back(singleDem.extract(pt.x, pt.y, ExtractMethod::bilinear));
    }
    //this test isn't for extract, but let's double check I did it right
    EXPECT_TRUE(extractedDemValues[0].has_value());
    EXPECT_TRUE(extractedDemValues[1].has_value());
    EXPECT_TRUE(extractedDemValues[2].has_value());
    EXPECT_FALSE(extractedDemValues[3].has_value());
    EXPECT_FALSE(extractedDemValues[4].has_value());

    size_t expectedFinalPointCount = 3; //unnormalizable points shouldn't be returned

    auto getNewMemoryStreamer = [&]() {
        return std::make_unique<LidarStreamerMemory>(inPoints, utm10, expectedExtent);
        };

    std::unique_ptr<LidarStreamer> normalizerOneDem{ new NormalizeByRaster(getNewMemoryStreamer(), singleDem) };

    auto testOneCycle = [&](const std::unique_ptr<LidarStreamer>& normalizer, size_t thisExpectedFinalPointCount) {
        EXPECT_TRUE(normalizer->hasMorePoints());
        EXPECT_EQ(normalizer->nPoints(), inPoints.size());
        EXPECT_EQ(normalizer->nPointsRemaining(), inPoints.size());
        EXPECT_TRUE(normalizer->getCoordRef().isConsistent(utm10));

        EXPECT_EQ(normalizer->getExtent().xmin(), expectedExtent.xmin());
        EXPECT_EQ(normalizer->getExtent().xmax(), expectedExtent.xmax());
        EXPECT_EQ(normalizer->getExtent().ymin(), expectedExtent.ymin());
        EXPECT_EQ(normalizer->getExtent().ymax(), expectedExtent.ymax());
        EXPECT_TRUE(normalizer->getExtent().crs().isConsistent(utm10));

        std::vector<LasPoint> actualPoints;
        size_t batchSize = 2;
        size_t prevNPointsRemaining = normalizer->nPointsRemaining();
        while (normalizer->hasMorePoints()) {
            auto batch = normalizer->getPoints(batchSize);
            EXPECT_TRUE(batch.size() <= std::min(batchSize, prevNPointsRemaining));
            actualPoints.insert(actualPoints.end(), batch.begin(), batch.end());
            prevNPointsRemaining -= batch.size();
        }
        EXPECT_EQ(actualPoints.size(), thisExpectedFinalPointCount);

        //scenarios where the final two points normalize tend to have weird edge cases, so I'm not testing their Z values
        for (size_t i = 0; i < expectedFinalPointCount; ++i) {
            EXPECT_EQ(actualPoints[i].x, inPoints[i].x);
            EXPECT_EQ(actualPoints[i].y, inPoints[i].y);
            EXPECT_EQ(actualPoints[i].intensity, inPoints[i].intensity);
            EXPECT_EQ(actualPoints[i].returnNumber, inPoints[i].returnNumber);

            EXPECT_NEAR(actualPoints[i].z, inPoints[i].z - extractedDemValues[i].value(), 0.01) << " for point " << i;
        }


        normalizer->reset();
    };
    {
        SCOPED_TRACE("First cycle single DEM");
        testOneCycle(normalizerOneDem, expectedFinalPointCount);
    }
    {
        SCOPED_TRACE("Second cycle single DEM");
        testOneCycle(normalizerOneDem, expectedFinalPointCount);
    }

    //now with two half-rasters which mosaic to be the same as the original raster
    Raster<coord_t> halfDem1 = cropRaster(singleDem, Extent{ 0,4,0,2,utm10 }, SnapType::out);
    Raster<coord_t> halfDem2 = cropRaster(singleDem, Extent{ 0,4,2,4,utm10 }, SnapType::out);

    NormalizeByRasterFactory halfDemFactory;
    halfDemFactory.addRaster(halfDem1);
    halfDemFactory.addRaster(halfDem2);
    std::unique_ptr<LidarStreamer> normalizerHalfDems = halfDemFactory.create(getNewMemoryStreamer());

    {
        SCOPED_TRACE("First cycle half DEMs");
        testOneCycle(normalizerHalfDems, expectedFinalPointCount);
    }
    {
        SCOPED_TRACE("Second cycle half DEMs");
        testOneCycle(normalizerHalfDems, expectedFinalPointCount);
    }

    //reprojecting one of the half dems, but leaving the other alone
    //because the bilinear interpolation algorithm we use really wants to return a value as often as possible,
    //the point inside nodata will now successfully extract
    Raster<coord_t> reprojectedHalfDem2 = transformRaster(halfDem2, utm11, ExtractMethod::bilinear);
    NormalizeByRasterFactory reprojectedHalfDemFactory;
    reprojectedHalfDemFactory.addRaster(halfDem1);
    reprojectedHalfDemFactory.addRaster(reprojectedHalfDem2);

    std::unique_ptr<LidarStreamer> normalizerReprojectedHalfDems = reprojectedHalfDemFactory.create(getNewMemoryStreamer());

    {
        SCOPED_TRACE("First cycle reprojected half DEMs");
        testOneCycle(normalizerReprojectedHalfDems, expectedFinalPointCount + 1);
    }
    {
        SCOPED_TRACE("Second cycle reprojected half DEMs");
        testOneCycle(normalizerReprojectedHalfDems, expectedFinalPointCount + 1);
    }

    Raster<coord_t> nonOverlappingDem{
        Alignment{Extent{10,14,10,14,utm10},4,4}
    };
    for (cell_t cell : CellIterator(nonOverlappingDem)) {
        nonOverlappingDem[cell] = cell;
        nonOverlappingDem[cell].has_value() = true;
    }
    NormalizeByRasterFactory nonOverlappingFactory;
    nonOverlappingFactory.addRaster(nonOverlappingDem);
    EXPECT_THROW(nonOverlappingFactory.create(getNewMemoryStreamer()), std::runtime_error);

    singleDem.defineCRS(utm11);
    NormalizeByRasterFactory crsOverrideFactory;
    crsOverrideFactory.addRaster(singleDem).setCrs(utm10);
    std::unique_ptr<LidarStreamer> normalizerCrsOverride = crsOverrideFactory.create(getNewMemoryStreamer());
    {
        SCOPED_TRACE("First cycle CRS override");
        testOneCycle(normalizerCrsOverride, expectedFinalPointCount);
    }
    {
        SCOPED_TRACE("Second cycle CRS override");
        testOneCycle(normalizerCrsOverride, expectedFinalPointCount);
    }

    singleDem.defineCRS(utm10);
    crsOverrideFactory = NormalizeByRasterFactory();
    crsOverrideFactory.setCrsOverride(utm11);
    EXPECT_THROW(std::unique_ptr<LidarStreamer> normalizerFactoryCrsOverride = crsOverrideFactory.create(getNewMemoryStreamer()), std::runtime_error);


    NormalizeByRasterFactory zUnitsOverrideFactory;
    Raster<coord_t> demInFeet{
        Alignment{Extent{0,4,0,4,utm10},4,4}
    };
    for (cell_t cell : CellIterator(demInFeet)) {
        demInFeet[cell] = cell * 3.28084;
        rowcol_t row = demInFeet.rowFromCell(cell);
        rowcol_t col = demInFeet.colFromCell(cell);
        if (row > 1 || col > 1) {
            demInFeet[cell].has_value() = true;
        }
        else {
            demInFeet[cell].has_value() = false;
        }
    }

    zUnitsOverrideFactory.addRaster(demInFeet).setZUnits(linearUnitPresets::internationalFoot);
    std::unique_ptr<LidarStreamer> normalizerZUnits = zUnitsOverrideFactory.create(getNewMemoryStreamer());

    {
        SCOPED_TRACE("First cycle z units override");
        testOneCycle(normalizerZUnits, expectedFinalPointCount);
    }
    {
        SCOPED_TRACE("Second cycle z units override");
        testOneCycle(normalizerZUnits, expectedFinalPointCount);
    }

    //the coarse dem is the same as the original dem in the first test, but all its values are 100
    //the fine dem has twice the resolution of the original dem, but its values are the same as the original dem
    Raster<coord_t> coarseDem{
        Alignment{Extent{0,4,0,4,utm10},4,4}
    };
    for (cell_t cell : CellIterator(coarseDem)) {
        coarseDem[cell] = 100;
        coarseDem[cell].has_value() = true;
        rowcol_t row = coarseDem.rowFromCell(cell);
        rowcol_t col = coarseDem.colFromCell(cell);
        if (row > 1 || col > 1) {
            coarseDem[cell].has_value() = true;
        }
        else {
            coarseDem[cell].has_value() = false;
        }
    }
    Raster<coord_t> fineDem{
        Alignment{Extent{0,4,0,4,utm10},8,8}
    };
    for (cell_t cell : CellIterator(fineDem)) {
        coord_t x = fineDem.xFromCell(cell);
        coord_t y = fineDem.yFromCell(cell);
        auto v = singleDem.extract(x, y, ExtractMethod::near);
        fineDem[cell].has_value() = v.has_value();
        if (v.has_value()) {
            fineDem[cell] = v.value();
        }
    }
    //unfortunately this changes the expected values for all the points
    extractedDemValues.clear();
    for (const auto& pt : inPoints) {
        extractedDemValues.push_back(fineDem.extract(pt.x, pt.y, ExtractMethod::bilinear));
    }
    //this test isn't for extract, but let's double check I did it right
    EXPECT_TRUE(extractedDemValues[0].has_value());
    EXPECT_TRUE(extractedDemValues[1].has_value());
    EXPECT_TRUE(extractedDemValues[2].has_value());
    EXPECT_FALSE(extractedDemValues[3].has_value());
    EXPECT_FALSE(extractedDemValues[4].has_value());

    NormalizeByRasterFactory multiResolutionFactory;
    multiResolutionFactory.addRaster(coarseDem);
    multiResolutionFactory.addRaster(fineDem);
    std::unique_ptr<LidarStreamer> normalizerMultiResolution = multiResolutionFactory.create(getNewMemoryStreamer());
    {
        SCOPED_TRACE("First cycle multi resolution DEMs");
        testOneCycle(normalizerMultiResolution, expectedFinalPointCount);
    }
    {
        SCOPED_TRACE("Second cycle multi resolution DEMs");
        testOneCycle(normalizerMultiResolution, expectedFinalPointCount);
    }
}