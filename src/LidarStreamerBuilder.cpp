#include"LidarStreamerBuilder.hpp"
#include"LidarStreamerHardDrive.hpp"
#include"LidarStreamerComposite.hpp"
#include"LidarStreamerMemory.hpp"
#include"LidarStreamerEmpty.hpp"

namespace lapis {
    LidarStreamerBuilder::LasSpecifier::LasSpecifier(const LidarStreamerBuilder& parent)
        : _parent(&parent)
    {}
    LidarStreamerBuilder::LasSpecifier::LasConfig LidarStreamerBuilder::LasSpecifier::addLas(const std::string& filename)
    {
        return addLas(std::filesystem::path(filename));
    }
    LidarStreamerBuilder::LasSpecifier::LasConfig LidarStreamerBuilder::LasSpecifier::addLas(const std::filesystem::path& filepath)
    {
        LasOverride lo;
        lo.filename = filepath;
        if (_parent->_lasCrsOverride) {
            lo.overrideCrs = _parent->_lasCrsOverride;
        }
        if (_parent->_lasZUnitsOverride) {
            lo.overrideZUnits = _parent->_lasZUnitsOverride;
        }
        _lasFiles.push_back(lo);
        return LasConfig(*this, _lasFiles.size() - 1);
    }
    LidarStreamerBuilder::LasSpecifier LidarStreamerBuilder::createLasSpecifier()
    {
        return LasSpecifier(*this);
    }
    LidarStreamerBuilder& LidarStreamerBuilder::setLasCrsOverride(const CoordRef& crs)
    {
        _lasCrsOverride = crs;
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::clearLasCrsOverride()
    {
        _lasCrsOverride.reset();
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::setLasZUnitsOverride(const LinearUnit& zUnits)
    {
        _lasZUnitsOverride = zUnits;
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::clearLasZUnitsOverride()
    {
        _lasZUnitsOverride.reset();
        return *this;
    }
    LidarStreamerBuilder::RasterConfig LidarStreamerBuilder::addDem(const std::string& filename)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        return _normalizeFactory->addRaster(filename);
    }
    LidarStreamerBuilder::RasterConfig LidarStreamerBuilder::addDem(const std::filesystem::path& filepath)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        return _normalizeFactory->addRaster(filepath);
    }
    LidarStreamerBuilder::RasterConfig LidarStreamerBuilder::addDem(const std::string& filename, const Alignment& a)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        return _normalizeFactory->addRaster(filename, a);
    }
    LidarStreamerBuilder::RasterConfig LidarStreamerBuilder::addDem(const std::filesystem::path& filepath, const Alignment& a)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        return _normalizeFactory->addRaster(filepath, a);
    }
    LidarStreamerBuilder::RasterConfig LidarStreamerBuilder::addDem(const Raster<coord_t>& raster)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        return _normalizeFactory->addRaster(raster);
    }
    LidarStreamerBuilder::RasterConfig LidarStreamerBuilder::addDem(Raster<coord_t>&& raster)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        return _normalizeFactory->addRaster(std::move(raster));
    }
    LidarStreamerBuilder& LidarStreamerBuilder::setDemCrsOverride(const CoordRef& crs)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        _normalizeFactory->setCrsOverride(crs);
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::clearDemCrsOverride()
    {
        if (_normalizeFactory) {
            _normalizeFactory->clearCrsOverride();
        }
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::setDemZUnitsOverride(const LinearUnit& zUnits)
    {
        if (!_normalizeFactory) {
            _normalizeFactory = NormalizeByRasterFactory();
        }
        _normalizeFactory->setZUnitsOverride(zUnits);
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::clearDemZUnitsOverride()
    {
        if (_normalizeFactory) {
            _normalizeFactory->clearZUnitsOverride();
        }
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::setExtent(const Extent& extent)
    {
        _extent = extent;
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::addFilter(std::shared_ptr<LasFilter> filter)
    {
        _filters.push_back(filter);
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::addDefaultFilters()
    {
        std::shared_ptr<LasFilter> blacklist = std::make_shared<LasFilterClassBlacklist>(std::unordered_set<std::uint8_t>{ 7, 9, 18 });
        std::shared_ptr<LasFilter> withheld = std::make_shared<LasFilterWithheld>();
        addFilter(blacklist);
        addFilter(withheld);
        return *this;
    }
    LidarStreamerBuilder& LidarStreamerBuilder::normalizeByDem()
    {
        auto oldChain = std::move(_decoratorChain);
        _decoratorChain = [this, oldChain = std::move(oldChain)](std::unique_ptr<LidarStreamer> base) mutable {
            auto decorated = oldChain ? oldChain(std::move(base)) : std::move(base);
            if (!_normalizeFactory) {
                throw std::runtime_error("No DEMs have been added to the builder, so normalizeByDem cannot be applied");
            }
            return _normalizeFactory->create(std::move(decorated));
            };
        return *this;
    }
    std::unique_ptr<LidarStreamer> LidarStreamerBuilder::LasSpecifier::buildInMemory() const
    {
        auto streamer = _buildBaseStreamer();
        std::vector<LasPoint> points;
        const std::span<const LasPoint> pointSpan = streamer->getPoints(streamer->nPoints());
        points.insert(points.end(), pointSpan.begin(), pointSpan.end());
        std::unique_ptr<LidarStreamer> inMemoryStreamer{ new LidarStreamerMemory(std::move(points), streamer->getCoordRef(), streamer->getExtent()) };
        inMemoryStreamer = _parent->_decoratorChain ? _parent->_decoratorChain(std::move(inMemoryStreamer)) : std::move(inMemoryStreamer);
        return inMemoryStreamer;
    }
    std::unique_ptr<LidarStreamer> LidarStreamerBuilder::LasSpecifier::buildHardDrive() const
    {
        auto streamer = _buildBaseStreamer();
        streamer = _parent->_decoratorChain ? _parent->_decoratorChain(std::move(streamer)) : std::move(streamer);
        return streamer;
    }
    void LidarStreamerBuilder::reset()
    {
        *this = LidarStreamerBuilder();
    }
    std::unique_ptr<LidarStreamer> LidarStreamerBuilder::LasSpecifier::_buildBaseStreamer() const
    {
        std::vector<std::unique_ptr<LidarStreamer>> baseStreamers;
        for (const LasOverride& las : _lasFiles) {
            Extent projE;
            if (_parent->_extent) {
                Extent lasExtent(las.filename.string());
                if (las.overrideCrs) {
                    lasExtent.defineCRS(*las.overrideCrs);

                }
                projE = QuadExtent(_parent->_extent.value(), lasExtent.crs()).outerExtent();
                if (!projE.overlaps(lasExtent)) {
                    continue;
                }
            }

            LasReader reader(las.filename.string());
            if (las.overrideCrs) {
                reader.defineCRS(*las.overrideCrs);
            }
            if (las.overrideZUnits) {
                reader.setZUnits(*las.overrideZUnits);
            }
            for (const auto& filter : _parent->_filters) {
                reader.addFilter(filter);
            }
            if (_parent->_extent) {
                reader.addFilter(std::make_shared<LasFilterExtent>(projE));
            }
            baseStreamers.emplace_back(new LidarStreamerHardDrive(std::move(reader)));
        }

        std::unique_ptr<LidarStreamer> streamer;
        if (baseStreamers.size() == 0) {
            streamer = std::make_unique<LidarStreamerEmpty>(CoordRef(), _parent->_extent.value_or(Extent()));
        }
        else if (baseStreamers.size() == 1) {
            streamer = std::move(baseStreamers[0]);
        }
        else {
            std::unique_ptr<LidarStreamerComposite> composite{ new LidarStreamerComposite() };
            for (auto& base : baseStreamers) {
                composite->addStream(std::move(base));
            }
            streamer = std::move(composite);
        }
        return streamer;
    }
    LidarStreamerBuilder::LasSpecifier::LasConfig::LasConfig(LasSpecifier& parent, size_t index)
        : _parent(parent), _index(index)
    {}
    LidarStreamerBuilder::LasSpecifier::LasConfig& LidarStreamerBuilder::LasSpecifier::LasConfig::setCrs(const CoordRef & crs)
    {
        _parent._lasFiles[_index].overrideCrs = crs;
        return *this;
    }
    LidarStreamerBuilder::LasSpecifier::LasConfig& LidarStreamerBuilder::LasSpecifier::LasConfig::setZUnits(const LinearUnit& zUnits)
    {
        _parent._lasFiles[_index].overrideZUnits = zUnits;
        return *this;
    }
}