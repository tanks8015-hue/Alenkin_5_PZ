// Alenkin_5_PZ.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Alenkin_5_PZ.h"
#define IDC_USERNAME_EDIT 101
#define IDC_PASSWORD_EDIT 102
#define IDC_LOGIN_BUTTON 103
#define MAX_LOADSTRING 100
#include <wincrypt.h> 
#include <sql.h>     
#include <sqlext.h>  
#include <string>

#pragma comment(lib, "advapi32.lib") // Подключаем библиотеку криптографии
// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALENKIN5PZ, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALENKIN5PZ));

    MSG msg;

    // Цикл основного сообщения:
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
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALENKIN5PZ));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ALENKIN5PZ);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

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
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
// Функция для генерации SHA-256 хэша средствами Windows API
std::wstring GenerateSHA256(const std::wstring& input)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::wstring hashStr = L"";

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            if (CryptHashData(hHash, (const BYTE*)input.c_str(), input.length() * sizeof(wchar_t), 0))
            {
                DWORD hashLen = 0;
                DWORD hashLenSize = sizeof(DWORD);
                CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &hashLenSize, 0);

                if (hashLen > 0)
                {
                    BYTE* pbHash = new BYTE[hashLen];
                    if (CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &hashLen, 0))
                    {
                        wchar_t hex[3];
                        for (DWORD i = 0; i < hashLen; i++)
                        {
                            swprintf_s(hex, 3, L"%02x", pbHash[i]);
                            hashStr += hex;
                        }
                    }
                    delete[] pbHash;
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return hashStr;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Глобальные переменные для элементов управления (чтобы считывать с них текст)
    static HWND hUsernameEdit;
    static HWND hPasswordEdit;
    static HWND hLoginButton;

    switch (message)
    {
    case WM_CREATE:
    {
        // Текст "Логин"
        CreateWindowW(L"STATIC", L"Логин:", WS_VISIBLE | WS_CHILD,
            50, 50, 100, 20, hWnd, NULL, hInst, NULL);

        // Поле ввода логина
        hUsernameEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            50, 70, 200, 25, hWnd, (HMENU)IDC_USERNAME_EDIT, hInst, NULL);

        // Текст "Пароль"
        CreateWindowW(L"STATIC", L"Пароль:", WS_VISIBLE | WS_CHILD,
            50, 100, 100, 20, hWnd, NULL, hInst, NULL);

        // Поле ввода пароля (с маской ES_PASSWORD для звездочек)
        hPasswordEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
            50, 120, 200, 25, hWnd, (HMENU)IDC_PASSWORD_EDIT, hInst, NULL);

        // Кнопка "Войти"
        hLoginButton = CreateWindowW(L"BUTTON", L"Войти", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 160, 200, 30, hWnd, (HMENU)IDC_LOGIN_BUTTON, hInst, NULL);

        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        // Проверяем, не нажата ли наша кнопка "Войти"
        if (wmId == IDC_LOGIN_BUTTON)
        {
            // Буферы для чтения логина и пароля
            wchar_t username[100];
            wchar_t password[100];

            GetWindowTextW(hUsernameEdit, username, 100);
            GetWindowTextW(hPasswordEdit, password, 100);

            // 1. Хэшируем пароль
            std::wstring passHash = GenerateSHA256(password);

            // 2. Читаем строку подключения из config.ini
            wchar_t connStr[256];
            GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");

            // Собираем сообщение для проверки (временно, чтобы убедиться, что всё работает)
            std::wstring debugMsg = L"Логин: " + std::wstring(username) +
                L"\nХэш пароля: " + passHash +
                L"\nСтрока БД: " + std::wstring(connStr);

            MessageBoxW(hWnd, debugMsg.c_str(), L"Отладка", MB_OK);
        }

        // Разобрать стандартный выбор в меню:
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
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
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

// Обработчик сообщений для окна "О программе".
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
