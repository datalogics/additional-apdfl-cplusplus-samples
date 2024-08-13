//
// Copyright (c) 2000-2024, Datalogics, Inc. All rights reserved.
//
// This sample retrieves a PDF's permissions information.

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#undef LITTLE_ENDIAN 
#include "PDFInit.h"
#include "CosCalls.h"
#include "CorCalls.h"
#include "ASCalls.h"
#include "PDCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "PSFCalls.h"
#include "DLExtrasCalls.h"
#include "InitializeLibrary.h"
#include "APDFLDoc.h"

#define DIR_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT "LockDocument.pdf"

#define LEN(x) ((sizeof x)/(sizeof x[0]))

int main(int argc, char** argv)
{
	ASInt32 docPerms[] = { pdPermOpen, pdPermSecure, pdPermPrint, pdPermEdit, pdPermCopy,
		pdPermEditNotes, pdPermSaveAs, pdPermExt, pdPrivPermFillandSign,
		pdPrivPermAccessible, pdPrivPermDocAssembly, pdPrivPermHighPrint, pdPermOwner };
	char* PermStr[] = { "Open", "Secure", "PrintLo", "Edit", "Copy", "EditNotes", "SaveAs", "Extraction",
		"FillAndSign", "Accessible", "DocAssembly", "PrintHigh", "Owner" };

	char* statStr[] = { "No", "Yes", "Obj?", "op?", "N/A", "Wait" };
	char* objStr[] = { "Doc", "Page", "Link", "Bookmark", "Thumbnail", "Annot", "Form",
		"Signature", "embdFile", "RdrAnnot" };
	char* OpStr[] = { "All", "Create", "Delete", "Modify", "Copy", "Accessible",
		"Select", "Open", "Secure", "PrintLo", "PrintHigh", "FillIn", "Rotate",
		"Crop", "SummarizNote", "Insert", "Replace", "Reorder", "FullSave",
		"Import", "Export", "Any", "Unknown", "SubStndAlone", "SpwnTemplate",
		"Online", "SummaryView", "BarCodPlnTxt", "UISave", "UIPrint", "UIemail" };

	PDPermReqObj testObjs[] = { PDPermReqObjDoc, PDPermReqObjPage, PDPermReqObjAnnot, PDPermReqObjForm };
	PDPermReqObj curObj;
	PDPermReqOpr curOp;
	PDPermReqStatus status;

	APDFLib libInit;
	PDDoc pddoc = NULL;

	ASErrorCode errCode = 0;
	if (libInit.isValid() == false)
	{
		errCode = libInit.getInitError();
		std::cout << "Initialization failed with code " << errCode << std::endl;
		return libInit.getInitError();
	}

	PDPrefSetAllowOpeningXFA(true);

	std::string csInputFileName(argc > 1 ? argv[1] : DIR_LOC DEF_INPUT);

	DURING
		APDFLDoc APDoc(csInputFileName.c_str(), true);
	pddoc = APDoc.getPDDoc();

	if (pddoc == NULL)
	{
		fprintf(stdout, "Unable to open file %s \n", csInputFileName.c_str());
	}
	else {
		StdSecurityData data = (StdSecurityData)PDDocGetSecurityData(pddoc);

		ASUns32 bitSet = false;
		if (data != NULL) {
			printf("\nStd Security Data Revision: %d\n", data->revision);

			// char buffer[33];
			// _itoa(data->perms, buffer, 2);
			// printf("\nStd Security Data Permission [%d:%s] bits:\n", data->perms, buffer);

			// non-standard function _itoa replaced with sprintf() for more universal compiler compatibility

			char buffer[33];
			sprintf(buffer, "\nStd Security Data Permission [%d:%s] bits:\n", data->perms, buffer);

			printf("\nStd Security Data Permission [%d:%s] bits:\n", data->perms, buffer);
			for (int i = 0; i < LEN(docPerms); i++)
			{
				bitSet = data->perms & docPerms[i];
				printf("%12s bit (0x%04x) set to:  %s\n", PermStr[i], docPerms[i],
					bitSet ? "True" : "False");
			}
			printf("\n");
		}


		char szOp[32];
		// Header Line:
		printf("\n%12s ( #): ", "*Operation*");
		for (int i = 0; i < LEN(testObjs); i++)
		{
			curObj = testObjs[i];
			sprintf(szOp, "%s (%d):", objStr[curObj - 1], curObj);
			printf("%-15s ", szOp);
		}
		printf("\n");

		// for each Operation:
		for (curOp = (PDPermReqOprLast - 1); curOp > 0; curOp--)
		{
			if (curOp == PDPermReqOprUnknownOpr)
				continue;

			printf("%12s (%2d): ", OpStr[curOp - 1], curOp);
			for (int i = 0; i < LEN(testObjs); i++)
			{
				curObj = testObjs[i];

				status = PDDocPermRequest(pddoc, curObj, curOp, data);
				sprintf(szOp, "%s: %s", objStr[curObj - 1], statStr[status + 1]);
				printf("%-15s ", szOp);
			}
			printf("\n");
		}
		printf("\n");
	}

	HANDLER
		char buf[256];

	ASGetErrorString(ERRORCODE, buf, sizeof(buf));
	fprintf(stderr, "\n*** Error [0x%8x]: %s\n", ERRORCODE, buf);
	END_HANDLER

		PDDocClose(pddoc);
}