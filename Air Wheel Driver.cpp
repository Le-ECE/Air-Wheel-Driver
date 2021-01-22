/* Air Wheel Driver.cpp 
Nhat Vu Le
Majed Qarmout
Haoran Zhou
Yu Zhang
*/

// Windows Imports
#include "framework.h"
#include "Windows.h"
#include "Xinput.h"

// OpenCV Imports
#include "Air Wheel Driver.h"
#include "opencv2\opencv.hpp"
#include "opencv2\highgui.hpp"

// ViGEm Imports
#include "ViGEm/Client.h"
#include "ViGEm/Util.h"
#include "ViGEm/Common.h"

// C/C++ Imports
#include "iostream"
#include <string>
#include "stdio.h"

// Additional Libraries
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib,"XInput.lib")
#pragma comment(lib,"Xinput9_1_0.lib")

// Constants and Definitions
#define WIN32_LEAN_AND_MEAN
#define MAX_LOADSTRING 100
#define TRIGGER_MIN 0
#define TRIGGER_MAX 255
#define THUMBSTICK_MIN -32768
#define THUMBSTICK_MAX 32767

// Namespaces
using namespace cv;

// Struct Declarations
typedef struct _CONTROLLER {
   USHORT buttonPressed;
   char leftThumbDirection;
   char rightThumbDirection;
   char leftTriggerPressed;
   char rightTriggerPressed;
} CONTROLLER, * PCONTROLLER;

typedef struct _GESTURE {
    std::string gestureName;
    SHORT leftThumbSens;
    SHORT rightThumbSens;
    SHORT leftTriggerSens;
    SHORT rightTriggerSens;
    CONTROLLER assignedAction;
} GESTURE, * PGESTURE;

// Forward declarations of functions 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// GUI Variables
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
// insert button variables here

// Gesture Variables
VideoCapture cap(0);                            // Initializes webcam capture
Mat frame;                                      // Matrix containing frame values

// Driver Variables
GESTURE gestureList[10];


//char buffer[100];
//if (rep.wButtons & XUSB_GAMEPAD_A) {
//    sprintf_s(buffer, "value: %lu\n", XUSB_GAMEPAD_BACK);
//    OutputDebugStringA(buffer);
//}

// Used to quickly assign action values
CONTROLLER assignAction(
    USHORT buttonPressed,
    char leftThumbDirection,
    char rightThumbDirection,
    char leftTriggerPressed,
    char rightTriggerPressed) 
{
    CONTROLLER toAssign;

    toAssign.buttonPressed = buttonPressed;
    toAssign.leftThumbDirection = leftThumbDirection;
    toAssign.rightThumbDirection = rightThumbDirection;
    toAssign.leftTriggerPressed = leftTriggerPressed;
    toAssign.rightTriggerPressed = rightTriggerPressed;

    return toAssign;
}

/* Main Function of Application
Continually polls the webcam and sends reports to driver
*/
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // Window Variables
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AIRWHEELDRIVER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)){
        return FALSE;
    }

    // Message Variables
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AIRWHEELDRIVER));
    MSG msg;

    // Spawn Virtual Controller
    const auto client = vigem_alloc();              // Initializes ViGEm API
    const auto retval = vigem_connect(client);      // Establishes connection to driver
    const auto pad = vigem_target_x360_alloc();     // Allocates handle for gamepad
    const auto pir = vigem_target_add(client, pad); // Adds client to bus (Plugs in controller)
    
    // Gesture List Initialization (TEST)
    GESTURE currentGesture; // Current gesture to be determined by hand tracking

    gestureList[0].gestureName = "Move Forward";
    gestureList[0].assignedAction = assignAction(0, 'N', 'C', 'U', 'U');        // Reads values from sliders and boxes set by user
    gestureList[0].leftThumbSens = 1;
    gestureList[0].rightThumbSens = 1;
    gestureList[0].leftTriggerSens = 1;
    gestureList[0].rightTriggerSens = 1;

    gestureList[1].gestureName = "Drive Forward";
    gestureList[1].assignedAction = assignAction(0, 'C', 'C', 'U', 'P');
    gestureList[1].leftThumbSens = 1;
    gestureList[1].rightThumbSens = 1;
    gestureList[1].leftTriggerSens = 1;
    gestureList[1].rightTriggerSens = 1;

    gestureList[2].gestureName = "Jump";
    gestureList[2].assignedAction = assignAction(XUSB_GAMEPAD_A, 'C', 'C', 'U', 'U');
    gestureList[2].leftThumbSens = 1;
    gestureList[2].rightThumbSens = 1;
    gestureList[2].leftTriggerSens = 1;
    gestureList[2].rightTriggerSens = 1;

    gestureList[3].gestureName = "Running Jump";
    gestureList[3].assignedAction = assignAction(XUSB_GAMEPAD_A, 'N', 'C', 'U', 'U');
    gestureList[3].leftThumbSens = 1;
    gestureList[3].rightThumbSens = 1;
    gestureList[3].leftTriggerSens = 1;
    gestureList[3].rightTriggerSens = 1;

    currentGesture = gestureList[1];  // MUST go after gesture declarations
    
    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);

    // Checks for Webcam and Driver Initialization Errors                                                
    if (!cap.isOpened())                        
        return -1;
    else if (client == nullptr){
        std::cerr << "Not enough memory for driver." << std::endl;
        return -1;
    }
    else if (!VIGEM_SUCCESS(retval)){
        std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
        return -1;
    }
    else if (!VIGEM_SUCCESS(pir)){
        std::cerr << "Target plugin failed with error code: 0x" << std::hex << pir << std::endl;
        return -1;
    }

    //report.wButtons = 0;
    //vigem_target_x360_update(client, pad, report);
    //Sleep(1000);

    /* Main message loop
    Dispatches message to WndProc
    Also used for polling applications
    */
    while (GetMessage(&msg, nullptr, 0, 0)){
       if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
       }

       // Button Press
       report.wButtons = currentGesture.assignedAction.buttonPressed;
       vigem_target_x360_update(client, pad, report);

       // Left Thumbstick
       if (currentGesture.assignedAction.leftThumbDirection == 'N') {
           if (report.sThumbLY < THUMBSTICK_MAX)
               report.sThumbLY+= currentGesture.leftThumbSens;
       }
       else if (currentGesture.assignedAction.leftThumbDirection == 'S') {
           if (report.sThumbLY > THUMBSTICK_MIN)
               report.sThumbLY-= currentGesture.leftThumbSens;
       }
       else if (currentGesture.assignedAction.leftThumbDirection == 'W') {
           if (report.sThumbLX > THUMBSTICK_MIN)
               report.sThumbLX-= currentGesture.leftThumbSens;
       }
       else if (currentGesture.assignedAction.leftThumbDirection == 'E') {
           if (report.sThumbLX < THUMBSTICK_MAX)
               report.sThumbLX+= currentGesture.leftThumbSens;
       }
       else {
           report.sThumbLX = 0;
           report.sThumbLY = 0;
       }

       // Right Thumbstick
       if (currentGesture.assignedAction.rightThumbDirection == 'N') {
           if (report.sThumbRY < THUMBSTICK_MAX)
               report.sThumbRY+= currentGesture.rightThumbSens;
       }
       else if (currentGesture.assignedAction.rightThumbDirection == 'S') {
           if (report.sThumbRY > THUMBSTICK_MIN)
               report.sThumbRY-= currentGesture.rightThumbSens;
       }
       else if (currentGesture.assignedAction.rightThumbDirection == 'W') {
           if (report.sThumbRX > THUMBSTICK_MIN)
               report.sThumbRX-= currentGesture.rightThumbSens;
       }
       else if (currentGesture.assignedAction.rightThumbDirection == 'E') {
           if (report.sThumbRX < THUMBSTICK_MAX)
               report.sThumbRX+= currentGesture.rightThumbSens;
       }
       else {
           report.sThumbRX = 0;
           report.sThumbRY = 0;
       }

       // Left Trigger
       if (currentGesture.assignedAction.leftTriggerPressed == 'P') {
           if (report.bLeftTrigger < TRIGGER_MAX)
               report.bLeftTrigger += currentGesture.leftTriggerSens;
       }
       else
           report.bLeftTrigger = 0;

       // Right Trigger
       if (currentGesture.assignedAction.rightTriggerPressed == 'P') {
           if (report.bRightTrigger < TRIGGER_MAX)
              report.bRightTrigger+= currentGesture.rightTriggerSens;
       }
       else
           report.bRightTrigger = 0;
      
       vigem_target_x360_update(client, pad, report);
       cap >> frame;
       imshow("Webcam Window", frame);
    }

    return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
// [Window Structure]
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AIRWHEELDRIVER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_AIRWHEELDRIVER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

    // [Window Creation]
    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        200, 200,
        NULL,
        NULL,
        hInstance,
        NULL
    );

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
// [Add functionality  here]
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // button handling
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
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
    // create message sent when window is created, in this case is when program opens
    case WM_CREATE:
    {
            CreateWindow(
            L"Button",
            L"A",
            WS_VISIBLE | WS_CHILD,
            150, // x
            140, // y
            100,
            40,
            hWnd,
            NULL,
            NULL,
            NULL
            );
        break;

        CreateWindow(
            L"Button",
            L"B",
            WS_VISIBLE | WS_CHILD,
            300, // x
            140, // y
            100,
            40,
            hWnd,
            NULL,
            NULL,
            NULL
        );
        break;
    }
    // paint message sent when system or another app makes a request to paint portion of window
    // i.e window snapping and min/max
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...

        TCHAR greeting[] = _T("Hello World!");
        TextOut(hdc, 5, 5, greeting, _tcslen(greeting));

        EndPaint(hWnd, &ps);
    }
    break;
    // when window is closed
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
// Processes messages for about dialog box
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
