#include <stdint.h>
#include <windows.h>
#include <xinput.h>

struct Win32_ScreenBuffer {
    BITMAPINFO Info;
    void* Buffer;
    int Width;
    int Height;
    int BytesPixel;
};

struct Win32_Size {
    int Width;
    int Height;
};

static bool Running;
static Win32_ScreenBuffer MainScreenBuffer;

#define XInputGetStatePrototype(Name) DWORD Name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define XInputSetStatePrototype(Name) DWORD Name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

#define XInputDeclare(Function) \
Function##Prototype(Function##Stub) { return 0; } \
typedef Function##Prototype(Function##Func); \
static Function##Func* Function##Ptr = Function##Stub; \

XInputDeclare(XInputGetState)
XInputDeclare(XInputSetState)

#define XInputGetState XInputGetStatePtr
#define XInputSetState XInputSetStatePtr

Win32_Size Win32_GetWindowSize(HWND Window){
    RECT Rect;
    GetClientRect(Window, &Rect);
    return Win32_Size { Rect.right - Rect.left, Rect.bottom - Rect.top };
}

void Win32_ResizeScreenBuffer(Win32_ScreenBuffer* ScreenBuffer, int Width, int Height){

    if(ScreenBuffer->Buffer){
        VirtualFree(ScreenBuffer->Buffer, 0, MEM_RELEASE);
    }

    ScreenBuffer->Width = Width;
    ScreenBuffer->Height = Height;
    ScreenBuffer->BytesPixel = 4;

    ScreenBuffer->Info.bmiHeader.biSize = sizeof(ScreenBuffer->Info.bmiHeader);
    ScreenBuffer->Info.bmiHeader.biWidth = ScreenBuffer->Width;
    ScreenBuffer->Info.bmiHeader.biHeight = -ScreenBuffer->Height;
    ScreenBuffer->Info.bmiHeader.biPlanes = 1;
    ScreenBuffer->Info.bmiHeader.biBitCount = 32;
    ScreenBuffer->Info.bmiHeader.biCompression = BI_RGB;
    
    const int BufferSize = ScreenBuffer->Width * ScreenBuffer->Height * ScreenBuffer->BytesPixel;
    ScreenBuffer->Buffer = VirtualAlloc(0, BufferSize, MEM_COMMIT, PAGE_READWRITE);
}

void Win32_DisplayScreenBuffer(Win32_ScreenBuffer ScreenBuffer, HDC DeviceContext, int Width, int Height){
    StretchDIBits(DeviceContext, 
                  0, 0, Width, Height, 
                  0, 0, ScreenBuffer.Width, ScreenBuffer.Height,
                  ScreenBuffer.Buffer, &ScreenBuffer.Info,
                  DIB_RGB_COLORS, SRCCOPY
    );
}

void Win32_LoadXInput(){
    HMODULE XInputLib = LoadLibrary(XINPUT_DLL_A);

    if(XInputLib){
        XInputGetState = (XInputGetStateFunc*)GetProcAddress(XInputLib, "XInputGetState");
        XInputSetState = (XInputSetStateFunc*)GetProcAddress(XInputLib, "XInputSetState");
    }
}

void Render(Win32_ScreenBuffer ScreenBuffer, int BlueOffset, int GreenOffset){
    uint8_t* Row = (uint8_t*)ScreenBuffer.Buffer;

    for(int Y = 0; Y < ScreenBuffer.Height; ++Y){
        uint32_t* Pixel = (uint32_t*)Row;
        for(int X = 0; X < ScreenBuffer.Width; ++X){
            uint8_t Blue = X + BlueOffset;
            uint8_t Green = Y + GreenOffset;
            *(Pixel++) = (Green << 8) | Blue;
        }
        Row += ScreenBuffer.Width * ScreenBuffer.BytesPixel;
    }
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){
    LRESULT Result = 0;

    switch(Message){
        case WM_DESTROY:
            Running = false;
            OutputDebugStringA("WM_DESTROY\n");
            break;
        case WM_CLOSE:
            Running = false;
            OutputDebugStringA("WM_CLOSE\n");
            break;
        case WM_ACTIVATEAPP:
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;
        case WM_SIZE:
            OutputDebugStringA("WM_SIZE\n");
            break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32_t KeyCode = WParam;
            bool WasDown = (LParam & (1 << 30)) != 0;
            bool isDown = (LParam & (1 << 31)) == 0;

            if(WasDown != isDown){
                if(KeyCode == 'W'){
                    OutputDebugStringA("W\n");
                } else if (KeyCode == 'A'){
                    OutputDebugStringA("A\n");
                } else if(KeyCode == 'S'){
                    OutputDebugStringA("S\n");
                } else if(KeyCode == 'D'){
                    OutputDebugStringA("D\n");
                }
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            Win32_Size WindowSize = Win32_GetWindowSize(Window);
            Win32_DisplayScreenBuffer(MainScreenBuffer, DeviceContext, WindowSize.Width, WindowSize.Height);
            EndPaint(Window, &Paint);
            break;
        }
        default:
            Result = DefWindowProc(Window, Message, WParam, LParam);
            break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow){
    
    Win32_LoadXInput();

    WNDCLASS WindowClass{}; // MainWindow class
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = &MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindow";

    RegisterClass(&WindowClass); // TODO: test on failure later
    HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "HandmadeHero",
                                       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                       CW_USEDEFAULT, 0, 0, Instance, 0
    ); // TODO: test on failure later

    Win32_ResizeScreenBuffer(&MainScreenBuffer, 1280, 720);
    Running = true;

    int BlueOffset = 0;
    int GreenOffset = 0;

    while(Running){
        MSG Message{};
        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
            if(Message.message == WM_QUIT)
                exit(-1); // TODO: Flush the resources and close the window?
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

        for(DWORD ControllerIdx = 0; ControllerIdx < XUSER_MAX_COUNT; ++ControllerIdx){
            XINPUT_STATE InputState{};
            if(XInputGetState(ControllerIdx, &InputState)){
                XINPUT_GAMEPAD* Gamepad = &InputState.Gamepad;

                bool Up = Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                bool Down = Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                bool Left = Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                bool Right = Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                bool Start = Gamepad->wButtons & XINPUT_GAMEPAD_START;
                bool Back = Gamepad->wButtons & XINPUT_GAMEPAD_BACK;
                bool LeftShoulder = Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                bool RightShoulder = Gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                bool AButton = Gamepad->wButtons & XINPUT_GAMEPAD_A;
                bool BButton = Gamepad->wButtons & XINPUT_GAMEPAD_B;
                bool XButton = Gamepad->wButtons & XINPUT_GAMEPAD_X;
                bool YButton = Gamepad->wButtons & XINPUT_GAMEPAD_Y;

                int16_t StickX = Gamepad->sThumbLX;
                int16_t StickY = Gamepad->sThumbLY;

            } else {
                // Controller is not available
            }
        }

        Render(MainScreenBuffer, BlueOffset, GreenOffset);
        HDC DeviceContext = GetDC(Window);
        Win32_Size WindowSize = Win32_GetWindowSize(Window);
        Win32_DisplayScreenBuffer(MainScreenBuffer, DeviceContext, WindowSize.Width, WindowSize.Height);
        ReleaseDC(Window,  DeviceContext);

        ++BlueOffset;
    }

    return 0;
}