//
// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// Sample: RenderPage
//
// This file contains declarations for the RenderPage class.
//

#include <stdio.h>
#include <string.h>

#include "PDFLExpT.h"
#include "AcroColorExpT.h"


class RenderPageParams
{
private:
	ASAtom              csAtom{ ASAtomNull };
	ASInt32             nComps, bitspercomp;
	char*               colorSpace;
	ASDouble			res;
	ASUns32				smoothflags, drawflags;
	PDOCContext			ocContext;
	int					numfilters;
	AC_RenderIntent		ri;
	ASBool				bVerbose;
	ASDoubleMatrix*		matrix;
	ASDoubleRect*		destRect;
	AC_Profile			outputProfile{ nullptr };

public:
	RenderPageParams();
	~RenderPageParams();

	void            SetColorSpace(const char *colorSpace);
	void			SetColorSpace(ASAtom atmColorSpace);
	ASAtom			ColorSpaceName() const;
	ASInt32			NumComps() const;

	void			setBitsPerComponents(ASInt32 bpc);
	ASInt32			BitsPerComponent() const;
	
	void			setResolution(double resolution);
	double			Resolution() const;
	
	void			setSmoothFlags(ASUns32 smoothFlags);
	ASUns32			SmoothFlags() const;

	void			setDrawFlags(ASUns32 drawFlags);
	ASUns32			DrawFlags() const;

	void			setOCContext(PDOCContext ocContext);
	PDOCContext		OCContext() const;

	void			setRenderIntent(AC_RenderIntent ri);
	AC_RenderIntent	RenderIntent() const;

	void			setVerbose(ASBool verbose);
	ASBool			verbose() const;

	void			setMatrix(ASDoubleMatrix* m);
	ASDoubleMatrix* getMatrix();

	void			setDestRect(ASDoubleRect* r);
	ASDoubleRect*	getDestRect();

	void			setOutputProfile(AC_Profile profile);
	AC_Profile		getOutputProfile() const;
};

class RenderPage 
{
private:
	RenderPageParams*	parms;
    PDPage              pdPage;
    PDEImageAttrs       attrs;
    PDEColorSpace       cs;
    ASAtom              csAtom;
    ASInt32             nComps;
	ASSize_t            bufferSize;
    ASInt32             bpc;
	ASUns32				smoothFlags;
    char*               buffer; 
	bool				padded;
	
public:
	RenderPage(PDPage &pdPage, ASFixedRect* updateRect, RenderPageParams* parms);

	~RenderPage();

    char*               GetImageBuffer();
    ASSize_t             GetImageBufferSize() const;
    PDEImage            GetPDEImage(ASFixedRect ImageRect);
};
