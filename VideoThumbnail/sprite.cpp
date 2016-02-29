//////////////////////////////////////////////////////////////////////////
//
// Sprite: Manages drawing the thumbnail bitmap.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include "videothumbnail.h"
#include "sprite.h"

#include <math.h>
#include <float.h>
#include <wincodec.h>


D2D1_RECT_F LetterBoxRectF(D2D1_SIZE_F aspectRatio, const D2D1_RECT_F &rcDest);

const float WOBBLE_ANGLE = 10.0f;
const float WOBBLE_DECAY = 0.25f;



inline D2D1_RECT_F operator+(const D2D1_RECT_F& r1, const D2D1_RECT_F& r2)
{
    return D2D1::RectF( r1.left + r2.left, r1.top + r2.top, r1.right + r2.right, r1.bottom + r2.bottom );
}

inline D2D1_RECT_F operator-(const D2D1_RECT_F& r1, const D2D1_RECT_F& r2)
{
    return D2D1::RectF( r1.left - r2.left, r1.top - r2.top, r1.right - r2.right, r1.bottom - r2.bottom );
}

inline D2D1_RECT_F operator*(const D2D1_RECT_F& r1, float scale)
{
    return D2D1::RectF( r1.left * scale, r1.top * scale, r1.right * scale, r1.bottom * scale );
}


inline float Width(const D2D1_RECT_F& rect)
{
    return rect.right - rect.left;
}

inline float Height(const D2D1_RECT_F& rect)
{
    return rect.bottom - rect.top;
}




//-------------------------------------------------------------------
// Sprite constructor
//-------------------------------------------------------------------

Sprite::Sprite()
 :  m_pBitmap(NULL),
    m_bAnimating(FALSE),
    m_timeStart(0),
    m_timeEnd(0),
    m_fAngle(0),
    m_theta(0),
    m_bTopDown(FALSE)
{
}

//-------------------------------------------------------------------
// Sprite destructor
//-------------------------------------------------------------------

Sprite::~Sprite()
{
    if (m_pBitmap)
    {
        m_pBitmap->Release();
    }
}


//-------------------------------------------------------------------
// SetBitmap: Sets the bitmap for the sprite.
//-------------------------------------------------------------------

void Sprite::SetBitmap(ID2D1Bitmap *pBitmap, const FormatInfo& format)
{
    SafeRelease(&m_pBitmap);

    if (pBitmap)
    {
        m_pBitmap = pBitmap;
        m_pBitmap->AddRef();
    }

    m_bTopDown = format.bTopDown;

    m_fill = m_nrcBound = D2D1::Rect<float>(0, 0, 0, 0);

	m_rotation = format.rotation;

    m_AspectRatio = D2D1::SizeF( (float)format.rcPicture.right, (float)format.rcPicture.bottom );
    m_sourceRect = D2D1::RectF((float)format.rcPicture.left, (float)format.rcPicture.top,
        (float)format.rcPicture.right, (float)format.rcPicture.bottom);
}

//extern "C"
//{
//	const GUID IID_IWICImagingFactory = { 0xec5ec8a9, 0xc395, 0x4314, 0x9c, 0x77, 0x54, 0xd7, 0xa9, 0x35, 0xff, 0x70 };
//	//const GUID CLSID_WICImagingFactory = { 0xcacaf262, 0x9370, 0x4615, 0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0xa };
//}

void Sprite::Save(LPCWSTR filePath, ID2D1RenderTarget *pRT, ID2D1Factory* pD2DFactory, WICRect destSize)
{
	HRESULT hr = S_OK;

	IWICImagingFactory *pWICFactory = NULL;	
	IWICBitmap *pWICBitmap = NULL;
	
	ID2D1RenderTarget *pWicRT = NULL;
	IWICBitmapEncoder *pEncoder = NULL;
	IWICBitmapFrameEncode *pFrameEncode = NULL;
	IWICStream *pStream = NULL;

	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&pWICFactory
		);	

	////
	//// Create IWICBitmap and RT
	////

	UINT sc_bitmapWidth = (UINT)(m_pBitmap->GetSize().width);
	UINT sc_bitmapHeight = (UINT)(m_pBitmap->GetSize().height);

	if (SUCCEEDED(hr))
	{		
		hr = pWICFactory->CreateBitmap(
			sc_bitmapWidth,
			sc_bitmapHeight,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapCacheOnLoad,
			&pWICBitmap
			);
	}

	if (SUCCEEDED(hr))
	{
		D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
		rtProps.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		rtProps.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;
		rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
		hr = pD2DFactory->CreateWicBitmapRenderTarget(
			pWICBitmap,
			&rtProps,
			&pWicRT
			);
	}

	ID2D1Bitmap* newBmp = NULL;
	D2D1_BITMAP_PROPERTIES bp2 = D2D1::BitmapProperties();
	bp2.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
	bp2.dpiX = bp2.dpiY = 0.0f;

	/*hr = converter->Initialize(
		scaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
		nullptr, 0.f, WICBitmapPaletteTypeMedianCut);*/
	//hr = pWICFactory->CreateBitmapFromSource(scaler, WICBitmapCacheOnLoad, &wicBitmap);
	
	
	//hr = scaler->CopyPixels(nullptr, 276 * 32, 276 * 32 * 276, reinterpret_cast<BYTE*>(pvImageBits));
	//hr = pWICFactory->CreateBitmapFromMemory(276, 276, GUID_WICPixelFormat32bppPBGRA, 276 * 32, 276 * 276 * 32, reinterpret_cast<BYTE*>(pvImageBits), &wicBitmap);


	//create scaler
	IWICBitmapScaler *scaler;
	IWICBitmapClipper *clipper;
	hr = pWICFactory->CreateBitmapScaler(&scaler);
	hr = pWICFactory->CreateBitmapClipper(&clipper);
	
	UINT destinationWidth = sc_bitmapWidth;
	UINT destinationHeight = sc_bitmapHeight;
	if (sc_bitmapWidth > sc_bitmapHeight)
	{
		destinationWidth = sc_bitmapHeight;
	}
	else
	{
		destinationHeight = sc_bitmapWidth;
	}
	//FLOAT scalar = static_cast<FLOAT>(276) / static_cast<FLOAT>(sc_bitmapHeight);
	//UINT destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(sc_bitmapWidth));
	//scalar = static_cast<FLOAT>(276) / static_cast<FLOAT>(sc_bitmapWidth);
	//UINT destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(sc_bitmapHeight));

	WICRect rcClip = { 0, 0, destinationWidth, destinationHeight };
	//WICRect rcScale = { 0, 0, 276, 276 };

	hr = clipper->Initialize(pWICBitmap, &rcClip);
	hr = scaler->Initialize(clipper, destSize.Width, destSize.Height, WICBitmapInterpolationModeFant);

	//if we need to rotate the thumb - create flipper
	IWICBitmapFlipRotator *flipper;
	if (m_rotation != MFVideoRotationFormat_0)
	{		
		hr = pWICFactory->CreateBitmapFlipRotator(&flipper);
		WICBitmapTransformOptions opt;
		switch (m_rotation)
		{
			case MFVideoRotationFormat_180:
				opt = WICBitmapTransformRotate180;
				break;
			case MFVideoRotationFormat_90:
				opt = WICBitmapTransformRotate90;
				break;
			case MFVideoRotationFormat_270:
				opt = WICBitmapTransformRotate270;
				break;
		}
		hr = flipper->Initialize(clipper, opt);
	}

	if (SUCCEEDED(hr))
	{
		//
		// Render into the bitmap
		//
		pWicRT->BeginDraw();

		pWicRT->Clear(D2D1::ColorF(D2D1::ColorF::White));


		/*	D2D1_SIZE_F sizeBitmap = m_AspectRatio;

		D2D1_SIZE_F sizeRT = pRT->GetSize();

		D2D1_RECT_F rect = D2D1::RectF();

		rect.right = Width(m_nrcBound) * sizeRT.width;
		rect.bottom = Height(m_nrcBound) * sizeRT.height;*/
		//pWicRT->DrawBitmap(m_pBitmap, LetterBoxRectF(m_AspectRatio, rect), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rect);
		pWicRT->DrawBitmap(m_pBitmap);


		hr = pWicRT->EndDraw();
	}	

	if (SUCCEEDED(hr))
	{

		//
		// Save image to file
		//
		hr = pWICFactory->CreateStream(&pStream);
	}

	WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
	if (SUCCEEDED(hr))
	{

		hr = pStream->InitializeFromFilename(filePath, GENERIC_WRITE);
	}
	if (SUCCEEDED(hr))
	{
		hr = pWICFactory->CreateEncoder(GUID_ContainerFormatJpeg, NULL, &pEncoder);
	}
	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
	}
	if (SUCCEEDED(hr))
	{
		hr = pEncoder->CreateNewFrame(&pFrameEncode, NULL);
	}
	if (SUCCEEDED(hr))
	{
		hr = pFrameEncode->Initialize(NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pFrameEncode->SetResolution(96, 96);
		hr = pFrameEncode->SetSize(destSize.Width, destSize.Height);
	}
	if (SUCCEEDED(hr))
	{
		hr = pFrameEncode->SetPixelFormat(&format);
	}
	if (SUCCEEDED(hr))
	{
		/*auto rect = WICRect();
		rect.Height = 276;
		rect.Width = 276;
		rect.X = 100;
		rect.Y = 100;*/
		if (m_rotation == MFVideoRotationFormat_0)
			hr = pFrameEncode->WriteSource(scaler, &destSize);
		else
			hr = pFrameEncode->WriteSource(flipper, &destSize);
	}
	if (SUCCEEDED(hr))
	{
		hr = pFrameEncode->Commit();
	}
	if (SUCCEEDED(hr))
	{
		hr = pEncoder->Commit();
	}

	SafeRelease(&pWICFactory);
	SafeRelease(&pWICBitmap);	
	SafeRelease(&pEncoder);
	SafeRelease(&pFrameEncode);
	SafeRelease(&pStream);
	SafeRelease(&pWicRT);
	SafeRelease(&clipper);
	SafeRelease(&scaler);
	if (m_rotation != MFVideoRotationFormat_0)
		SafeRelease(&flipper);
}

//-------------------------------------------------------------------
// Clear: Clears the bitmap.
//-------------------------------------------------------------------

void Sprite::Clear()
{
    SafeRelease(&m_pBitmap);

    m_fill = m_nrcBound = D2D1::Rect<float>(0, 0, 0, 0);

    m_AspectRatio = D2D1::SizeF(1, 1);
}


//-------------------------------------------------------------------
// AnimateBoundingBox
//
// Applies an animation path to the sprite.
//
// bound2: Final position of the bounding box, as a normalized rect.
// time: Current clock time.
// duration: Length of the animation, in seconds.
//-------------------------------------------------------------------

void Sprite::AnimateBoundingBox(const D2D1_RECT_F& bound2, float time, float duration)
{
    if (duration == 0.0f)
    {
        // Apply the new position immediately

        m_nrcBound = bound2;
        m_bAnimating = FALSE;
        m_fAngle = 0.0f;
    }
    else
    {
        // Set the animation parameters.

        m_timeStart = time;
        m_timeEnd = time + duration;

        m_nrcAnimDistance = bound2 - m_nrcBound;
        m_nrcStartPosition = m_nrcBound;

        m_fAngle = WOBBLE_ANGLE;

        m_bAnimating = TRUE;
    }
}


//-------------------------------------------------------------------
// Update: Updates the sprite, based on the clock time.
//-------------------------------------------------------------------

void Sprite::Update(ID2D1HwndRenderTarget *pRT, float time)
{
    if (GetState() == CLEAR)
    {
        return; // No bitmap; nothing to do.
    }

    if ((m_timeStart < time) && (m_timeEnd > time))
    {
        // We are in the middle of an animation. Interpolate the position.

        D2D1_RECT_F v = m_nrcAnimDistance * ( (time - m_timeStart) / (m_timeEnd - m_timeStart) );

        m_nrcBound = v + m_nrcStartPosition;
    }
    else if (m_bAnimating && time >= m_timeEnd)
    {
        // We have reached the end of an animation.
        // Set the final position (avoids any rounding errors)

        m_nrcBound = m_nrcStartPosition + m_nrcAnimDistance;
        m_bAnimating = FALSE;
    }

    // Compute the correct letterbox for the bitmap.
    //
    // TODO: Strictly, this only needs to be update if the bitmap changes
    //       or the size of the render target changes.

    D2D1_SIZE_F sizeBitmap = m_AspectRatio;

    D2D1_SIZE_F sizeRT = pRT->GetSize();

    D2D1_RECT_F rect = D2D1::RectF();

    rect.right = Width(m_nrcBound) * sizeRT.width;
    rect.bottom = Height(m_nrcBound) * sizeRT.height;

    m_fill = LetterBoxRectF(sizeBitmap, rect);
}


//-------------------------------------------------------------------
// Draw: Draws the sprite.
//-------------------------------------------------------------------

void Sprite::Draw(ID2D1HwndRenderTarget *pRT)
{
    if (GetState() == CLEAR)
    {
        return; // No bitmap; nothing to do.
    }

    D2D1_SIZE_F sizeRT = pRT->GetSize();

    const float width = Width(m_nrcBound) * sizeRT.width;
    const float height = Height(m_nrcBound) * sizeRT.height;

    // Start with an identity transform.
    D2D1::Matrix3x2F  mat = D2D1::Matrix3x2F::Identity();

    // If the image is bottom-up, flip around the x-axis.
    if (m_bTopDown == 0)
    {
        mat = D2D1::Matrix3x2F::Scale( D2D1::SizeF(1, -1), D2D1::Point2F(0, height/2) );
    }

    // Apply wobble.
    if (m_fAngle >= FLT_EPSILON)
    {
        mat = mat * D2D1::Matrix3x2F::Rotation( m_fAngle * sinf(m_theta) , D2D1::Point2F( width/2, height/2 ) );

        // Reduce the wobble by the decay factor...
        m_theta += WOBBLE_DECAY;

        m_fAngle -= WOBBLE_DECAY;

        if (m_fAngle <= FLT_EPSILON)
        {
            m_fAngle = 0.0f;
        }
    }

    // Now translate the image relative to the bounding box.
    mat = mat * D2D1::Matrix3x2F::Translation(  m_nrcBound.left * sizeRT.width, m_nrcBound.top * sizeRT.height );

    pRT->SetTransform(mat);

    m_mat = mat;

//    pRT->DrawBitmap(m_pBitmap, m_fill);

    pRT->DrawBitmap(m_pBitmap, m_fill, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_sourceRect);
}


//-------------------------------------------------------------------
// HitTest: Returns true if (x,y) falls within the bitmap.
//-------------------------------------------------------------------

BOOL Sprite::HitTest(int x, int y)
{
    D2D1::Matrix3x2F mat = m_mat;

    // Use the inverse of our current transform matrix to transform the
    // point (x,y) from render-target space to model space.

    mat.Invert();

    D2D1_POINT_2F pt = mat.TransformPoint( D2D1::Point2F((float)x, (float)y) );

    if (pt.x >= m_fill.left && pt.x <= m_fill.right && pt.y >= m_fill.top && pt.y <= m_fill.bottom)
    {
        return true;
    }
    else
    {
        return false;
    }
}



//-------------------------------------------------------------------
// LetterBoxRectF
//
// Given a destination rectangle (rcDest) and an aspect ratio,
// returns a letterboxed rectangle within rcDest.
//-------------------------------------------------------------------

D2D1_RECT_F LetterBoxRectF(D2D1_SIZE_F aspectRatio, const D2D1_RECT_F &rcDest)
{
    float width, height;

    float SrcWidth = aspectRatio.width;
    float SrcHeight = aspectRatio.height;
    float DestWidth = Width(rcDest);
    float DestHeight = Height(rcDest);

    D2D1_RECT_F rcResult = D2D1::RectF();

    // Avoid divide by zero (even though MulDiv handles this)
    if (SrcWidth == 0 || SrcHeight == 0)
    {
        return rcResult;
    }

   // First try: Letterbox along the sides. ("pillarbox")
    width = DestHeight * SrcWidth / SrcHeight;
    height = DestHeight;
    if (width > DestWidth)
    {
        // Letterbox along the top and bottom.
        width = DestWidth;
        height = DestWidth * SrcHeight / SrcWidth;
    }

    // Fill in the rectangle

    rcResult.left = rcDest.left + ((DestWidth - width) / 2);
    rcResult.right = rcResult.left + width;
    rcResult.top = rcDest.top + ((DestHeight - height) / 2);
    rcResult.bottom = rcResult.top + height;

    return rcResult;
}
