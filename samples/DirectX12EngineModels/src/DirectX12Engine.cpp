#include <DirectX12Engine.h>

#include <EffectPSO.h>
#include <SceneVisitor.h>

#include <GameFramework/Window.h>

#include <dx12lib/CommandList.h>
#include <dx12lib/CommandQueue.h>
#include <dx12lib/Device.h>
#include <dx12lib/GUI.h>
#include <dx12lib/Helpers.h>
#include <dx12lib/Material.h>
#include <dx12lib/Mesh.h>
#include <dx12lib/RootSignature.h>
#include <dx12lib/Scene.h>
#include <dx12lib/SceneNode.h>
#include <dx12lib/SwapChain.h>
#include <dx12lib/Texture.h>

#include <assimp/DefaultLogger.hpp>

#include <DirectXColors.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <d3dx12.h>

#include <ShObjIdl.h>  // For IFileOpenDialog
#include <shlwapi.h>

#include <regex>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DX12_Library;

// Builds a look-at (world) matrix from a point, up and direction vectors.
XMMATRIX XM_CALLCONV LookAtMatrix( FXMVECTOR Position, FXMVECTOR Direction, FXMVECTOR Up )
{
    assert( !XMVector3Equal( Direction, XMVectorZero() ) );
    assert( !XMVector3IsInfinite( Direction ) );
    assert( !XMVector3Equal( Up, XMVectorZero() ) );
    assert( !XMVector3IsInfinite( Up ) );

    XMVECTOR R2 = XMVector3Normalize( Direction );

    XMVECTOR R0 = XMVector3Cross( Up, R2 );
    R0          = XMVector3Normalize( R0 );

    XMVECTOR R1 = XMVector3Cross( R2, R0 );

    XMMATRIX M( R0, R1, R2, Position );

    return M;
}

// A regular express used to extract the relavent part of an Assimp log message.
static const std::regex gs_AssimpLogRegex( R"((?:Debug|Info|Warn|Error),\s*(.*)\n)" );

template<spdlog::level::level_enum lvl>
class LogStream : public Assimp::LogStream
{
public:
    LogStream( Logger logger )
    : m_Logger( logger )
    {}

    virtual void write( const char* message ) override
    {
        // Extract just the part of the message we want to log with spdlog.
        std::cmatch match;
        std::regex_search( message, match, gs_AssimpLogRegex );

        if ( match.size() > 1 )
        {
            m_Logger->log( lvl, match.str( 1 ) );
        }
    }

private:
    Logger m_Logger;
};

using DebugLogStream = LogStream<spdlog::level::debug>;
using InfoLogStream  = LogStream<spdlog::level::info>;
using WarnLogStream  = LogStream<spdlog::level::warn>;
using ErrorLogStream = LogStream<spdlog::level::err>;

DirectX12Engine::DirectX12Engine( const std::wstring& name, int width, int height, bool vSync )
: m_ScissorRect { 0, 0, LONG_MAX, LONG_MAX }
, m_Viewport( CD3DX12_VIEWPORT( 0.0f, 0.0f, static_cast<float>( width ), static_cast<float>( height ) ) )
, m_CameraController( m_Camera )
, m_AnimateLights( false )
, m_Fullscreen( false )
, m_AllowFullscreenToggle( true )
, m_ShowFileOpenDialog( false )
, m_CancelLoading( false )
, m_ShowControls( true )
, m_ShowInspector( true )
, m_Width( width )
, m_Height( height )
, m_IsLoading( true )
, m_FPS( 0.0f )
{
#if _DEBUG
    Device::EnableDebugLayer();
#endif
    // Create a spdlog logger for the demo.
    m_Logger = GameFramework::Get().CreateLogger( "05-Models" );
    // Create logger for assimp.
    auto assimpLogger = GameFramework::Get().CreateLogger( "ASSIMP" );

    // Setup assimp logging.
#if defined( _DEBUG )
    Assimp::Logger::LogSeverity logSeverity = Assimp::Logger::VERBOSE;
#else
    Assimp::Logger::LogSeverity logSeverity = Assimp::Logger::NORMAL;
#endif
    // Create a default logger with no streams (we'll supply our own).
    auto assimpDefaultLogger = Assimp::DefaultLogger::create( "", logSeverity, 0 );
    assimpDefaultLogger->attachStream( new DebugLogStream( assimpLogger ), Assimp::Logger::Debugging );
    assimpDefaultLogger->attachStream( new InfoLogStream( assimpLogger ), Assimp::Logger::Info );
    assimpDefaultLogger->attachStream( new WarnLogStream( assimpLogger ), Assimp::Logger::Warn );
    assimpDefaultLogger->attachStream( new ErrorLogStream( assimpLogger ), Assimp::Logger::Err );

    // Create  window for rendering to.
    m_Window = GameFramework::Get().CreateWindow( name, width, height );

    // Hookup Window callbacks.
    m_Window->Update += UpdateEvent::slot( &DirectX12Engine::OnUpdate, this );
    m_Window->Resize += ResizeEvent::slot( &DirectX12Engine::OnResize, this );
    m_Window->DPIScaleChanged += DPIScaleEvent::slot( &DirectX12Engine::OnDPIScaleChanged, this );
    m_Window->KeyPressed += KeyboardEvent::slot( &DirectX12Engine::OnKeyPressed, this );
    m_Window->KeyReleased += KeyboardEvent::slot( &DirectX12Engine::OnKeyReleased, this );
    m_Window->MouseMoved += MouseMotionEvent::slot( &DirectX12Engine::OnMouseMoved, this );
    m_Window->MouseButtonPressed += MouseButtonEvent::slot(&DirectX12Engine::OnMousePressed, this);
    m_Window->MouseButtonReleased += MouseButtonEvent::slot( &DirectX12Engine::OnMouseReleased, this );
}

DirectX12Engine::~DirectX12Engine()
{
    Assimp::DefaultLogger::kill();
}

uint32_t DirectX12Engine::Run()
{

    LoadContent();

    // Only show the window after content has been loaded.
    m_Window->Show();

    auto retCode = GameFramework::Get().Run();

    // Make sure the loading task is finished
    m_LoadingTask.get();

    UnloadContent();

    return retCode;
}

bool DirectX12Engine::LoadingProgress( float loadingProgress )
{
    m_LoadingProgress = loadingProgress;

    // This function should return false to cancel the loading process.
    return !m_CancelLoading;
}

bool DirectX12Engine::LoadScene( const std::wstring& sceneFile )
{
    using namespace std::placeholders;  // For _1 used to denote a placeholder argument for std::bind.

    m_IsLoading     = true;
    m_CancelLoading = false;

    auto& commandQueue = m_Device->GetCommandQueue( D3D12_COMMAND_LIST_TYPE_COPY );
    auto  commandList  = commandQueue.GetCommandList();

    // Load a scene, passing an optional function object for receiving loading progress events.
    m_LoadingText = std::string( "Loading " ) + ConvertString( sceneFile ) + "...";
    auto scene    = commandList->LoadSceneFromFile( sceneFile, std::bind( &DirectX12Engine::LoadingProgress, this, _1 ) );
    
    if ( scene )
    {
        // Scale the scene so it fits in the camera frustum.
        DirectX::BoundingSphere s;
        BoundingSphere::CreateFromBoundingBox( s, scene->GetAABB() );
        auto scale = 50.0f / ( s.Radius * 2.0f );
        s.Radius *= scale;

        scene->GetRootNode()->SetLocalTransform( XMMatrixScaling( scale, scale, scale ) );
        scene->GetRootNode()->SetName( ConvertString( sceneFile ) );
        // Position the camera so that it is looking at the loaded scene.
        auto cameraRotation   = m_Camera.get_Rotation();
        auto cameraFoV        = m_Camera.get_FoV();
        auto distanceToObject = s.Radius / std::tanf( XMConvertToRadians( cameraFoV ) / 2.0f );

        auto cameraPosition = XMVectorSet( 0, 0, -distanceToObject, 1 );
        //        cameraPosition      = XMVector3Rotate( cameraPosition, cameraRotation );
        auto focusPoint = XMVectorSet( s.Center.x * scale, s.Center.y * scale, s.Center.z * scale, 1.0f );
        cameraPosition  = cameraPosition + focusPoint;

        m_Camera.set_Translation( cameraPosition );
        m_Camera.set_FocalPoint( focusPoint );

    }
    
    //Track all the objects being loaded in the scene.
    m_AssetsList.push_back( scene );

    commandQueue.ExecuteCommandList( commandList );

    // Ensure that the scene is completely loaded before rendering.
    commandQueue.Flush();

    // Loading is finished.
    m_IsLoading = false;

    return scene != nullptr;
}

void DirectX12Engine::LoadContent()
{
    m_Device = Device::Create();
    m_Logger->info( L"Device created: {}", m_Device->GetDescription() );

    m_SwapChain = m_Device->CreateSwapChain( m_Window->GetWindowHandle(), DXGI_FORMAT_R8G8B8A8_UNORM );
    m_GUI       = m_Device->CreateGUI( m_Window->GetWindowHandle(), m_SwapChain->GetRenderTarget() );

    // This magic here allows ImGui to process window messages.
    GameFramework::Get().WndProcHandler += WndProcEvent::slot( &GUI::WndProcHandler, m_GUI );

    // Start the loading task to perform async loading of the scene file.
    m_LoadingTask = std::async( std::launch::async, std::bind( &DirectX12Engine::LoadScene, this,
                                                               L"Assets/Models/crytek-sponza/sponza_nobanner.obj" ) );

    // Load a few (procedural) models to represent the light sources in the scene.
    auto& commandQueue = m_Device->GetCommandQueue( D3D12_COMMAND_LIST_TYPE_COPY );
    auto  commandList  = commandQueue.GetCommandList();




    // Create an inverted (reverse winding order) cube so the insides are not clipped.
    m_Skybox = commandList->CreateCube( 1.0f, true );
    m_Sphere = commandList->CreateSphere( 0.1f );
    m_Cone   = commandList->CreateCone( 0.1f, 0.2f );
    m_Axis   = commandList->LoadSceneFromFile( L"Assets/Models/axis_of_evil.nff" );

    
    //Example of manual loading assets into the scene
    //Create some default models for the scene
    m_Avocado = commandList->LoadSceneFromFile( L"Assets/Models/Avakado/Avakado.fbx" );
    m_Avocado->GetRootNode()->SetName( "Avocado" );//Set the name so it can be used as ID in the list.
    m_Ship    = commandList->LoadSceneFromFile( L"Assets/Models/Ship/full_scene.fbx" );
    m_Ship->GetRootNode()->SetName( "Ship" );
    m_Tree1 = commandList->LoadSceneFromFile( L"Assets/Models/Trees/Gledista_Triacanthos.obj" );
    m_Tree2 = commandList->LoadSceneFromFile( L"Assets/Models/Trees/Gledista_Triacanthos_2.obj" );
    m_Tree3 = commandList->LoadSceneFromFile( L"Assets/Models/Trees/Gledista_Triacanthos_3.obj" );
    m_Tree4 = commandList->LoadSceneFromFile( L"Assets/Models/Trees/Gledista_Triacanthos_5.obj" );
    m_Tree5 = commandList->LoadSceneFromFile( L"Assets/Models/Trees/Gledista_Triacanthos_6.obj" );
    m_Ground = commandList->LoadSceneFromFile( L"Assets/Models/Ground/uploads_files_2481142_Rocky_terrain2.obj" );
    m_Rock = commandList->LoadSceneFromFile( L"Assets/Models/Rocks/RockSet05A.obj" );

    //Add them to the Asset list so they are tracked
    m_AssetsList.push_back( m_Avocado );
    m_AssetsList.push_back( m_Ship );
    m_AssetsList.push_back( m_Tree1 );
    m_AssetsList.push_back( m_Tree2 );
    m_AssetsList.push_back( m_Tree3 );
    m_AssetsList.push_back( m_Tree4 );
    m_AssetsList.push_back( m_Tree5 );
    m_AssetsList.push_back( m_Ground );
    m_AssetsList.push_back( m_Rock );
    /*
     * Scene manipulated models.
     */
    XMMATRIX scaleMatrix = XMMatrixScaling( 0.01f, 0.01f, 0.01f );
    XMMATRIX translationMatrix = XMMatrixTranslation( 0.0f, 50.0f, 0.0f );
    XMMATRIX rotationMatrix    = XMMatrixRotationX( ToRadians( 180 ));
    m_AssetsList[0]->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.008f,0.008f,0.008f ) * XMMatrixTranslation( -4.8f,19.36f,23.89f ));
    m_Avocado->GetRootNode()->SetLocalTransform( scaleMatrix * translationMatrix * rotationMatrix );
    m_Ship->GetRootNode()->SetLocalTransform( scaleMatrix * translationMatrix * rotationMatrix );
    m_Tree1->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.4f,0.4f,0.4f ) * XMMatrixTranslation( -6.0f,19.0f,11.5f ) );
    m_Tree2->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.33f,0.33f,0.33f ) * XMMatrixTranslation( 18.1f,17.5f,25.0f ) );
    m_Tree3->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.29f,0.29f,0.29f ) * XMMatrixTranslation( -2.6f,19.0f,38.0f ) );
    m_Tree4->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.17f,0.17f,0.17f ) * XMMatrixTranslation( -21.2f,17.8f,19.0f ) );
    m_Tree5->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.005f,0.005f,0.005f ) * XMMatrixTranslation( -9.0f,20.0f,10.0f ) );
    m_Ground->GetRootNode()->SetLocalTransform( XMMatrixScaling( 12.8f,12.8,12.8 ) * XMMatrixTranslation( -3.5f,3.5f,19.1f ) );
    m_Rock->GetRootNode()->SetLocalTransform( XMMatrixScaling( 0.46f,0.65,0.46 ) * XMMatrixTranslation( -22.0f,19.0f,-10.0f ) );
    

    auto fence = commandQueue.ExecuteCommandList( commandList );

    // Create a PSOs
    m_LightingPSO = std::make_shared<EffectPSO>( m_Device, true, false );
    m_DecalPSO    = std::make_shared<EffectPSO>( m_Device, true, true );
    m_UnlitPSO    = std::make_shared<EffectPSO>( m_Device, false, false );

    // Create a color buffer with sRGB for gamma correction.
    DXGI_FORMAT backBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Check the best multisample quality level that can be used for the given back buffer format.
    DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels( backBufferFormat );

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D( backBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
                                                   sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET );
    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format   = colorDesc.Format;
    colorClearValue.Color[0] = 0.4f;
    colorClearValue.Color[1] = 0.6f;
    colorClearValue.Color[2] = 0.9f;
    colorClearValue.Color[3] = 1.0f;

    auto colorTexture = m_Device->CreateTexture( colorDesc, &colorClearValue );
    colorTexture->SetName( L"Color Render Target" );

    // Create a depth buffer.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D( depthBufferFormat, m_Width, m_Height, 1, 1, sampleDesc.Count,
                                                   sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL );
    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format       = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    auto depthTexture = m_Device->CreateTexture( depthDesc, &depthClearValue );
    depthTexture->SetName( L"Depth Render Target" );

    // Attach the textures to the render target.
    m_RenderTarget.AttachTexture( AttachmentPoint::Color0, colorTexture );
    m_RenderTarget.AttachTexture( AttachmentPoint::DepthStencil, depthTexture );

    // Make sure the copy command queue is finished before leaving this function.
    commandQueue.WaitForFenceValue( fence );
}

void DirectX12Engine::UnloadContent()
{
    m_Skybox.reset();

    m_GraceCathedralTexture.reset();
    m_GraceCathedralCubemap.reset();

    m_SkyboxSignature.reset();
    m_HDRRootSignature.reset();
    m_SDRRootSignature.reset();
    m_SkyboxPipelineState.reset();
    m_HDRPipelineState.reset();
    m_SDRPipelineState.reset();
    m_UnlitPipelineState.reset();


    m_HDRRenderTarget.Reset();

    m_GUI.reset();
    m_SwapChain.reset();
    m_Device.reset();
}

//Help function to calculate distance
float Distance( const XMVECTOR& v1, const XMVECTOR& v2 )
{
    XMVECTOR vectorSub = XMVectorSubtract( v1, v2 );
    XMVECTOR length    = XMVector2Length( vectorSub );

    float distance = 0.0f;
    XMStoreFloat( &distance, length );
    return distance;
}

void DirectX12Engine::DeleteEntity()
{
    if ( m_Scene != nullptr )//Find the entity in the list and removes its reference
    {
        for ( auto it = m_AssetsList.begin(); it != m_AssetsList.end(); ++it )
        {
            if ( *it == m_Scene )
            {
                m_AssetsList.erase( it );
                break;
            }
        }
        std::get_deleter<Scene>( m_Scene );
    }
}

void DirectX12Engine::NearestEntity()
{
    XMVECTOR entityPixel=XMVectorSet(0.0f,0.0f,0.0f,0.0f);
    float    nearestDistance= 200.0f;
    bool     isBehind        = false;
    for (auto it:m_AssetsList)
    {
        entityPixel=m_Camera.PixelFromWorldPt( entityPixel, it->GetRootNode()->GetPosition(), m_Width, m_Height, isBehind );
        if (isBehind)
        {
            std::string name          = it->GetRootNode()->GetName();
            float pixelDistance = Math::Distance2D( XMVectorSet( m_MouseX, m_MouseY, 0.0f, 0.0f ), entityPixel );
            if (pixelDistance<nearestDistance)
            {
                if ( m_ModelSelected)
                {
                    m_Scene = it;
                    m_ModelSelected = false;
                }
                nearestDistance = pixelDistance;
            }
        }
    }
}

void DirectX12Engine::OnUpdate( UpdateEventArgs& e )
{
    static uint64_t frameCount = 0;
    static double   totalTime  = 0.0;

    totalTime += e.DeltaTime;
    frameCount++;

    if ( totalTime > 1.0 )
    {
        m_FPS = frameCount / totalTime;

        wchar_t buffer[512];
        ::swprintf_s( buffer, L"Models [FPS: %f]", m_FPS );
        m_Window->SetWindowTitle( buffer );

        frameCount = 0;
        totalTime  = 0.0;
    }

    if ( m_ShowFileOpenDialog )
    {
        m_ShowFileOpenDialog = false;
        OpenFile();
    }

    m_SwapChain->WaitForSwapChain();

    // Process keyboard, mouse, and pad input.
    GameFramework::Get().ProcessInput();
    m_CameraController.Update( e );

    // Move the Axis model to the focal point of the camera.
    XMVECTOR cameraPoint = m_Camera.get_FocalPoint();
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector( cameraPoint );
    XMMATRIX scaleMatrix = XMMatrixScaling( 0.01f, 0.01f, 0.01f );
    m_Axis->GetRootNode()->SetLocalTransform( scaleMatrix * translationMatrix );
     if ( m_Scene != nullptr )
    {
        m_Scene->GetRootNode()->SetPosition( cameraPoint );
    }
    XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();

    const int numDirectionalLights = 3;

    static const XMVECTORF32 LightColors[] = { Colors::White, Colors::OrangeRed, Colors::Blue };

    static float lightAnimTime = 0.0f;
    if ( m_AnimateLights )
    {
        lightAnimTime += static_cast<float>( e.DeltaTime ) * 0.5f * XM_PI;
    }

    const float radius                 = 1.0f;
    float       directionalLightOffset = numDirectionalLights > 0 ? 2.0f * XM_PI / numDirectionalLights : 0;

    m_DirectionalLights.resize( numDirectionalLights );
    for ( int i = 0; i < numDirectionalLights; ++i )
    {
        DirectionalLight& l = m_DirectionalLights[i];

        float angle = lightAnimTime + directionalLightOffset * i;

        XMVECTORF32 positionWS = { static_cast<float>( std::cos( angle ) ) * radius,
                                   static_cast<float>( std::sin( angle ) ) * radius, radius, 1.0f };

        XMVECTOR directionWS = XMVector3Normalize( XMVectorNegate( positionWS ) );
        XMVECTOR directionVS = XMVector3TransformNormal( directionWS, viewMatrix );

        XMStoreFloat4( &l.DirectionWS, directionWS );
        XMStoreFloat4( &l.DirectionVS, directionVS );

        l.Color = XMFLOAT4( LightColors[i] );
    }

    m_LightingPSO->SetDirectionalLights( m_DirectionalLights );
    m_DecalPSO->SetDirectionalLights( m_DirectionalLights );

    NearestEntity();

    OnRender();
}

//Handles the rendering window resize events
//Don't confuse with objects rendered in resizing
void DirectX12Engine::OnResize( ResizeEventArgs& e ) 
{
    m_Logger->info( "Resize: {}, {}", e.Width, e.Height );

    m_Width  = std::max( 1, e.Width );
    m_Height = std::max( 1, e.Height );

    m_Camera.set_Projection( 45.0f, m_Width / (float)m_Height, 0.1f, 100.0f );
    m_Viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, static_cast<float>( m_Width ), static_cast<float>( m_Height ) );

    m_RenderTarget.Resize( m_Width, m_Height );

    m_SwapChain->Resize( m_Width, m_Height );
}

void DirectX12Engine::OnRender()
{
    // This is done here to prevent the window switching to fullscreen while rendering the GUI.
    m_Window->SetFullscreen( m_Fullscreen );

    auto& commandQueue = m_Device->GetCommandQueue( D3D12_COMMAND_LIST_TYPE_DIRECT );
    auto  commandList  = commandQueue.GetCommandList();

    const auto& renderTarget = m_IsLoading ? m_SwapChain->GetRenderTarget() : m_RenderTarget;

    if ( m_IsLoading )
    {
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        commandList->ClearTexture( renderTarget.GetTexture( AttachmentPoint::Color0 ), clearColor );

        // TODO: Render a loading screen.
    }
    else
    {
        SceneVisitor opaquePass( *commandList, m_Camera, *m_LightingPSO, false );
        SceneVisitor transparentPass( *commandList, m_Camera, *m_DecalPSO, true );
        SceneVisitor unlitPass( *commandList, m_Camera, *m_UnlitPSO, false );

        // Clear the render targets.
        {
            FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

            commandList->ClearTexture( renderTarget.GetTexture( AttachmentPoint::Color0 ), clearColor );
            commandList->ClearDepthStencilTexture( renderTarget.GetTexture( AttachmentPoint::DepthStencil ),
                                                   D3D12_CLEAR_FLAG_DEPTH );
        }

        commandList->SetViewport( m_Viewport );
        commandList->SetScissorRect( m_ScissorRect );
        commandList->SetRenderTarget( m_RenderTarget );

        // Render the scene.
    
        m_Axis->Accept( unlitPass );
        
        for ( auto it: m_AssetsList )
        {
            it->Accept( opaquePass );
            it->Accept( transparentPass );
        }
        

        MaterialProperties lightMaterial = Material::Black;
        for ( const auto& l: m_PointLights )
        {
            lightMaterial.Emissive = l.Color;
            auto lightPos          = XMLoadFloat4( &l.PositionWS );
            auto worldMatrix       = XMMatrixTranslationFromVector( lightPos );

            m_Sphere->GetRootNode()->SetLocalTransform( worldMatrix );
            m_Sphere->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties( lightMaterial );
            m_Sphere->Accept( unlitPass );
        }

        for ( const auto& l: m_SpotLights )
        {
            lightMaterial.Emissive = l.Color;
            XMVECTOR lightPos      = XMLoadFloat4( &l.PositionWS );
            XMVECTOR lightDir      = XMLoadFloat4( &l.DirectionWS );
            XMVECTOR up            = XMVectorSet( 0, 1, 0, 0 );

            // Rotate the cone so it is facing the Z axis.
            auto rotationMatrix = XMMatrixRotationX( XMConvertToRadians( -90.0f ) );
            auto worldMatrix    = rotationMatrix * LookAtMatrix( lightPos, lightDir, up );

            m_Cone->GetRootNode()->SetLocalTransform( worldMatrix );
            m_Cone->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties( lightMaterial );
            m_Cone->Accept( unlitPass );
        }

        // Resolve the MSAA render target to the swapchain's backbuffer.
        auto swapChainBackBuffer = m_SwapChain->GetRenderTarget().GetTexture( AttachmentPoint::Color0 );
        auto msaaRenderTarget    = m_RenderTarget.GetTexture( AttachmentPoint::Color0 );

        commandList->ResolveSubresource( swapChainBackBuffer, msaaRenderTarget );
    }

    OnGUI( commandList, m_SwapChain->GetRenderTarget() );

    commandQueue.ExecuteCommandList( commandList );

    m_SwapChain->Present();
}


void DirectX12Engine::OnRotateY(float amount)
{
    amount *= 45.0f;
    auto rotationMatrix = XMMatrixRotationY( XMConvertToRadians( amount ) );
    if(m_Scene!=nullptr)m_Scene->GetRootNode()->SetLocalTransform( m_Scene->GetRootNode()->GetLocalTransform() * rotationMatrix );
}

void DirectX12Engine::OnRotateX( float amount )
{
    amount *= 45.0f;
    auto rotationMatrix = XMMatrixRotationX( XMConvertToRadians( amount ) );
    if ( m_Scene != nullptr )m_Scene->GetRootNode()->SetLocalTransform( m_Scene->GetRootNode()->GetLocalTransform() * rotationMatrix );
}

void DirectX12Engine::OnRotateZ( float amount )
{
    amount *= 45.0f;
    auto rotationMatrix = XMMatrixRotationX( XMConvertToRadians( amount ) );
    if ( m_Scene != nullptr )m_Scene->GetRootNode()->SetLocalTransform( m_Scene->GetRootNode()->GetLocalTransform() * rotationMatrix );
}

void DirectX12Engine::OnResetRotation( )
{
    if ( m_Scene != nullptr )m_Scene->GetRootNode()->SetLocalTransform( m_Scene->GetRootNode()->GetDefaultTransform() );
}

void DirectX12Engine::OnScaleUp()
{
    XMMATRIX scaleMatrix = XMMatrixScaling( 2.0f, 2.0f, 2.0f );
    if ( m_Scene != nullptr )m_Scene->GetRootNode()->SetLocalTransform( m_Scene->GetRootNode()->GetLocalTransform() * scaleMatrix );
}

void DirectX12Engine::OnScaleDown()
{
    XMMATRIX scaleMatrix = XMMatrixScaling( 0.5f, 0.5f, 0.5f );
    if ( m_Scene != nullptr )m_Scene->GetRootNode()->SetLocalTransform( m_Scene->GetRootNode()->GetLocalTransform() * scaleMatrix );
}

void DirectX12Engine::OnKeyPressed( KeyEventArgs& e )
{
    if ( !ImGui::GetIO().WantCaptureKeyboard)
    {
        switch ( e.Key )
        {
        case KeyCode::Escape:
            GameFramework::Get().Stop();
            break;
        case KeyCode::Space:
            m_AnimateLights = !m_AnimateLights;
            break;
        case KeyCode::Enter:
            if ( e.Alt )
            {
            case KeyCode::F11:
                if ( m_AllowFullscreenToggle )
                {
                    m_Fullscreen = !m_Fullscreen;  // Defer window resizing until OnUpdate();
                    // Prevent the key repeat to cause multiple resizes.
                    m_AllowFullscreenToggle = false;
                }
                break;
            }
        case KeyCode::V:
            m_SwapChain->ToggleVSync();
            break;
        case KeyCode::R:
            // Reset camera transform
            m_CameraController.ResetView();
            break;
        case KeyCode::O:
            if ( e.Control )
            {
                OpenFile();
            }
            break;
        case KeyCode::NumPad4:
            OnRotateY( -1.0f );
            break;
        case KeyCode::NumPad6:
            OnRotateY( 1.0f );
        case KeyCode::NumPad7:
            OnRotateX( -1.0f );
            break;
        case KeyCode::NumPad9:
            OnRotateX( 1.0f );
            break;
        case KeyCode::NumPad8:
            OnRotateZ( -1.0f );
            break;
        case KeyCode::NumPad2:
            OnRotateZ( 1.0f );
            break;
        case KeyCode::NumPad5:
            OnResetRotation();
            break;
        case KeyCode::NumPad1:
            OnScaleDown();
            break;
        case KeyCode::NumPad3:
            OnScaleUp();
            break;
        case KeyCode::LButton:
            m_ModelSelected = true;
            break;
        case KeyCode::C:
            m_ModelInteracted = false;
            m_ModelSelected = false;
            m_Scene         = nullptr;
            break;
        case KeyCode::Delete:
            DeleteEntity();
            m_ModelInteracted = false;
            m_ModelSelected   = false;
            break;
        }
    }
}

void DirectX12Engine::OnKeyReleased( KeyEventArgs& e )
{
    

    if ( !ImGui::GetIO().WantCaptureKeyboard)
    {
        switch ( e.Key )
        {
        case KeyCode::Enter:
            if ( e.Alt )
            {
            case KeyCode::F11:
                m_AllowFullscreenToggle = true;
            }
            break;
        case KeyCode::LButton:
            m_ModelSelected = false;
            m_Scene         = nullptr;
            break;
        }
    }
}

void DirectX12Engine::OnMousePressed( MouseButtonEventArgs& m ) 
{
    if ( !ImGui::GetIO().WantCaptureMouse )
    {
        switch ( m.Button )
        {
        case MouseButton::Left:
            m_ModelSelected = true;
            break;
        case MouseButton::Right:
            m_Scene = nullptr;
            break;
        }
    }
}

void DirectX12Engine::OnMouseReleased( MouseButtonEventArgs& m )
{
    if ( !ImGui::GetIO().WantCaptureMouse )
    {
        switch ( m.Button )
        {
        case MouseButton::Left:
            m_ModelSelected = false;
            break;
        }
    }
}

void DirectX12Engine::OnMouseMoved( MouseMotionEventArgs& e )
{
    if ( !ImGui::GetIO().WantCaptureMouse ) {}
    m_MouseX = e.X;
    m_MouseY = e.Y;
}

void DirectX12Engine::OnDPIScaleChanged( DPIScaleEventArgs& e )
{
    m_GUI->SetScaling( e.DPIScale );
}

static void HelpMarker( const char* desc )
{
    ImGui::TextDisabled( "(?)" );
    if ( ImGui::IsItemHovered() )
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
        ImGui::TextUnformatted( desc );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void DirectX12Engine::OnGUI( const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget )
{
    m_GUI->NewFrame();

    if ( m_IsLoading )
    {
        // Show a progress bar.
        ImGui::SetNextWindowPos( ImVec2( m_Window->GetClientWidth() / 2.0f, m_Window->GetClientHeight() / 2.0f ), 0,
                                 ImVec2( 0.5, 0.5 ) );
        ImGui::SetNextWindowSize( ImVec2( m_Window->GetClientWidth() / 2.0f, 0 ) );

        ImGui::Begin( "Loading", nullptr,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                          ImGuiWindowFlags_NoScrollbar );
        ImGui::ProgressBar( m_LoadingProgress );
        ImGui::Text( m_LoadingText.c_str() );
        if ( !m_CancelLoading )
        {
            if ( ImGui::Button( "Cancel" ) )
            {
                m_CancelLoading = true;
            }
        }
        else
        {
            ImGui::Text( "Cancel Loading..." );
        }

        ImGui::End();
    }

    if ( ImGui::BeginMainMenuBar() )
    {
        if ( ImGui::BeginMenu( "File" ) )
        {
            if ( ImGui::MenuItem( "Open file...", "Ctrl+O", nullptr, !m_IsLoading ) )
            {
                m_ShowFileOpenDialog = true;
            }
            ImGui::Separator();
            if ( ImGui::MenuItem( "Exit", "Esc" ) )
            {
                GameFramework::Get().Stop();
            }
            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu( "View" ) )
        {
            ImGui::MenuItem( "Controls", nullptr, &m_ShowControls );

            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu( "Options" ) )
        {
            bool vSync = m_SwapChain->GetVSync();
            if ( ImGui::MenuItem( "V-Sync", "V", &vSync ) )
            {
                m_SwapChain->SetVSync( vSync );
            }

            bool fullscreen = m_Window->IsFullscreen();
            if ( ImGui::MenuItem( "Full screen", "Alt+Enter", &fullscreen ) )
            {
                // m_Window->SetFullscreen( fullscreen );
                // Defer the window resizing until the reference to the render target is released.
                m_Fullscreen = fullscreen;
            }

            ImGui::MenuItem( "Animate Lights", "Space", &m_AnimateLights );

            bool invertY = m_CameraController.IsInverseY();
            if ( ImGui::MenuItem( "Inverse Y", nullptr, &invertY ) )
            {
                m_CameraController.SetInverseY( invertY );
            }
            if ( ImGui::MenuItem( "Reset view", "R" ) )
            {
                m_CameraController.ResetView();
            }

            ImGui::EndMenu();
        }

        char buffer[256];
        {
            sprintf_s( buffer, _countof( buffer ), "FPS: %.2f (%.2f ms)  ", m_FPS, 1.0 / m_FPS * 1000.0 );
            auto fpsTextSize = ImGui::CalcTextSize( buffer );
            ImGui::SameLine( ImGui::GetWindowWidth() - fpsTextSize.x );
            ImGui::Text( buffer );
        }

        ImGui::EndMainMenuBar();
    }

    if ( m_ShowInspector ) 
    {
        ImGui::Begin( "Inspector", &m_ShowInspector );
        ImGui::SetWindowSize( ImVec2(200.0f,700.0f) );
        ImGui::SetWindowPos( ImVec2( 0.0f, 20.0f ) );
        for( auto it : m_AssetsList )
        {
            ImGui::Selectable( (const char*)it->GetRootNode()->GetName().c_str(), &it->GetRootNode()->GetSelection() );
        }

        
        
        ImGui::End();
    }

    if ( m_ShowControls )
    {
        ImGui::Begin( "Controls", &m_ShowControls );

        ImGui::Text( "KEYBOARD CONTROLS" );
        ImGui::BulletText( "ESC: Terminate application" );
        ImGui::BulletText( "Alt+Enter: Toggle fullscreen" );
        ImGui::BulletText( "F11: Toggle fullscreen" );
        ImGui::BulletText( "W: Move camera forward" );
        ImGui::BulletText( "A: Move camera left" );
        ImGui::BulletText( "S: Move camera backward" );
        ImGui::BulletText( "D: Move camera right" );
        ImGui::BulletText( "Q: Move camera down" );
        ImGui::BulletText( "E: Move camera up" );
        ImGui::BulletText( "R: Reset view" );
        ImGui::BulletText( "Shift: Boost move/rotate speed" );
        ImGui::BulletText( "Space: Animate lights" );
        ImGui::Separator();

        ImGui::Text( "MOUSE CONTROLS" );
        ImGui::BulletText( "MMB: Rotate camera" );
        ImGui::BulletText( "Mouse wheel: Zoom in/out on focal point" );
        ImGui::BulletText( "LMB: Select an object" );
        ImGui::BulletText( "RMB: Deselect an object" );

        ImGui::Separator();

        ImGui::Text( "GAMEPAD CONTROLS" );
        ImGui::BulletText( "Left analog stick: Move camera" );
        ImGui::BulletText( "Right analog stick: Rotate camera around the focal point" );
        ImGui::BulletText( "Left trigger: Move camera down" );
        ImGui::BulletText( "Right trigger: Move camera up" );
        ImGui::BulletText( "Hold left or right stick: Boost move/rotate speed" );
        ImGui::BulletText( "D-Pad up/down: Zoom in/out on focal point" );
        ImGui::Separator();

        ImGui::Text( "MODEL CONTROLS" );
        ImGui::BulletText( "While Selected Delete a model: Delete" );
        ImGui::BulletText( "Move with Camera Controls." );
        ImGui::BulletText( "Rotate Z: Num8 and Num2" );
        ImGui::BulletText( "Rotate Y: Num4 and Num6" );
        ImGui::BulletText( "Rotate X: Num7 and Num9" );
        ImGui::BulletText( "Scale Up: Num3" );
        ImGui::BulletText( "Scale Down: Num1" );
        ImGui::BulletText( "Reset Scale and Rotation: Num5" );








        ImGui::End();
    }

    m_GUI->Render( commandList, renderTarget );
}

// Open a file dialog for the user to select a scene to load.
// @see https://docs.microsoft.com/en-us/windows/win32/learnwin32/example--the-open-dialog-box
// @see
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/winui/shell/appplatform/commonfiledialog/CommonFileDialogApp.cpp
// @see https://www.codeproject.com/Articles/16678/Vista-Goodies-in-C-Using-the-New-Vista-File-Dialog
void DirectX12Engine::OpenFile()
{
    // clang-format off
    static const COMDLG_FILTERSPEC g_FileFilters[] = { 
        { L"Autodesk", L"*.fbx" }, 
        { L"Collada", L"*.dae" },
        { L"glTF", L"*.gltf;*.glb" },
        { L"Blender 3D", L"*.blend" },
        { L"3ds Max 3DS", L"*.3ds" },
        { L"3ds Max ASE", L"*.ase" },
        { L"Wavefront Object", L"*.obj" },
        { L"Industry Foundation Classes (IFC/Step)", L"*.ifc" },
        { L"XGL", L"*.xgl;*.zgl" },
        { L"Stanford Polygon Library", L"*.ply" },
        { L"AutoCAD DXF", L"*.dxf" },
        { L"LightWave", L"*.lws" },
        { L"LightWave Scene", L"*.lws" },
        { L"Modo", L"*.lxo" },
        { L"Stereolithography", L"*.stl" },
        { L"DirectX X", L"*.x" },
        { L"AC3D", L"*.ac" },
        { L"Milkshape 3D", L"*.ms3d" },
        { L"TrueSpace", L"*.cob;*.scn" },
        { L"Ogre XML", L"*.xml" },
        { L"Irrlicht Mesh", L"*.irrmesh" },
        { L"Irrlicht Scene", L"*.irr" },
        { L"Quake I", L"*.mdl" },
        { L"Quake II", L"*.md2" },
        { L"Quake III", L"*.md3" },
        { L"Quake III Map/BSP", L"*.pk3" },
        { L"Return to Castle Wolfenstein", L"*.mdc" },
        { L"Doom 3", L"*.md5*" },
        { L"Valve Model", L"*.smd;*.vta" },
        { L"Open Game Engine Exchange", L"*.ogx" },
        { L"Unreal", L"*.3d" },
        { L"BlitzBasic 3D", L"*.b3d" },
        { L"Quick3D", L"*.q3d;*.q3s" },
        { L"Neutral File Format", L"*.nff" },
        { L"Sense8 WorldToolKit", L"*.nff" },
        { L"Object File Format", L"*.off" },
        { L"PovRAY Raw", L"*.raw" },
        { L"Terragen Terrain", L"*.ter" },
        { L"Izware Nendo", L"*.ndo" },
        { L"All Files", L"*.*" }
    };
    // clang-format on

    ComPtr<IFileOpenDialog> pFileOpen;
    HRESULT     hr = CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS( &pFileOpen ) );

    if ( SUCCEEDED( hr ) )
    {
        // Create an event handling object, and hook it up to the dialog.
        // ComPtr<IFileDialogEvents> pDialogEvents;
        // hr = DialogEventHandler_CreateInstance( IID_PPV_ARGS( &pDialogEvents ) );

        if ( SUCCEEDED( hr ) )
        {
            // Setup filters.
            hr = pFileOpen->SetFileTypes( _countof( g_FileFilters ), g_FileFilters );
            pFileOpen->SetFileTypeIndex( 40 );  // All Files (*.*)

            // Show the open dialog box.
            if ( SUCCEEDED( pFileOpen->Show( m_Window->GetWindowHandle() ) ) )
            {
                ComPtr<IShellItem> pItem;
                if ( SUCCEEDED( pFileOpen->GetResult( &pItem ) ) )
                {
                    PWSTR pszFilePath;
                    if ( SUCCEEDED( pItem->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath ) ) )
                    {
                        // try to load the scene file (asynchronously).
                        m_LoadingTask =
                            std::async( std::launch::async, std::bind( &DirectX12Engine::LoadScene, this, pszFilePath ) );

                        CoTaskMemFree( pszFilePath );
                    }
                }
            }
        }
    }
}

