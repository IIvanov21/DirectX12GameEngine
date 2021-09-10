#include <Camera.h>
#include <DirectXMath.h>
using namespace DirectX;

Camera::Camera()
: m_ViewDirty( true )
, m_InverseViewDirty( true )
, m_ProjectionDirty( true )
, m_InverseProjectionDirty( true )
, m_vFoV( 45.0f )
, m_AspectRatio( 1.0f )
, m_zNear( 0.1f )
, m_zFar( 100.0f )
{
    pData                = (AlignedData*)_aligned_malloc( sizeof( AlignedData ), 16 );
    pData->m_Translation = XMVectorZero();
    pData->m_Rotation    = XMQuaternionIdentity();
    pData->m_FocalPoint  = XMVectorZero();
}

Camera::~Camera()
{
    _aligned_free( pData );
}

void XM_CALLCONV Camera::set_LookAt( FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up )
{
    pData->m_ViewMatrix = XMMatrixLookAtLH( eye, target, up );

    pData->m_Translation = eye;
    pData->m_Rotation    = XMQuaternionRotationMatrix( XMMatrixTranspose( pData->m_ViewMatrix ) );

    m_InverseViewDirty = true;
    m_ViewDirty        = false;
}

XMMATRIX Camera::get_ViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }
    return pData->m_ViewMatrix;
}

XMMATRIX Camera::get_InverseViewMatrix() const
{
    if ( m_ViewDirty || m_InverseViewDirty )
    {
        pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, get_ViewMatrix() );
        m_InverseViewDirty         = false;
    }

    return pData->m_InverseViewMatrix;
}




//-----------------------------------------------------------------------------
// Camera picking
//-----------------------------------------------------------------------------


XMVECTOR Camera::PixelFromWorldPt( DirectX::XMVECTOR PixelPoint, DirectX::XMVECTOR worldPoint, uint32_t ViewportWidth,
                               uint32_t ViewportHeight, bool& isBehind )
{
    DirectX::XMVECTOR viewportPoint =XMVectorSet( XMVectorGetX( worldPoint ), XMVectorGetY( worldPoint ), XMVectorGetZ( worldPoint ), 1.0f );
    XMMATRIX temp1 = get_ProjectionMatrix();
    XMMATRIX temp2 = get_ViewMatrix(); 
    pData->m_MatProView = XMMatrixMultiply( temp1, temp2 );

    viewportPoint = XMVector4Transform( viewportPoint, pData->m_ViewMatrix * pData->m_ProjectionMatrix );

    if ( XMVectorGetW( viewportPoint ) < 0 )
    {
        isBehind = false;
        return PixelPoint;
    }
    
    viewportPoint=XMVectorSetX( viewportPoint, XMVectorGetX( viewportPoint ) / XMVectorGetW( viewportPoint ) );
    viewportPoint=XMVectorSetY( viewportPoint, XMVectorGetY( viewportPoint ) / XMVectorGetW( viewportPoint ) );

    PixelPoint=XMVectorSetX( PixelPoint, fabsf(( XMVectorGetX( viewportPoint ) + 1.0f ) * ViewportWidth * 0.5f ));
    PixelPoint=XMVectorSetY( PixelPoint, fabsf(( 1.0f - XMVectorGetY( viewportPoint ) ) * ViewportHeight * 0.5f ));
    PixelPoint=XMVectorSetZ( PixelPoint, 0.0f );
    PixelPoint=XMVectorSetW( PixelPoint, 0.0f );

    isBehind = true;
    return PixelPoint;
}

/**
 * Return pixel coordinates corresponding to given world point when viewing from this
 * camera. Pass the viewport width and height. The returned XMVector3 contains the pixel
 * coordinates in x and y and the Z-distance to the world point in z. If the Z-distance
 * is less than the camera near clip , then the world
 * point is behind the camera and the 2D x and y coordinates are to be ignored.
 */
DirectX::XMVECTOR Camera::WorldPtFromPixel( DirectX::XMVECTOR entityVector, uint32_t viewportWidth, uint32_t viewportHeight )
{
    DirectX::XMVECTOR cameraPoint = XMVectorSet(0.0f,0.0f,0.0f,0.0f);
    cameraPoint=XMVectorSetX( cameraPoint, XMVectorGetX( entityVector ) / ( viewportWidth * 0.5f ) - 1.0f );
    cameraPoint=XMVectorSetY( cameraPoint, 1.0f - XMVectorGetY( entityVector ) / ( viewportHeight * 0.5f ) );
    cameraPoint=XMVectorSetZ( cameraPoint, 0.0f );
    cameraPoint=XMVectorSetW( cameraPoint, m_zNear );

    cameraPoint=XMVectorSetX( cameraPoint, XMVectorGetX( cameraPoint ) * XMVectorGetW( cameraPoint ) );
    cameraPoint=XMVectorSetY( cameraPoint, XMVectorGetY( cameraPoint ) * XMVectorGetW( cameraPoint ) );
    cameraPoint=XMVectorSetZ( cameraPoint, XMVectorGetZ( cameraPoint ) * XMVectorGetW( cameraPoint ) );

    XMMATRIX temp1               = get_ProjectionMatrix();
    XMMATRIX temp2               = get_ViewMatrix();
    pData->m_MatProView          = XMMatrixMultiply( temp1, temp2 );
    DirectX::XMVECTOR worldPoint = XMVector3Transform( cameraPoint, XMMatrixInverse( nullptr, pData->m_ProjectionMatrix ) );
    worldPoint = XMVectorSetW( worldPoint, 0.0f );
    return worldPoint;
}

void Camera::set_Projection( float fovy, float aspect, float zNear, float zFar )
{
    m_vFoV        = fovy;
    m_AspectRatio = aspect;
    m_zNear       = zNear;
    m_zFar        = zFar;

    m_ProjectionDirty        = true;
    m_InverseProjectionDirty = true;
}

XMMATRIX Camera::get_ProjectionMatrix() const
{
    if ( m_ProjectionDirty )
    {
        UpdateProjectionMatrix();
    }

    return pData->m_ProjectionMatrix;
}

XMMATRIX Camera::get_InverseProjectionMatrix() const
{
    if ( m_ProjectionDirty || m_InverseProjectionDirty )
    {
        UpdateInverseProjectionMatrix();
    }

    return pData->m_InverseProjectionMatrix;
}

void Camera::set_FoV( float fovy )
{
    if ( m_vFoV != fovy )
    {
        m_vFoV                   = fovy;
        m_ProjectionDirty        = true;
        m_InverseProjectionDirty = true;
    }
}

float Camera::get_FoV() const
{
    return m_vFoV;
}

void XM_CALLCONV Camera::set_Translation( FXMVECTOR translation )
{
    pData->m_Translation = translation;
    m_ViewDirty          = true;
}

XMVECTOR Camera::get_Translation() const
{
    return pData->m_Translation;
}

void XM_CALLCONV Camera::set_FocalPoint(DirectX::FXMVECTOR focalPoint) 
{
    pData->m_FocalPoint = focalPoint;
    m_ViewDirty         = true;
}

DirectX::XMVECTOR Camera::get_FocalPoint() const 
{
    return pData->m_FocalPoint;
}

void Camera::set_Rotation( FXMVECTOR rotation )
{
    pData->m_Rotation  = rotation;
    m_ViewDirty        = true;
}

XMVECTOR Camera::get_Rotation() const
{
    return pData->m_Rotation;
}



void XM_CALLCONV Camera::Translate( FXMVECTOR translation, Space space )
{
    switch ( space )
    {
    case Space::Local:
    {
        pData->m_Translation += XMVector3Rotate( translation, pData->m_Rotation );
    }
    break;
    case Space::World:
    {
        pData->m_Translation += translation;
    }
    break;
    }

    pData->m_Translation = XMVectorSetW( pData->m_Translation, 1.0f );

    m_ViewDirty        = true;
}

void XM_CALLCONV Camera::MoveFocalPoint( DirectX::FXMVECTOR focalPoint, Space space )
{
    switch ( space )
    {
    case Space::Local:
    {
        pData->m_FocalPoint += XMVector3Rotate( focalPoint, pData->m_Rotation );
    }
    break;
    case Space::World:
    {
        pData->m_FocalPoint += focalPoint;
    }
    break;
    }

    pData->m_FocalPoint = XMVectorSetW( pData->m_FocalPoint, 1.0f );

    m_ViewDirty = true;
}


void Camera::Rotate( FXMVECTOR quaternion )
{
    pData->m_Rotation = XMQuaternionMultiply( quaternion, pData->m_Rotation );

    m_ViewDirty        = true;
}

void Camera::UpdateViewMatrix() const
{
    XMMATRIX rotationMatrix    = XMMatrixTranspose( XMMatrixRotationQuaternion( pData->m_Rotation ) );
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector( -( pData->m_Translation ) );
    XMMATRIX focalMatrix       = XMMatrixTranslationFromVector( -( pData->m_FocalPoint ) );

    pData->m_ViewMatrix = focalMatrix * rotationMatrix * translationMatrix;

    m_InverseViewDirty = true;
    m_ViewDirty        = false;
}

void Camera::UpdateInverseViewMatrix() const
{
    if ( m_ViewDirty )
    {
        UpdateViewMatrix();
    }

    pData->m_InverseViewMatrix = XMMatrixInverse( nullptr, pData->m_ViewMatrix );
    m_InverseViewDirty         = false;
}

void Camera::UpdateProjectionMatrix() const
{
    pData->m_ProjectionMatrix =
        XMMatrixPerspectiveFovLH( XMConvertToRadians( m_vFoV ), m_AspectRatio, m_zNear, m_zFar );

    m_ProjectionDirty        = false;
    m_InverseProjectionDirty = true;
}

void Camera::UpdateInverseProjectionMatrix() const
{
    pData->m_InverseProjectionMatrix = XMMatrixInverse( nullptr, get_ProjectionMatrix() );
    m_InverseProjectionDirty         = false;
}
