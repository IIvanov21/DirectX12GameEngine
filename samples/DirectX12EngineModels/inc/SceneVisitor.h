#pragma once
#include <dx12lib/Visitor.h>

class Camera;
class EffectPSO;

namespace DX12_Library
{
class CommandList;
}

class SceneVisitor : public DX12_Library::Visitor
{
public:
    /**
     * Constructor for the SceneVisitor.
     * @param commandList The CommandList that is used to render the meshes in the scene.
     * @param camera The camera that is used to render the scene. This is required for setting up the MVP matrix.
     * @param pso The Pipeline state object to use for rendering the geometry in the scene.
     * @param transparent Whether to draw transparent geometry during this pass.
     */
    SceneVisitor( DX12_Library::CommandList& commandList, const Camera& camera, EffectPSO& pso, bool transparent );

    // For this sample, we don't need to do anything when visiting the Scene.
    virtual void Visit( DX12_Library::Scene& scene ) override;
    // For this sample, we need to set the MVP matrix of the scene node.
    virtual void Visit( DX12_Library::SceneNode& sceneNode ) override;
    // When visiting a mesh, the mesh must be rendered.
    virtual void Visit( DX12_Library::Mesh& mesh ) override;

private:
    DX12_Library::CommandList& m_CommandList;
    const Camera&         m_Camera;
    EffectPSO&     m_LightingPSO;
    bool                  m_TransparentPass;
};