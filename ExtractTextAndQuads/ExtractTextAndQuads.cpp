//
// Copyright (c) 2025, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// This sample extracts text from a PDF input documents and saves the
// content to a UTF-8 plaintext file.
//
// Command-line:    <input-pdf>  <output-name>       (Both optional)
//

#include <iostream>
#include <fstream>
#include <cstring>

#include "InitializeLibrary.h"
#include "APDFLDoc.h"

#include "DLExtrasCalls.h"

#define DIR_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT_1 "ExtractUnicodeText.pdf"
#define DEF_OUTPUT_1 "ExtractText-out.txt"

typedef struct {
	ASBool bVerbose = true;
	ASBool bNoLastWordInRegion = false;
	ASBool bNoTag = false;
	ASBool bNoAnots = false;
	ASBool bPreciseQuad = false;
	ASBool bShowQuads = false;
	ASBool bShowCharQuads = false;
} CMDOPTS;

int main(int argc, char** argv)
{
	APDFLib libInit;
	ASErrorCode errCode = 0;
	if (libInit.isValid() == false)
	{
		errCode = libInit.getInitError();
		std::cerr << "Initialization failed with code " << errCode << std::endl;
		return libInit.getInitError();
	}
	//ASBool bVerbose = true;
	CMDOPTS opts;

	int curArg = 1;

	while (argc > curArg)
	{
		if (strcmp(argv[curArg], "-relax") == 0)
		{
			PDPrefSetAllowRelaxedSyntax(true);
		}
		else if (strcmp(argv[curArg], "-nolastword") == 0) //makes this comparable to how Adobe's WordFinder would handle
		{
			opts.bNoLastWordInRegion = true;
		}
		else if (strcmp(argv[curArg], "-notagextract") == 0)
		{
			opts.bNoTag = true;
		}
		else if (strcmp(argv[curArg], "-noannots") == 0)
		{
			opts.bNoAnots = true;
		}
		else if (strcmp(argv[curArg], "-precisequads") == 0)
		{
			opts.bPreciseQuad = true;
		}
		else if (strcmp(argv[curArg], "-showquads") == 0)
		{
			opts.bShowQuads = true;
		}
		else if (strcmp(argv[curArg], "-showcharquads") == 0)
		{
			opts.bShowCharQuads = true;
		}
		else if (strcmp(argv[curArg], "-quiet") == 0)
		{
			opts.bVerbose = false;
		}
		else
			break;
		++curArg;
	}


	std::string csInputFileName1(argc > curArg ? argv[curArg] : DIR_LOC DEF_INPUT_1);
	++curArg;
	std::string csOutputFileName1(argc > curArg ? argv[curArg] : DEF_OUTPUT_1);

	DURING

		std::ofstream outputFile(csOutputFileName1.c_str());

	if (outputFile.is_open())
	{
		APDFLDoc document(csInputFileName1.c_str(), true);

		//Use default settings for the PDWordFinder. See the sample TextSearch for an example of PDWordFinder settings.
		PDWordFinderConfigRec wfConfig;
		memset(&wfConfig, 0, sizeof(PDWordFinderConfigRec));
		wfConfig.recSize = sizeof(PDWordFinderConfigRec);

		wfConfig.disableTaggedPDF = opts.bNoTag;
		wfConfig.noAnnots = opts.bNoAnots;
		wfConfig.preciseQuad = opts.bPreciseQuad;

		// Here, we set the boolean value is set to true, in order to extract text in unicode.
		PDWordFinder pdWordFinder = PDDocCreateWordFinderEx(document.getPDDoc(), WF_LATEST_VERSION, true, &wfConfig);

		ASInt32 numWords;
		PDWord wordArray;

		if (opts.bVerbose)
		{
			std::cout << "Extracting from "
				<< csInputFileName1.c_str() << "; saving to "
				<< csOutputFileName1.c_str() << std::endl;
		}

		int numPages = PDDocGetNumPages(document.getPDDoc());
		for (int pgNum = 0; pgNum < numPages; pgNum++)
		{
			outputFile << "\t\t- page " << pgNum << " - " << std::endl;

			if (opts.bVerbose)
				std::cout << "Page " << pgNum << " of " << numPages << "." << std::endl;

			PDWordFinderAcquireWordList(pdWordFinder, pgNum, &wordArray, NULL, NULL, &numWords);

			for (ASInt32 index = 0; index < numWords; ++index)
			{
				ASUTF8Val* utf8String;
				PDWord pdWord = PDWordFinderGetNthWord(pdWordFinder, index);


				ASText asText = ASTextNew();
				PDWordGetASText(pdWord, 0, asText);

				//Get the endian neutral utf8 string.
				utf8String = reinterpret_cast<ASUTF8Val*>(ASTextGetUnicodeCopy(asText, kUTF8));

				ASUns16 wordAttrs = PDWordGetAttr(pdWord);

				outputFile << utf8String;

				if (opts.bShowQuads) //show these in square brackets.
				{
					int numQuads = PDWordGetNumQuads(pdWord);
					for (int cq = 0; cq < numQuads; cq++)
					{
						ASFixedQuad q;

						PDWordGetNthQuad(pdWord, cq, &q);
						outputFile << " [(" << ASFixedToFloat(q.bl.h) << "," << ASFixedToFloat(q.bl.v) << "), ";
						outputFile << "(" << ASFixedToFloat(q.br.h) << "," << ASFixedToFloat(q.br.v) << "), ";
						outputFile << "(" << ASFixedToFloat(q.tr.h) << "," << ASFixedToFloat(q.tr.v) << "), ";
						outputFile << "(" << ASFixedToFloat(q.tl.h) << "," << ASFixedToFloat(q.tl.v) << ")]." << std::endl;
					}
				}

				if (opts.bShowCharQuads) // show these in curly braces.
				{
					int nHiliteChars = PDWordGetNumHiliteChar(pdWord); // effectively word length in glyphs, I believe. 
					//Between ligatures and unicode character composition, HiliteChars does not necessarily correspond 1:1 to Unicode Characters.
					int nWordLen = PDWordGetLength(pdWord); // length in Bytes

					for (int hiliteChar = 0; hiliteChar < nHiliteChars; hiliteChar++)
					{
						int byteIdx = PDWordGetByteIdxFromHiliteChar(pdWord, hiliteChar);
						if (byteIdx < nWordLen) //should always be true
						{
							ASFixedQuad q;

							PDWordGetCharQuad(pdWord, byteIdx, &q);
							outputFile << " {(" << ASFixedToFloat(q.bl.h) << "," << ASFixedToFloat(q.bl.v) << "), ";
							outputFile << "(" << ASFixedToFloat(q.br.h) << "," << ASFixedToFloat(q.br.v) << "), ";
							outputFile << "(" << ASFixedToFloat(q.tr.h) << "," << ASFixedToFloat(q.tr.v) << "), ";
							outputFile << "(" << ASFixedToFloat(q.tl.h) << "," << ASFixedToFloat(q.tl.v) << ")}." << std::endl;
						}
					}
				}

				if (WXE_ADJACENT_TO_SPACE & wordAttrs)
				{
					outputFile << " ";
				}

				if ((WXE_LAST_WORD_ON_LINE & wordAttrs) == WXE_LAST_WORD_ON_LINE ||
					(!opts.bNoLastWordInRegion && PDWordIsLastWordInRegion(pdWord)))
				{
					outputFile << std::endl;
				}

				ASfree(utf8String);
				ASTextDestroy(asText);
			}
			PDWordFinderReleaseWordList(pdWordFinder, 0);

		}
		outputFile << "\t\t-fin-" << std::endl;

		//Close any remaining resources. APDFLDoc's destructor will take care of closing the documents.
		outputFile.close();
		PDWordFinderDestroy(pdWordFinder);
	}
	else
	{
		std::cerr << "Error opening output file." << std::endl;
	}

	HANDLER
		errCode = ERRORCODE;
	libInit.displayError(errCode);
	END_HANDLER

		return errCode;
};
