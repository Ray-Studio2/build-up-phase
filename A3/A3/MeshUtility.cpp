#include "Utility.h"
#include "MeshResource.h"
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust triangulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

using namespace A3;

tinyobj::ObjReaderConfig    tinyObjConfig;
tinyobj::ObjReader          tinyObjReader;

void Utility::loadMeshFile( MeshResource& outMesh, const std::string& filePath )
{
    if( !tinyObjReader.ParseFromFile( filePath, tinyObjConfig ) )
    {
        if( !tinyObjReader.Error().empty() )
        {
            std::cerr << "TinyObjReader: " << tinyObjReader.Error();
        }
        exit( 1 );
    }

    if( !tinyObjReader.Warning().empty() )
    {
        std::cout << "TinyObjReader: " << tinyObjReader.Warning();
    }

    const auto& attrib = tinyObjReader.GetAttrib();
    const auto& shapes = tinyObjReader.GetShapes();

    for( const auto& shape : shapes )
    {
        outMesh.vertexData.reserve( shape.mesh.indices.size() + outMesh.vertexData.size() );
        outMesh.indexData.reserve( shape.mesh.indices.size() + outMesh.indexData.size() );

        for( const auto& index : shape.mesh.indices )
        {
            Vertex v = {};

            v.position[ 0 ] = attrib.vertices[ 3 * index.vertex_index + 0 ];
            v.position[ 1 ] = attrib.vertices[ 3 * index.vertex_index + 1 ];
            v.position[ 2 ] = attrib.vertices[ 3 * index.vertex_index + 2 ];

            if( !attrib.normals.empty() && index.normal_index >= 0 )
            {
                v.normal[ 0 ] = attrib.normals[ 3 * index.normal_index + 0 ];
                v.normal[ 1 ] = attrib.normals[ 3 * index.normal_index + 1 ];
                v.normal[ 2 ] = attrib.normals[ 3 * index.normal_index + 2 ];
            }

            if( !attrib.colors.empty() )
            {
                v.color[ 0 ] = attrib.colors[ 3 * index.vertex_index + 0 ];
                v.color[ 1 ] = attrib.colors[ 3 * index.vertex_index + 1 ];
                v.color[ 2 ] = attrib.colors[ 3 * index.vertex_index + 2 ];
            }

            if( !attrib.texcoords.empty() && index.texcoord_index >= 0 )
            {
                v.uv[ 0 ] = attrib.texcoords[ 2 * index.texcoord_index + 0 ];
                v.uv[ 1 ] = attrib.texcoords[ 2 * index.texcoord_index + 1 ];
            }

            outMesh.vertexData.push_back( v );
            outMesh.indexData.push_back( static_cast< uint32_t >( outMesh.indexData.size() ) ); // TODO: repeated vertex
        }
    }
}