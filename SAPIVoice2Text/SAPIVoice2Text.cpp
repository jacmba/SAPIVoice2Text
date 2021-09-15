// SAPIVoice2Text.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SAPIVoice2Text.h"

#define MAX_LOADSTRING 100
#define WM_RECOEVENT WM_USER+1

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void launchRecognition(HWND);
void stopRecognition();
void handleEvent(HWND);
LPCWSTR extractInput(CSpEvent);

HWND resultWnd;
CComPtr<ISpRecognizer> cpEngine;
CComPtr<ISpRecoContext> cpRecoCtx;
CComPtr<ISpRecoGrammar> cpRecoGrammar;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SAPIVOICE2TEXT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SAPIVOICE2TEXT));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAPIVOICE2TEXT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SAPIVOICE2TEXT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_RECOEVENT:
		handleEvent(hWnd);
		break;
	case WM_CREATE:
		resultWnd = CreateWindow(L"Static", L"Foo bar", WS_CHILD | WS_VISIBLE, 50, 50, 600, 400, hWnd, 0, hInst, 0);
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDM_FILE_START_RECOGNITION:
				SetWindowText(resultWnd, L"Starting voice recognition...");
				try {
					launchRecognition(hWnd);
				}
				catch (LPCWSTR e) {
					MessageBox(hWnd, e, L"Error", MB_ICONERROR);
				}
				break;
			case IDM_FILE_STOP_RECOGNITION:
				SetWindowText(resultWnd, L"Stopping voice recognition...");
				stopRecognition();
				break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Start voice recognition
void launchRecognition(HWND hwnd) {
	if(FAILED(::CoInitialize(NULL))) {
		throw L"Unable to initialize COM objects";
	}

	ULONGLONG ullGramId = 1;
	HRESULT hr = cpEngine.CoCreateInstance(CLSID_SpSharedRecognizer);
	if (FAILED(hr)) {
		throw L"Unable to create recognition engine";
	}

	hr = cpEngine->CreateRecoContext(&cpRecoCtx);
	if (FAILED(hr)) {
		throw L"Failed command recognition";
	}

	hr = cpRecoCtx->SetNotifyWindowMessage(hwnd, WM_RECOEVENT, 0, 0);
	if (FAILED(hr)) {
		throw L"Failed setting notification window";
	}

	ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) |
		SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_FALSE_RECOGNITION) |
		SPFEI(SPEI_HYPOTHESIS) | SPFEI(SPEI_INTERFERENCE) | SPFEI(SPEI_RECO_OTHER_CONTEXT) |
		SPFEI(SPEI_REQUEST_UI) | SPFEI(SPEI_RECO_STATE_CHANGE) |
		SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE);
	hr = cpRecoCtx->SetInterest(ullInterest, ullInterest);
	if (FAILED(hr)) {
		throw L"Failed to create interest";
	}

	hr = cpRecoCtx->CreateGrammar(ullGramId, &cpRecoGrammar);
	if (FAILED(hr)) {
		throw L"Unable to create grammar";
	}

	hr = cpRecoGrammar->LoadDictation(0, SPLO_STATIC);
	if (FAILED(hr)) {
		throw L"Failed to load dictation";
	}

	hr = cpRecoGrammar->SetDictationState(SPRS_ACTIVE);
	if (FAILED(hr)) {
		throw L"Failed activating dictation";
	}
}

// Stop voice recognition
void stopRecognition() {
	if (cpRecoGrammar) {
		cpRecoGrammar.Release();
	}
	if (cpRecoCtx) {
		cpRecoCtx->SetNotifySink(NULL);
		cpRecoCtx.Release();
	}
	if (cpEngine) {
		cpEngine.Release();
	}
	CoUninitialize();
}

// Handle voice events
void handleEvent(HWND hwnd) {
	CSpEvent event;
	LPCWSTR text = NULL;

	while(event.GetFrom(cpRecoCtx) == S_OK) {
		switch (event.eEventId)
		{
		case SPEI_HYPOTHESIS:
			text = extractInput(event);
			break;
		default:
			text = L"Received some voice event";
			break;
		}

		if (text != NULL) {
			SetWindowText(resultWnd, text);
		}
	}
}

// Attempt to extract given phrase
LPCWSTR extractInput(CSpEvent event) {
	HRESULT hr = S_OK;
	CComPtr<ISpRecoResult> cpRecoResult;
	SPPHRASE *phrase;
	WCHAR *text;

	cpRecoResult = event.RecoResult();

	hr = cpRecoResult->GetPhrase(&phrase);

	if (SUCCEEDED(hr)) {
		if (event.eEventId == SPEI_FALSE_RECOGNITION) {
			CoTaskMemFree(phrase);
			return L"False recognition";
		}
		else {
			hr = cpRecoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &text, NULL);
		}
	}
	else {
		CoTaskMemFree(phrase);
		return L"Failed extracting phrase";
	}

	CoTaskMemFree(phrase);
	return text;
}
