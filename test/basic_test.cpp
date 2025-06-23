#include <iostream>
#include "gdal_priv.h"

int main() {
    GDALAllRegister();
    
    // Check if our plugin is loaded
    GDALDriver* poDriver = GDALGetDriverByName("ZarrSRSModifier");
    if (poDriver) {
        std::cout << "Plugin loaded successfully: " 
                  << poDriver->GetDescription() << std::endl;
        std::cout << "Long name: " 
                  << poDriver->GetMetadataItem(GDAL_DMD_LONGNAME) << std::endl;
        return 0;
    } else {
        std::cout << "Plugin failed to load" << std::endl;
        return 1;
    }
}
