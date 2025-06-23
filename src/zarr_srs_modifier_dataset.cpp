#include "zarr_srs_modifier/zarr_srs_modifier_dataset.h"
#include "cpl_error.h"

ZarrSRSModifierDataset::ZarrSRSModifierDataset() : m_poOriginalDataset(nullptr) {
}

ZarrSRSModifierDataset::~ZarrSRSModifierDataset() {
    // Cleanup handled by smart pointer
}

GDALDataset* ZarrSRSModifierDataset::Open(GDALOpenInfo* poOpenInfo) {
    // First, check if this is a Zarr dataset
    if (!Identify(poOpenInfo)) {
        return nullptr;
    }

    // Get the original Zarr driver
    GDALDriver* poZarrDriver = static_cast<GDALDriver*>(GDALGetDriverByName("Zarr"));
    if (!poZarrDriver) {
        CPLError(CE_Failure, CPLE_AppDefined, 
                "Original Zarr driver not available");
        return nullptr;
    }

    // Open with the original Zarr driver using composition
    auto poOriginal = std::unique_ptr<GDALDataset>(
        poZarrDriver->pfnOpen(poOpenInfo));
    if (!poOriginal) {
        return nullptr;
    }

    // Create our wrapper dataset
    auto poDS = std::make_unique<ZarrSRSModifierDataset>();
    poDS->m_poOriginalDataset = std::move(poOriginal);

    // Copy basic properties from composed dataset
    poDS->nRasterXSize = poDS->m_poOriginalDataset->GetRasterXSize();
    poDS->nRasterYSize = poDS->m_poOriginalDataset->GetRasterYSize();
    poDS->SetDescription(poDS->m_poOriginalDataset->GetDescription());

    return poDS.release();
}

int ZarrSRSModifierDataset::Identify(GDALOpenInfo* poOpenInfo) {
    // Delegate identification to original Zarr driver
    GDALDriver* poZarrDriver = static_cast<GDALDriver*>(GDALGetDriverByName("Zarr"));
    if (!poZarrDriver || !poZarrDriver->pfnIdentify) {
        return FALSE;
    }
    
    return poZarrDriver->pfnIdentify(poOpenInfo);
}

int ZarrSRSModifierDataset::GetRasterXSize() {
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterXSize() : 0;
}

int ZarrSRSModifierDataset::GetRasterYSize() {
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterYSize() : 0;
}

int ZarrSRSModifierDataset::GetRasterCount() {
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterCount() : 0;
}

GDALRasterBand* ZarrSRSModifierDataset::GetRasterBand(int nBand) {
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterBand(nBand) : nullptr;
}

const OGRSpatialReference* ZarrSRSModifierDataset::GetSpatialRef() const {
    // For now, just delegate to original dataset
    return m_poOriginalDataset ? m_poOriginalDataset->GetSpatialRef() : nullptr;
}

CPLErr ZarrSRSModifierDataset::GetGeoTransform(double* padfTransform) {
    return m_poOriginalDataset ? 
           m_poOriginalDataset->GetGeoTransform(padfTransform) : CE_Failure;
}
