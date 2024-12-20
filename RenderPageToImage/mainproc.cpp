//
// Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// Sample: RenderPageToImage - Demonstrates the process of rasterizing an area of a PDF page 
//   and saving it to an Image File
//

#include "PERCalls.h"
#include "DLExtrasCalls.h"
#include "AcroColorCalls.h"
#include "APDFLDoc.h"
#include "InitializeLibrary.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "RenderPage.h"

#define DIR_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT "RenderPage.pdf"
constexpr auto DEF_OUTPUT = "RenderPageToImage-out.png";

constexpr auto RESOLUTION = 300.0;        // Other common choices might be 72.0, 150.0, 200.0, 300.0, or 600.0;
constexpr auto COLORSPACE = "DeviceRGB";  // Typically this, DeviceGray or DeviceCMYK;
constexpr auto BPC = 8;             // This must be 8 for DeviceRGB and DeviceCYMK, ;
                                    //  1, 8, or 24 for DeviceGray

static std::vector<char> ReadFromFile(const char* path)
{
	std::vector<char> ret;

	if (path && strlen(path))
	{
		std::ifstream file(path, std::ios::binary);
		if (file.is_open())
		{
			ret.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
		}
	}
	return ret;
}


int main(int argc, char** argv)
{
	ASErrorCode errCode = 0;

	// Initialize the library
	APDFLib libInit;

	// If library in initialization failed.
	if (libInit.isValid() == false)
	{
		errCode = libInit.getInitError();
		std::cout << "Initialization failed with code " << errCode << std::endl;
		return libInit.getInitError();
	}

	int curArg = 1;
	RenderPageParams parms;
	parms.setVerbose(FALSE);
	parms.SetColorSpace(ASAtomFromString(COLORSPACE));
	parms.setResolution(RESOLUTION);
	parms.setBitsPerComponents(BPC);
	parms.setRenderIntent(AC_Perceptual);
	ASFixedRect fCropRect = { fixedZero,fixedZero,fixedZero,fixedZero };
	ASBool bUseSpecifiedRect = FALSE;
	ASDoubleMatrix specifiedMatrix = { 1,0,0,1,0,0 };
	ASDoubleRect specifiedDest = { 0,0,0,0 };
	ASUns32 smoothFlags = kPDPageDrawSmoothText | kPDPageDrawSmoothLineArt | kPDPageDrawSmoothImage;
	ASUns32 drawFlags = kPDPageDoLazyErase | kPDPageUseAnnotFaces;
	ASText layerName = ASTextNew(); //initialized to an empty string/
	int pageNum = 0;
	AC_Profile outputProfile{ nullptr };

	while (argc > curArg)
	{
		if (strcmp(argv[curArg], "-relax") == 0)
		{
			PDPrefSetAllowRelaxedSyntax(true);
		}
		else if (strcmp(argv[curArg], "-verbose") == 0)
		{
			parms.setVerbose(TRUE);
		}
		else if (strcmp(argv[curArg], "-quiet") == 0)
		{
			parms.setVerbose(FALSE);
		}
		else if (strcmp(argv[curArg], "-xfa") == 0)
		{
			PDPrefSetAllowOpeningXFA(true);
		}
		else if (strcmp(argv[curArg], "-blackpointcompensation") == 0)
		{
			if (!PDPrefGetBlackPointCompensation())
				PDPrefSetBlackPointCompensation(true);
		}
		else if (strcmp(argv[curArg], "-noblackpointcompensation") == 0)
		{
			if (PDPrefGetBlackPointCompensation())
				PDPrefSetBlackPointCompensation(false);
		}
		else if (strcmp(argv[curArg], "-memtempfiles") == 0)
		{
			ASRamFileSysSetLimitKB(0);
			ASSetTempFileSys(ASGetRamFileSys());
		}
		else if (strcmp(argv[curArg], "-pg") == 0)
		{
			pageNum = atoi(argv[++curArg]);
		}
		else if (strcmp(argv[curArg], "-bpc") == 0)
		{
			parms.setBitsPerComponents(atoi(argv[++curArg]));
		}
		else if (strcmp(argv[curArg], "-res") == 0)
		{
			parms.setResolution(atof(argv[++curArg]));
		}
		else if (strcmp(argv[curArg], "-rgb") == 0)
		{
			parms.SetColorSpace(ASAtomFromString("DeviceRGB"));
		}
		else if (strcmp(argv[curArg], "-cmyk") == 0)
		{
			parms.SetColorSpace(ASAtomFromString("DeviceCMYK"));
		}
		else if (strcmp(argv[curArg], "-gray") == 0)
		{
			parms.SetColorSpace(ASAtomFromString("DeviceGray"));
		}
		else if (strcmp(argv[curArg], "-rgba") == 0) //experimental
		{
			parms.SetColorSpace(ASAtomFromString("DeviceRGBA"));
		}
		else if (strcmp(argv[curArg], "-grayworkingprofile") == 0)
		{
			std::vector<char> ProfBuf;
			ProfBuf = ReadFromFile(argv[++curArg]);
			if (ProfBuf.size())
			{
				PDPrefSetWorkingGray(&ProfBuf[0], static_cast<ASUns32>(ProfBuf.size()));
			}
		}
		else if (strcmp(argv[curArg], "-rgbworkingprofile") == 0)
		{
			std::vector<char> ProfBuf;
			ProfBuf = ReadFromFile(argv[++curArg]);
			if (ProfBuf.size())
			{
				PDPrefSetWorkingRGB(&ProfBuf[0], static_cast<ASUns32>(ProfBuf.size()));
			}
		}
		else if (strcmp(argv[curArg], "-cmykworkingprofile") == 0)
		{
			std::vector<char> ProfBuf;
			ProfBuf = ReadFromFile(argv[++curArg]);
			if (ProfBuf.size())
			{
				PDPrefSetWorkingCMYK(&ProfBuf[0], static_cast<ASUns32>(ProfBuf.size()));
			}
		}
		else if (strcmp(argv[curArg], "-targetprofile") == 0)
		{
			std::vector<char> ProfBuf;
			ProfBuf = ReadFromFile(argv[++curArg]);
			if (ProfBuf.size())
			{
				ACMakeBufferProfile(&outputProfile, &ProfBuf[0], static_cast<ASUns32>(ProfBuf.size()));
			}
		}
		else if (strcmp(argv[curArg], "-layer") == 0) //experimental
		{
			ASTextDestroy(layerName);
			layerName = ASTextFromUnicode(reinterpret_cast<const ASUTF16Val*>(argv[++curArg]), kUTF8);
		}
		else if (strcmp(argv[curArg], "-nosmoothtext") == 0)
		{
			smoothFlags &= ~kPDPageDrawSmoothText;
		}
		else if (strcmp(argv[curArg], "-nosmoothlineart") == 0)
		{
			smoothFlags &= ~kPDPageDrawSmoothLineArt;
		}
		else if (strcmp(argv[curArg], "-nosmoothimage") == 0)
		{
			smoothFlags &= ~kPDPageDrawSmoothImage;
		}
		else if (strcmp(argv[curArg], "-ddrsmoothtext") == 0)
		{
			smoothFlags |= kPDPageDrawSmoothAATextDDR;
		}
		else if (strcmp(argv[curArg], "-smoothbicubic") == 0) //Note: needs to be combined with -antialias and -nosmoothimage
		{
			smoothFlags |= kPDPageImageResampleBicubic;
		}
		else if (strcmp(argv[curArg], "-smoothlinear") == 0) //Note: needs to be combined with -antialias and -nosmoothimage
		{
			smoothFlags |= kPDPageImageResampleLinear; // effectively equivalent to kPDPageDrawSmoothImage when combined with kPDPageImageAntiAlias
		}
		else if (strcmp(argv[curArg], "-antialias") == 0) //Note: needs to be combined -nosmoothimage and either -smoothbicubic or -smoothlinear
		{
			smoothFlags |= kPDPageImageAntiAlias;
		}
		else if (strcmp(argv[curArg], "-noannotfaces") == 0)
		{
			drawFlags &= ~kPDPageUseAnnotFaces;
		}
		else if (strcmp(argv[curArg], "-nolazyerase") == 0)
		{
			drawFlags &= ~kPDPageDoLazyErase;
		}
		else if (strcmp(argv[curArg], "-overprintpreview") == 0)
		{
			drawFlags |= kPDPageDisplayOverPrintPreview;
		}
		else if (strcmp(argv[curArg], "-abscolmetric") == 0)
		{
			parms.setRenderIntent(AC_AbsColorimetric);
		}
		else if (strcmp(argv[curArg], "-relcolmetric") == 0)
		{
			parms.setRenderIntent(AC_RelColorimetric);
		}
		else if (strcmp(argv[curArg], "-saturation") == 0)
		{
			parms.setRenderIntent(AC_Saturation);
		}
		else if (strcmp(argv[curArg], "-profileintent") == 0)
		{
			parms.setRenderIntent(AC_UseProfileIntent);
		}
		else if (strcmp(argv[curArg], "-gstateintent") == 0)
		{
			parms.setRenderIntent(AC_UseGStateIntent);
		}
		else if (strcmp(argv[curArg], "-rect") == 0)
		{
			bUseSpecifiedRect = TRUE;
			fCropRect.left = FloatToASFixed(atof(argv[++curArg]));
			fCropRect.bottom = FloatToASFixed(atof(argv[++curArg]));
			fCropRect.right = FloatToASFixed(atof(argv[++curArg]));
			fCropRect.top = FloatToASFixed(atof(argv[++curArg]));
		}
		else if (strcmp(argv[curArg], "-dest") == 0)
		{
			specifiedDest.left = atof(argv[++curArg]);
			specifiedDest.bottom = atof(argv[++curArg]);
			specifiedDest.right = atof(argv[++curArg]);
			specifiedDest.top = atof(argv[++curArg]);
			parms.setDestRect(&specifiedDest);
		}
		else if (strcmp(argv[curArg], "-matrix") == 0)
		{
			specifiedMatrix.a = atof(argv[++curArg]);
			specifiedMatrix.b = atof(argv[++curArg]);
			specifiedMatrix.c = atof(argv[++curArg]);
			specifiedMatrix.d = atof(argv[++curArg]);
			specifiedMatrix.h = atof(argv[++curArg]);
			specifiedMatrix.v = atof(argv[++curArg]);
			parms.setMatrix(&specifiedMatrix);
		}
		else
			break;
		++curArg;
	}

	std::string csInputFileName(argc > curArg ? argv[curArg] : DIR_LOC DEF_INPUT);
	++curArg;
	std::string csOutputFileName(argc > curArg ? argv[curArg] : DEF_OUTPUT);

	if (parms.verbose())
	{
		std::cout << "Rendering " << csInputFileName.c_str() << " to " << csOutputFileName.c_str()
			<< " with " << std::endl << " Resolution of " << parms.Resolution() << ", Colorspace "
			<< ASAtomGetString(parms.ColorSpaceName()) << ", and BPC " << parms.BitsPerComponent() << std::endl;
	}
	DURING

		// Open the input document and acquire the desired page
		APDFLDoc inDoc(csInputFileName.c_str(), true);
	    PDPage pdPage = inDoc.getPage(pageNum);

		if (!bUseSpecifiedRect)
			PDPageGetCropBox(pdPage, &fCropRect);

		ASFixedRect fOutRect;
		PDRotate rotation = PDPageGetRotate(pdPage);
		if (rotation == pdRotate90 || rotation == pdRotate270)
		{
			//if the source page is rotated perpendicular, then swap the dimensions for the output rect.
			//ASFixedRect Members are: left,top,right,bottom
			fOutRect = { fCropRect.bottom, fCropRect.right, fCropRect.top, fCropRect.left };
		}
		else
			fOutRect = fCropRect;

		if (parms.verbose())
			std::cout << "Rendering page " << pageNum << " area: " << ((fOutRect.right - fOutRect.left) * 0.125 / fixedNine) << " * " << ((fOutRect.top - fOutRect.bottom) * 0.125 / fixedNine) << " inches." << std::endl;


		//if specified, only render the optional content for a particular layer (along with non-optional content), otherwise use the default currently visible layers.
		PDOCContext curContext = PDDocGetOCContext(inDoc.getPDDoc());
		if (!ASTextIsEmpty(layerName))
		{
			PDOCG* ocgs = PDPageGetOCGs(pdPage);
			int limit = PDDocGetNumOCGs(inDoc.getPDDoc());
			int n = 0;
			std::cout << "looking for: [" << reinterpret_cast<char*>(ASTextGetUnicodeCopy(layerName, kUTF8)) << "]" << std::endl;
			while (n < limit && ocgs[n] != NULL)
			{
				std::cout << "layer: [" << reinterpret_cast<char*>(ASTextGetUnicodeCopy(PDOCGGetName(ocgs[n]), kUTF8)) << "]\n" << std::endl;
				if (ASTextCmp(layerName, PDOCGGetName(ocgs[n])) == 0)
				{
					curContext = PDOCContextNew(kOCCInit_ON, NULL, NULL, inDoc.getPDDoc());
					PDOCG layers[2] = { NULL,ocgs[n] };
					ASBool state = true;
					PDOCContextSetOCGStates(curContext, layers, &state);
				}
				++n;
			}
		}

		parms.setOCContext(curContext);
		parms.setDrawFlags(drawFlags);
		parms.setSmoothFlags(smoothFlags);
		parms.setOutputProfile(outputProfile);

		// Construction of the drawPage object does all the work to rasterize the page
		RenderPage drawPage(pdPage, &fCropRect, &parms);

		DLPDEImageExportParams exportParams = DLPDEImageGetExportParams();
		exportParams.ExportHorizontalDPI = exportParams.ExportVerticalDPI = parms.Resolution();

		ASPathName outPath;
		ASText textToCreatePath = NULL; // Text object to create ASPathName
		// Determine size of wchar_t on system and get the ASText
		if (sizeof(wchar_t) == 2)
			textToCreatePath = ASTextFromUnicode(reinterpret_cast<const ASUTF16Val*>(csOutputFileName.c_str()), kUTF16HostEndian);
		else
			textToCreatePath = ASTextFromUnicode(reinterpret_cast<const ASUTF16Val*>(csOutputFileName.c_str()), kUTF32HostEndian);

		outPath = ASFileSysCreatePathFromDIPathText(NULL, textToCreatePath, NULL);

		// The call to GetPDEImage synthesizes a PDEImage object from the rasterized PDF page
		// created in the constructor, suitable for extracting to an image file.
		PDEImage pageImage = drawPage.GetPDEImage(fOutRect);

		DLExportPDEImage(pageImage, outPath, ExportType_PNG, exportParams);

		// clean up 
		PDPageRelease(pdPage);
		ASTextDestroy(textToCreatePath);
		ASFileSysReleasePath(NULL, outPath);
		PDERelease(reinterpret_cast<PDEObject>(pageImage));

	HANDLER
		errCode = ERRORCODE;
	    libInit.displayError(errCode);
	END_HANDLER

		if (outputProfile != nullptr)
			ACUnReferenceProfile(outputProfile);

	return errCode;
}
