#pragma once



#include "Camera.h"
#include "Light.h"

#include <GameFramework/GameFramework.h>

#include <dx12lib/RenderTarget.h>

#include <DirectXMath.h>

#include <string>

namespace DX12_Library
{
class CommandList;
class Device;
class GUI;
class Mesh;
class PipelineStateObject;
class RootSignature;
class Scene;
class ShaderResourceView;
class SwapChain;
class Texture;
}  // namespace DX12_Library

class Window;  // From GameFramework.

class DirectX12HDR
{
public:
    DirectX12HDR( const std::wstring& name, int width, int height, bool vSync = false );
    virtual ~DirectX12HDR();

    /**
     * Start the main game loop.
     */
    uint32_t Run();

    /**
     *  Load content required for the demo.
     */
    bool LoadContent();

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    void UnloadContent();

protected:
    /**
     *  Update the game logic.
     */
    void OnUpdate( UpdateEventArgs& e );

    /**
     *  Render stuff.
     */
    void OnRender();

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    void OnKeyPressed( KeyEventArgs& e );

    /**
     * Invoked when a key on the keyboard is released.
     */
    void OnKeyReleased( KeyEventArgs& e );

    /**
     * Invoked when the mouse is moved over the registered window.
     */
    void OnMouseMoved( MouseMotionEventArgs& e );

    /**
     * Invoked when the mouse wheel is scrolled while the registered window has focus.
     */
    void OnMouseWheel( MouseWheelEventArgs& e );

    void RescaleHDRRenderTarget( float scale );
    void OnResize( ResizeEventArgs& e );

    void OnDPIScaleChanged( DPIScaleEventArgs& e );

    void OnGUI( const std::shared_ptr<DX12_Library::CommandList>& commandList, const DX12_Library::RenderTarget& renderTarget );

private:
    std::shared_ptr<DX12_Library::Device>    m_Device;
    std::shared_ptr<DX12_Library::SwapChain> m_SwapChain;
    std::shared_ptr<DX12_Library::GUI>       m_GUI;

    std::shared_ptr<Window> m_Window;

    // Some geometry to render.
    std::shared_ptr<DX12_Library::Scene> m_Cube;
    std::shared_ptr<DX12_Library::Scene> m_Sphere;
    std::shared_ptr<DX12_Library::Scene> m_Cone;
    std::shared_ptr<DX12_Library::Scene> m_Cylinder;
    std::shared_ptr<DX12_Library::Scene> m_Torus;
    std::shared_ptr<DX12_Library::Scene> m_Plane;

    std::shared_ptr<DX12_Library::Scene> m_Skybox;

    std::shared_ptr<DX12_Library::Texture> m_DefaultTexture;
    std::shared_ptr<DX12_Library::Texture> m_BeKind;
    std::shared_ptr<DX12_Library::Texture> m_MinecraftTexture;
    std::shared_ptr<DX12_Library::Texture> m_MarsTexture;
    std::shared_ptr<DX12_Library::Texture> m_MineLisaTexture;
    std::shared_ptr<DX12_Library::Texture> m_EveningSun;
    std::shared_ptr<DX12_Library::Texture> m_EviningSunCubemap;
    std::shared_ptr<DX12_Library::ShaderResourceView> m_EviningSunCubemapSRV;

    // HDR Render target
    DX12_Library::RenderTarget m_HDRRenderTarget;
    std::shared_ptr<DX12_Library::Texture> m_HDRTexture;

    // Root signatures
    std::shared_ptr<DX12_Library::RootSignature> m_SkyboxSignature;
    std::shared_ptr<DX12_Library::RootSignature> m_HDRRootSignature;
    std::shared_ptr<DX12_Library::RootSignature> m_SDRRootSignature;

    // Pipeline state object.
    // Skybox PSO
    std::shared_ptr<DX12_Library::PipelineStateObject> m_SkyboxPipelineState;
    std::shared_ptr<DX12_Library::PipelineStateObject> m_HDRPipelineState;
    // HDR -> SDR tone mapping PSO.
    std::shared_ptr<DX12_Library::PipelineStateObject> m_SDRPipelineState;
    // Unlit pixel shader (for rendering the light sources)
    std::shared_ptr<DX12_Library::PipelineStateObject> m_UnlitPipelineState;

    D3D12_RECT m_ScissorRect;

    Camera m_Camera;
    struct alignas( 16 ) CameraData
    {
        DirectX::XMVECTOR m_InitialCamPos;
        DirectX::XMVECTOR m_InitialCamRot;
        float             m_InitialFov;
    };
    CameraData* m_pAlignedCameraData;

    // Camera controller
    float m_Forward;
    float m_Backward;
    float m_Left;
    float m_Right;
    float m_Up;
    float m_Down;

    float m_Pitch;
    float m_Yaw;

    // Rotate the lights in a circle.
    bool m_AnimateLights;
    // Set to true if the Shift key is pressed.
    bool m_Shift;

    int  m_Width;
    int  m_Height;
    bool m_VSync;
    bool m_Fullscreen;

    // Scale the HDR render target to a fraction of the window size.
    float m_RenderScale;

    // Define some lights.
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight>  m_SpotLights;

    Logger m_Logger;
};