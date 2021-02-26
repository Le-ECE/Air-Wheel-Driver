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
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

// ViGEm Imports
#include "ViGEm/Client.h"
#include "ViGEm/Util.h"
#include "ViGEm/Common.h"

// C/C++ Imports
#include <iostream>
#include <string>
#include "stdio.h"
#include <thread>

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
using namespace std;

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
Mat gray_image(Mat img_gray, Mat img_roi);
Mat threshold_image(Mat img_gray, Mat img_roi);

// GUI Variables
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


// Gesture Variables
VideoCapture cam(0);                            // Initializes webcam capture
Mat img, img_threshold, img_gray, img_roi;

// Driver Variables
GESTURE gestureList[10];
SHORT gestureSelect;
GESTURE currentGesture; // Current gesture to be determined by hand tracking


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

Mat gray_image(Mat img_gray, Mat img_roi)
{
    cvtColor(img_roi, img_gray, COLOR_BGR2GRAY);
    GaussianBlur(img_gray, img_gray, Size(19, 19), 0.0, 0);
    return img_gray;
}

Mat threshold_image(Mat img_gray, Mat img_threshold)
{
    threshold(img_gray, img_threshold, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
    return img_threshold;
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


    gestureList[0].gestureName = "No Input";
    gestureList[0].assignedAction = assignAction(0, 'C', 'C', 'U', 'U');        
    gestureList[0].leftThumbSens = 0;
    gestureList[0].rightThumbSens = 0;
    gestureList[0].leftTriggerSens = 0;
    gestureList[0].rightTriggerSens = 0;

    gestureList[1].gestureName = "Button A";
    gestureList[1].assignedAction = assignAction(XUSB_GAMEPAD_A, 'C', 'C', 'U', 'U');
    gestureList[1].leftThumbSens = 0;
    gestureList[1].rightThumbSens = 0;
    gestureList[1].leftTriggerSens = 0;
    gestureList[1].rightTriggerSens = 0;

    gestureList[2].gestureName = "Button B";
    gestureList[2].assignedAction = assignAction(XUSB_GAMEPAD_B, 'C', 'C', 'U', 'U');
    gestureList[2].leftThumbSens = 0;
    gestureList[2].rightThumbSens = 0;
    gestureList[2].leftTriggerSens = 0;
    gestureList[2].rightTriggerSens = 0;

    gestureList[3].gestureName = "Button X";
    gestureList[3].assignedAction = assignAction(XUSB_GAMEPAD_X, 'C', 'C', 'U', 'U');
    gestureList[3].leftThumbSens = 0;
    gestureList[3].rightThumbSens = 0;
    gestureList[3].leftTriggerSens = 0;
    gestureList[3].rightTriggerSens = 0;

    gestureList[4].gestureName = "Button Y";
    gestureList[4].assignedAction = assignAction(XUSB_GAMEPAD_Y, 'C', 'C', 'U', 'U');
    gestureList[4].leftThumbSens = 0;
    gestureList[4].rightThumbSens = 0;
    gestureList[4].leftTriggerSens = 0;
    gestureList[4].rightTriggerSens = 0;

    gestureList[5].gestureName = "Move Forward";
    gestureList[5].assignedAction = assignAction(0, 'N', 'C', 'U', 'U');
    gestureList[5].leftThumbSens = 25;
    gestureList[5].rightThumbSens = 25;
    gestureList[5].leftTriggerSens = 5;
    gestureList[5].rightTriggerSens = 5;

    gestureSelect = 0;
    currentGesture = gestureList[gestureSelect];  // MUST go after gesture declarations
    
    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);

    // Checks for Webcam and Driver Initialization Errors                                                
    if (!cam.isOpened())                        
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

    char a[40];
    int count = 0;
    bool b;

    /* Main message loop
    Dispatches message to WndProc
    Also used for polling applications
    */
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        b = cam.read(img);

        if (!b) {
            std::cerr << "Can't read from webcam." << std::endl;
            return -1;
        }

        Rect roi(0, 0, img.cols, img.rows);
        img_roi = img(roi);
        img_gray = gray_image(img_gray, img_roi);
        img_threshold = threshold_image(img_gray, img_threshold);

                vector<vector<Point> >contours;
                vector<Vec4i>hierarchy;
                findContours(img_threshold, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point());
                if (contours.size() > 0) {
                    int indexOfBiggestContour = -1;
                    int sizeOfBiggestContour = 0;

                    for (int i = 0; i < contours.size(); i++) {
                        if (contours[i].size() > sizeOfBiggestContour) {
                            sizeOfBiggestContour = contours[i].size();
                            indexOfBiggestContour = i;
                        }
                    }
                    vector<vector<int> >hull(contours.size());
                    vector<vector<Point> >hullPoint(contours.size());
                    vector<vector<Vec4i> >defects(contours.size());
                    vector<vector<Point> >defectPoint(contours.size());
                    vector<vector<Point> >contours_poly(contours.size());
                    Point2f rect_point[4];
                    vector<Rect> boundRect(contours.size());
                    for (int i = 0; i < contours.size(); i++) {
                        if (contourArea(contours[i]) > 5000) {
                            convexHull(contours[i], hull[i], true);
                            convexityDefects(contours[i], hull[i], defects[i]);
                            if (indexOfBiggestContour == i) {
                                for (int k = 0; k < hull[i].size(); k++) {
                                    int ind = hull[i][k];
                                    hullPoint[i].push_back(contours[i][ind]);
                                }
                                count = 0;

                                for (int k = 0; k < defects[i].size(); k++) {
                                    if (defects[i][k][3] > 13 * 256) {
                                        /*   int p_start=defects[i][k][0];   */
                                        int p_end = defects[i][k][1];
                                        int p_far = defects[i][k][2];
                                        defectPoint[i].push_back(contours[i][p_far]);
                                        circle(img_roi, contours[i][p_end], 3, Scalar(0, 255, 0), 2);
                                        count++;
                                    }

                                }

                                if (count >= 0 && count <= 5)
                                    currentGesture = gestureList[count];
                                else
                                    currentGesture = gestureList[0];

                                putText(img, "Count: "+std::to_string(count), Point(70, 70), FONT_HERSHEY_SIMPLEX, 3, Scalar(255, 0, 0), 2, 8, false);

                            }
                        }

                    }

                   // if (waitKey(30) == 27) {
                    //    return;
                   // }

                }

        //cap >> frame;
        imshow("Original_image", img);
        //imshow("Webcam Window", frame);

        // Button Press
       report.wButtons = currentGesture.assignedAction.buttonPressed;

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
        600, 600,
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

        if (hWnd == BN_CLICKED && lParam!= 0) {
            if (gestureSelect == 0)
                gestureSelect = 1;
            else if (gestureSelect == 1)
                gestureSelect = 0;
            currentGesture = gestureList[gestureSelect];
        }
    }
    break;
    // create message sent when window is created, in this case is when program opens
    case WM_CREATE:
    {
            CreateWindow(
            L"Button",
            L"Gesture 1",
            WS_VISIBLE | WS_CHILD,
            0, // x
            0, // y
            100,
            40,
            hWnd,
            NULL,
            NULL,
            NULL
            );
        break;

        //CreateWindow(
         //   L"Button",
         //   L"Gesture 2",
         //   WS_VISIBLE | WS_CHILD,
         //   100, // x
         //   100, // y
         //   100,
         //   40,
         //   hWnd,
         //   NULL,
         //   NULL,
         //   NULL
        //);
        //break;
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
