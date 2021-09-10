#pragma once

namespace DX12_Library
{
class Scene;
class SceneNode;
class Mesh;
/*
 * A visitor simply allows the rendering of a scene object
 * in three different ways.
 */
class Visitor
{
public:
    Visitor()          = default;
    virtual ~Visitor() = default;

    virtual void Visit( Scene& scene ) = 0;
    virtual void Visit( SceneNode& sceneNode ) = 0;
    virtual void Visit( Mesh& mesh )           = 0;
};

}  // namespace DX12_Library