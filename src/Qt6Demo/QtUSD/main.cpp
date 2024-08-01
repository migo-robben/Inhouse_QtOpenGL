#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/plug/registry.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

int main() {
    // 类似于设置环境变了 PXR_PLUGINPATH_NAME
    PlugRegistry::GetInstance().RegisterPlugins("E:/DEV_PROJECTS/WSRD/USD_v22.11_Static/Release/release/lib/usd");
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    // Do something with the stage...

    return 0;
}
