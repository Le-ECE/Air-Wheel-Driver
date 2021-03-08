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
#include "iostream"
#include "fstream"
#include "string"
#include "stdio.h"
#include "stdlib.h"

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
typedef struct _GESTURE {
    std::string name;
    USHORT button;
    SHORT leftX, leftY, rightX, rightY;
    BYTE leftTrig, rightTrig;
} GESTURE, * PGESTURE;

typedef struct _PROFILE {
    std::string name;
    GESTURE list[10];

} PROFILE, * PPROFILE;

typedef struct _SETTINGS {
    PROFILE currentProfile;

} SETTINGS, * PSETTINGS;

// Forward declarations of functions 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
Mat gray_image(Mat img_gray, Mat img_roi);
Mat threshold_image(Mat img_gray, Mat img_roi);
int driverInitialize(void);
void captureGesture(void);
void sendReport(void);

// GUI Variables
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Settings Variables
std::string path;
std::string defaultProfile;
fstream settingsFile;
fstream profileFile;
SETTINGS currentSettings;
PROFILE currentProfile;

// Gesture Variables
int indexOfBiggestContour, sizeOfBiggestContour, fingerCount, ind, i, k;
Mat img, img_threshold, img_gray, img_roi;
VideoCapture cam(0);                            // Initializes webcam capture
GESTURE currentGesture; 						// Current gesture to be determined by hand tracking
XUSB_REPORT report;

// Spawn Virtual Controller
const auto client = vigem_alloc();              // Initializes ViGEm API
const auto retval = vigem_connect(client);      // Establishes connection to driver
const auto pad = vigem_target_x360_alloc();     // Allocates handle for gamepad
const auto pir = vigem_target_add(client, pad); // Adds client to bus (Plugs in controller)

// Used to quickly assign action values
GESTURE configGesture(
    std::string name,
    USHORT button,
    SHORT leftX,
    SHORT leftY,
    SHORT rightX,
    SHORT rightY,
    BYTE leftTrig,
    BYTE rightTrig
) {
    GESTURE assign;

    assign.name = name;
    assign.button = button;
    assign.leftX = leftX;
    assign.leftY = leftY;
    assign.rightX = rightX;
    assign.rightY = rightY;
    assign.leftTrig = leftTrig;
    assign.rightTrig = rightTrig;

    return assign;
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

int driverInitialize(void) {
    XUSB_REPORT_INIT(&report);						// Allocates memory for and initializes gamepad report

    // Checks for Webcam and Driver Initialization Errors   
    if (!cam.isOpened())
        return -1;
	else if (!cam.read(img)) {
        cout << "Cannot read from camera." << std::endl;
        return -1;
    }
    else if (client == nullptr) {
        std::cerr << "Not enough memory for driver." << std::endl;
        return -1;
    }
    else if (!VIGEM_SUCCESS(retval)) {
        std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
        return -1;
    }
    else if (!VIGEM_SUCCESS(pir)) {
        std::cerr << "Target plugin failed with error code: 0x" << std::hex << pir << std::endl;
        return -1;
    }

    // Add code to load profile from file
        //path = (string)(getenv("APPDATA")) + "\Air Wheel Driver\settings.ini";
        //settings.open(appdata, std::fstream::in | std::fstream::out || std::fstream::app);
        //TCHAR profile[100];
        //GetPrivateProfileString(_T("Profile"), _T("name"), _T(""), profile, 100, _T(":\\settings.ini"));

    // Gesture List Initialization (TEST)
    currentProfile.name = "Profile 1";
    currentProfile.list[0] = configGesture("None", 0, 0, 0, 0, 0, 0, 0);
    currentProfile.list[1] = configGesture("1 Finger", XUSB_GAMEPAD_A, 0, 0, 30000, 30000, 255, 255);
    currentProfile.list[2] = configGesture("2 Fingers", XUSB_GAMEPAD_B, 30000, 30000, 0, 0, 0, 0);
    currentProfile.list[3] = configGesture("3 Fingers", XUSB_GAMEPAD_X, 0, 0, 30000, 30000, 255, 255);
    currentProfile.list[4] = configGesture("4 Fingers", XUSB_GAMEPAD_Y, 30000, 30000, 0, 0, 0, 0);
    currentProfile.list[5] = configGesture("5 Fingers", 0x3000, 0, 0, 30000, 30000, 255, 255);

    currentGesture = currentProfile.list[0]; // Default Gesture (This line must be run after assigning gestures)
    return 0;
}

void captureGesture(void) {
	cam.read(img);
    Rect roi(0, 0, img.cols, img.rows);
    img_roi = img(roi);
    img_gray = gray_image(img_gray, img_roi);
    img_threshold = threshold_image(img_gray, img_threshold);

    vector<vector<Point> >contours;
    vector<Vec4i>hierarchy;
    findContours(img_threshold, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point());

    if (contours.size() > 0) {
        indexOfBiggestContour = -1;
        sizeOfBiggestContour = 0;

        for (i = 0; i < contours.size(); i++) {
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

        for (i = 0; i < contours.size(); i++) {
            if (contourArea(contours[i]) > 5000) {
                convexHull(contours[i], hull[i], true);
                convexityDefects(contours[i], hull[i], defects[i]);
                fingerCount = 0;

                if (indexOfBiggestContour == i) {
                    for (k = 0; k < hull[i].size(); k++) {
                        ind = hull[i][k];
                        hullPoint[i].push_back(contours[i][ind]);
                    }
                    for (k = 0; k < defects[i].size(); k++) {
                        if (defects[i][k][3] > 13 * 256) {
                            int p_end = defects[i][k][1];
                            int p_far = defects[i][k][2];
                            defectPoint[i].push_back(contours[i][p_far]);
                            circle(img_roi, contours[i][p_end], 3, Scalar(0, 255, 0), 2);
                            fingerCount++;
                        }
                    }
                }
            }
        }
    }

    if (fingerCount >= 0 && fingerCount <= 5)
        currentGesture = currentProfile.list[fingerCount];
    else
        currentGesture = currentProfile.list[0];

    putText(img, "Count: " + std::to_string(fingerCount), Point(70, 70), FONT_HERSHEY_SIMPLEX, 3, Scalar(255, 0, 0), 2, 8, false);
    imshow("Original_image", img);
}

void sendReport(void) {
    // Button Press
    report.wButtons = currentGesture.button;
    report.sThumbLX = currentGesture.leftX;
    report.sThumbLY = currentGesture.leftY;
    report.sThumbRX = currentGesture.rightX;
    report.sThumbRY = currentGesture.rightY;
    report.bLeftTrigger = currentGesture.leftTrig;
    report.bRightTrigger = currentGesture.rightTrig;
    vigem_target_x360_update(client, pad, report);
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
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    // Message Variables
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AIRWHEELDRIVER));
    MSG msg;

    // Initializes Driver
    if (driverInitialize() == -1)
        return -1;

    /* Main message loop
    Dispatches message to WndProc
    Also used for polling applications
    */
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

		// Polling code
        captureGesture();
        sendReport();
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