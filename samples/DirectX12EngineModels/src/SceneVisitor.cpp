#include <SceneVisitor.h>

#include <EffectPSO.h>
#include <Camera.h>

#include <dx12lib/CommandList.h>
#include <dx12lib/IndexBuffer.h>
#include <dx12lib/Material.h>
#include <dx12lib/Mesh.h>
#include <dx12lib/SceneNode.h>

#include <DirectXMath.h>

using namespace DX12_Library;
using namespace DirectX;

SceneVisitor::SceneVisitor( CommandList& commandList, const Camera& camera, EffectPSO& pso, bool transparent )
: m_CommandList( commandList )
, m_Camera( camera )
, m_LightingPSO( pso )
, m_TransparentPass(transparent)
{}

void SceneVisitor::Visit( DX12_Library::Scene& scene )
{
    m_LightingPSO.SetViewMatrix( m_Camera.get_ViewMatrix() );
    m_LightingPSO.SetProjectionMatrix( m_Camera.get_ProjectionMatrix() );
}

void SceneVisitor::Visit( DX12_Library::SceneNode& sceneNode )
{
    auto world = sceneNode.GetWorldTransform();
    m_LightingPSO.SetWorldMatrix( world );
}

void SceneVisitor::Visit( Mesh& mesh )
{
    auto material = mesh.GetMaterial();
    if ( material->IsTransparent() == m_TransparentPass )
    {
        m_LightingPSO.SetMaterial( material );

        m_LightingPSO.Apply( m_CommandList );
        mesh.Draw( m_CommandList );
    }
}