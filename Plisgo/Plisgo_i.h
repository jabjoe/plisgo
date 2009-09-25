

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __Plisgo_i_h__
#define __Plisgo_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPlisgoFolder_FWD_DEFINED__
#define __IPlisgoFolder_FWD_DEFINED__
typedef interface IPlisgoFolder IPlisgoFolder;
#endif 	/* __IPlisgoFolder_FWD_DEFINED__ */


#ifndef __IPlisgoExtractIcon_FWD_DEFINED__
#define __IPlisgoExtractIcon_FWD_DEFINED__
typedef interface IPlisgoExtractIcon IPlisgoExtractIcon;
#endif 	/* __IPlisgoExtractIcon_FWD_DEFINED__ */


#ifndef __IPlisgoView_FWD_DEFINED__
#define __IPlisgoView_FWD_DEFINED__
typedef interface IPlisgoView IPlisgoView;
#endif 	/* __IPlisgoView_FWD_DEFINED__ */


#ifndef __IPlisgoMenu_FWD_DEFINED__
#define __IPlisgoMenu_FWD_DEFINED__
typedef interface IPlisgoMenu IPlisgoMenu;
#endif 	/* __IPlisgoMenu_FWD_DEFINED__ */


#ifndef __IPlisgoData_FWD_DEFINED__
#define __IPlisgoData_FWD_DEFINED__
typedef interface IPlisgoData IPlisgoData;
#endif 	/* __IPlisgoData_FWD_DEFINED__ */


#ifndef __IPlisgoEnumFORMATETC_FWD_DEFINED__
#define __IPlisgoEnumFORMATETC_FWD_DEFINED__
typedef interface IPlisgoEnumFORMATETC IPlisgoEnumFORMATETC;
#endif 	/* __IPlisgoEnumFORMATETC_FWD_DEFINED__ */


#ifndef __IPlisgoDropSource_FWD_DEFINED__
#define __IPlisgoDropSource_FWD_DEFINED__
typedef interface IPlisgoDropSource IPlisgoDropSource;
#endif 	/* __IPlisgoDropSource_FWD_DEFINED__ */


#ifndef __PlisgoFolder_FWD_DEFINED__
#define __PlisgoFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoFolder PlisgoFolder;
#else
typedef struct PlisgoFolder PlisgoFolder;
#endif /* __cplusplus */

#endif 	/* __PlisgoFolder_FWD_DEFINED__ */


#ifndef __PlisgoExtractIcon_FWD_DEFINED__
#define __PlisgoExtractIcon_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoExtractIcon PlisgoExtractIcon;
#else
typedef struct PlisgoExtractIcon PlisgoExtractIcon;
#endif /* __cplusplus */

#endif 	/* __PlisgoExtractIcon_FWD_DEFINED__ */


#ifndef __PlisgoView_FWD_DEFINED__
#define __PlisgoView_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoView PlisgoView;
#else
typedef struct PlisgoView PlisgoView;
#endif /* __cplusplus */

#endif 	/* __PlisgoView_FWD_DEFINED__ */


#ifndef __PlisgoMenu_FWD_DEFINED__
#define __PlisgoMenu_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoMenu PlisgoMenu;
#else
typedef struct PlisgoMenu PlisgoMenu;
#endif /* __cplusplus */

#endif 	/* __PlisgoMenu_FWD_DEFINED__ */


#ifndef __PlisgoData_FWD_DEFINED__
#define __PlisgoData_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoData PlisgoData;
#else
typedef struct PlisgoData PlisgoData;
#endif /* __cplusplus */

#endif 	/* __PlisgoData_FWD_DEFINED__ */


#ifndef __PlisgoEnumFORMATETC_FWD_DEFINED__
#define __PlisgoEnumFORMATETC_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoEnumFORMATETC PlisgoEnumFORMATETC;
#else
typedef struct PlisgoEnumFORMATETC PlisgoEnumFORMATETC;
#endif /* __cplusplus */

#endif 	/* __PlisgoEnumFORMATETC_FWD_DEFINED__ */


#ifndef __PlisgoDropSource_FWD_DEFINED__
#define __PlisgoDropSource_FWD_DEFINED__

#ifdef __cplusplus
typedef class PlisgoDropSource PlisgoDropSource;
#else
typedef struct PlisgoDropSource PlisgoDropSource;
#endif /* __cplusplus */

#endif 	/* __PlisgoDropSource_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IPlisgoFolder_INTERFACE_DEFINED__
#define __IPlisgoFolder_INTERFACE_DEFINED__

/* interface IPlisgoFolder */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoFolder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01AAB0B6-BBF0-41D8-93EC-D1D33CC97F90")
    IPlisgoFolder : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoFolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoFolder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoFolder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoFolder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoFolder * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoFolder * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoFolder * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoFolder * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoFolderVtbl;

    interface IPlisgoFolder
    {
        CONST_VTBL struct IPlisgoFolderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoFolder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoFolder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoFolder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoFolder_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoFolder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoFolder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoFolder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoFolder_INTERFACE_DEFINED__ */


#ifndef __IPlisgoExtractIcon_INTERFACE_DEFINED__
#define __IPlisgoExtractIcon_INTERFACE_DEFINED__

/* interface IPlisgoExtractIcon */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoExtractIcon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("54B1AF8E-C9C3-4B0D-849E-E9922A9B08AC")
    IPlisgoExtractIcon : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoExtractIconVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoExtractIcon * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoExtractIcon * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoExtractIcon * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoExtractIcon * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoExtractIcon * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoExtractIcon * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoExtractIcon * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoExtractIconVtbl;

    interface IPlisgoExtractIcon
    {
        CONST_VTBL struct IPlisgoExtractIconVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoExtractIcon_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoExtractIcon_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoExtractIcon_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoExtractIcon_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoExtractIcon_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoExtractIcon_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoExtractIcon_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoExtractIcon_INTERFACE_DEFINED__ */


#ifndef __IPlisgoView_INTERFACE_DEFINED__
#define __IPlisgoView_INTERFACE_DEFINED__

/* interface IPlisgoView */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("67876E11-00DD-4769-B6EF-DAB4C647851B")
    IPlisgoView : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoView * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoView * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoView * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoView * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoView * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoViewVtbl;

    interface IPlisgoView
    {
        CONST_VTBL struct IPlisgoViewVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoView_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoView_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoView_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoView_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoView_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoView_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoView_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoView_INTERFACE_DEFINED__ */


#ifndef __IPlisgoMenu_INTERFACE_DEFINED__
#define __IPlisgoMenu_INTERFACE_DEFINED__

/* interface IPlisgoMenu */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0D761E7E-9D3C-41FE-B250-C181AE25BD89")
    IPlisgoMenu : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoMenu * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoMenu * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoMenu * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoMenu * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoMenuVtbl;

    interface IPlisgoMenu
    {
        CONST_VTBL struct IPlisgoMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoMenu_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoMenu_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoMenu_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoMenu_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoMenu_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoMenu_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoMenu_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoMenu_INTERFACE_DEFINED__ */


#ifndef __IPlisgoData_INTERFACE_DEFINED__
#define __IPlisgoData_INTERFACE_DEFINED__

/* interface IPlisgoData */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FF9CD024-4DF9-43F5-BEF1-7A79D5A1C844")
    IPlisgoData : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoData * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoData * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoData * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoData * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoDataVtbl;

    interface IPlisgoData
    {
        CONST_VTBL struct IPlisgoDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoData_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoData_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoData_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoData_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoData_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoData_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoData_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoData_INTERFACE_DEFINED__ */


#ifndef __IPlisgoEnumFORMATETC_INTERFACE_DEFINED__
#define __IPlisgoEnumFORMATETC_INTERFACE_DEFINED__

/* interface IPlisgoEnumFORMATETC */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoEnumFORMATETC;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B12ADF19-B092-445B-9468-3E190A16C424")
    IPlisgoEnumFORMATETC : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoEnumFORMATETCVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoEnumFORMATETC * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoEnumFORMATETC * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoEnumFORMATETC * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoEnumFORMATETC * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoEnumFORMATETC * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoEnumFORMATETC * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoEnumFORMATETC * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoEnumFORMATETCVtbl;

    interface IPlisgoEnumFORMATETC
    {
        CONST_VTBL struct IPlisgoEnumFORMATETCVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoEnumFORMATETC_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoEnumFORMATETC_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoEnumFORMATETC_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoEnumFORMATETC_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoEnumFORMATETC_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoEnumFORMATETC_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoEnumFORMATETC_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoEnumFORMATETC_INTERFACE_DEFINED__ */


#ifndef __IPlisgoDropSource_INTERFACE_DEFINED__
#define __IPlisgoDropSource_INTERFACE_DEFINED__

/* interface IPlisgoDropSource */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IPlisgoDropSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A8859C3-EB36-42DC-A5C7-96EFF3BBFD64")
    IPlisgoDropSource : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPlisgoDropSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPlisgoDropSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPlisgoDropSource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPlisgoDropSource * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPlisgoDropSource * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPlisgoDropSource * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPlisgoDropSource * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPlisgoDropSource * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPlisgoDropSourceVtbl;

    interface IPlisgoDropSource
    {
        CONST_VTBL struct IPlisgoDropSourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPlisgoDropSource_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPlisgoDropSource_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPlisgoDropSource_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPlisgoDropSource_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPlisgoDropSource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPlisgoDropSource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPlisgoDropSource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPlisgoDropSource_INTERFACE_DEFINED__ */



#ifndef __PlisgoLib_LIBRARY_DEFINED__
#define __PlisgoLib_LIBRARY_DEFINED__

/* library PlisgoLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_PlisgoLib;

EXTERN_C const CLSID CLSID_PlisgoFolder;

#ifdef __cplusplus

class DECLSPEC_UUID("ADA19F85-EEB6-46F2-B8B2-2BD977934A79")
PlisgoFolder;
#endif

EXTERN_C const CLSID CLSID_PlisgoExtractIcon;

#ifdef __cplusplus

class DECLSPEC_UUID("11E54957-7EED-4F3C-A4C1-46788757EC62")
PlisgoExtractIcon;
#endif

EXTERN_C const CLSID CLSID_PlisgoView;

#ifdef __cplusplus

class DECLSPEC_UUID("E8716A04-546B-4DB1-BB55-0A230E20A0C5")
PlisgoView;
#endif

EXTERN_C const CLSID CLSID_PlisgoMenu;

#ifdef __cplusplus

class DECLSPEC_UUID("BBF30838-6C39-4D2B-83FA-4654A3725674")
PlisgoMenu;
#endif

EXTERN_C const CLSID CLSID_PlisgoData;

#ifdef __cplusplus

class DECLSPEC_UUID("3F57F545-286B-4881-8484-466812610AC8")
PlisgoData;
#endif

EXTERN_C const CLSID CLSID_PlisgoEnumFORMATETC;

#ifdef __cplusplus

class DECLSPEC_UUID("41788118-D766-476B-A44A-9DA50DD1B96D")
PlisgoEnumFORMATETC;
#endif

EXTERN_C const CLSID CLSID_PlisgoDropSource;

#ifdef __cplusplus

class DECLSPEC_UUID("257BAE7D-AD3C-4F0F-9232-B9B18E546106")
PlisgoDropSource;
#endif
#endif /* __PlisgoLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


