#pragma once

#include <vector>
#include <memory>

namespace A3
{
class SceneObject;
class MeshObject;

class Scene
{
public:
	Scene();
	~Scene();

	void beginFrame();
	void endFrame();

	std::vector<MeshObject*> collectMeshObjects() const;

	void markSceneDirty() { bSceneDirty = true; }
	bool isSceneDirty() const { return bSceneDirty; }

private:
	bool bSceneDirty;

	std::vector<std::unique_ptr<SceneObject>> objects;
};
}