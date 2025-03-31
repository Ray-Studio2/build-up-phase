#pragma once

#include "SceneObject.h"

namespace A3
{
struct MeshResource;

class MeshObject : public SceneObject
{
public:
	MeshObject( MeshResource* inResource )
		: resource( inResource )
	{}

	MeshResource* getResource() { return resource; }

	virtual bool canRender() override { return true; }

private:
	MeshResource* resource;
};
}