#pragma once

#include "gdal_priv.h"
#include "gdal_pam.h"
#include "cpl_conv.h"
#include <memory>

class ZarrSRSModifierDataset : public GDALPamDataset {
private:
    // Composition: delegate to original Zarr dataset
    std::unique_ptr<GDALDataset> m_poOriginalDataset;

    std::unique_ptr<OGRSpatialReference> m_poCustomCRS; // Spatial reference
    std::string m_osOriginalCRSMetadata; // Original CRS metadata

    // Configuration options
    std::string m_osTransformType; // Type of transformation to apply
    int m_nTargetEPSG;

public:
    ZarrSRSModifierDataset();
    ~ZarrSRSModifierDataset() override;

    // Static methods for driver registration
    static GDALDataset* Open(GDALOpenInfo* poOpenInfo);
    static int Identify(GDALOpenInfo* poOpenInfo);
     CPLErr CreateCRSFromEPSG(const char* pszEPSGCode);

    // Basic dataset interface - delegate to original
    int GetRasterXSize() ;
    int GetRasterYSize();
    int GetRasterCount();
    GDALRasterBand* GetRasterBand(int nBand);
    
    // Enhanced spatial reference methods (these CAN be overridden)
    const OGRSpatialReference* GetSpatialRef() const override;
    CPLErr GetGeoTransform(double* padfTransform) override;
    CPLErr SetSpatialRef(const OGRSpatialReference* poSRS) override;

    private:
    // SRS parsing and handling methods
    CPLErr ParseCRSMetadata();
    CPLErr ExtractSRSFromJSON(const CPLJSONObject& oCRS);
    CPLErr ValidateSRSFormats(const CPLJSONObject& oCRS);
    OGRErr importFromEPSG(int nCode);

    void InitializeConfiguration();
};
   