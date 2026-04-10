#include"test_pch.hpp"
#include"..\AllCsmMakers.hpp"

TEST(CsmTest, CsmMaxHeightTest) {
    using namespace lapis;

    Alignment a{ Extent(0, 10, 0, 10), 10, 10 };

    auto addPoint = [](std::vector<LasPoint>&points, Raster<csm_t>& r, coord_t x, coord_t y, coord_t z) {
        points.emplace_back(x, y, z, 0, 0);
        auto v = r.atXY(x, y);
        v.has_value() = true;
        v.value() = std::max(v.value(), z);
    };
    auto addPointRadius = [](std::vector<LasPoint>&points, Raster<csm_t>& r, coord_t x, coord_t y, coord_t z, coord_t radius) {
        points.emplace_back(x, y, z, 0, 0);
        coord_t sqrtTwo = std::sqrt(2.);
        std::vector<std::pair<coord_t, coord_t>> offsets = {
            {0, 0},
            {radius, 0},
            {-radius, 0},
            {0, radius},
            {0, -radius},
            {radius / sqrtTwo, radius / sqrtTwo},
            {radius / sqrtTwo, -radius / sqrtTwo},
            {-radius / sqrtTwo, radius / sqrtTwo},
            {-radius / sqrtTwo, -radius / sqrtTwo} };

        for (const auto& offset : offsets) {
            coord_t offsetX = offset.first;
            coord_t offsetY = offset.second;
            if (!r.contains(x + offsetX, y + offsetY)) {
                continue;
            }
            auto v = r.atXY(x + offsetX, y + offsetY);
            v.has_value() = true;
            v.value() = std::max(v.value(), z);
        }
        };
    auto getInitializedRaster = [&]() {
        Raster<csm_t> r{ a };
        for (cell_t cell : CellIterator(a)) {
            auto v = r.atCell(cell);
            v.value() = std::numeric_limits<coord_t>::lowest();
        }
        return r;
    };
    auto compareRasters = [](const Raster<csm_t>& expected, const Raster<csm_t>& result) {
        EXPECT_LE(result.xmin(), expected.xmin());
        EXPECT_LE(result.ymin(), expected.ymin());
        EXPECT_GE(result.xmax(), expected.xmax());
        EXPECT_GE(result.xmax(), expected.xmax());
        EXPECT_TRUE(expected.crs().isConsistent(result.crs()));

        for (cell_t expectedCell : CellIterator(expected)) {
            coord_t x = expected.xFromCell(expectedCell);
            coord_t y = expected.yFromCell(expectedCell);
            auto expectedV = expected.atCell(expectedCell);
            auto resultV = result.atXY(x, y);
            if (expectedV.has_value()) {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_NEAR(expectedV.value(), resultV.value(), 0.01);
            }
            else {
                EXPECT_FALSE(resultV.has_value());
            }
        }
        };
    
    //basic test
    {
        SCOPED_TRACE("Basic test");
        Raster<csm_t> expected = getInitializedRaster();

        std::vector<LasPoint> points;

        addPoint(points, expected, 1.1, 1.2, 5);
        addPoint(points, expected, 1.2, 1.1, 10); //same cell, higher point
        addPoint(points, expected, 2.5, 2.5, 3);
        addPoint(points, expected, 5.3, 5.7, 7);

        CsmMaxHeight maker{ a };
        maker.addPoints(points);
        Raster<csm_t> result = maker.getRaster();

        compareRasters(expected, result);
    }

    //addPointsUnsafe
    {
        SCOPED_TRACE("addPointsUnsafe test");
        Raster<csm_t> expected = getInitializedRaster();
        std::vector<LasPoint> points;
        addPoint(points, expected, 1.1, 1.2, 5);
        addPoint(points, expected, 1.2, 1.1, 10); //same cell, higher point
        addPoint(points, expected, 2.5, 2.5, 3);
        addPoint(points, expected, 5.3, 5.7, 7);
        CsmMaxHeight maker{ a };
        maker.addPointsUnsafe(points);
        Raster<csm_t> result = maker.getRaster();
        compareRasters(expected, result);
    }

    //with a footprint diameter
    {
        SCOPED_TRACE("Footprint diameter test");
        Raster<csm_t> expected = getInitializedRaster();
        coord_t radius = 1.0;
        std::vector<LasPoint> points;
        addPointRadius(points, expected, 1.1, 1.2, 5, radius);
        addPointRadius(points, expected, 1.2, 1.1, 10, radius); //same cell, higher point
        addPointRadius(points, expected, 2.5, 2.5, 3, radius);
        addPointRadius(points, expected, 1.9, 2.5, 4, radius); //within a radius of the previous point
        addPointRadius(points, expected, 0.3, 1.3, 7, radius); //within a diagonal radius of the previous point
        addPointRadius(points, expected, 5.3, 5.7, 7, radius);
        CsmMaxHeight maker{ a, radius * 2 };
        maker.addPoints(points);
        Raster<csm_t> result = maker.getRaster();
        compareRasters(expected, result);
    }

    //footprint diameter + addPointsUnsafe
    {
        SCOPED_TRACE("Footprint diameter + addPointsUnsafe");
        Raster<csm_t> expected = getInitializedRaster();
        coord_t radius = 1.0;
        std::vector<LasPoint> points;
        addPointRadius(points, expected, 1.1, 1.2, 5, radius);
        addPointRadius(points, expected, 1.2, 1.1, 10, radius); //same cell, higher point
        addPointRadius(points, expected, 2.5, 2.5, 3, radius);
        addPointRadius(points, expected, 1.9, 2.5, 4, radius); //within a radius of the previous point
        addPointRadius(points, expected, 0.3, 1.3, 7, radius); //within a diagonal radius of the previous point
        addPointRadius(points, expected, 5.3, 5.7, 7, radius);

        CsmMaxHeight maker{ a, radius * 2 };
        maker.addPointsUnsafe(points);
        Raster<csm_t> result = maker.getRaster();
        compareRasters(expected, result);
    }
}

TEST(CsmTest, CsmSmootherTest) {
    using namespace lapis;

    Raster<coord_t> constantValue{ Alignment{Extent(0,4,0,4),4,4} };
    //top row is noData
    for (cell_t cell : CellIterator(constantValue)) {
        rowcol_t row = constantValue.rowFromCell(cell);
        auto v = constantValue.atCell(cell);
        if (row == 0) {
            v.has_value() = false;
        }
        else {
            v.has_value() = true;
            v.value() = 1;
        }
    }

    std::unique_ptr<CsmMaker> constantValueMaker = std::make_unique<CsmPreMade>(constantValue);
    CsmSmoother smoother{ std::move(constantValueMaker), 3 };
    Raster<csm_t> smoothed = smoother.getRaster();

    //we expect equality of the input and output
    ASSERT_TRUE((Alignment)smoothed == (Alignment)constantValue);
    for (cell_t cell : CellIterator(constantValue)) {
        auto expectedV = constantValue.atCell(cell);
        auto resultV = smoothed.atCell(cell);
        if (!expectedV.has_value()) {
            EXPECT_FALSE(resultV.has_value());
        }
        else {
            EXPECT_TRUE(resultV.has_value());
            EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
        }
    }


    Raster<csm_t> allDifferent{ Alignment{Extent(0,4,0,4),4,4} };
    //still noData on the top row
    for (cell_t cell : CellIterator(allDifferent)) {
        rowcol_t row = allDifferent.rowFromCell(cell);
        auto v = allDifferent.atCell(cell);
        if (row == 0) {
            v.has_value() = false;
        }
        else {
            v.has_value() = true;
            v.value() = (csm_t)cell + 1;
        }
    }

    std::unique_ptr<CsmMaker> allDifferentMaker = std::make_unique<CsmPreMade>(allDifferent);
    CsmSmoother allDifferentSmoother{ std::move(allDifferentMaker), 3 };
    Raster<csm_t> allDifferentSmoothed = allDifferentSmoother.getRaster();

    auto testOneOutput = [&](const Raster<csm_t>& input, const Raster<csm_t>& output, int windowRadius) {
        ASSERT_TRUE((Alignment)output == (Alignment)input);

        for (cell_t cell : CellIterator(input)) {
            rowcol_t row = input.rowFromCell(cell);
            rowcol_t col = input.colFromCell(cell);
            auto expectedV = input.atCell(cell);
            auto resultV = output.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
                continue;
            }


            EXPECT_TRUE(resultV.has_value());
            //don't want to re-implement the logic in the tests so I'm going to instead test that the smoothed value is between the min and max of the 3x3 window around the cell
            coord_t minV = expectedV.value();
            coord_t maxV = expectedV.value();
            for (rowcol_t r = row - windowRadius; r <= row + windowRadius; ++r) {
                for (rowcol_t c = col - windowRadius; c <= col + windowRadius; ++c) {
                    if (r < 0 || r >= allDifferent.nrow() || c < 0 || c >= allDifferent.ncol()) {
                        continue;
                    }
                    auto v = allDifferent.atRC(r, c);
                    if (!v.has_value()) {
                        continue;
                    }
                    if (v.value() < minV) {
                        minV = v.value();
                    }
                    if (v.value() > maxV) {
                        maxV = v.value();
                    }
                }
            }
            EXPECT_GE(resultV.value(), minV) << "In cell (" << row << ", " << col << ")";
            EXPECT_LE(resultV.value(), maxV) << "In cell (" << row << ", " << col << ")";
        }
        };
    testOneOutput(allDifferent, allDifferentSmoothed, 1);

    allDifferentMaker = std::make_unique<CsmPreMade>(allDifferent);
    CsmSmoother allDifferentSmoother5{ std::move(allDifferentMaker), 5 };
    Raster<csm_t> allDifferentSmoothed5 = allDifferentSmoother5.getRaster();
    testOneOutput(allDifferent, allDifferentSmoothed5, 2);

    CsmSmoother allDifferentSmoother1{ std::make_unique<CsmPreMade>(allDifferent), 1 };
    Raster<csm_t> allDifferentSmoothed1 = allDifferentSmoother1.getRaster();
    ASSERT_TRUE((Alignment)allDifferentSmoothed1 == (Alignment)allDifferent);
    for (cell_t cell : CellIterator(allDifferent)) {
        auto expectedV = allDifferent.atCell(cell);
        auto resultV = allDifferentSmoothed1.atCell(cell);
        if (!expectedV.has_value()) {
            EXPECT_FALSE(resultV.has_value());
        }
        else {
            EXPECT_TRUE(resultV.has_value());
            EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
        }
    }
    
}

TEST(CsmTest, CsmFillerTest) {
    using namespace lapis;

    //leaves non-holes alone
    {
        Raster<csm_t> fullyFilled{ Alignment{Extent(0,10,0,10), 10, 10} };
        for (cell_t cell : CellIterator(fullyFilled)) {
            auto v = fullyFilled.atCell(cell);
            v.has_value() = true;
            v.value() = 5.0;
        }

        CsmFiller filler{ std::make_unique<CsmPreMade>(fullyFilled), 4, 3.0 };
        Raster<csm_t> result = filler.getRaster();

        for (cell_t cell : CellIterator(fullyFilled)) {
            EXPECT_TRUE(result.atCell(cell).has_value());
            EXPECT_DOUBLE_EQ(result.atCell(cell).value(), 5.0);
        }
    }

    //constant value in non-holes
    {
        Raster<csm_t> oneHole{ Alignment{Extent(0,6,0,6), 6, 6} };
        for (cell_t cell : CellIterator(oneHole)) {
            auto v = oneHole.atCell(cell);
            v.has_value() = true;
            v.value() = 10.0;
        }
        auto hole = oneHole.atRC(3, 3);
        hole.has_value() = false;

        CsmFiller filler{ std::make_unique<CsmPreMade>(oneHole), 4, 2.0 };
        Raster<csm_t> result = filler.getRaster();

        auto filled = result.atRC(3, 3);
        EXPECT_TRUE(filled.has_value());
        EXPECT_NEAR(filled.value(), 10.0, 0.1);
    }

    //filled values should resemble their surroundings
    {
        Raster<csm_t> gradient{ Alignment{Extent(0,10,0,10), 10, 10} };
        for (cell_t cell : CellIterator(gradient)) {
            rowcol_t col = gradient.colFromCell(cell);
            auto v = gradient.atCell(cell);
            v.has_value() = true;
            v.value() = static_cast<csm_t>(col); // Values increase left to right
        }
        // Create hole at (5,5)
        auto hole = gradient.atRC(5, 5);
        hole.has_value() = false;

        CsmFiller filler{ std::make_unique<CsmPreMade>(gradient), 4, 2.0 };
        Raster<csm_t> result = filler.getRaster();

        auto filled = result.atRC(5, 5);
        EXPECT_TRUE(filled.has_value());
        // Should be close to 5 (the column value)
        EXPECT_NEAR(filled.value(), 5.0, 1.0);
    }

    //lowing neighborsneeded should increase the number of holes filled
    {
        Raster<csm_t> sparseData{ Alignment{Extent(0,10,0,10), 10, 10} };
        for (cell_t cell : CellIterator(sparseData)) {
            rowcol_t row = sparseData.rowFromCell(cell);
            rowcol_t col = sparseData.colFromCell(cell);
            auto v = sparseData.atCell(cell);
            if (row == 0 || row == 9 || col == 0 || col == 9) {
                v.has_value() = true;
                v.value() = 5.0;
            }
            else {
                v.has_value() = false;
            }
        }

        CsmFiller strictFiller{ std::make_unique<CsmPreMade>(sparseData), 6, 1.0 };
        Raster<csm_t> strictResult = strictFiller.getRaster();

        CsmFiller lenientFiller{ std::make_unique<CsmPreMade>(sparseData), 2, 1.0 };
        Raster<csm_t> lenientResult = lenientFiller.getRaster();

        // Count filled cells
        int strictFilled = 0, lenientFilled = 0;
        for (cell_t cell : CellIterator(sparseData)) {
            if (strictResult.atCell(cell).has_value()) strictFilled++;
            if (lenientResult.atCell(cell).has_value()) lenientFilled++;
        }

        EXPECT_GT(lenientFilled, strictFilled) << "Lower neighborsNeeded should fill more cells";
    }

    //increasing lookdist should fill more holes
    {
        Raster<csm_t> sparseData{ Alignment{Extent(0,20,0,20), 20, 20} };
        for (cell_t cell : CellIterator(sparseData)) {
            rowcol_t row = sparseData.rowFromCell(cell);
            rowcol_t col = sparseData.colFromCell(cell);
            auto v = sparseData.atCell(cell);
            if ((row < 2 || row > 17) && (col < 2 || col > 17)) {
                v.has_value() = true;
                v.value() = 5.0;
            }
            else {
                v.has_value() = false;
            }
        }

        CsmFiller shortFiller{ std::make_unique<CsmPreMade>(sparseData), 4, 2.0 };
        Raster<csm_t> shortResult = shortFiller.getRaster();

        CsmFiller longFiller{ std::make_unique<CsmPreMade>(sparseData), 4, 15.0 };
        Raster<csm_t> longResult = longFiller.getRaster();

        int shortFilled = 0, longFilled = 0;
        for (cell_t cell : CellIterator(sparseData)) {
            if (shortResult.atCell(cell).has_value()) shortFilled++;
            if (longResult.atCell(cell).has_value()) longFilled++;
        }

        EXPECT_GT(longFilled, shortFilled) << "Larger lookDist should fill more cells";
    }

    //holes with insufficient neighbors don't get filled
    {
        Raster<csm_t> edgeHole{ Alignment{Extent(0,5,0,5), 5, 5} };
        for (cell_t cell : CellIterator(edgeHole)) {
            auto v = edgeHole.atCell(cell);
            v.has_value() = true;
            v.value() = 5.0;
        }
        auto corner = edgeHole.atRC(0, 0);
        corner.has_value() = false;
        auto nextToCorner = edgeHole.atRC(0, 1);
        nextToCorner.has_value() = false;

        CsmFiller strictFiller{ std::make_unique<CsmPreMade>(edgeHole), 8, 1.0 };
        Raster<csm_t> result = strictFiller.getRaster();

        EXPECT_FALSE(result.atRC(0, 0).has_value());
    }

    //if the nearest neighbor is too far away, it doesn't count
    {
        Raster<csm_t> distantHole{ Alignment{Extent(0,20,0,20), 20, 20} };
        for (cell_t cell : CellIterator(distantHole)) {
            auto v = distantHole.atCell(cell);
            v.has_value() = false;
            rowcol_t row = distantHole.rowFromCell(cell);
            rowcol_t col = distantHole.colFromCell(cell);
            if (row == 19 || col == 19) {
                v.has_value() = true;
                v.value() = 5.0;
            }
        }

        CsmFiller filler{ std::make_unique<CsmPreMade>(distantHole), 1, 5.0 };
        Raster<csm_t> result = filler.getRaster();

        EXPECT_FALSE(result.atRC(0, 0).has_value());
    }
}

TEST(CsmTest, CsmMosaicerTest) {
    using namespace lapis;

    Alignment requestAlign{ 0,0,5,5,1,1, CoordRef("26911") };

    //two aligns, overlapping in the middle column
    Alignment left{ Extent{0,3,0,5,CoordRef("26911") }, 5,3};
    Alignment right{ Extent{2,5,0,5,CoordRef("26911") }, 5,3 };


    //middle column:
    //row 0-1: both have values
    //row 2: neither has value
    //row 3: only left has value
    //row 4: only right has value

    Raster<csm_t> leftRaster{ left };
    for (cell_t cell : CellIterator(leftRaster)) {
        auto v = leftRaster.atCell(cell);
        v.has_value() = true;
        v.value() = 1;

        rowcol_t row = leftRaster.rowFromCell(cell);
        rowcol_t col = leftRaster.colFromCell(cell);

        if (col == 2 && row == 2) {
            v.has_value() = false;
        }
        if (col == 2 && row == 4) {
            v.has_value() = false;
        }
    }
    Raster<csm_t> rightRaster{ right };
    for (cell_t cell : CellIterator(rightRaster)) {
        auto v = rightRaster.atCell(cell);
        v.has_value() = true;
        v.value() = 2;
        rowcol_t row = rightRaster.rowFromCell(cell);
        rowcol_t col = rightRaster.colFromCell(cell);

        if (col == 0 && row == 2) {
            v.has_value() = false;
        }
        if (col == 0 && row == 3) {
            v.has_value() = false;
        }
    }

    CsmMosaicerFactory factory;
    factory.addRaster(leftRaster);
    factory.addRaster(rightRaster);

    auto csm = factory.create(requestAlign,CsmMaker::mergeByMax);
    Raster<csm_t> result = csm->getRaster();

    ASSERT_TRUE((Alignment)result == requestAlign);
    //col 0: all rows should be 1
    //col 1: all rows should be 1
    //col 2: rows 0-1 should be 2, row 2 should be noData, row 3 should be 1, row 4 should be 2
    //col 3: all rows should be 2
    //col 4: all rows should be 2
    for (cell_t cell : CellIterator(result)) {
        rowcol_t row = result.rowFromCell(cell);
        rowcol_t col = result.colFromCell(cell);
        auto v = result.atCell(cell);
        if (col == 0 || col == 1) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 1.0);
        }
        else if (col == 2) {
            if (row == 0 || row == 1) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 2.0);
            }
            else if (row == 2) {
                EXPECT_FALSE(v.has_value());
            }
            else if (row == 3) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 1.0);
            }
            else if (row == 4) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 2.0);
            }
        }
        else if (col == 3 || col == 4) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 2.0);
        }
    }

    csm = factory.create(requestAlign, CsmMaker::mergeByMean);
    result = csm->getRaster();
    
    ASSERT_TRUE((Alignment)result == requestAlign);
    for (cell_t cell : CellIterator(result)) {
        rowcol_t row = result.rowFromCell(cell);
        rowcol_t col = result.colFromCell(cell);
        auto v = result.atCell(cell);
        if (col == 0 || col == 1) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 1.0);
        }
        else if (col == 2) {
            if (row == 0 || row == 1) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 1.5);
            }
            else if (row == 2) {
                EXPECT_FALSE(v.has_value());
            }
            else if (row == 3) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 1.0);
            }
            else if (row == 4) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 2.0);
            }
        }
        else if (col == 3 || col == 4) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 2.0);
        }
    }


    Raster<csm_t> rightReprojected = transformRaster(rightRaster, CoordRef("26910"), ExtractMethod::bilinear);
    //predicting the exact values after going through this is a bit tricky; I'm going to check that the left column is 1, the right column is 2, and the other columns are either noData, 1, or 2.
    factory = CsmMosaicerFactory();
    factory.addRaster(leftRaster);
    factory.addRaster(rightReprojected);
    csm = factory.create(requestAlign, CsmMaker::mergeByMax);
    result = csm->getRaster();

    ASSERT_TRUE((Alignment)result == requestAlign);
    for (cell_t cell : CellIterator(result)) {
        rowcol_t row = result.rowFromCell(cell);
        rowcol_t col = result.colFromCell(cell);
        auto v = result.atCell(cell);
        if (col == 0 || col == 1) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 1.0);
        }
        else if (col == 3 || col == 4) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 2.0);
        }
        else if (col == 2) {
            if (v.has_value()) {
                EXPECT_TRUE(v.has_value());
                EXPECT_TRUE(v.value() == 1.0 || v.value() == 2.0);
            }
        }
    }

    //the alternate create method should completely ignore CRS
    rightRaster.defineCRS(CoordRef("26910"));
    factory = CsmMosaicerFactory();
    factory.addRaster(leftRaster);
    factory.addRaster(rightRaster);
    csm = factory.createAssumeAllRastersAlign(requestAlign, CsmMaker::mergeByMax);
    result = csm->getRaster();

    ASSERT_TRUE((Alignment)result == requestAlign);
    for (cell_t cell : CellIterator(result)) {
        rowcol_t row = result.rowFromCell(cell);
        rowcol_t col = result.colFromCell(cell);
        auto v = result.atCell(cell);
        if (col == 0 || col == 1) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 1.0);
        }
        else if (col == 2) {
            if (row == 0 || row == 1) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 2.0);
            }
            else if (row == 2) {
                EXPECT_FALSE(v.has_value());
            }
            else if (row == 3) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 1.0);
            }
            else if (row == 4) {
                EXPECT_TRUE(v.has_value());
                EXPECT_DOUBLE_EQ(v.value(), 2.0);
            }
        }
        else if (col == 3 || col == 4) {
            EXPECT_TRUE(v.has_value());
            EXPECT_DOUBLE_EQ(v.value(), 2.0);
        }
    }
}

TEST(CsmTest, CsmMakerBuilderTest) {
    //the individual CsmMakers have their own tests. So the strategy here is to make sure that the output from the builder matches what you'd get if you make the same pipeline without the builder
    
    using namespace lapis;
    Alignment align{ 0,0,5,5,1,1 };

    std::vector<LasPoint> points;
    for (cell_t cell : CellIterator(align)) {
        rowcol_t row = align.rowFromCell(cell);
        rowcol_t col = align.colFromCell(cell);
        coord_t z = row + col; //values from 0 to 8
        if (z == 4) {
            continue; //leave random holes
        }
        points.push_back(LasPoint(align.xFromCol(col), align.yFromRow(row), z, 0, 0));
    }


    //basic flow
    {
        std::unique_ptr<CsmMaker> maker = std::make_unique<CsmMaxHeight>(align);
        maker->addPoints(points);
        Raster<csm_t> expected = maker->getRaster();

        CsmMakerBuilder builder = CsmMakerBuilder::maxHeight();
        std::unique_ptr<CsmMaker> builtMaker = builder.build(align);
        builtMaker->addPoints(points);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == align);
        for (cell_t cell : CellIterator(align)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //testing the templated fromAlgo
    {
        std::unique_ptr<CsmMaker> maker = std::make_unique<CsmMaxHeight>(align);
        maker->addPoints(points);
        Raster<csm_t> expected = maker->getRaster();

        CsmMakerBuilder builder = CsmMakerBuilder::fromAlgo<CsmMaxHeight>();
        std::unique_ptr<CsmMaker> builtMaker = builder.build(align);
        builtMaker->addPoints(points);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == align);
        for (cell_t cell : CellIterator(align)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //adding a decorator
    {
        std::unique_ptr<CsmMaker> maker = std::make_unique<CsmMaxHeight>(align);
        maker = std::make_unique<CsmSmoother>(std::move(maker), 3);
        maker->addPoints(points);
        Raster<csm_t> expected = maker->getRaster();

        CsmMakerBuilder builder = CsmMakerBuilder::fromAlgo<CsmMaxHeight>().apply<CsmSmoother>(3);
        std::unique_ptr<CsmMaker> builtMaker = builder.build(align);
        builtMaker->addPoints(points);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == align);
        for (cell_t cell : CellIterator(align)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //two decorators
    {
        std::unique_ptr<CsmMaker> maker = std::make_unique<CsmMaxHeight>(align);
        maker = std::make_unique<CsmSmoother>(std::move(maker), 3);
        maker = std::make_unique<CsmFiller>(std::move(maker), 4, 2.0);
        maker->addPoints(points);
        Raster<csm_t> expected = maker->getRaster();

        CsmMakerBuilder builder = CsmMakerBuilder::fromAlgo<CsmMaxHeight>().apply<CsmSmoother>(3).apply<CsmFiller>(4, 2.0);
        std::unique_ptr<CsmMaker> builtMaker = builder.build(align);
        builtMaker->addPoints(points);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == align);
        for (cell_t cell : CellIterator(align)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //two decorators in the other order, to make sure the builder respects order
    {
        std::unique_ptr<CsmMaker> maker = std::make_unique<CsmMaxHeight>(align);
        maker = std::make_unique<CsmFiller>(std::move(maker), 4, 2.0);
        maker = std::make_unique<CsmSmoother>(std::move(maker), 3);
        maker->addPoints(points);
        Raster<csm_t> expected = maker->getRaster();

        CsmMakerBuilder builder = CsmMakerBuilder::fromAlgo<CsmMaxHeight>().apply<CsmFiller>(4, 2.0).apply<CsmSmoother>(3);
        std::unique_ptr<CsmMaker> builtMaker = builder.build(align);
        builtMaker->addPoints(points);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == align);
        for (cell_t cell : CellIterator(align)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }


    //mosaicer setup
    Alignment mosaicAlign{ 0,0,5,5,1,1, CoordRef("26911") };
    Alignment left{ Extent{0,3,0,5,CoordRef("26911") }, 5,3 };
    Alignment right{ Extent{2,5,0,5,CoordRef("26911") }, 5,3 };

    Raster<csm_t> leftRaster{ left };
    for (cell_t cell : CellIterator(leftRaster)) {
        auto v = leftRaster.atCell(cell);
        v.has_value() = true;
        v.value() = 1;

        rowcol_t row = leftRaster.rowFromCell(cell);
        rowcol_t col = leftRaster.colFromCell(cell);

        if (col == 2 && row == 2) {
            v.has_value() = false;
        }
        if (col == 2 && row == 4) {
            v.has_value() = false;
        }
    }
    Raster<csm_t> rightRaster{ right };
    for (cell_t cell : CellIterator(rightRaster)) {
        auto v = rightRaster.atCell(cell);
        v.has_value() = true;
        v.value() = 2;
        rowcol_t row = rightRaster.rowFromCell(cell);
        rowcol_t col = rightRaster.colFromCell(cell);

        if (col == 0 && row == 2) {
            v.has_value() = false;
        }
        if (col == 0 && row == 3) {
            v.has_value() = false;
        }
    }


    //mosaicer simple test
    {
        CsmMosaicerFactory factory;
        factory.addRaster(leftRaster);
        factory.addRaster(rightRaster);
        auto csm = factory.create(mosaicAlign, CsmMaker::mergeByMax);
        Raster<csm_t> expected = csm->getRaster();

        auto mosaicer = CsmMakerBuilder::fromMosaic(CsmMaker::mergeByMax);
        mosaicer.addRaster(leftRaster).addRaster(rightRaster);
        std::unique_ptr<CsmMaker> builtMaker = mosaicer.build(mosaicAlign);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == mosaicAlign);
        for (cell_t cell : CellIterator(mosaicAlign)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //mosaicer with decorator
    {
        CsmMosaicerFactory factory;
        factory.addRaster(leftRaster);
        factory.addRaster(rightRaster);
        auto csm = factory.create(mosaicAlign, CsmMaker::mergeByMax);
        csm = std::make_unique<CsmSmoother>(std::move(csm), 3);
        Raster<csm_t> expected = csm->getRaster();

        auto mosaicer = CsmMakerBuilder::fromMosaic(CsmMaker::mergeByMax).apply<CsmSmoother>(3);
        mosaicer.addRaster(leftRaster).addRaster(rightRaster);
        std::unique_ptr<CsmMaker> builtMaker = mosaicer.build(mosaicAlign);
        Raster<csm_t> result = builtMaker->getRaster();

        ASSERT_TRUE((Alignment)result == mosaicAlign);
        for (cell_t cell : CellIterator(mosaicAlign)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //mosaicer with two decorators
    {
        CsmMosaicerFactory factory;
        factory.addRaster(leftRaster);
        factory.addRaster(rightRaster);
        auto csm = factory.create(mosaicAlign, CsmMaker::mergeByMax);
        csm = std::make_unique<CsmSmoother>(std::move(csm), 3);
        csm = std::make_unique<CsmFiller>(std::move(csm), 4, 2.0);
        Raster<csm_t> expected = csm->getRaster();
        auto mosaicer = CsmMakerBuilder::fromMosaic(CsmMaker::mergeByMax).apply<CsmSmoother>(3).apply<CsmFiller>(4, 2.0);
        mosaicer.addRaster(leftRaster).addRaster(rightRaster);
        std::unique_ptr<CsmMaker> builtMaker = mosaicer.build(mosaicAlign);
        Raster<csm_t> result = builtMaker->getRaster();
        ASSERT_TRUE((Alignment)result == mosaicAlign);
        for (cell_t cell : CellIterator(mosaicAlign)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }

    //mosaicer with two decorators, other order
    {
        CsmMosaicerFactory factory;
        factory.addRaster(leftRaster);
        factory.addRaster(rightRaster);
        auto csm = factory.create(mosaicAlign, CsmMaker::mergeByMax);
        csm = std::make_unique<CsmFiller>(std::move(csm), 4, 2.0);
        csm = std::make_unique<CsmSmoother>(std::move(csm), 3);
        Raster<csm_t> expected = csm->getRaster();
        auto mosaicer = CsmMakerBuilder::fromMosaic(CsmMaker::mergeByMax).apply<CsmFiller>(4, 2.0).apply<CsmSmoother>(3);
        mosaicer.addRaster(leftRaster).addRaster(rightRaster);
        std::unique_ptr<CsmMaker> builtMaker = mosaicer.build(mosaicAlign);
        Raster<csm_t> result = builtMaker->getRaster();
        ASSERT_TRUE((Alignment)result == mosaicAlign);
        for (cell_t cell : CellIterator(mosaicAlign)) {
            auto expectedV = expected.atCell(cell);
            auto resultV = result.atCell(cell);
            if (!expectedV.has_value()) {
                EXPECT_FALSE(resultV.has_value());
            }
            else {
                EXPECT_TRUE(resultV.has_value());
                EXPECT_DOUBLE_EQ(expectedV.value(), resultV.value());
            }
        }
    }
}