#include <comdef.h>
#include <Exdisp.h>
#include <string>
#include <tchar.h>
#include <Windows.h>

using namespace std;

class WebBrowser : public IOleClientSite, public IOleInPlaceSite, public IStorage {
public:
    WebBrowser(HWND hWndParent);

    bool CreateBrowser();

    RECT PixelToHiMetric(const RECT& _rc);

    void SetRect(const RECT& _rc);

    // ----- Control methods -----

    void GoBack();

    void GoForward();

    void Refresh();

    void Navigate(wstring szUrl);

    // ----- IUnknown -----

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef(void);

    ULONG STDMETHODCALLTYPE Release(void);

    // ---------- IOleWindow ----------

    HRESULT STDMETHODCALLTYPE GetWindow(__RPC__deref_out_opt HWND* phwnd) override;

    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode) override;

    // ---------- IOleInPlaceSite ----------

    HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void) override;

    HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void) override;

    HRESULT STDMETHODCALLTYPE OnUIActivate(void) override;

    HRESULT STDMETHODCALLTYPE GetWindowContext(
        __RPC__deref_out_opt IOleInPlaceFrame** ppFrame,
        __RPC__deref_out_opt IOleInPlaceUIWindow** ppDoc,
        __RPC__out LPRECT lprcPosRect,
        __RPC__out LPRECT lprcClipRect,
        __RPC__inout LPOLEINPLACEFRAMEINFO lpFrameInfo) override;

    HRESULT STDMETHODCALLTYPE Scroll(SIZE scrollExtant) override;

    HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL fUndoable) override;

    HWND GetControlWindow();

    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void) override;

    HRESULT STDMETHODCALLTYPE DiscardUndoState(void) override;

    HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void) override;

    HRESULT STDMETHODCALLTYPE OnPosRectChange(__RPC__in LPCRECT lprcPosRect) override;

    // ---------- IOleClientSite ----------

    HRESULT STDMETHODCALLTYPE SaveObject(void) override;

    HRESULT STDMETHODCALLTYPE
    GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, __RPC__deref_out_opt IMoniker** ppmk) override;

    HRESULT STDMETHODCALLTYPE GetContainer(__RPC__deref_out_opt IOleContainer** ppContainer) override;

    HRESULT STDMETHODCALLTYPE ShowObject(void) override;
    HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL fShow) override;

    HRESULT STDMETHODCALLTYPE RequestNewObjectLayout(void) override;

    // ----- IStorage -----

    HRESULT STDMETHODCALLTYPE CreateStream(
        __RPC__in_string const OLECHAR* pwcsName,
        DWORD grfMode,
        DWORD reserved1,
        DWORD reserved2,
        __RPC__deref_out_opt IStream** ppstm) override;

    HRESULT STDMETHODCALLTYPE
    OpenStream(const OLECHAR* pwcsName, void* reserved1, DWORD grfMode, DWORD reserved2, IStream** ppstm) override;

    HRESULT STDMETHODCALLTYPE CreateStorage(
        __RPC__in_string const OLECHAR* pwcsName,
        DWORD grfMode,
        DWORD reserved1,
        DWORD reserved2,
        __RPC__deref_out_opt IStorage** ppstg) override;

    HRESULT STDMETHODCALLTYPE OpenStorage(
        __RPC__in_opt_string const OLECHAR* pwcsName,
        __RPC__in_opt IStorage* pstgPriority,
        DWORD grfMode,
        __RPC__deref_opt_in_opt SNB snbExclude,
        DWORD reserved,
        __RPC__deref_out_opt IStorage** ppstg) override;

    HRESULT STDMETHODCALLTYPE
    CopyTo(DWORD ciidExclude, const IID* rgiidExclude, __RPC__in_opt SNB snbExclude, IStorage* pstgDest) override;

    HRESULT STDMETHODCALLTYPE MoveElementTo(
        __RPC__in_string const OLECHAR* pwcsName,
        __RPC__in_opt IStorage* pstgDest,
        __RPC__in_string const OLECHAR* pwcsNewName,
        DWORD grfFlags) override;

    HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) override;

    HRESULT STDMETHODCALLTYPE Revert(void) override;

    HRESULT STDMETHODCALLTYPE
    EnumElements(DWORD reserved1, void* reserved2, DWORD reserved3, IEnumSTATSTG** ppenum) override;

    HRESULT STDMETHODCALLTYPE DestroyElement(__RPC__in_string const OLECHAR* pwcsName) override;

    HRESULT STDMETHODCALLTYPE
    RenameElement(__RPC__in_string const OLECHAR* pwcsOldName, __RPC__in_string const OLECHAR* pwcsNewName) override;

    HRESULT STDMETHODCALLTYPE SetElementTimes(
        __RPC__in_opt_string const OLECHAR* pwcsName,
        __RPC__in_opt const FILETIME* pctime,
        __RPC__in_opt const FILETIME* patime,
        __RPC__in_opt const FILETIME* pmtime) override;

    HRESULT STDMETHODCALLTYPE SetClass(__RPC__in REFCLSID clsid) override;
    HRESULT STDMETHODCALLTYPE SetStateBits(DWORD grfStateBits, DWORD grfMask) override;

    HRESULT STDMETHODCALLTYPE Stat(__RPC__out STATSTG* pstatstg, DWORD grfStatFlag) override;

protected:
    IOleObject*         oleObject;
    IOleInPlaceObject*  oleInPlaceObject;
    IWebBrowser2*       webBrowser2;
    LONG                iComRefCount;
    RECT                rObject;
    HWND                hWndParent;
    HWND                hWndControl;
};
