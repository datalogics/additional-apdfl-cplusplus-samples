//
// Copyright (c) 2017-2025, Datalogics, Inc. All rights reserved.
//
// A test application for exploring saving PDF files with experimental brotli compression. Not production code.
// The output can be viewed with PDFViewer, or inspected with the .NetFramework PDFObjectExplorer sample.
//
// Command-line: [-brotli] [-quiet] [-compressed] [-removeascii] [-replacelzw] <input-file> <output-file>   
//

#include <iostream>
#include <time.h>
#include "InitializeLibrary.h"
#include "ASExtraCalls.h"
#include "CosCalls.h"
#include "PDCalls.h"
#include "DLExtrasCalls.h"


#include "APDFLDoc.h"

#define INPUT_DIR "../../../../Resources/Sample_Input/"
#define DEF_INPUT "toOptimize.pdf"
#define DEF_OUTPUT "savePDF-out.pdf"


struct myPMClientDataRec {
    ASDuration duration, currValue;
    ASUTF16Val* utf8text;
};
ASBool myCancelProc(void* clientData)
{
    // To actually cancel, return true.
    // To determine which you want to do, you might examine information in
    // clientData that was passed in PDDocSaveParamsRec.cancelProcClientData
    static long i = 0;
    fprintf(stdout, "\tcancel opportunity %d\n", ++i);
    return false;
}

void myPMBeginOperationProc(void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;

    fprintf(stdout, "\tbegin Operation\n");
    cd->duration = 0;
    cd->currValue = 0;
    //	cd->utf8text = NULL;
}

void myPMEndOperationProc(void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;
    fprintf(stdout, "\tEnd Operation %s\n", cd->utf8text);
    if (cd->utf8text != NULL)
        ASfree(cd->utf8text);
    cd->utf8text = NULL;
}

void myPMSetDurationProc(ASDuration duration, void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;
    cd->duration = duration;
}

void myPMSetCurrValueProc(ASDuration currValue, void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;
    if (cd->utf8text != NULL)
        fprintf(stdout, "\t\tCurrently at %d of %d for %s\n",
            currValue, cd->duration, cd->utf8text);
    else
        fprintf(stdout, "\t\tCurrently at %d of %d\n",
            currValue, cd->duration);

    cd->currValue = currValue;
}

ASDuration myPMGetDurationProc(void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;
    return cd->duration;
}

ASDuration myPMGetCurrValueProc(void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;
    return cd->currValue;
}

void myPMSetTextProc(ASText text, void* clientData)
{
    struct myPMClientDataRec* cd = (myPMClientDataRec*)clientData;
    cd->utf8text = ASTextGetUnicodeCopy(text, kUTF8);
}


int main(int argc, char **argv) {
    ASErrorCode errCode = 0;
    APDFLib lib;
    if (lib.isValid() == false) {
        errCode = lib.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return lib.getInitError();
    }

    ASBool bVerbose = true;
    int nOutLevel = 6;
    int curArg = 1;
    ASUns32 saveFlags2 = 0;
    ASBool bSaveUncompressedFirst = FALSE;

    while (argc > curArg)
    {
        if (strcmp(argv[curArg], "-relax") == 0)
        {
            PDPrefSetAllowRelaxedSyntax(true);
        }
        else if (strcmp(argv[curArg], "-quiet") == 0)
        {
            bVerbose = false;
        }
        else if (strcmp(argv[curArg], "-uncompressed") == 0)
        {
            saveFlags2 |= PDSaveUncompressed;
        }
        else if (strcmp(argv[curArg], "-uncompressedFirst") == 0)
        {
            bSaveUncompressedFirst = TRUE;
        }
        else if (strcmp(argv[curArg], "-compressed") == 0)
        {
            saveFlags2 |= PDSaveCompressed;
        }
        else if (strcmp(argv[curArg], "-removeascii") == 0)
        {
            saveFlags2 |= PDSaveRemoveASCIIFilters;
        }
        else if (strcmp(argv[curArg], "-addflate") == 0)
        {
            saveFlags2 |= PDSaveAddFlate;
        }
        else if (strcmp(argv[curArg], "-replacelzw") == 0)
        {
            saveFlags2 |= PDSaveReplaceLZW;
        }
        else if (strcmp(argv[curArg], "-brotli") == 0)
        {
            saveFlags2 |= (1UL << 16); /*PDSaveAddBrotli*/
        }

        else
            break;
        ++curArg;
    }

    std::string csInputFileName(argc > curArg ? argv[curArg] : INPUT_DIR DEF_INPUT);
    ++curArg;
    std::string csOutputFileName(argc > curArg ? argv[curArg] : DEF_OUTPUT);

    if (bVerbose)
    {
        std::cout << "Will open file " << csInputFileName.c_str() << " and save as " << csOutputFileName.c_str() << std::endl;
    }
    PDDoc pdDoc;

    DURING
        ASPathName  InPath = APDFLDoc::makePath(csInputFileName.c_str());
    pdDoc = PDDocOpen(InPath, NULL, NULL, true);
    ASFileSysReleasePath(NULL, InPath);
    HANDLER
        std::cout << "Could not open file " << csInputFileName.c_str() << std::endl;
    APDFLib::displayError(ERRORCODE);
    return ERRORCODE;
    END_HANDLER

        // Determine if copying is permitted from this document
        bool DocumentEncrypted(false);
    DURING
        PDDocAuthorize(pdDoc, pdPermAll, NULL);
    PDDocSetNewCryptHandler(pdDoc, ASAtomNull);
    HANDLER
        DocumentEncrypted = true;
    END_HANDLER

        if (DocumentEncrypted)
        {
            std::cout << "Document is encoded, and copy is not permitted!\n";
            return -2;
        }

    clock_t start_time[2], stop_time[2]; //pcg
    double duration;//pcg

    DURING
        // Save document
        ASPathName path = APDFLDoc::makePath(csOutputFileName.c_str());
       PDDocSaveParamsRec docSaveParamsRec;
        memset(&docSaveParamsRec, 0, sizeof(docSaveParamsRec));
        docSaveParamsRec.size = sizeof(PDDocSaveParamsRec);
        docSaveParamsRec.saveFlags = PDSaveFull | PDSaveCollectGarbage;
        docSaveParamsRec.saveFlags2 = saveFlags2;
        docSaveParamsRec.newPath = path;
        docSaveParamsRec.major = 2;
        docSaveParamsRec.minor = 0;

        if (bVerbose)
        {
            ASProgressMonitorRec myPM;
            myPM.size = sizeof(myPM);
            myPM.beginOperation = myPMBeginOperationProc;
            myPM.endOperation = myPMEndOperationProc;
            myPM.getCurrValue = myPMGetCurrValueProc;
            myPM.getDuration = myPMGetDurationProc;
            myPM.setCurrValue = myPMSetCurrValueProc;
            myPM.setDuration = myPMSetDurationProc;
            myPM.setText = myPMSetTextProc;

            myPMClientDataRec myPMclientData;

            docSaveParamsRec.mon = &myPM;
            docSaveParamsRec.monClientData = (void*)&myPMclientData;
        }

        start_time[0] = clock();
        if (bSaveUncompressedFirst)
        {
            docSaveParamsRec.saveFlags |= PDSaveLeaveOpen | PDSaveKeepModDate;
            docSaveParamsRec.saveFlags2 = PDSaveUncompressed | PDSaveOriginalMetaData;
            docSaveParamsRec.newPath = ASFileSysGetTempPathName(NULL, NULL);
            PDDocSaveWithParams(pdDoc, &docSaveParamsRec);
            ASFileSysReleasePath(NULL,docSaveParamsRec.newPath);
            docSaveParamsRec.newPath = path;
            docSaveParamsRec.saveFlags = PDSaveFull | PDSaveCollectGarbage;
            docSaveParamsRec.saveFlags2 = saveFlags2;
        }
        PDDocSaveWithParams(pdDoc, &docSaveParamsRec);
        stop_time[0] = clock();

    duration = ((double)(stop_time[0] - start_time[0]) / CLOCKS_PER_SEC);

    fprintf(stdout, "%s total time: %2.2f s.\n", csOutputFileName.c_str(), duration);

    ASFileSysReleasePath(NULL, path);

    HANDLER
    {
        errCode = ERRORCODE;
        lib.displayError(errCode);
        std::cout << "Problem writing file \"" << csOutputFileName.c_str() << "\"" << std::endl;
        return -8;
    }
    END_HANDLER
    
    PDDocRelease(pdDoc);
    return 0;
}
