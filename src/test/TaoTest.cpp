#include"test_pch.hpp"
#include"..\AllTaoAlgos.hpp"

TEST(TaoTest, TaoIdHighPointTest) {
    using namespace lapis;

    Raster<csm_t> csm{ Alignment{Extent{0,5,0,5},5,5} };

    for (cell_t cell : CellIterator(csm)) {
        auto v = csm.atCell(cell);
        v.has_value() = true;
        v.value() = 1;
    }

    csm.atRC(0, 0) = 10;
    csm.atRC(2, 0) = 10;
    csm.atRC(4, 1) = 10;

    SequentialTaoIdGenerator idGen;

    //basic test
    {
        SCOPED_TRACE("Basic test");
        TaoIdHighPoints algo{ 5,0 };
        auto result = algo.process(csm, idGen);

        std::unordered_set<cell_t> expected{ csm.cellFromRowCol(0,0), csm.cellFromRowCol(2, 0), csm.cellFromRowCol(4,1) };
        EXPECT_EQ(expected.size(), result.size());
        for (const auto& tao : result) {
            EXPECT_TRUE(expected.contains(tao.location));
            expected.erase(tao.location);
        }
    }

    //test with min dist
    {
        SCOPED_TRACE("Test with min dist");
        TaoIdHighPoints algo{ 5,2.5 };
        auto result = algo.process(csm, idGen);
        std::unordered_set<cell_t> expected{ csm.cellFromRowCol(0,0), csm.cellFromRowCol(4,1) };
        EXPECT_EQ(expected.size(), result.size());
        for (const auto& tao : result) {
            EXPECT_TRUE(expected.contains(tao.location));
            expected.erase(tao.location);
        }
    }
}

static void buildFakeTree(lapis::Raster<lapis::csm_t>& r, lapis::coord_t xCenter, lapis::coord_t yCenter, lapis::csm_t height) {
    //the specified point should have height of height, and there should be a vaguely circlish area around it with decreasing height as you get further from the center
    for (lapis::cell_t cell : lapis::CellIterator(r)) {
        lapis::coord_t x = r.xFromCell(cell);
        lapis::coord_t y = r.yFromCell(cell);
        lapis::coord_t dist = std::sqrt((x - xCenter) * (x - xCenter) + (y - yCenter) * (y - yCenter));
        if (dist > 3) {
            continue;
        }
        lapis::csm_t v = std::max(0., height - dist);
        r.atCell(cell) = v;
    }
};
static void buildRandomRaster(lapis::Raster<lapis::csm_t>& r, int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<lapis::coord_t> dist(0, 10);
    for (lapis::cell_t cell : lapis::CellIterator(r)) {
        r.atCell(cell) = dist(rng);
    }
    };

TEST(TaoTest, WatershedTest) {
    using namespace lapis;
    //we are checking for the following invariants:
    //1. the raster always exists
    //2. the unique values of the raster are exactly the same as the ids in the input tao list, excluding those whose cell is nodata, below minht, or outside the unbuffered extent
    //4. the cell in the taolist is the maximum (or tied) within its segment
    //5. the polygons always exist
    //6. the polygons contain exactly those cells whose values match the ID field of the polygon
    //7. the ID field of the polygons matches the expected IDs that pass the check in 2

    Alignment buffered = Alignment{ Extent{0,20,0,20},20,20 };
    Extent unbuffered = Extent{ 5,15,5,15 };

    constexpr csm_t minHt = 2;
    constexpr csm_t maxHt = 100;

    auto oneTest = [&](const Raster<csm_t>& r) {
        TaoIdHighPoints idAlgo{ minHt,0 };
        SequentialTaoIdGenerator idGen;
        auto taos = idAlgo.process(r, idGen);
        TaoSegWatershed watershedAlgo{ minHt, maxHt };
        auto result = watershedAlgo.process(r, unbuffered, taos);

        //1
        EXPECT_TRUE(result.segmentRaster.has_value());

        //2
        struct IDHasher {
            std::size_t operator()(const IDedTao& tao) const {
                return std::hash<taoid_t>()(tao.id);
            }
        };
        struct IDEqual {
            bool operator()(const IDedTao& a, const IDedTao& b) const {
                return a.id == b.id;
            }
        };
        std::unordered_set<IDedTao, IDHasher, IDEqual> expectedIds;
        for (const auto& tao : taos) {
            auto csmV = r.atCell(tao.location);
            coord_t x = r.xFromCell(tao.location);
            coord_t y = r.yFromCell(tao.location);
            if (!csmV.has_value() || csmV.value() < minHt || !unbuffered.contains(x, y)) {
                continue;
            }
            expectedIds.insert(tao);
        }
        std::unordered_set<taoid_t> actualIds;
        for (cell_t cell : CellIterator(result.segmentRaster.value())) {
            auto v = result.segmentRaster.value().atCell(cell);
            if (!v.has_value()) {
                continue;
            }
            actualIds.insert((taoid_t)v.value());
        };
        EXPECT_EQ(expectedIds.size(), actualIds.size());
        for (const auto& tao : expectedIds) {
            EXPECT_TRUE(actualIds.contains(tao.id));
        }

        //4
        for (const auto& tao : taos) {
            auto csmV = r.atCell(tao.location);
            coord_t x = r.xFromCell(tao.location);
            coord_t y = r.yFromCell(tao.location);
            if (!csmV.has_value() || csmV.value() < minHt || !unbuffered.contains(x, y)) {
                continue;
            }
            auto segV = result.segmentRaster.value().atCell(tao.location);
            EXPECT_TRUE(segV.has_value());
            EXPECT_EQ(segV.value(), tao.id);

            csm_t maxCsmWithThatId = csmV.value();
            for (cell_t cell : CellIterator(r)) {
                auto segV = result.segmentRaster.value().atCell(cell);
                if (!segV.has_value() || segV.value() != tao.id) {
                    continue;
                }
                auto csmV2 = r.atCell(cell);
                EXPECT_TRUE(csmV2.has_value());
                if (csmV2.value() > maxCsmWithThatId) {
                    FAIL() << "Found a cell with the same ID but higher CSM value than the tao cell";
                }
            }
        }

        //5
        EXPECT_TRUE(result.segmentPolygons.has_value());

        //6
        auto& polys = result.segmentPolygons.value();
        for (cell_t cell : CellIterator(result.segmentRaster.value())) {
            auto segV = result.segmentRaster.value().atCell(cell);
            if (!segV.has_value()) {
                continue;
            }

            for (const auto& feature : polys) {
                if (feature.getGeometry().containsPoint(r.xFromCell(cell), r.yFromCell(cell))) {
                    EXPECT_EQ(feature.getNumericField<taoid_t>("ID"), segV.value());
                }
                else {
                    EXPECT_NE(feature.getNumericField<taoid_t>("ID"), segV.value());
                }
            }
        }

        //7
        EXPECT_EQ(polys.nFeature(), expectedIds.size());
        std::unordered_map<taoid_t, bool> foundId;
        for (const auto& tao : expectedIds) {
            foundId[tao.id] = false;
        }
        for (const auto& feature : polys) {
            taoid_t id = feature.getNumericField<taoid_t>("ID");
            for (const auto& tao : expectedIds) {
                if (tao.id == id) {
                    EXPECT_FALSE(foundId[tao.id]);
                    foundId[tao.id] = true;
                }
            }
        }
        };

    {
        SCOPED_TRACE("Test with fake trees");
        Raster<csm_t> r{ buffered };
        buildFakeTree(r, 10, 10, 10);
        buildFakeTree(r, 7, 7, 5);
        buildFakeTree(r, 13, 7, 5);
        buildFakeTree(r, 2, 2, 20); //outside buffered extent
        buildFakeTree(r, 13, 13, 1); //below minHt
        oneTest(r);
    }

    {
        SCOPED_TRACE("Test with random raster");
        Raster<csm_t> r{ buffered };
        buildRandomRaster(r, 0);
        oneTest(r);
    }
}

TEST(TaoTest, McGaugheyTest) {
    //this is extremely difficult to test meaningfully, because the only truth is the output from a program which has hardcoded parameters
    // (which are defaults but customizable in this version)
    //As usual in difficult cases, we are just testing some invariants:
    //1. the vector always exists, and the raster never exists
    //2. each tao in the input has a corresponding polygon in the output, excluding taos outside the unbuffered extent.
    // It will be contained in that polygon, and the ID field will match that tao's id
    // it is, unfortunately, possible for two polygons to contain the same tao high point
    // but only one will match the id
    //3. each multipolygon has exactly one polygon, with no inner rings, and an outer ring with exactly nvertices vertices
    //this unfortunately leaves a lot untested

    using namespace lapis;

    constexpr csm_t minHt = 2;
    Alignment buffered = Alignment{ Extent{0,20,0,20},20,20 };
    Extent unbuffered = Extent{ 5,15,5,15 };

    auto oneTest = [&](const Raster<csm_t>& r,
        int nVertices,
        csm_t slopeChangeMultiplier,
        csm_t heightCutoffMultiplier,
        coord_t maxDistMultiplier,
        McGaugheySmoothType smoothType) {

        TaoIdHighPoints idAlgo{ minHt,0 };
        SequentialTaoIdGenerator idGen;
        auto taos = idAlgo.process(r, idGen);

        TaoSegMcGaughey algo{ nVertices, slopeChangeMultiplier, heightCutoffMultiplier, maxDistMultiplier, smoothType };
        auto result = algo.process(r, unbuffered, taos);

        //1
        EXPECT_FALSE(result.segmentRaster.has_value());
        EXPECT_TRUE(result.segmentPolygons.has_value());

        //2
        size_t matched = 0;
        std::unordered_map<taoid_t, bool> foundId;
        for (const auto& tao : taos) {
            auto csmV = r.atCell(tao.location);
            coord_t x = r.xFromCell(tao.location);
            coord_t y = r.yFromCell(tao.location);
            if (!csmV.has_value() || csmV.value() < minHt || !unbuffered.contains(x, y)) {
                continue;
            }
            for (const auto& feature : result.segmentPolygons.value()) {
                if (feature.getGeometry().containsPoint(x, y)) {
                    if (feature.getNumericField<taoid_t>("ID") == tao.id) {
                        EXPECT_FALSE(foundId[tao.id]);
                        matched++;
                        foundId[tao.id] = true;
                    }
                }
            }
        }
        EXPECT_EQ(matched, result.segmentPolygons.value().nFeature());

        //3
        for (const auto& feature : result.segmentPolygons.value()) {
            const auto& geom = feature.getGeometry();
            ASSERT_EQ(geom.nPolygon(), 1);
            const auto& poly = geom.begin();
            EXPECT_EQ(poly->nInnerRings(), 0);
            EXPECT_EQ(poly->getOuterRing().size(), nVertices + 1); //+1 because the first vertex is repeated
        }
        };

    {
        SCOPED_TRACE("Test with fake trees, default params");
        Raster<csm_t> r{ buffered };
        buildFakeTree(r, 10, 10, 10);
        buildFakeTree(r, 7, 7, 5);
        buildFakeTree(r, 13, 7, 5);
        buildFakeTree(r, 2, 2, 20); //outside buffered extent
        buildFakeTree(r, 13, 13, 1); //below minHt
        oneTest(r, 16, 4, 2./3, 3./4, McGaugheySmoothType::fusion);
    }

    for (int i = 0; i <= 10; ++i) {
        SCOPED_TRACE("Random test " + std::to_string(i));
        Raster<csm_t> r{ buffered };
        buildRandomRaster(r, i);
        std::mt19937 rng(i);
        //nVertices between 3 and 20
        int nVertices = std::uniform_int_distribution<int>(3, 20)(rng);
        //slopeChangeMultiplier between 2 and 5
        double slopeChangeMultiplier = std::uniform_real_distribution<csm_t>(2, 5)(rng);
        //heightCutoffMultiplier between 0.5 and 0.9
        double heightCutoffMultiplier = std::uniform_real_distribution<double>(0.5, 0.9)(rng);
        //maxDistMultiplier between 0.5 and 1.5
        double maxDistMultiplier = std::uniform_real_distribution<double>(0.5, 1.5)(rng);
        //smoothType randomly chosen
        McGaugheySmoothType smoothType = static_cast<McGaugheySmoothType>(std::uniform_int_distribution<int>(0, 2)(rng));

        oneTest(r, nVertices, slopeChangeMultiplier, heightCutoffMultiplier, maxDistMultiplier, smoothType);
    }
}