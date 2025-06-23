#include "gdal_priv.h"
#include "zarr_srs_modifier/zarr_srs_modifier_dataset.h"

void GDALRegister_ZarrSRSModifier() {
    if (!GDAL_CHECK_VERSION("ZarrSRSModifier"))
        return;
    
    if (GDALGetDriverByName("ZarrSRSModifier") != nullptr)
        return;
    
    GDALDriver* poDriver = new GDALDriver();
    poDriver->SetDescription("ZarrSRSModifier");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, 
                             "Zarr with SRS Modification Support");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSIONS, "zarr");
    
    // Use composition for driver functionality
    poDriver->pfnOpen = ZarrSRSModifierDataset::Open;
    poDriver->pfnIdentify = ZarrSRSModifierDataset::Identify;
    
    GetGDALDriverManager()->RegisterDriver(poDriver);
}

// For deferred loading support (GDAL 3.9+)
extern "C" void GDALRegisterMe() {
    GDALRegister_ZarrSRSModifier();
}
