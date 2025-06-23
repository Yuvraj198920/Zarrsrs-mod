#pragma once

#include "gdal_priv.h"
#include "gdal_pam.h"
#include "cpl_conv.h"
#include <memory>

class ZarrSRSModifierDataset : public GDALPamDataset {
private:
    // Composition: delegate to original Zarr dataset
    std::unique_ptr<GDALDataset> m_poOriginalDataset;

public:
    ZarrSRSModifierDataset();
    ~ZarrSRSModifierDataset() override;

    // Static methods for driver registration
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);

    // Basic dataset interface - delegate to original
    int GetRasterXSize();
    int GetRasterYSize();
    int GetRasterCount();
    GDALRasterBand* GetRasterBand(int nBand);
    
    // Spatial reference methods (to be enhanced later)
    const OGRSpatialReference* GetSpatialRef() const override;
    CPLErr GetGeoTransform(double* padfTransform) override;
};
