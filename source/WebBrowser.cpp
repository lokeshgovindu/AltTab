#include "WebBrowser.h"

WebBrowser::WebBrowser(HWND _hWndParent) {
    iComRefCount = 0;
    ::SetRect(&rObject, -300, -300, 300, 300);
    hWndParent = _hWndParent;

    if (CreateBrowser() == FALSE) {
        return;
    }

    ShowWindow(GetControlWindow(), SW_SHOW);

    this->Navigate(_T("about:blank"));
}

bool WebBrowser::CreateBrowser() {
    HRESULT hr;
    hr = ::OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW, nullptr, this, this, (void**)&oleObject);

    if (FAILED(hr)) {
        MessageBox(nullptr, _T("Cannot create oleObject CLSID_WebBrowser"), _T("Error"), MB_ICONERROR);
        return FALSE;
    }

    hr = oleObject->SetClientSite(this);
    hr = OleSetContainedObject(oleObject, TRUE);

    RECT posRect;
    ::SetRect(&posRect, -300, -300, 300, 300);
    hr = oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, this, -1, hWndParent, &posRect);
    if (FAILED(hr)) {
        MessageBox(nullptr, _T("oleObject->DoVerb() failed"), _T("Error"), MB_ICONERROR);
        return FALSE;
    }

    hr = oleObject->QueryInterface(&webBrowser2);
    if (FAILED(hr)) {
        MessageBox(nullptr, _T("oleObject->QueryInterface(&webBrowser2) failed"), _T("Error"), MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

RECT WebBrowser::PixelToHiMetric(const RECT& _rc) {
    static bool s_initialized = false;
    static int s_pixelsPerInchX, s_pixelsPerInchY;
    if (!s_initialized) {
        HDC hdc = ::GetDC(NULL);
        s_pixelsPerInchX = ::GetDeviceCaps(hdc, LOGPIXELSX);
        s_pixelsPerInchY = ::GetDeviceCaps(hdc, LOGPIXELSY);
        ::ReleaseDC(0, hdc);
        s_initialized = true;
    }

    RECT rc;
    rc.left   = MulDiv(2540, _rc.left  , s_pixelsPerInchX);
    rc.top    = MulDiv(2540, _rc.top   , s_pixelsPerInchY);
    rc.right  = MulDiv(2540, _rc.right , s_pixelsPerInchX);
    rc.bottom = MulDiv(2540, _rc.bottom, s_pixelsPerInchY);
    return rc;
}

void WebBrowser::SetRect(const RECT& _rc) {
    rObject = _rc;

    {
        RECT hiMetricRect = PixelToHiMetric(rObject);
        SIZEL sz;
        sz.cx = hiMetricRect.right - hiMetricRect.left;
        sz.cy = hiMetricRect.bottom - hiMetricRect.top;
        oleObject->SetExtent(DVASPECT_CONTENT, &sz);
    }

    if (oleInPlaceObject != 0) {
        oleInPlaceObject->SetObjectRects(&rObject, &rObject);
    }
}

// ----- Control methods -----

void WebBrowser::GoBack() {
    this->webBrowser2->GoBack();
}

void WebBrowser::GoForward() {
    this->webBrowser2->GoForward();
}

void WebBrowser::Refresh() {
    this->webBrowser2->Refresh();
}

void WebBrowser::Navigate(wstring szUrl) {
    bstr_t url(szUrl.c_str());
    variant_t flags(0x02u); //navNoHistory
    this->webBrowser2->Navigate(url, &flags, 0, 0, 0);
}

// ----- IUnknown -----

HRESULT STDMETHODCALLTYPE WebBrowser::QueryInterface(REFIID riid, void** ppvObject) {
    if (riid == __uuidof(IUnknown)) {
        (*ppvObject) = static_cast<IOleClientSite*>(this);
    } else if (riid == __uuidof(IOleInPlaceSite)) {
        (*ppvObject) = static_cast<IOleInPlaceSite*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebBrowser::AddRef(void) {
    iComRefCount++;
    return iComRefCount;
}

ULONG STDMETHODCALLTYPE WebBrowser::Release(void) {
    iComRefCount--;
    return iComRefCount;
}

// ---------- IOleWindow ----------

HRESULT STDMETHODCALLTYPE WebBrowser::GetWindow(__RPC__deref_out_opt HWND* phwnd) {
    (*phwnd) = hWndParent;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::ContextSensitiveHelp(BOOL fEnterMode) {
    return E_NOTIMPL;
}

// ---------- IOleInPlaceSite ----------

HRESULT STDMETHODCALLTYPE WebBrowser::CanInPlaceActivate(void) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OnInPlaceActivate(void) {
    OleLockRunning(oleObject, TRUE, FALSE);
    oleObject->QueryInterface(&oleInPlaceObject);
    oleInPlaceObject->SetObjectRects(&rObject, &rObject);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OnUIActivate(void) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::GetWindowContext(
    __RPC__deref_out_opt IOleInPlaceFrame** ppFrame,
    __RPC__deref_out_opt IOleInPlaceUIWindow** ppDoc,
    __RPC__out LPRECT lprcPosRect,
    __RPC__out LPRECT lprcClipRect,
    __RPC__inout LPOLEINPLACEFRAMEINFO lpFrameInfo) {
    HWND hwnd = hWndParent;

    (*ppFrame)                 = nullptr;
    (*ppDoc)                   = nullptr;
    (*lprcPosRect).left        = rObject.left;
    (*lprcPosRect).top         = rObject.top;
    (*lprcPosRect).right       = rObject.right;
    (*lprcPosRect).bottom      = rObject.bottom;
    *lprcClipRect              = *lprcPosRect;

    lpFrameInfo->fMDIApp       = false;
    lpFrameInfo->hwndFrame     = hwnd;
    lpFrameInfo->haccel        = nullptr;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::Scroll(SIZE scrollExtant) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OnUIDeactivate(BOOL fUndoable) {
    return S_OK;
}

HWND WebBrowser::GetControlWindow() {
    if (hWndControl != 0)
        return hWndControl;

    if (oleInPlaceObject == 0)
        return 0;

    oleInPlaceObject->GetWindow(&hWndControl);
    return hWndControl;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OnInPlaceDeactivate(void) {
    hWndControl = 0;
    oleInPlaceObject = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::DiscardUndoState(void) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::DeactivateAndUndo(void) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OnPosRectChange(__RPC__in LPCRECT lprcPosRect) {
    return E_NOTIMPL;
}

// ---------- IOleClientSite ----------

HRESULT STDMETHODCALLTYPE WebBrowser::SaveObject(void) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
WebBrowser::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, __RPC__deref_out_opt IMoniker** ppmk) {
    if ((dwAssign == OLEGETMONIKER_ONLYIFTHERE) && (dwWhichMoniker == OLEWHICHMK_CONTAINER))
        return E_FAIL;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::GetContainer(__RPC__deref_out_opt IOleContainer** ppContainer) {
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE WebBrowser::ShowObject(void) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OnShowWindow(BOOL fShow) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::RequestNewObjectLayout(void) {
    return E_NOTIMPL;
}

// ----- IStorage -----

HRESULT STDMETHODCALLTYPE WebBrowser::CreateStream(
    __RPC__in_string const OLECHAR* pwcsName,
    DWORD grfMode,
    DWORD reserved1,
    DWORD reserved2,
    __RPC__deref_out_opt IStream** ppstm) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
WebBrowser::OpenStream(const OLECHAR* pwcsName, void* reserved1, DWORD grfMode, DWORD reserved2, IStream** ppstm) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::CreateStorage(
    __RPC__in_string const OLECHAR* pwcsName,
    DWORD grfMode,
    DWORD reserved1,
    DWORD reserved2,
    __RPC__deref_out_opt IStorage** ppstg) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::OpenStorage(
    __RPC__in_opt_string const OLECHAR* pwcsName,
    __RPC__in_opt IStorage* pstgPriority,
    DWORD grfMode,
    __RPC__deref_opt_in_opt SNB snbExclude,
    DWORD reserved,
    __RPC__deref_out_opt IStorage** ppstg) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
WebBrowser::CopyTo(DWORD ciidExclude, const IID* rgiidExclude, __RPC__in_opt SNB snbExclude, IStorage* pstgDest) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::MoveElementTo(
    __RPC__in_string const OLECHAR* pwcsName,
    __RPC__in_opt IStorage* pstgDest,
    __RPC__in_string const OLECHAR* pwcsNewName,
    DWORD grfFlags) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::Commit(DWORD grfCommitFlags) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::Revert(void) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
WebBrowser::EnumElements(DWORD reserved1, void* reserved2, DWORD reserved3, IEnumSTATSTG** ppenum) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::DestroyElement(__RPC__in_string const OLECHAR* pwcsName) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
WebBrowser::RenameElement(__RPC__in_string const OLECHAR* pwcsOldName, __RPC__in_string const OLECHAR* pwcsNewName) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::SetElementTimes(
    __RPC__in_opt_string const OLECHAR* pwcsName,
    __RPC__in_opt const FILETIME* pctime,
    __RPC__in_opt const FILETIME* patime,
    __RPC__in_opt const FILETIME* pmtime) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::SetClass(__RPC__in REFCLSID clsid) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebBrowser::SetStateBits(DWORD grfStateBits, DWORD grfMask) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebBrowser::Stat(__RPC__out STATSTG* pstatstg, DWORD grfStatFlag) {
    return E_NOTIMPL;
}
