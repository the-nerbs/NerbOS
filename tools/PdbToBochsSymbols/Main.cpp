#include <cstdio>
#include <cinttypes>
#include <string>

#include <Windows.h>
#include <dia2.h>

#include <comdef.h>
#include <comdefsp.h>
#include <tchar.h>

struct CommandLine
{
    std::wstring pdbPath;
    std::wstring outPath;
    bool success;
};

#define DEF_COM_SMARTPTR(I)  _COM_SMARTPTR_TYPEDEF(I, __uuidof(I))
DEF_COM_SMARTPTR(IDiaDataSource);
DEF_COM_SMARTPTR(IDiaSession);
DEF_COM_SMARTPTR(IDiaSymbol);
DEF_COM_SMARTPTR(IDiaEnumSymbols);
DEF_COM_SMARTPTR(IDiaImageData);
DEF_COM_SMARTPTR(IDiaEnumDebugStreams);
DEF_COM_SMARTPTR(IDiaEnumDebugStreamData);
#undef DEF_COM_SMARTPTR

CommandLine ReadCmdLine(int argc, const wchar_t* argv[]);
IDiaImageDataPtr GetImageData(IDiaSession* pSession);
void ThrowIfFailed(HRESULT hr);

class DiaLoadCallbacks : public IUnknown
{
public:
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override
    {
        if (ppvObject == nullptr)
            return E_POINTER;

        if (InlineIsEqualGUID(IID_IUnknown, riid))
        {
            *ppvObject = (void*)static_cast<IUnknown*>(this);
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    virtual ULONG __stdcall AddRef(void) override
    {
        return 0x1000;
    }

    virtual ULONG __stdcall Release(void) override
    {
        return 0x1000;
    }

};


int wmain(int argc, const wchar_t* argv[])
{
    int status = EXIT_FAILURE;

    CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    try
    {
        CommandLine cmdLine = ReadCmdLine(argc, argv);
        if (!cmdLine.success)
        {
            printf("Usage:  <exe> <pdb/exe path> <output path>\n");
            return EXIT_FAILURE;
        }

        FILE* fs = nullptr;
        _wfopen_s(&fs, cmdLine.outPath.c_str(), L"w");

        IDiaDataSourcePtr pDataSource{ CLSID_DiaSource, nullptr, CLSCTX_INPROC_SERVER };
        if (FAILED(pDataSource->loadDataFromPdb(_bstr_t{ cmdLine.pdbPath.c_str() })))
        {
            DiaLoadCallbacks cb;
            ThrowIfFailed(pDataSource->loadDataForExe(_bstr_t{ cmdLine.pdbPath.c_str() }, OLESTR("."), &cb));
        }

        IDiaSessionPtr pSession;
        ThrowIfFailed(pDataSource->openSession(&pSession));

        IDiaImageDataPtr pImageData = GetImageData(pSession);
        ULONGLONG baseAddress;
        if (pImageData != nullptr
            && SUCCEEDED(pImageData->get_imageBase(&baseAddress)))
        {
            ThrowIfFailed(pSession->put_loadAddress(baseAddress));
        }

        IDiaSymbolPtr pGlobalScope;
        ThrowIfFailed(pSession->get_globalScope(&pGlobalScope));

        IDiaEnumSymbolsPtr pEnumCompilands;
        ThrowIfFailed(pGlobalScope->findChildren(SymTagCompiland, nullptr, nsNone, &pEnumCompilands));

        IDiaSymbolPtr pCompiland;
        ULONG celt;
        while (SUCCEEDED(pEnumCompilands->Next(1, &pCompiland, &celt)) && (celt == 1))
        {
            // for debugging purposes, get the object name.
            _bstr_t moduleName;
            pCompiland->get_name(moduleName.GetAddress());

            IDiaEnumSymbolsPtr pEnumChildren;
            HRESULT hr = pCompiland->findChildren(SymTagNull, nullptr, nsNone, &pEnumChildren);
            if (SUCCEEDED(hr))
            {
                IDiaSymbolPtr pSymbol;
                ULONG celtChildren = 0;

                while (SUCCEEDED(pEnumChildren->Next(1, &pSymbol, &celtChildren)) && (celtChildren == 1))
                {
                    _bstr_t symName;
                    if (pSymbol->get_name(symName.GetAddress()) == S_OK)
                    {
                        ULONGLONG va;
                        if (pSymbol->get_virtualAddress(&va) == S_OK)
                        {
                            fprintf(fs, "%" PRIX64 " %s\n", va, static_cast<const char*>(symName));
                        }
                    }
                }
            }
        }

        fflush(fs);
        fclose(fs);
        fs = nullptr;

        status = EXIT_SUCCESS;
    }
    catch (const _com_error& ex)
    {
        _tprintf("Error: %s\n", ex.ErrorMessage());
    }

    return status;
}

CommandLine ReadCmdLine(int argc, const wchar_t* argv[])
{
    CommandLine cmd{};

    if (argc == 3)
    {
        cmd.pdbPath = argv[1];
        cmd.outPath = argv[2];
        cmd.success = true;
    }
    else
    {
        cmd.success = false;
    }

    return cmd;
}
IDiaImageDataPtr GetImageData(IDiaSession* pSession)
{
    IDiaEnumDebugStreamsPtr pStreamsList;
    ThrowIfFailed(pSession->getEnumDebugStreams(&pStreamsList));
    
    LONG numStreams = 0;
    ThrowIfFailed(pStreamsList->get_Count(&numStreams));

    for (LONG i = 0; i < numStreams; i++)
    {
        IDiaEnumDebugStreamDataPtr pStream;
        ULONG fetched = 0;
        pStreamsList->Next(1, &pStream, &fetched);

        if (fetched == 1)
        {
            IDiaImageDataPtr pImageData;
            pStream->QueryInterface(&pImageData);
            if (pImageData != nullptr)
            {
                return pImageData;
            }
        }
    }

    return nullptr;
}

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw _com_error{ hr };
    }
}
