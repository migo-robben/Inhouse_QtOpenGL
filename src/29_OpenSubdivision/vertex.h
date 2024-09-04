//
// Created by PC on 9/2/2024.
//

#ifndef VERTEX_H
#define VERTEX_H

namespace Geometry
{
    static float g_verts[24] = {-1.0f, -1.0f,  1.0f,
                                 1.0f, -1.0f,  1.0f,
                                -1.0f,  1.0f,  1.0f,
                                 1.0f,  1.0f,  1.0f,
                                -1.0f,  1.0f, -1.0f,
                                 1.0f,  1.0f, -1.0f,
                                -1.0f, -1.0f, -1.0f,
                                 1.0f, -1.0f, -1.0f};

    static int g_nverts = 8;
    static int g_nfaces = 6;

    static int g_vertsperface[6] = { 4, 4, 4, 4, 4, 4 };

    static int g_vertIndices[24] = { 0, 1, 3, 2,
                                     2, 3, 5, 4,
                                     4, 5, 7, 6,
                                     6, 7, 1, 0,
                                     1, 7, 5, 3,
                                     6, 0, 2, 4 };
}

#endif //VERTEX_H
