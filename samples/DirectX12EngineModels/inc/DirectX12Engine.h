#pragma once
#include "Camera.h"
#include "CameraController.h"
#include "Light.h"

#include <GameFramework/GameFramework.h>

#include <dx12lib/RenderTarget.h>

#include <d3d12.h>  // For D3D12_RECT

#include <future>  // For std::future.
#include <memory>
#include <string>

namespace DX12_Library
{
class ShaderResourceView;
class CommandList;
class Device;
class GUI;
class PipelineStateObject;
class RenderTarget;
class RootSignature;
class Scene;
class SwapChain;
}  // namespace DX12_Library

class EffectPSO;

class DirectX12Engine
{
public:
    DirectX12Engine( const std::wstring& name, int width, int height, bool vSync = false );
    ~DirectX12Engine();

    /**
     * Start the main game loop.
     */
    uint32_t Run();

    /**
     * Load content requred for the demo.
     */
    void LoadContent();

    /**
     * Unload content that was loaded in LoadContent.
     */
    void UnloadContent();

    /**
     * Interact with models.
     */
    void SelectModel();

    void DeleteEntity();

    void NearestEntity();

protected:
    /**
     * Update game logic.
     */
    void OnUpdate( UpdateEventArgs& e );

    void RescaleHDRRenderTarget( float scale );

    /**
     * Window is being resized.
     */
    void OnResize( ResizeEventArgs& e );

    /**
     * Render stuff.
     */
    void OnRender();

    void OnRotateY( float amount );

    void OnRotateX( float amount );

    void OnRotateZ( float amount );

    void OnResetRotation();

    void OnScaleUp();

    void OnScaleDown();

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
    virtual void OnMouseMoved( MouseMotionEventArgs& e );

    /**
     * Handle DPI change events.
     */
    void OnDPIScaleChanged( DPIScaleEventArgs& e );

    /**
     * Render ImGUI stuff.
     */
    void OnGUI( const std::shared_ptr<DX12_Library::CommandList>& commandList, const DX12_Library::RenderTarget& renderTarget );

private:
    /**
     * Load all of the assets (scene file, shaders, etc...).
     * This is executed as an async task so that we can render a loading screen in the main thread.
     */
    bool LoadScene( const std::wstring& sceneFile );

    /**
     * Opens a File dialog and loads a new scene file.
     */
    void OpenFile();

    void SaveFile();


    /**
     * This function is called to report the loading progress of the scene. This is useful for updating the loading
     * progress bar.
     *
     * @param progress The loading progress (as a normalized float in the range [0...1].
     *
     * @returns true to continue loading or false to cancel loading.
     */
    bool LoadingProgress( float loadingProgress );

    std::shared_ptr<DX12_Library::Device>    m_Device;
    std::shared_ptr<DX12_Library::SwapChain> m_SwapChain;
    std::shared_ptr<DX12_Library::GUI>       m_GUI;

    std::shared_ptr<DX12_Library::Scene> m_Scene;


    std::shared_ptr<DX12_Library::Scene> m_Skybox;
    std::shared_ptr<DX12_Library::Texture>            m_GraceCathedralTexture;
    std::shared_ptr<DX12_Library::Texture>            m_GraceCathedralCubemap;
    std::shared_ptr<DX12_Library::ShaderResourceView> m_GraceCathedralCubemapSRV;

    // HDR Render target
    DX12_Library::RenderTarget             m_HDRRenderTarget;
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

    // Some scenes to represent the light sources.
    std::shared_ptr<DX12_Library::Scene> m_Sphere;
    std::shared_ptr<DX12_Library::Scene> m_Cone;
    std::shared_ptr<DX12_Library::Scene> m_Axis;
    std::shared_ptr<DX12_Library::Scene> m_Avocado;
    std::shared_ptr<DX12_Library::Scene> m_Ship;
    std::shared_ptr<DX12_Library::Scene> m_Tree1;
    std::shared_ptr<DX12_Library::Scene> m_Tree2;
    std::shared_ptr<DX12_Library::Scene> m_Tree3;
    std::shared_ptr<DX12_Library::Scene> m_Tree4;
    std::shared_ptr<DX12_Library::Scene> m_Tree5;
    std::shared_ptr<DX12_Library::Scene> m_Rock;
    std::shared_ptr<DX12_Library::Scene> m_Build;
    std::shared_ptr<DX12_Library::Scene> m_Ground;
    
    std::vector<std::shared_ptr<DX12_Library::Scene>> m_AssetsList;
    // Pipeline state object for rendering the scene.
    std::shared_ptr<EffectPSO> m_LightingPSO;
    std::shared_ptr<EffectPSO> m_DecalPSO;
    std::shared_ptr<EffectPSO> m_UnlitPSO;

    // Render target
    DX12_Library::RenderTarget m_RenderTarget;

    std::shared_ptr<Window> m_Window;

    D3D12_RECT     m_ScissorRect;
    D3D12_VIEWPORT m_Viewport;

    Camera           m_Camera;
    CameraController m_CameraController;
    Logger           m_Logger;

    int  m_Width;
    int  m_Height;
    bool m_VSync;

    // Define some lights.
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight>  m_SpotLights;
    std::vector<DirectionalLight> m_DirectionalLights;

    // Scale the HDR render target to a fraction of the window size.
    float m_RenderScale;

    // Rotate the lights in a circle.
    bool m_AnimateLights;
    bool              m_ModelSelected   = false;
    bool              m_ModelInteracted = false;
    bool              m_Fullscreen;
    bool              m_AllowFullscreenToggle;
    bool              m_ShowFileOpenDialog;
    bool              m_CancelLoading;
    bool              m_ShowControls;
    std::atomic_bool  m_IsLoading;
    std::future<bool> m_LoadingTask;
    float             m_LoadingProgress;
    std::string       m_LoadingText;
    float             m_MouseX;
    float             m_MouseY;

    float m_FPS;
};