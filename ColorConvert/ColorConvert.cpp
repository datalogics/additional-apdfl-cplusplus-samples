//
// Copyright (c) 2017-2024, Datalogics, Inc. All rights reserved.
//
// Sample demonstrating use of APDFL’s Color Conversion functions with callbacks.
//

#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>

#include "APDFLDoc.h"
#include "InitializeLibrary.h"
#include "AcroColorCalls.h"

#include "PERCalls.h"
#include "PagePDECntCalls.h"
#include "ASCalls.h"

#define INPUT_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT "ducky.pdf"
#define DEF_OUTPUT "ColorConvert-out.pdf"

std::vector<char> ReadFromFile(const char* path)
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

struct myPMClientDataRec {
    ASDuration duration, currValue;
    ASUTF16Val* utf8text;
};

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

void myPDColorConvertReportProc(PDColorConvertObjectAttributes objectType,
    PDColorConvertSpaceType colorSpaceType, PDColorConvertActionType action,
    PDCompletionCode completionCode, PDReasonCode reasonCode, void* userData)
{
    std::cout << "+ ";

    if (objectType & kColorConvObj_Image)
        std::cout << "\t\tImage ";
    else if (objectType & kColorConvObj_Text)
        std::cout << "\t\tText ";
    else if (objectType & kColorConvObj_LineArt)
        std::cout << "\t\tLineArt ";

    if (colorSpaceType & kColorConvDeviceSpace)
        std::cout << "Device ";
    else if (colorSpaceType & kColorConvCalibratedSpace)
        std::cout << "Calibrated ";
    if (colorSpaceType & kColorConvRGBSpace)
        std::cout << "RGB Color Space ";
    if (colorSpaceType & kColorConvCMYKSpace)
        std::cout << "CMYK Color Space ";
    if (colorSpaceType & kColorConvGraySpace)
        std::cout << "Gray Space ";

    switch (action)
    {
        case
            /** Do nothing but handle ink aliases. */
        kColorConvPreserve: std::cout << "preserve "; break;

            /** Convert to target space. */
        case kColorConvConvert: std::cout << "Convert "; break;

            /** Convert calibrated space to device space. */
        case kColorConvDecalibrate: std::cout << "decalibrate "; break;

            /** Convert NChannel space to DeviceN space. */
        case kColorConvDownConvert: std::cout << "To DeviceN "; break;

            /** Convert spot to Alternate Space*/
        case kColorConvToAltSpace:std::cout << "To Alt Space "; break;
        default: break;
    }

    switch (completionCode)
    {
    case PDCompletionSuccess: std::cout << "success "; break;

        /** Continue. */
    case PDCompletionContinue: std::cout << "continue "; break;

        /** Abort. */
    case PDCompletionAbort:std::cout << "Abort "; break;
    default: break;
    }

    if (reasonCode == PDReasonNotImplemented)
        std::cout << "Not Implemented.";

    std::cout << std::endl;
}

int main(int argc, char** argv) {
    ASErrorCode errCode = 0;

    // Initialize the Adobe PDF Library.
    APDFLib lib;
    if (lib.isValid() == false) {
        errCode = lib.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return errCode;
    }

    int curArg = 1;
    int pageNum = 0;
    ASBool bAllPages = FALSE;
    ASBool bEmbed = FALSE;
    ASBool bPreserveBlack = FALSE;
    ASBool bPreserveCMYKPrimaries = FALSE;
    ASBool bGrayToK = FALSE;
    AC_RenderIntent ri = AC_AbsColorimetric;

    char* defaultDescr = "SWOP";
    char* profilePath = NULL;
    char* profileDescrKey = defaultDescr;
    while (argc > curArg)
    {
        if (strcmp(argv[curArg], "-all") == 0)
        {
            bAllPages = TRUE;
        }
        else if (strcmp(argv[curArg], "-pg") == 0)
        {
            pageNum = atoi(argv[++curArg]);
        }
        else if (strcmp(argv[curArg], "-profile") == 0)
        {
            profilePath = argv[++curArg];
        }
        else if (strcmp(argv[curArg], "-profiledescr") == 0)
        {
            profileDescrKey = argv[++curArg];
        }

        else if (strcmp(argv[curArg], "-embed") == 0)
        {
            bEmbed = TRUE;
        }
        else if (strcmp(argv[curArg], "-preserveblack") == 0)
        {
            bPreserveBlack = TRUE;
        }
        else if (strcmp(argv[curArg], "-preservecmyk") == 0)
        {
            bPreserveCMYKPrimaries = TRUE;
        }
        else if (strcmp(argv[curArg], "-graytocmyk") == 0)
        {
            bGrayToK = TRUE;
        }
        else
            break;
        ++curArg;
    }

    std::string csInputFileName(argc > curArg ? argv[curArg] : INPUT_LOC DEF_INPUT);
    ++curArg;
    std::string csOutputFileName(argc > curArg ? argv[curArg] : DEF_OUTPUT);

    AC_Profile iccProfile = NULL;
    char profileDescr[128] = "";
    AC_String profACString;
    AC_Error ret;

    if (profilePath != NULL)
    {
        std::vector<char> targetBuffer = ReadFromFile(profilePath);

        ACMakeBufferProfile(&iccProfile, &targetBuffer[0], targetBuffer.size());
        ASUns32 profCount, bufUsed;
        AC_ColorSpace acCS;

        ACProfileColorSpace(iccProfile, &acCS);
        ret = ACProfileDescription(iccProfile, &profACString);
        ret = ACStringASCII(profACString, profileDescr, &bufUsed, sizeof(profileDescr));
        ACUnReferenceString(profACString);
    }
    else {
        AC_SelectorCode selCodes[] =
        {
            AC_Selector_CMYK_StandardOutput,
            AC_Selector_CMYK_OtherOutputCapable,
            AC_Selector_RGB_Standard,
            AC_Selector_RGB_OtherOutputCapable,
            AC_Selector_Gray_Standard,
            AC_Selector_DotGain_Standard,
            AC_Selector_DotGain_Other,
            AC_Selector_MaxEnum
        };

        int curSel = 0;
        AC_ProfileList profList;
        ASUTF16Val utf16Buf[1024];
        ASUns32 profCount, bufUsed;
        ASBool bFound = FALSE;

        while (!bFound && selCodes[curSel] != AC_Selector_MaxEnum)
        {
            AC_SelectorCode curCode = selCodes[curSel];

            ret = ACMakeProfileList(&profList, curCode);
            ret = ACProfileListCount(profList, &profCount);
            int candidate = 0;

            while (candidate < profCount) {
                ret = ACProfileListItemDescription(profList, candidate, &profACString);
                ret = ACProfileFromDescription(&iccProfile, profACString);
                ret = ACStringASCII(profACString, profileDescr, &bufUsed, sizeof(profileDescr));
                ACUnReferenceString(profACString);
                std::string csCandidate(profileDescr);

                if (csCandidate.find(profileDescrKey) != std::string::npos)
                {
                    bFound = TRUE;
                    break;
                }

                candidate++;
                ACUnReferenceProfile(iccProfile);
                memset(&profileDescr, 0x0, sizeof(profileDescr));
            }
            ACUnReferenceProfileList(profList);
            curSel++;
        }
    }


    std::cout << "setting " << profileDescr << " as OutputIntent for " << csInputFileName.c_str()
        << " and write output to " << csOutputFileName.c_str() << std::endl;

    DURING

        APDFLDoc APDoc(csInputFileName.c_str(), true);
    PDDoc doc = APDoc.getPDDoc();

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
    memset(&myPMclientData, 0x0, sizeof(myPMClientDataRec));

    ASBool bChanged = FALSE;
    ASBool result = FALSE;

    PDColorConvertParamsRecEx convParmsEx;
    memset(&convParmsEx, 0x0, sizeof(PDColorConvertParamsRecEx));
    convParmsEx.mSize = sizeof(PDColorConvertParamsRecEx);
    convParmsEx.mNumActions = 1;
    convParmsEx.intentGray = convParmsEx.intentRGB = convParmsEx.intentCMYK = AC_UseProfileIntent;

    convParmsEx.mActions = reinterpret_cast<PDColorConvertActionEx>(ASmalloc(sizeof(PDColorConvertActionRecEx) * convParmsEx.mNumActions));
    memset(convParmsEx.mActions, 0x0, sizeof(PDColorConvertActionRecEx) * convParmsEx.mNumActions);
    convParmsEx.mActions[0].mSize = sizeof(PDColorConvertActionRec);
    convParmsEx.mActions[0].mMatchAttributesAny = kColorConvObj_AnyObject;
    convParmsEx.mActions[0].mMatchSpaceTypeAny = kColorConvAnySpace;
    convParmsEx.mActions[0].mMatchIntent = AC_UseProfileIntent;
    convParmsEx.mActions[0].mConvertIntent = AC_AbsColorimetric;
    convParmsEx.mActions[0].mAction = kColorConvConvert;
    convParmsEx.mActions[0].mEmbed = bEmbed;
    convParmsEx.mActions[0].mConvertProfile = iccProfile;
    convParmsEx.mActions[0].mPreserveBlack = bPreserveBlack;
    convParmsEx.mActions[0].mPreserveCMYKPrimaries = bPreserveCMYKPrimaries;
    convParmsEx.mActions[0].mPromoteGrayToCMYK = bGrayToK;

    if (!bAllPages)
        result = PDDocColorConvertPageEx(doc, &convParmsEx, pageNum, &myPM, &myPMclientData, myPDColorConvertReportProc, NULL, &bChanged);
    else
    {
        int numPages = PDDocGetNumPages(doc);
        for (int i = 0; i < numPages; i++)
        {
            ASBool bPageChanged = FALSE;
            result |= PDDocColorConvertPageEx(doc, &convParmsEx, i, &myPM, &myPMclientData, myPDColorConvertReportProc, NULL, &bPageChanged);
            bChanged |= bPageChanged;
        }
    }

    ACUnReferenceProfile(iccProfile);
    ASfree(convParmsEx.mActions);

    // if (bChanged)
    APDoc.saveDoc(csOutputFileName.c_str());
    HANDLER
        errCode = ERRORCODE;
    lib.displayError(errCode);
    END_HANDLER

        return errCode;
};