//
// Copyright (c) 2017,2024 Datalogics, Inc. All rights reserved.
//
// The CreateNestedLayers sample program demonstrates how to programmatically add nested layers to a PDF
// document.
//
// Command-line:  <output-file>   <number-of-pages>    (Both optional)
//

#include <iostream>

#include "InitializeLibrary.h"
#include "APDFLDoc.h"

#include "PSFCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "CosCalls.h"

#define DEF_OUTPUT "CreateLayers-out.pdf"


PDEFont fontMaker(const char* fontName, const char* fontType, PDEFontCreateFlags flags)
{
	PDEFontAttrs fontAttrs;
	memset(&fontAttrs, 0, sizeof(fontAttrs));
	fontAttrs.name = ASAtomFromString(fontName);
	fontAttrs.type = ASAtomFromString(fontType);

	//Locate the system font that corresponds to the PDEFontAttrs struct we just set.
	PDSysFont sysFont = PDFindSysFont(&fontAttrs, sizeof(fontAttrs), 0);

	//Create the CourierStd Type1 font with embed flag set.       
	PDEFont font = PDEFontCreateFromSysFont(sysFont, flags); //kPDEFontCreateEmbedded

	return font;
}

PDEContainer makeOCMDContainer(PDEContent content, PDOCMD ocmd)
{
	//Create an empty container for the text.
	PDEContainer container = PDEContainerCreate(ASAtomFromString("OC"), NULL, true);

	// Place them into this container
	PDEContainerSetContent(container, content);

	//Set the container's membership dictionary to the text layer.
	PDEElementSetOCMD((PDEElement)container, ocmd);
	return container;
}

PDEContainer makeOCGContainer(PDEContent content, PDOCG ocg)
{
	//Create an empty container for the text.
	PDEContainer container = PDEContainerCreate(ASAtomFromString("OC"), NULL, true);

	// Place them into this container
	PDEContainerSetContent(container, content);

	//Set the OCG's dictionary to the container .
	CosObj ocgObj = PDOCGGetCosObj(ocg);
	PDEContainerSetDict(container, &ocgObj , false);

	return container;
}


// Helper function to create a PDEText object
PDEText textMakerEx(std::string displayText, PDEFont font, ASDoubleMatrix* textMatrix, PDEGraphicState* gs, PDETextState* ts)
{
	// Create a new text run
	PDEText textObj = PDETextCreate();

	//Adding the text run to the PDE text object.
	PDETextAddEx(
		textObj,                                  // Text container to add to. 
		kPDETextRun,                              // kPDETextRun or kPDETextChar as appropriate
		0,                                        // The index after which to add the text run.
		(Uns8 *)displayText.c_str(),              // Text to add.    
		displayText.length(),                     // Length of text. 
		font,                              // Font to apply to text. 
		gs, sizeof(PDEGraphicState),                  // PDEGraphicState and its size.
		ts, sizeof(PDETextState),                               // Text state and its size.
		textMatrix,                              // Matrix containing size and location for the text.
		NULL);                                    // Stroke matrix for the line width when stroking text.  


	return textObj;
}

void setGstateRGBFillColor(PDEGraphicState* gState, unsigned char red, unsigned char green, unsigned char blue)
{
	//Release the fill color space before modifiying it.
	PDERelease(reinterpret_cast<PDEObject>(gState->fillColorSpec.space));

	//We are using the RGB color space in this case. Default value is "DeviceGray".     
	gState->fillColorSpec.space = PDEColorSpaceCreateFromName(ASAtomFromString("DeviceRGB"));

	gState->fillColorSpec.value.color[0] = ASFloatToFixed(red * 1.0f / 255.0f);      //Initially value = 0 (Black.)
	gState->fillColorSpec.value.color[1] = ASFloatToFixed(green * 1.0f / 255.0f);    //In this case the object will be painted purple.
	gState->fillColorSpec.value.color[2] = ASFloatToFixed(blue * 1.0f / 255.0f);
}


int main(int argc, char** argv)
{
    APDFLib libInit;
    ASErrorCode errCode = 0;
    if (libInit.isValid() == false)
    {
        errCode = libInit.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return libInit.getInitError();
    }

    std::string csOutputFileName ( argc > 1 ? argv[1] : DEF_OUTPUT );
    std::cout << "Creating new document " << csOutputFileName.c_str() <<
                 " and inserting 2 layers..." << std::endl;

DURING

// Step 1) Create a pdf document and obtain a reference to it's first page, and its' content

    APDFLDoc doc;

    //Insert a standard 8.5 inch x 11 inch page into the document.
    doc.insertPage ( ASFloatToFixed ( 8.5 * 72 ), ASFloatToFixed ( 11 * 72 ), PDBeforeFirstPage );
    PDPage page = doc.getPage(0);
    PDEContent pageContent = PDPageAcquirePDEContent(page, NULL);

// Step 2) Set up the optional content groups, commonly referred to as layers.

    //Create optional content groups (Layers) for texts and annotations.
	PDOCG optionalGroupNestedLayers = PDOCGCreate(doc.pdDoc, ASTextFromPDText("Nested Layers"));
	PDOCG optionalGroupNestedLayer1 = PDOCGCreate(doc.pdDoc, ASTextFromPDText("Nested Layer 1"));
	PDOCG optionalGroupNestedLayer2 = PDOCGCreate(doc.pdDoc, ASTextFromPDText("Nested Layer 2"));

    //Set the their initial state to visible.
    PDOCConfig  ocConfig = PDDocGetOCConfig(doc.pdDoc);
    PDOCGSetInitialState(optionalGroupNestedLayers, ocConfig, true);

    CosObj order, subOrder;
	int insertPos = 0;
	PDOCConfigGetOCGOrder(ocConfig, &order);
	if (CosObjGetType(order) != CosArray)
	{
		order = CosNewArray(PDDocGetCosDoc(doc.getPDDoc()), true, 4);//The order of the optional content.
	}
	else
	{
		insertPos = CosArrayLength(order);
	}
	
	subOrder = CosNewArray(PDDocGetCosDoc(doc.getPDDoc()), true, 4);//The order of the nested optional content.

    //Insert the layers as a cosObject in the pdf. 
    CosArrayInsert(order, insertPos++, PDOCGGetCosObj(optionalGroupNestedLayers));
	CosArrayInsert(subOrder, 0, PDOCGGetCosObj(optionalGroupNestedLayer1));
	CosArrayInsert(subOrder, 1, PDOCGGetCosObj(optionalGroupNestedLayer2));
	CosArrayInsert(order, insertPos++, subOrder);

    //Put the new order back as a part of the pdf's configuration.
    PDOCConfigSetOCGOrder(ocConfig, order);

    //Create a layer array in order to properly pass the layer into the membership dictionary creation function.
	PDOCG pdDocArrayNestedLayers[2];
    pdDocArrayNestedLayers[0] = optionalGroupNestedLayers;
    pdDocArrayNestedLayers[1] = NULL;
	//Obtain the membership dictionary of the text layer.
	PDOCMD optionalGroupMDNestedLayers = PDOCMDCreate(doc.pdDoc, pdDocArrayNestedLayers, kOCMDVisibility_AllOn);

	PDOCG pdDocArrayNestedLayer1[2];
	pdDocArrayNestedLayer1[0] = optionalGroupNestedLayer1;
	pdDocArrayNestedLayer1[1] = NULL;
	PDOCMD optionalGroupMDNestedLayer1 = PDOCMDCreate(doc.pdDoc, pdDocArrayNestedLayer1, kOCMDVisibility_AllOn);

	PDOCG pdDocArrayNestedLayer2[2];
	pdDocArrayNestedLayer2[0] = optionalGroupNestedLayer2;
	pdDocArrayNestedLayer2[1] = NULL;
	PDOCMD optionalGroupMDNestedLayer2 = PDOCMDCreate(doc.pdDoc, pdDocArrayNestedLayer2, kOCMDVisibility_AllOn);


// Step 3) Add text to the page and set what layer they belong to.
	PDEFont myfont = fontMaker("MyriadPro-Regular", "TrueType", kPDEFontCreateEmbedOpenType);
	PDEGraphicState darkvioletGS, darkolivegreenGS, saddlebrownGS;

	PDEDefaultGState(&darkvioletGS, sizeof(PDEGraphicState));
	setGstateRGBFillColor(&darkvioletGS, 0x94, 0x00, 0xd3);

	PDEDefaultGState(&darkolivegreenGS, sizeof(PDEGraphicState));
	setGstateRGBFillColor(&darkolivegreenGS, 0x55, 0x6b, 0x2f); //556b2f

	PDEDefaultGState(&saddlebrownGS, sizeof(PDEGraphicState));
	setGstateRGBFillColor(&saddlebrownGS, 0x8b, 0x45, 0x13); //8b4513

	PDETextState tState;
	memset(&tState, 0x0, sizeof(PDETextState));
	tState.fontSize = FloatToASFixed(16.0);


	PDEContainer container0;
	{
	PDEContent subcontent0 = PDEContentCreate();
	PDEContent subcontent1 = PDEContentCreate();
	PDEContent subcontent2 = PDEContentCreate();
	PDEContent subcontent3 = PDEContentCreate();

	ASDoubleMatrix  nestedlayer1Matrix = { 1, 0, 0, 1, 108, 704 }; //translation matrix for positioning text at a specific location.
	PDEText elem = textMakerEx("nested Layer 1", myfont, &nestedlayer1Matrix, &darkolivegreenGS, &tState);
	PDEContentAddElem(subcontent1, kPDEBeforeFirst, reinterpret_cast<PDEElement>(elem));
	PDEContainer container1 = makeOCGContainer(subcontent1, optionalGroupNestedLayer1);
	PDERelease(reinterpret_cast<PDEObject>(elem));
	
	ASDoubleMatrix  nestedlayer2Matrix = { 1, 0, 0, 1, 108, 688 };
	elem = textMakerEx("nested Layer 2", myfont, &nestedlayer2Matrix, &darkvioletGS, &tState);
	PDEContentAddElem(subcontent2, kPDEBeforeFirst, reinterpret_cast<PDEElement>(elem));
	PDEContainer container2 = makeOCGContainer(subcontent2, optionalGroupNestedLayer2);
	PDERelease(reinterpret_cast<PDEObject>(elem));

	ASDoubleMatrix  nestedlayersMatrix = { 1, 0, 0, 1, 72, 720 };
	elem = textMakerEx("Nested layers", myfont, &nestedlayersMatrix, &saddlebrownGS, &tState);
	PDEContentAddElem(subcontent0, kPDEBeforeFirst, reinterpret_cast<PDEElement>(elem));
	PDERelease(reinterpret_cast<PDEObject>(elem));

	PDEContentAddElem(subcontent0, kPDEAfterLast, reinterpret_cast<PDEElement>(container1));
	PDEContentAddElem(subcontent0, kPDEAfterLast, reinterpret_cast<PDEElement>(container2));


	ASDoubleMatrix  nestedlayer1Part2Matrix = { 1, 0, 0, 1, 108, 672 }; //translation matrix for positioning text at a specific location.
	elem = textMakerEx("nested Layer 1, Part 2", myfont, &nestedlayer1Part2Matrix, &darkolivegreenGS, &tState);
	PDEContentAddElem(subcontent3, kPDEBeforeFirst, reinterpret_cast<PDEElement>(elem));
	PDERelease(reinterpret_cast<PDEObject>(elem));

	PDEContainer container3 = makeOCGContainer(subcontent3, optionalGroupNestedLayer1);
	PDEContentAddElem(subcontent0, kPDEAfterLast, reinterpret_cast<PDEElement>(container3));

	container0 = makeOCMDContainer(subcontent0, optionalGroupMDNestedLayers);

	PDERelease(reinterpret_cast<PDEObject>(container1));
	PDERelease(reinterpret_cast<PDEObject>(container2));
	PDERelease(reinterpret_cast<PDEObject>(subcontent0));
	PDERelease(reinterpret_cast<PDEObject>(subcontent1));
	PDERelease(reinterpret_cast<PDEObject>(subcontent2));
	PDERelease(reinterpret_cast<PDEObject>(subcontent3));
	}

    //Add the nested container to the page's content
    PDEContentAddElem(pageContent, kPDEAfterLast, reinterpret_cast<PDEElement>(container0));                   

    //Set the content back into the page.
    PDPageSetPDEContentCanRaise(page, NULL);                                                    

// Step 5) Save the output document and exit.

    //Release objects no longer in use
    PDPageReleasePDEContent(page, NULL);
    PDPageRelease(page);
	PDERelease(reinterpret_cast<PDEObject>(myfont));
	PDERelease(reinterpret_cast<PDEObject>(darkvioletGS.fillColorSpec.space));
	PDERelease(reinterpret_cast<PDEObject>(darkolivegreenGS.fillColorSpec.space));
	PDERelease(reinterpret_cast<PDEObject>(saddlebrownGS.fillColorSpec.space));


    doc.saveDoc ( csOutputFileName.c_str(), PDSaveFull | PDSaveLinearized);

HANDLER
    errCode = ERRORCODE;
    libInit.displayError(errCode);
END_HANDLER

    return errCode;
}
