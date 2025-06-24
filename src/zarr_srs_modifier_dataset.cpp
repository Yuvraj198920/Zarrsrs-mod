#include "zarr_srs_modifier/zarr_srs_modifier_dataset.h"
#include "cpl_error.h"
#include "cpl_string.h"

ZarrSRSModifierDataset::ZarrSRSModifierDataset() : m_poOriginalDataset(nullptr),
                                                   m_poCustomCRS(nullptr),
                                                   m_nTargetEPSG(0)
{
}

ZarrSRSModifierDataset::~ZarrSRSModifierDataset()
{
    // Cleanup handled by smart pointers
}

GDALDataset *ZarrSRSModifierDataset::Open(GDALOpenInfo *poOpenInfo)
{
    // First, check if this is a Zarr dataset
    if (!Identify(poOpenInfo))
    {
        return nullptr;
    }

    // Get the original Zarr driver
    GDALDriver *poZarrDriver = static_cast<GDALDriver *>(GDALGetDriverByName("Zarr"));
    if (!poZarrDriver)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Original Zarr driver not available");
        return nullptr;
    }

    // Open with the original Zarr driver using composition
    auto poOriginal = std::unique_ptr<GDALDataset>(
        poZarrDriver->pfnOpen(poOpenInfo));
    if (!poOriginal)
    {
        return nullptr;
    }

    // Create our wrapper dataset
    auto poDS = std::make_unique<ZarrSRSModifierDataset>();
    poDS->m_poOriginalDataset = std::move(poOriginal);

    // Copy basic properties from composed dataset
    poDS->nRasterXSize = poDS->m_poOriginalDataset->GetRasterXSize();
    poDS->nRasterYSize = poDS->m_poOriginalDataset->GetRasterYSize();
    poDS->SetDescription(poDS->m_poOriginalDataset->GetDescription());

    // Initialize configuration and parse CRS metadata
    poDS->InitializeConfiguration();
    CPLErr eErr = poDS->ParseCRSMetadata();
    if (eErr != CE_None)
    {
        CPLError(CE_Warning, CPLE_AppDefined,
                 "Failed to parse CRS metadata, using original SRS");
    }

    return poDS.release();
}

CPLErr ZarrSRSModifierDataset::CreateCRSFromEPSG(const char *pszEPSGCode)
{
    // Extract numeric EPSG code from string like "EPSG:32632"
    int nEPSGCode = atoi(pszEPSGCode + 5); // Skip "EPSG:" prefix

    // Create SRS using standard GDAL method
    m_poCustomCRS = std::make_unique<OGRSpatialReference>();
    OGRErr eSRSErr = m_poCustomCRS->importFromEPSG(nEPSGCode);

    if (eSRSErr == OGRERR_NONE)
    {
        return CE_None;
    }

    return CE_Failure;
}

CPLErr ZarrSRSModifierDataset::ParseCRSMetadata()
{
    if (!m_poOriginalDataset)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Original dataset not available for CRS parsing");
        return CE_Failure;
    }

    // Attempt to get CRS metadata
    const char *pszCRSMetadata = m_poOriginalDataset->GetMetadataItem("_CRS");
    if (!pszCRSMetadata)
    {
        const char *pszHorizontalCRS = m_poOriginalDataset->GetMetadataItem("horizontal_CRS_code");
        if (pszHorizontalCRS)
        {
            return CreateCRSFromEPSG(pszHorizontalCRS);
        }
        CPLError(CE_Warning, CPLE_AppDefined,
                 "No _CRS metadata found in Zarr dataset - using original SRS");
        return CE_Warning;
    }

    // Validate JSON format
    if (strlen(pszCRSMetadata) == 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Empty _CRS metadata found");
        return CE_Failure;
    }

    // Parse and validate JSON structure
    CPLJSONDocument oDoc;
    if (!oDoc.LoadMemory(pszCRSMetadata))
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Failed to parse _CRS metadata as valid JSON: %s",
                 CPLGetLastErrorMsg());
        return CE_Failure;
    }

    CPLJSONObject oRoot = oDoc.GetRoot();
    return ExtractSRSFromJSON(oRoot);
}

CPLErr ZarrSRSModifierDataset::ExtractSRSFromJSON(const CPLJSONObject &oCRS)
{
    // Validate the CRS object structure
    CPLErr eErr = ValidateSRSFormats(oCRS);
    if (eErr != CE_None)
    {
        return eErr;
    }

    // Create a new SRS object
    m_poCustomCRS = std::make_unique<OGRSpatialReference>();

    // Try to import SRS following GDAL's precedence: url -> wkt -> projjson
    if (oCRS.GetObj("url").IsValid())
    {
        std::string osURL = oCRS.GetString("url");
        if (!osURL.empty())
        {
            OGRErr eSRSErr = m_poCustomCRS->importFromUrl(osURL.c_str());
            if (eSRSErr == OGRERR_NONE)
            {
                CPLDebug("ZarrSRSModifier", "Successfully imported SRS from URL: %s",
                         osURL.c_str());
                return CE_None;
            }
        }
    }

    if (oCRS.GetObj("wkt").IsValid())
    {
        std::string osWKT = oCRS.GetString("wkt");
        if (!osWKT.empty())
        {
            OGRErr eSRSErr = m_poCustomCRS->importFromWkt(osWKT.c_str());
            if (eSRSErr == OGRERR_NONE)
            {
                CPLDebug("ZarrSRSModifier", "Successfully imported SRS from WKT");
                return CE_None;
            }
        }
    }

    if (oCRS.GetObj("projjson").IsValid())
    {
        std::string osPROJJSON = oCRS.GetObj("projjson").ToString();
        if (!osPROJJSON.empty())
        {
            OGRErr eSRSErr = m_poCustomCRS->importFromProj4(osPROJJSON.c_str());
            if (eSRSErr == OGRERR_NONE)
            {
                CPLDebug("ZarrSRSModifier", "Successfully imported SRS from PROJJSON");
                return CE_None;
            }
        }
    }

    CPLError(CE_Failure, CPLE_AppDefined,
             "Failed to import SRS from any available format in _CRS metadata");
    m_poCustomCRS.reset();
    return CE_Failure;
}

CPLErr ZarrSRSModifierDataset::ValidateSRSFormats(const CPLJSONObject &oCRS)
{
    // Check if at least one of the required formats is present
    bool bHasValidFormat = false;

    if (oCRS.GetObj("url").IsValid() && !oCRS.GetString("url").empty())
    {
        bHasValidFormat = true;
        CPLDebug("ZarrSRSModifier", "Found URL format in _CRS");
    }

    if (oCRS.GetObj("wkt").IsValid() && !oCRS.GetString("wkt").empty())
    {
        bHasValidFormat = true;
        CPLDebug("ZarrSRSModifier", "Found WKT format in _CRS");
    }

    if (oCRS.GetObj("projjson").IsValid())
    {
        bHasValidFormat = true;
        CPLDebug("ZarrSRSModifier", "Found PROJJSON format in _CRS");
    }

    if (!bHasValidFormat)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "_CRS metadata does not contain any valid format (url, wkt, projjson)");
        return CE_Failure;
    }

    return CE_None;
}

// Configuration options for SRS transformation
#define ZARR_SRS_TRANSFORM_TYPE "ZARR_SRS_TRANSFORM_TYPE"
#define ZARR_SRS_TARGET_EPSG "ZARR_SRS_TARGET_EPSG"
#define ZARR_SRS_CUSTOM_WKT "ZARR_SRS_CUSTOM_WKT"
#define ZARR_SRS_DEBUG "ZARR_SRS_DEBUG"

void ZarrSRSModifierDataset::InitializeConfiguration()
{
    // Read transformation strategy
    m_osTransformType = CPLGetConfigOption(ZARR_SRS_TRANSFORM_TYPE, "NONE");

    // Read target EPSG code
    const char *pszTargetEPSG = CPLGetConfigOption(ZARR_SRS_TARGET_EPSG, "0");
    m_nTargetEPSG = atoi(pszTargetEPSG);

    // Enable debug output if requested
    if (CPLTestBool(CPLGetConfigOption(ZARR_SRS_DEBUG, "NO")))
    {
        CPLSetConfigOption("CPL_DEBUG", "ON");
        CPLSetConfigOption("GDAL_DATA", CPLGetConfigOption("GDAL_DATA", ""));
    }

    // Log configuration
    CPLDebug("ZarrSRSModifier",
             "Configuration loaded - Transform: %s, Target EPSG: %d",
             m_osTransformType.c_str(), m_nTargetEPSG);
}

const OGRSpatialReference *ZarrSRSModifierDataset::GetSpatialRef() const
{
    // Return custom SRS if available, otherwise delegate to original
    if (m_poCustomCRS)
    {
        return m_poCustomCRS.get();
    }

    return m_poOriginalDataset ? m_poOriginalDataset->GetSpatialRef() : nullptr;
}

int ZarrSRSModifierDataset::Identify(GDALOpenInfo *poOpenInfo)
{
    // Delegate identification to original Zarr driver
    GDALDriver *poZarrDriver = static_cast<GDALDriver *>(GDALGetDriverByName("Zarr"));
    if (!poZarrDriver || !poZarrDriver->pfnIdentify)
    {
        return FALSE;
    }

    int nResult = poZarrDriver->pfnIdentify(poOpenInfo);
    if (nResult > 0)
    {
        // Return a higher confidence score to take precedence
        return 100; // Higher than typical identification scores
    }
    return FALSE;
}

int ZarrSRSModifierDataset::GetRasterXSize()
{
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterXSize() : 0;
}

int ZarrSRSModifierDataset::GetRasterYSize()
{
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterYSize() : 0;
}

int ZarrSRSModifierDataset::GetRasterCount()
{
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterCount() : 0;
}

GDALRasterBand *ZarrSRSModifierDataset::GetRasterBand(int nBand)
{
    return m_poOriginalDataset ? m_poOriginalDataset->GetRasterBand(nBand) : nullptr;
}

CPLErr ZarrSRSModifierDataset::GetGeoTransform(double *padfTransform)
{
    return m_poOriginalDataset ? m_poOriginalDataset->GetGeoTransform(padfTransform) : CE_Failure;
}

CPLErr ZarrSRSModifierDataset::SetSpatialRef(const OGRSpatialReference *poSRS)
{
    // This will be enhanced in subsequent tasks
    // For now, just store the SRS for future modification
    if (poSRS)
    {
        m_poCustomCRS = std::make_unique<OGRSpatialReference>();
        char *pszWKT = nullptr;
        poSRS->exportToWkt(&pszWKT);
        if (pszWKT)
        {
            m_poCustomCRS->importFromWkt(pszWKT);
            CPLFree(pszWKT);
        }
        return CE_None;
    }

    return CE_Failure;
}
