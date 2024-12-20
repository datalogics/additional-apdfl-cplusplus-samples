//
// Copyright (c) 2007-2018, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// The RenderPage sample program shows how to render a PDF document page to memory.
//
// For more detail see the description of the RenderPage sample program on our Developer’s site, 
// http://dev.datalogics.com/adobe-pdf-library/sample-program-descriptions/c1samples#renderpage

#include "RenderPage.h"
#include <math.h>
#include <assert.h>
#include <time.h>
#include <iostream>

#include "ASCalls.h"
#include "PDCalls.h"
#include "DLExtrasCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"

// These are utility routines to convert Rects and Matrices between ASDouble and
// ASReal, and ASFixed.
// ASFixed was the original method of specifing "real" numbers in APDFL. It is still widely present in APDFL interfaces, though it is 
//   limited by both it's resolution (0.0001 typically) and it range (+- 32767). There is a full complement of methods for combining
//   matrices, and transforming and comparing rectangles. Internal to APDFL, ASFixed is now seldom used.
// ASReal was introduced into APDFL later. It is implemented as a float, and it does not have the full 
//   complement of transform methods. It is used here because it is needed in the interface to PDPageDrawContentsToMemoryWithParams.
// ASDouble was introduced most recently. It has a full complement of transformation methods. Many interfaces to APDFL have been updated
//   (Generally by the addition of "Ex" to the interface name) to provide/accept such values.
//
//  However, conversion between these forms is not always supplied. These routines provided the conversions needed for this sample.
static void ASDoubleRectToASReal (ASRealRect &out, ASDoubleRect &in)
{
    out.left = (ASReal)in.left;
    out.right = (ASReal)in.right;
    out.top = (ASReal)in.top;
    out.bottom = (ASReal)in.bottom;
}

static void ASDoubleMatrixToASReal (ASRealMatrix &out, ASDoubleMatrix &in)
{
    out.a = (ASReal)in.a;
    out.b = (ASReal)in.b;
    out.c = (ASReal)in.c;
    out.d = (ASReal)in.d;
    out.tx = (ASReal)in.h;
    out.ty = (ASReal)in.v;
}

static void ASDoubleMatrixToASFixed(ASFixedMatrix &out, ASDoubleMatrix &in)
{
	out.a = FloatToASFixed(in.a);
	out.b = FloatToASFixed(in.b);
	out.c = FloatToASFixed(in.c);
	out.d = FloatToASFixed(in.d);
	out.h = FloatToASFixed(in.h);
	out.v = FloatToASFixed(in.v);
}


static void ASFixedRectToASDouble (ASDoubleRect &out, ASFixedRect &in)
{
    out.left = ASFixedToFloat (in.left);
    out.right = ASFixedToFloat (in.right);
    out.top = ASFixedToFloat (in.top);
    out.bottom = ASFixedToFloat (in.bottom);
}

static void ASFixedMatrixToASDouble (ASDoubleMatrix &out, ASFixedMatrix &in)
{
    out.a = ASFixedToFloat (in.a);
    out.b = ASFixedToFloat (in.b);
    out.c = ASFixedToFloat (in.c);
    out.d = ASFixedToFloat (in.d);
    out.h = ASFixedToFloat (in.h);
    out.v = ASFixedToFloat (in.v);
}

static void ASDoubleToFixedRect (ASFixedRect &out, ASDoubleRect &in)
{
    out.left = FloatToASFixed (in.left);
    out.right = FloatToASFixed (in.right);
    out.top = FloatToASFixed (in.top);
    out.bottom = FloatToASFixed (in.bottom);
}

static void flipBytes(void *start, int len)
{
	unsigned char byteHolder;
	unsigned char *pos = (unsigned char *)start,
		*end = (unsigned char *)start + (len - 1);

	if (!start || len <= 1)
		return;

	while (pos < end)
	{
		byteHolder = *end;
		*end = *pos;
		*pos = byteHolder;
		++pos;
		--end;
	}
}


RenderPageParams::RenderPageParams()
{
	bitspercomp = 8;
	ri = AC_RelColorimetric;
	colorSpace = NULL;
	bVerbose = FALSE;
	drawflags = 0;
	smoothflags = 0;
	nComps = 1;
	numfilters = 0;
	res = 72;
	matrix = NULL;
	destRect = NULL;
	ocContext = NULL;
}

RenderPageParams::~RenderPageParams()
{
}

void RenderPageParams::SetColorSpace(const char *colorSpace)
{
	SetColorSpace(ASAtomFromString(colorSpace));
}

void RenderPageParams::SetColorSpace(ASAtom atmColorSpace)
{
	static const ASAtom atmDeviceRGB = ASAtomFromString("DeviceRGB");
	static const ASAtom atmDeviceCMYK = ASAtomFromString("DeviceCMYK");
	static const ASAtom atmDeviceGray = ASAtomFromString("DeviceGray");
	static const ASAtom atmDeviceRGBA = ASAtomFromString("DeviceRGBA");

	csAtom = atmColorSpace;

	if (csAtom == atmDeviceGray)
	{
		nComps = 1;
	}
	else if (csAtom == atmDeviceRGB)
	{
		nComps = 3;
	}
	else if (csAtom == atmDeviceCMYK)
	{
		nComps = 4;
	}
	else if (csAtom == atmDeviceRGBA)
	{
		nComps = 4;
	}
	else
	{
		// Not a valid/currently supported colorspace
		ASRaise(genErrBadParm);
	}

}
ASAtom RenderPageParams::ColorSpaceName() const
{
	return csAtom;
}

ASInt32	RenderPageParams::NumComps() const
{
	return nComps;
}

void RenderPageParams::setBitsPerComponents(ASInt32 bpc)
{
	bitspercomp = bpc;
	if (csAtom == ASAtomFromString("DeviceRGB") || csAtom == ASAtomFromString("DeviceCMYK") || csAtom == ASAtomFromString("DeviceRGBA"))
	{
		if (bitspercomp != 8)
		{
			// Reset to 8
			bitspercomp = 8;
		}
	}
	else if (csAtom == ASAtomFromString("DeviceGray"))
	{
		if ((bitspercomp != 1) && (bitspercomp != 8) && (bitspercomp != 24))
		{
			// Reset to 8
			bitspercomp = 8;
		}
	}
}

ASInt32	RenderPageParams::BitsPerComponent() const
{
	return bitspercomp;
}

void RenderPageParams::setRenderIntent(AC_RenderIntent renderIntent)
{
	ri = renderIntent;
}

AC_RenderIntent RenderPageParams::RenderIntent() const
{
	return ri;
}

void RenderPageParams::setResolution(double resolution)
{
	//Set resolution.
//  A PDF "unit" is, by default, 1/72nd of an inch. So a resolution of 72 is one PDF "unit" per pixel. 
//  if no resolution is set, we will use 72 DPI as our image reslution. This sample does not attempt to
//  support different horiziontal and vertical resolutions. APDFL, can easily support them by using a 
//  different scale factor in the scale matrix "a" (horiziontal) and "d" (vertical) members. The scale 
//  factors are simply (72.0 / resolution).
	res = 72.0;
	if (resolution > 0.0)
		res = resolution;
}

double RenderPageParams::Resolution() const
{
	return res;
}

void RenderPageParams::setSmoothFlags(ASUns32 smoothFlags)
{
	smoothflags = smoothFlags;
}

ASUns32	RenderPageParams::SmoothFlags() const
{
	return smoothflags;
}

void RenderPageParams::setDrawFlags(ASUns32 drawFlags)
{
	drawflags = drawFlags;
}

ASUns32	RenderPageParams::DrawFlags() const
{
	return drawflags;
}

void RenderPageParams::setOCContext(PDOCContext context)
{
	ocContext = context;
}

PDOCContext RenderPageParams::OCContext() const
{
	return ocContext;
}

void RenderPageParams::setMatrix(ASDoubleMatrix* m)
{
	matrix = m;
}

ASDoubleMatrix* RenderPageParams::getMatrix()
{
	return matrix;
}

void RenderPageParams::setDestRect(ASDoubleRect* r)
{
	destRect = r;
}

ASDoubleRect* RenderPageParams::getDestRect()
{
	return destRect;
}

void RenderPageParams::setOutputProfile(AC_Profile profile)
{
	outputProfile = profile;
}

AC_Profile RenderPageParams::getOutputProfile() const
{
	if (outputProfile != nullptr)
		return outputProfile;
	else
		return NULL;
}

void RenderPageParams::setVerbose(ASBool verbose)
{
	bVerbose = verbose;
}

ASBool RenderPageParams::verbose() const
{
	return bVerbose;
}

static ASBool renderpageProgressProc(float current, const char *name, ASInt32
	/* it's a PDFLFlattenProgressMarker (see PDFLPrint.h) */ stage, void *clientData)
{
	std::cout << name << " stage " << stage << ": " << current * 100 << "%" << std::endl;
	return TRUE;
}

static ASBool renderpageCancelProc(void *clientData)
{
	std::cout << ".";
	return false;
}
// This both constructs the RenderPage object, and creates the page rendering. 
//  The rendered page can be accessed as a bitmap via the methods GetImageBuffer() and GetImageBufferSize(), or as a PDEImage, 
//  via the method GetPDEImage(). The PDEImage creation will be deferred until it is requested.
RenderPage::RenderPage(PDPage& pdPage, ASFixedRect* fixedUpdateRect, RenderPageParams* inParms)
{
	parms = inParms;
	clock_t start_time[2] = { 0,0 }, stop_time[2] = { 0,0 };

	ASDoubleRect updateRect;
	ASFixedRectToASDouble(updateRect, *fixedUpdateRect);

	//Set resolution.
	//  A PDF "unit" is, by default, 1/72nd of an inch. So a resolution of 72 is one PDF "unit" per pixel. 
	//  if no resolution is set, we will use 72 DPI as our image reslution. This sample does not attempt to
	//  support different horizontal and vertical resolutions. APDFL, can easily support them by using a 
	//  different scale factor in the scale matrix "a" (horizontal) and "d" (vertical) members. The scale 
	//  factors are simply (resolution /72.0).
	double scaleFactor = parms->Resolution() / 72.0;

	//Get the colorspace atom, set the number of components per colorspace
	//  and store the appropriate colorspace for an output PDEImage
	//  This sample will raise an exception if the color space is not one of
	//  DeviceGray, DeviceRGB, or DeviceCMYK. APDFL supports a number of additional
	//  color spaces. The image created may also be conformed to a given ICC Profile.
	csAtom = parms->ColorSpaceName();
	nComps = parms->NumComps();
	// initialize the output colorspace for the PDEImage we'll generate in MakePDEImage
	cs = PDEColorSpaceCreateFromName(csAtom);

	//The size of each color component to be represented in the image.
	bpc = parms->BitsPerComponent();

	// Set up attributes for the PDEImage to be made by GetPDEImage
	//   Height and Width in pixels will be added as they are known.
	memset(&attrs, 0, sizeof(PDEImageAttrs));
	attrs.flags = kPDEImageExternal;
	attrs.bitsPerComponent = bpc;

	// Get the matrix that transforms user space coordinates to Image coordinates, taking into account page 
	// rotation. Note that page rotation is clockwise, so pdRotate90 is effectively a rotation of -90 degrees. 
	// Also note that Page coordinates have their origin in the lower-left while image coordinates have their 
	// origin in the upper-right, so the matrix must also mirror vertically.
	PDRotate rotation = PDPageGetRotate(pdPage);
	ASDoubleMatrix updateMatrix = { 1,0,0,1,0,0 };
	if (parms->getMatrix() != NULL)
	{
		ASDoubleMatrix* srcM = parms->getMatrix();
		updateMatrix.a = srcM->a;
		updateMatrix.b = srcM->b;
		updateMatrix.c = srcM->c;
		updateMatrix.d = srcM->d;
		updateMatrix.h = srcM->h;
		updateMatrix.v = srcM->v;
	}
	else
	{
		ASFixedRect* fRectP = fixedUpdateRect;
		ASDoubleMatrix flipMatrix = { 1, 0, 0, -1, 0, ASFixedToFloat(fRectP->top - fRectP->bottom) };

		ASDoubleMatrix scaleMatrix = { scaleFactor, 0, 0, scaleFactor, 0, 0 };

		switch (rotation)
		{
			//Note, rotation is clockwise.
		case pdRotate0:
			updateMatrix = { 1, 0, 0, 1, -ASFixedToFloat(fRectP->left), -ASFixedToFloat(fRectP->bottom) };
			break;
		case pdRotate90:
			updateMatrix = { 0, -1, 1, 0, -ASFixedToFloat(fRectP->bottom), ASFixedToFloat(fRectP->right) };
			flipMatrix.v = ASFixedToFloat(fRectP->right - fRectP->left);
			break;
		case pdRotate180:
			updateMatrix = { -1, 0, 0, -1, ASFixedToFloat(fRectP->right), ASFixedToFloat(fRectP->top) };
			break;
		case pdRotate270:
			updateMatrix = { 0, 1, -1, 0, ASFixedToFloat(fRectP->top), -ASFixedToFloat(fRectP->left) };
			flipMatrix.v = ASFixedToFloat(fRectP->right - fRectP->left);
			break;
		}
		ASDoubleMatrixConcat(&updateMatrix, &flipMatrix, &updateMatrix);
		ASDoubleMatrixConcat(&updateMatrix, &scaleMatrix, &updateMatrix);
	}

	// Set up the destination rectangle. 
	// This is a description of the image in pixels, so it will always
	// have it's origin at 0,0.
	ASDoubleRect doubleDestRect = { 0,0,0,0 };
	ASRealRect realDestRect;
	if (parms->getDestRect() != NULL)
	{
		ASDoubleRect* src = parms->getDestRect();

		doubleDestRect.left = src->left;
		doubleDestRect.bottom = src->bottom;
		doubleDestRect.right = src->right;
		doubleDestRect.top = src->top;
	}
	else {
		ASDoubleMatrixTransformRect(&doubleDestRect, &updateMatrix, &updateRect);
	}
	ASDoubleRectToASReal(realDestRect, doubleDestRect);

	assert(((ASInt32)realDestRect.left) == 0);
	assert(((ASInt32)realDestRect.bottom) == 0);

	attrs.width = (ASInt32)floor(realDestRect.right + 0.5);
	attrs.height = (ASInt32)floor(realDestRect.top + 0.5);

	// This is a bit clumsy, because features were added over time. The matrices and rectangles in this
	// interface use ASReal as their base, rather than ASDouble. But there is not a complete set of 
	// concatenation and transformation methods for ASReal. So we generally generate the matrix and 
	// rectangle values using ASDouble, and convert to ASReal. 
	ASRealRect realUpdateRect;
	ASRealMatrix realUpdateMatrix;
	ASDoubleRectToASReal(realUpdateRect, updateRect);
	ASDoubleMatrixToASReal(realUpdateMatrix, updateMatrix);

	//Allocate the buffer for storing the rendered page content
	// It is important that ALL of the flags and options used in the actual draw be set the same here!
	// Calling this interface with drawParms.bufferSize or drawParams.buffer equal to zero will return the size of the buffer
	//   needed to contain this image. This is the most certain way to get the correct buffer size. If we call this routine with a buffer
	//   that is not large enough to contain the image, we will not draw the image, but will simply, silently, return the size of the buffer
	//   needed!

	// "Best Practice" is to use PDPageDrawContentsToMemoryWithParams, as it allows
		// the matrix and rects to be specified in floating point, eliminating the need
		// to test for ASFixed Overflows.
	PDPageDrawMParamsRec drawParams;
	memset(&drawParams, 0, sizeof(PDPageDrawMParamsRec));
	drawParams.size = sizeof(PDPageDrawMParamsRec);
	drawParams.csAtom = csAtom;
	drawParams.bpc = bpc;
	drawParams.clientOCContext = parms->OCContext();
	drawParams.iccProfile = parms->getOutputProfile();

	// For this example we will smooth (anti-alias) all of the marks. For a given application,
	// this may or may not be desireable. See the enumeration PDPageDrawSmoothFlags for the full set of options.
	drawParams.smoothFlags = parms->SmoothFlags();

	// The DoLazyErase flag is usually, if not always turned on, UseAnnotFaces will cause annotations
	// in the page to be displayed, and kPDPageDsiplayOverprintPreview will display the page showing 
	// overprinting. The precise meaning of these flags, as well as others that may be used here, can be
	// seen in the definition of PDPageDrawFlags
	drawParams.flags = parms->DrawFlags();

	drawParams.renderIntent = parms->RenderIntent();

	drawParams.asRealDestRect = &realDestRect;                // This is where the image is drawn on the resultant bitmap.
	//   It is generally set at 0, 0 and width/height in pixels.
	drawParams.asRealUpdateRect = &realUpdateRect;           // This is the portion of the document to be drawn. If omitted, 
	// it will be the document media box, which is generally what is wanted.
	drawParams.asRealMatrix = &realUpdateMatrix;             // the matrix is used to translate coordinates within the UpdateRect to pixels in the DestRect.

	drawParams.progressProc = parms->verbose() ? renderpageProgressProc : NULL;
	drawParams.cancelProc = parms->verbose() ? renderpageCancelProc : NULL;

	// Additional values in this record control such features as drawing separations, 
	// specifiying a desired output profile, selecting optional content, and providing for 
	// a progress reporting callback.

	bufferSize = PDPageDrawContentsToMemoryWithParams(pdPage, &drawParams);   // This call, with a NULL buffer pointer, returns needed buffer size

	//  One frequent failure point in rendering images is being unable to allocate sufficient contiguous space 
	//  for the bitmap buffer. Here, that will be indicated by a zero value for drawParams.buffer after the 
	//  call to malloc. If the buffer size is larger than the internal limit of malloc, it may also raise an
	//  interupt! Catch these conditions here, and raise an out of memory error to the caller.
	try
	{
		buffer = (char*)ASmalloc(bufferSize);
		if (!buffer)
			ASRaise(genErrNoMemory);
		memset(buffer, 0x7F, bufferSize);
	}
	catch (...)
	{
		ASRaise(genErrNoMemory);
	}

	static const ASAtom atmDeviceRGBA = ASAtomFromString("DeviceRGBA");
	if (csAtom == atmDeviceRGBA)
	{
		/* Initialize the buffer with the Alpha
			 * channel initialized to zero. It really doesn't matter
			 * what the RGB channels initialize too, as the alpha of zero
			 * will make it transparent
			 */
		for (ASInt32 offset = 0; offset < bufferSize; offset += 4)
		{
			buffer[offset + 3] = 0x00;
		}
	}

	// With these values in place, the next call to PDPageDrawContentsToMemoryWithParams() will fill the bitmap.
	drawParams.bufferSize = bufferSize;
	drawParams.buffer = buffer;

	// Render page content to the bitmap buffer
	start_time[0] = clock();
	PDPageDrawContentsToMemoryWithParams(pdPage, &drawParams);
	stop_time[0] = clock();

	padded = (((nComps % 4) != 0) ? true : false); //note: this flag is so we don't remove padding more than once,max.

	if (parms->verbose())
	{
		double duration = ((double)(stop_time[0] - start_time[0]) / CLOCKS_PER_SEC);

		std::cout << "\nRendering time: " << duration << " s." << std::endl;

	}
}

RenderPage::~RenderPage() 
{ 
    if(buffer)
        ASfree (buffer);
    buffer = NULL;

    PDERelease(reinterpret_cast<PDEObject>(cs));
}

char * RenderPage::GetImageBuffer()
{
    return buffer;
}

ASSize_t RenderPage::GetImageBufferSize() const
{
    return bufferSize;
}

// This method will scale the image to fit the imageRect.  
// If the ImageRect does not have the same aspect ratio as the original updateRect,
// then the image will appear distorted.
PDEImage RenderPage::GetPDEImage(ASFixedRect imageRect)
{
	// Set up the static colorspace atoms
	static const ASAtom sDeviceRGB_K = ASAtomFromString("DeviceRGB");
	static const ASAtom sDeviceCMYK_K = ASAtomFromString("DeviceCMYK");
	static const ASAtom sDeviceGray_K = ASAtomFromString("DeviceGray");
	static const ASAtom sDeviceRGBA_K = ASAtomFromString("DeviceRGBA");

	// The bitmap data generated by PDPageDrawContentsTo* uses 32-bit aligned rows. 
	// The PDF image operator expects, however, 8-bit aligned image rows. 
	// To remedy this difference, we check to see if the 32-bit aligned width
	// is different from the 8-bit aligned width. If so, we fix the image data by 
	// stripping off the padding at the end of each row.
	if (padded)
	{
		ASSize_t createdWidth = (((static_cast<ASSize_t>(attrs.width * bpc * nComps) + 31) / 32) * 4);
		ASSize_t desiredWidth = static_cast<ASSize_t>(attrs.width * bpc * nComps) / 8;

		if (createdWidth != desiredWidth)
		{
			for (int row = 1; row < attrs.height; row++)
				memmove(&buffer[row * desiredWidth], &buffer[row * createdWidth], desiredWidth);
			bufferSize = static_cast<ASSize_t>(desiredWidth * attrs.height);
		}
		padded = false;
	}

    //Create the image matrix using the imageRect passed in.
	ASDoubleMatrix imageMatrix = {
		ASFixedToFloat(imageRect.right - imageRect.left),
		0,
		0, 
		ASFixedToFloat(imageRect.top - imageRect.bottom),
		ASFixedToFloat(imageRect.left),
		ASFixedToFloat(imageRect.bottom) 
	};

	PDEImage image = NULL;
	if (csAtom == sDeviceRGBA_K)
	{
		PDEImage        imageMask = NULL;
		PDEColorSpace cs1 = PDEColorSpaceCreateFromName(sDeviceRGB_K);
		PDEColorSpace cs2 = PDEColorSpaceCreateFromName(sDeviceGray_K);
		/* Seperate the alpha info from the color info */
		ASSize_t AlphaSize = bufferSize / 4;
		ASSize_t ColorSize = bufferSize - AlphaSize;

		/* Allocate alpha and color buffers */
		ASUns8* ColorBuffer = reinterpret_cast<ASUns8*>(ASmalloc(static_cast<ASSize_t>(attrs.width *attrs.height * 3)));
		ASUns8* AlphaBuffer = reinterpret_cast<ASUns8*>(ASmalloc(static_cast<ASSize_t>(attrs.width * attrs.height)));

		/* Do the separation - this is not an optimized implementation */
		for (ASInt32 Index = 0; Index < AlphaSize; Index++)
		{
			ColorBuffer[(Index * 3) + 0] = buffer[(Index * 4) + 0];
			ColorBuffer[(Index * 3) + 1] = buffer[(Index * 4) + 1];
			ColorBuffer[(Index * 3) + 2] = buffer[(Index * 4) + 2];
			AlphaBuffer[Index] = buffer[(Index * 4) + 3];
		}

		// Create an image XObject from the bitmap buffer to embed in the output document
		imageMask = PDEImageCreateEx(&attrs,
			sizeof(attrs),
			&imageMatrix,
			0,
			cs2,
			NULL,
			NULL,
			0,
			AlphaBuffer,
			AlphaSize);

		image = PDEImageCreateEx(&attrs,
			sizeof(attrs),
			&imageMatrix,
			0,
			cs1,
			NULL,
			NULL,
			0,
			ColorBuffer,
			ColorSize);

		PDEImageSetSMask(image, imageMask);
		PDERelease((PDEObject)imageMask);
		PDERelease((PDEObject)cs1);
		PDERelease((PDEObject)cs2);
		ASfree(ColorBuffer);
		ASfree(AlphaBuffer);
	}
	else {    // Create an image XObject from the bitmap buffer to embed in the output document
		image = PDEImageCreateEx(&attrs,
			sizeof(attrs),
			&imageMatrix,
			0,
			cs,
			NULL,
			NULL,
			0,
			reinterpret_cast<ASUns8*>(buffer),
			bufferSize);
	}

    return image;
}
