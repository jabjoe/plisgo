

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Thu Sep 10 10:47:26 2009
 */
/* Compiler settings for .\Plisgo.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "Plisgo_i.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _Plisgo_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } Plisgo_MIDL_TYPE_FORMAT_STRING;

typedef struct _Plisgo_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } Plisgo_MIDL_PROC_FORMAT_STRING;

typedef struct _Plisgo_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } Plisgo_MIDL_EXPR_FORMAT_STRING;


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const Plisgo_MIDL_TYPE_FORMAT_STRING Plisgo__MIDL_TypeFormatString;
extern const Plisgo_MIDL_PROC_FORMAT_STRING Plisgo__MIDL_ProcFormatString;
extern const Plisgo_MIDL_EXPR_FORMAT_STRING Plisgo__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoFolder_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoFolder_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoExtractIcon_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoExtractIcon_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoView_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoView_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoMenu_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoMenu_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoData_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoData_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoEnumFORMATETC_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoEnumFORMATETC_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPlisgoDropSource_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPlisgoDropSource_ProxyInfo;



#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const Plisgo_MIDL_PROC_FORMAT_STRING Plisgo__MIDL_ProcFormatString =
    {
        0,
        {

			0x0
        }
    };

static const Plisgo_MIDL_TYPE_FORMAT_STRING Plisgo__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IPlisgoFolder, ver. 0.0,
   GUID={0x01AAB0B6,0xBBF0,0x41D8,{0x93,0xEC,0xD1,0xD3,0x3C,0xC9,0x7F,0x90}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoFolder_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoFolder_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoFolder_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoFolder_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoFolder_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoFolderProxyVtbl = 
{
    0,
    &IID_IPlisgoFolder,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoFolder_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoFolderStubVtbl =
{
    &IID_IPlisgoFolder,
    &IPlisgoFolder_ServerInfo,
    7,
    &IPlisgoFolder_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPlisgoExtractIcon, ver. 0.0,
   GUID={0x54B1AF8E,0xC9C3,0x4B0D,{0x84,0x9E,0xE9,0x92,0x2A,0x9B,0x08,0xAC}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoExtractIcon_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoExtractIcon_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoExtractIcon_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoExtractIcon_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoExtractIcon_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoExtractIconProxyVtbl = 
{
    0,
    &IID_IPlisgoExtractIcon,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoExtractIcon_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoExtractIconStubVtbl =
{
    &IID_IPlisgoExtractIcon,
    &IPlisgoExtractIcon_ServerInfo,
    7,
    &IPlisgoExtractIcon_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPlisgoView, ver. 0.0,
   GUID={0x67876E11,0x00DD,0x4769,{0xB6,0xEF,0xDA,0xB4,0xC6,0x47,0x85,0x1B}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoView_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoView_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoView_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoView_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoView_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoViewProxyVtbl = 
{
    0,
    &IID_IPlisgoView,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoView_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoViewStubVtbl =
{
    &IID_IPlisgoView,
    &IPlisgoView_ServerInfo,
    7,
    &IPlisgoView_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPlisgoMenu, ver. 0.0,
   GUID={0x0D761E7E,0x9D3C,0x41FE,{0xB2,0x50,0xC1,0x81,0xAE,0x25,0xBD,0x89}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoMenu_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoMenu_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoMenu_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoMenu_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoMenu_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoMenuProxyVtbl = 
{
    0,
    &IID_IPlisgoMenu,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoMenu_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoMenuStubVtbl =
{
    &IID_IPlisgoMenu,
    &IPlisgoMenu_ServerInfo,
    7,
    &IPlisgoMenu_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPlisgoData, ver. 0.0,
   GUID={0xFF9CD024,0x4DF9,0x43F5,{0xBE,0xF1,0x7A,0x79,0xD5,0xA1,0xC8,0x44}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoData_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoData_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoData_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoData_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoData_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoDataProxyVtbl = 
{
    0,
    &IID_IPlisgoData,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoData_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoDataStubVtbl =
{
    &IID_IPlisgoData,
    &IPlisgoData_ServerInfo,
    7,
    &IPlisgoData_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPlisgoEnumFORMATETC, ver. 0.0,
   GUID={0xB12ADF19,0xB092,0x445B,{0x94,0x68,0x3E,0x19,0x0A,0x16,0xC4,0x24}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoEnumFORMATETC_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoEnumFORMATETC_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoEnumFORMATETC_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoEnumFORMATETC_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoEnumFORMATETC_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoEnumFORMATETCProxyVtbl = 
{
    0,
    &IID_IPlisgoEnumFORMATETC,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoEnumFORMATETC_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoEnumFORMATETCStubVtbl =
{
    &IID_IPlisgoEnumFORMATETC,
    &IPlisgoEnumFORMATETC_ServerInfo,
    7,
    &IPlisgoEnumFORMATETC_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPlisgoDropSource, ver. 0.0,
   GUID={0x4A8859C3,0xEB36,0x42DC,{0xA5,0xC7,0x96,0xEF,0xF3,0xBB,0xFD,0x64}} */

#pragma code_seg(".orpc")
static const unsigned short IPlisgoDropSource_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IPlisgoDropSource_ProxyInfo =
    {
    &Object_StubDesc,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoDropSource_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPlisgoDropSource_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Plisgo__MIDL_ProcFormatString.Format,
    &IPlisgoDropSource_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IPlisgoDropSourceProxyVtbl = 
{
    0,
    &IID_IPlisgoDropSource,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *) (INT_PTR) -1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IPlisgoDropSource_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IPlisgoDropSourceStubVtbl =
{
    &IID_IPlisgoDropSource,
    &IPlisgoDropSource_ServerInfo,
    7,
    &IPlisgoDropSource_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    Plisgo__MIDL_TypeFormatString.Format,
    0, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x70001f4, /* MIDL Version 7.0.500 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * _Plisgo_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IPlisgoViewProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPlisgoEnumFORMATETCProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPlisgoDataProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPlisgoMenuProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPlisgoExtractIconProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPlisgoFolderProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPlisgoDropSourceProxyVtbl,
    0
};

const CInterfaceStubVtbl * _Plisgo_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IPlisgoViewStubVtbl,
    ( CInterfaceStubVtbl *) &_IPlisgoEnumFORMATETCStubVtbl,
    ( CInterfaceStubVtbl *) &_IPlisgoDataStubVtbl,
    ( CInterfaceStubVtbl *) &_IPlisgoMenuStubVtbl,
    ( CInterfaceStubVtbl *) &_IPlisgoExtractIconStubVtbl,
    ( CInterfaceStubVtbl *) &_IPlisgoFolderStubVtbl,
    ( CInterfaceStubVtbl *) &_IPlisgoDropSourceStubVtbl,
    0
};

PCInterfaceName const _Plisgo_InterfaceNamesList[] = 
{
    "IPlisgoView",
    "IPlisgoEnumFORMATETC",
    "IPlisgoData",
    "IPlisgoMenu",
    "IPlisgoExtractIcon",
    "IPlisgoFolder",
    "IPlisgoDropSource",
    0
};

const IID *  _Plisgo_BaseIIDList[] = 
{
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    0
};


#define _Plisgo_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _Plisgo, pIID, n)

int __stdcall _Plisgo_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _Plisgo, 7, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _Plisgo, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _Plisgo, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _Plisgo, 7, *pIndex )
    
}

const ExtendedProxyFileInfo Plisgo_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _Plisgo_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _Plisgo_StubVtblList,
    (const PCInterfaceName * ) & _Plisgo_InterfaceNamesList,
    (const IID ** ) & _Plisgo_BaseIIDList,
    & _Plisgo_IID_Lookup, 
    7,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#pragma optimize("", on )
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

