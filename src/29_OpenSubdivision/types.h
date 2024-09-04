//
// Created by PC on 9/2/2024.
//

#ifndef TYPES_H
#define TYPES_H

enum Scheme {
    kBilinear=0,
    kCatmark,
    kLoop
};

enum KernelType {
    kCPU = 0,
    kOPENMP = 1,
    kTBB = 2,
    kCUDA = 3,
    kCL = 4,
    kGLSL = 5,
    kGLSLCompute = 6
};

enum ShadingMode {
    kShadingMaterial,
    kShadingVaryingColor,
    kShadingInterleavedVaryingColor,
    kShadingFaceVaryingColor,
    kShadingPatchType,
    kShadingPatchDepth,
    kShadingPatchCoord,
    kShadingNormal
};

enum EndCap {
    kEndCapBilinearBasis = 0,
    kEndCapBSplineBasis,
    kEndCapGregoryBasis,
    kEndCapLegacyGregory
};

enum DisplayStyle {
    kDisplayStyleWire,
    kDisplayStyleShaded,
    kDisplayStyleWireOnShaded
};

#endif //TYPES_H
