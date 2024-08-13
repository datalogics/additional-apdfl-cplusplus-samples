//
// Copyright (c) 2018-2024, Datalogics, Inc. All rights reserved.
//
// This sample demonstrates how to resize a page.
//
//  Command-line:   <input-file>  <output-file>     (Both optional)


#include "InitializeLibrary.h"
#include "APDFLDoc.h"
#include "ASExtraCalls.h"
#include "CosCalls.h"
#include "CorCalls.h"
#include "ASCalls.h"
#include "PDCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "PSFCalls.h"
#include <iostream>

#define INPUT_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT "ducky.pdf"
#define DEF_OUTPUT "pageResize-out.pdf"

/**
@param fOldMediaBox The input rect
@param dim1 one of the output dimensions
@param dim2 the other output dimensions
@param fNewRect the output rectangle to be filled with dimensions aligned to the input rect
*/
void sizeNewPage(ASFixedRect* fOldMediaBox, ASFixed dim1, ASFixed dim2, ASFixedRect* fNewRect)
{
	ASFixed biggerDim = dim1 > dim2 ? dim1 : dim2;
	ASFixed smallerDim = dim1 > dim2 ? dim2 : dim1;
	if ((fOldMediaBox->top - fOldMediaBox->bottom) > (fOldMediaBox->right - fOldMediaBox->left))
	{
		fNewRect->right = smallerDim;
		fNewRect->top = biggerDim;
	}
	else
	{
		fNewRect->right = biggerDim;
		fNewRect->top = smallerDim;
	}
	fNewRect->left = fNewRect->bottom = fixedZero;
}

/**
@param fOldMediaBox the original page size
@param fNewMediaBox the new page size
@param fMatrix the output matrix for scaling the old to the new and centering the content, to be filled.
*/
void calcScalingMatrix(ASFixedRect* fOldMediaBox, ASFixedRect* fNewMediaBox, ASDoubleMatrix* matrix)
{
	ASFixed fOldWidth = fOldMediaBox->right - fOldMediaBox->left;
	ASFixed fNewWidth = fNewMediaBox->right - fNewMediaBox->left;
	ASFixed fOldHeight = fOldMediaBox->top - fOldMediaBox->bottom;
	ASFixed fNewHeight = fNewMediaBox->top - fNewMediaBox->bottom;
	double widthScaling = ASFixedToFloat(fNewWidth) / ASFixedToFloat(fOldWidth);
	double heightScaling = ASFixedToFloat(fNewHeight) / ASFixedToFloat(fOldHeight);
	double ScalingFactor = min(widthScaling, heightScaling);

	// if we need to scale up by less than 10%, then keep the scale as is.
	if (ScalingFactor > 1.0 && ((1.0 - ScalingFactor) < 0.10))
		ScalingFactor = 1.0;

	double scaledWidth = ScalingFactor * ASFixedToFloat(fOldWidth);
	double scaledHeight = ScalingFactor * ASFixedToFloat(fOldHeight);

	double leftOffset = ASFixedToFloat(fNewMediaBox->left) + (0.5 * (ASFixedToFloat(fNewWidth) - scaledWidth));
	double bottomOffset = ASFixedToFloat(fNewMediaBox->bottom) + (0.5 * (ASFixedToFloat(fNewHeight) - scaledHeight));

	matrix->a = matrix->d = ScalingFactor;
	matrix->b = matrix->c = 0.0;
	matrix->h = leftOffset;
	matrix->v = bottomOffset;
}

void doImposition(PDDoc doc, ASFixed dim1, ASFixed dim2)
{
	int originalPages = PDDocGetNumPages(doc);
	for (int currentpage = 0; currentpage < originalPages; currentpage++)
	{
		PDPage pdPage = PDDocAcquirePage(doc, currentpage);
		PDEContent pdeContent = PDPageAcquirePDEContent(pdPage, 0);
		ASFixedRect origRect, newRect, fOldBBoxRect;
		ASDoubleMatrix matrix;
		CosObj cosContent, cosRes;

		PDPageGetCropBox(pdPage, &fOldBBoxRect);

		PDEContentAttrs ContentAttrs;
		memset(&ContentAttrs, 0, sizeof(ContentAttrs));
		ContentAttrs.flags = kPDEContentToForm;
		ContentAttrs.formType = 1;
		ContentAttrs.bbox = fOldBBoxRect;

		PDEFilterArray filter;
		filter.numFilters = 1;
		filter.spec[0].decodeParms = CosNewNull();
		filter.spec[0].encodeParms = CosNewNull();
		filter.spec[0].name = ASAtomFromString("FlateDecode");
		filter.spec[0].padding = 0;

		PDEContentToCosObj(pdeContent, kPDEContentToForm | kPDEContentFormFromPage | kPDEContentUseMaxPrecision, &ContentAttrs, sizeof(ContentAttrs), PDDocGetCosDoc(doc), &filter, &cosContent, &cosRes);

		PDPageGetMediaBox(pdPage, &origRect);
		sizeNewPage(&origRect, dim1, dim2, &newRect);
		calcScalingMatrix(&origRect, &newRect, &matrix);

		// Note: this will break any logical structure/tagging the source page may have had.
		PDEForm newForm = PDEFormCreateFromCosObjEx(&cosContent, &cosRes, &matrix);

		PDPage newPage = PDDocCreatePage(doc, originalPages + currentpage - 1, newRect);
		PDPageSetRotate(newPage, PDPageGetRotate(pdPage));

		PDEContent newContent = PDPageAcquirePDEContent(newPage, 0);
		PDEContentAddElem(newContent, kPDEBeforeFirst, (PDEElement)newForm);
		PDERelease((PDEObject)newForm);

		PDPageSetPDEContent(newPage, 0);
		PDPageNotifyContentsDidChange(newPage);
		PDPageReleasePDEContent(newPage, 0);
		PDPageRelease(newPage);

		PDPageReleasePDEContent(pdPage, NULL);
		PDPageRelease(pdPage);
		pdPage = NULL;

	}
	// Note: Bookmarks and Link Annotations may not be pointing to the right pages.
	PDDocDeletePages(doc, 0, originalPages - 1, NULL, NULL);
}




int main(int argc, char** argv)
{
	APDFLib lib;
	ASErrorCode errCode = 0;
	if (lib.isValid() == false)
	{
		errCode = lib.getInitError();
		std::cout << "Initialization failed with code " << errCode << std::endl;
		return errCode;
	}

	std::string csInputFileName(argc > 1 ? argv[1] : INPUT_LOC DEF_INPUT);
	std::string csOutputFileName(argc > 2 ? argv[2] : DEF_OUTPUT);
	std::cout << "Will modify " << csInputFileName.c_str()
		<< " and save as " << csOutputFileName.c_str() << std::endl;

	DURING

	// Open the Document
	ASPathName asPathName = ASFileSysCreatePathFromDIPath(NULL, csInputFileName.c_str(), NULL);
	PDDoc pdDoc = PDDocOpen(asPathName, NULL, NULL, true);
	ASFileSysReleasePath(NULL, asPathName);

	// Resizing operation
	doImposition(pdDoc, fixedOne * 306, fixedOne * 396);


	// Save and exit
	asPathName = ASFileSysCreatePathFromDIPath(NULL, csOutputFileName.c_str(), NULL);
	PDDocSave(pdDoc, PDSaveFull, asPathName, NULL, NULL, NULL);
	ASFileSysReleasePath(NULL, asPathName);
	PDDocClose(pdDoc);

	HANDLER
		errCode = ERRORCODE;
	lib.displayError(errCode);
	END_HANDLER

		return errCode;
}