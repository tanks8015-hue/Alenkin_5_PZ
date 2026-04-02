// Alenkin_5_PZ.cpp
#include "framework.h"
#include "Alenkin_5_PZ.h"
#include <wincrypt.h> 
#include <sql.h>      
#include <sqlext.h>   
#include <string>

#pragma comment(lib, "advapi32.lib")

#define MAX_LOADSTRING 100

// ID для элементов интерфейса
#define IDC_USERNAME_EDIT 101
#define IDC_PASSWORD_EDIT 102
#define IDC_LOGIN_BUTTON 103

// Глобальные переменные:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
WCHAR szDashboardClass[] = L"DashboardWindow";

// АНОНСЫ (Объявления функций):
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DashboardWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Функция для генерации SHA-256 хэша
std::wstring GenerateSHA256(const std::wstring& input)
{
    // Броня от пустого пароля (чтобы не было вылета)
    if (input.empty()) return L"";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.length(), NULL, 0, NULL, NULL);
    std::string utf8_input(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), (int)input.length(), &utf8_input[0], size_needed, NULL, NULL);

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::wstring hashStr = L"";

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            if (CryptHashData(hHash, (const BYTE*)utf8_input.c_str(), (DWORD)utf8_input.length(), 0))
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALENKIN5PZ, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    // ВАЖНО: {0} очищает мусор в оперативной памяти!
    WNDCLASSEXW wcexDash = { 0 };
    wcexDash.cbSize = sizeof(WNDCLASSEX);
    wcexDash.style = CS_HREDRAW | CS_VREDRAW;
    wcexDash.lpfnWndProc = DashboardWndProc;
    wcexDash.cbClsExtra = 0;
    wcexDash.cbWndExtra = 0;
    wcexDash.hInstance = hInstance;
    wcexDash.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALENKIN5PZ));
    wcexDash.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexDash.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexDash.lpszMenuName = NULL;
    wcexDash.lpszClassName = szDashboardClass;
    wcexDash.hIconSm = LoadIcon(wcexDash.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassExW(&wcexDash);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALENKIN5PZ));
    MSG msg = { 0 };

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALENKIN5PZ));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ALENKIN5PZ);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hUsernameEdit;
    static HWND hPasswordEdit;
    static HWND hLoginButton;

    switch (message)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Логин:", WS_VISIBLE | WS_CHILD, 50, 50, 100, 20, hWnd, NULL, hInst, NULL);
        hUsernameEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 50, 70, 200, 25, hWnd, (HMENU)IDC_USERNAME_EDIT, hInst, NULL);
        CreateWindowW(L"STATIC", L"Пароль:", WS_VISIBLE | WS_CHILD, 50, 100, 100, 20, hWnd, NULL, hInst, NULL);
        hPasswordEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL, 50, 120, 200, 25, hWnd, (HMENU)IDC_PASSWORD_EDIT, hInst, NULL);
        hLoginButton = CreateWindowW(L"BUTTON", L"Войти", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 50, 160, 200, 30, hWnd, (HMENU)IDC_LOGIN_BUTTON, hInst, NULL);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        if (wmId == IDC_LOGIN_BUTTON)
        {
            wchar_t username[100] = { 0 };
            wchar_t password[100] = { 0 };

            GetWindowTextW(hUsernameEdit, username, 100);
            GetWindowTextW(hPasswordEdit, password, 100);

            if (wcslen(username) == 0 || wcslen(password) == 0) {
                MessageBoxW(hWnd, L"Введите логин и пароль!", L"Ошибка", MB_ICONWARNING);
                return 0;
            }

            std::wstring passHash = GenerateSHA256(password);
            wchar_t connStr[256] = { 0 };
            GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");

            SQLHENV hEnv = NULL;
            SQLHDBC hDbc = NULL;
            SQLHSTMT hStmt = NULL;
            SQLRETURN retCode;

            SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
            SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
            SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

            retCode = SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

            if (SQL_SUCCEEDED(retCode)) {
                SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                wchar_t query[512];
                // Используем %ls для защиты от несовпадения типов строк
                swprintf_s(query, 512, L"SELECT UserID, RoleID FROM Users WHERE Username = '%ls' AND PasswordHash = '%ls'", username, passHash.c_str());
                retCode = SQLExecDirectW(hStmt, query, SQL_NTS);

                if (SQL_SUCCEEDED(retCode)) {
                    if (SQLFetch(hStmt) == SQL_SUCCESS) {
                        SQLINTEGER userId = 0, roleId = 0;
                        SQLLEN cbUserId = 0, cbRoleId = 0;
                        SQLGetData(hStmt, 1, SQL_C_SLONG, &userId, sizeof(userId), &cbUserId);
                        SQLGetData(hStmt, 2, SQL_C_SLONG, &roleId, sizeof(roleId), &cbRoleId);

                        // Обязательно закрываем курсор чтения
                        SQLCloseCursor(hStmt);

                        // ТЕСТОВАЯ ПРОВЕРКА: вылезет окошко, подтверждающее, что база пустила
                        MessageBoxW(hWnd, L"Доступ разрешен! Загружаю Дашборд...", L"Успех", MB_OK);

                        SQLHSTMT hLogStmt = NULL;
                        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hLogStmt);
                        wchar_t logQuery[512];
                        swprintf_s(logQuery, 512, L"INSERT INTO Sessions (UserID, IPAddress) VALUES (%d, '127.0.0.1')", userId);
                        SQLExecDirectW(hLogStmt, logQuery, SQL_NTS);
                        SQLFreeHandle(SQL_HANDLE_STMT, hLogStmt);
                        
                        HWND hDash = CreateWindowW(szDashboardClass, L"Главный дашборд - Панель управления",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInst, nullptr);

                        if (hDash) {
                            ShowWindow(hDash, SW_SHOW);
                            UpdateWindow(hDash);

                            ShowWindow(hWnd, SW_HIDE);
                        }
                        else {
                            MessageBoxW(NULL, L"Окно дашборда не смогло открыться!", L"Критическая ошибка", MB_ICONERROR);
                            PostQuitMessage(1);
                        }
                    }
                    else {
                        MessageBoxW(hWnd, L"Неверный логин или пароль!", L"Отказ в доступе", MB_ICONWARNING);
                    }
                }
                else {
                    MessageBoxW(hWnd, L"Ошибка выполнения SQL-запроса.", L"Ошибка БД", MB_ICONERROR);
                }
                // Полностью освобождаем память запроса
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                SQLDisconnect(hDbc);
            }
            else {
                MessageBoxW(hWnd, L"Не удалось подключиться к БД. Проверь config.ini", L"Ошибка сети", MB_ICONERROR);
            }
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

            return 0;
        }

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

// Обработчик окна Главного Дашборда
LRESULT CALLBACK DashboardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        TextOutW(hdc, 50, 50, L"Добро пожаловать в систему управления производством!", 52);
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