#pragma once


#include <dx12lib/Visitor.h>

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
     */
    SceneVisitor( DX12_Library::CommandList& commandList );

    // For this sample, we don't need to do anything when visiting the Scene.
    virtual void Visit( DX12_Library::Scene& scene ) override {}
    // For this sample, we don't need to do anything when visiting the SceneNode.
    virtual void Visit( DX12_Library::SceneNode& sceneNode ) override {}
    // When visiting a mesh, the mesh must be rendered.
    virtual void Visit( DX12_Library::Mesh& mesh ) override;

private:
    DX12_Library::CommandList& m_CommandList;
};