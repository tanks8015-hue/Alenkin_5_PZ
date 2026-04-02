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

#pragma comment(lib, "advapi32.lib")
HINSTANCE hInst;    
WCHAR szDashboardClass[] = L"DashboardWindow";
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
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
    // Регистрация класса окна Дашборда
    WNDCLASSEXW wcexDash;
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
std::wstring GenerateSHA256(const std::wstring& input)
{
    // 1. Конвертируем wstring (UTF-16) в string (UTF-8)
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0, NULL, NULL);
    std::string utf8_input(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), &utf8_input[0], size_needed, NULL, NULL);

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::wstring hashStr = L"";

    // 2. Хэшируем
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            // Скармливаем нормальную UTF-8 строку!
            if (CryptHashData(hHash, (const BYTE*)utf8_input.c_str(), utf8_input.length(), 0))
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
LRESULT CALLBACK DashboardWndProc(HWND, UINT, WPARAM, LPARAM);
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

            // 3. Подключение к БД через ODBC и проверка пользователя
            SQLHENV hEnv = NULL;
            SQLHDBC hDbc = NULL;
            SQLHSTMT hStmt = NULL;
            SQLRETURN retCode;

            // Инициализируем окружение ODBC
            SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
            SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
            SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

            // Пытаемся подключиться по строке из config.ini
            retCode = SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

            if (SQL_SUCCEEDED(retCode)) {
                SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

                // Строго прописываем нужные столбцы, никакого SELECT * 
                wchar_t query[512];
                swprintf_s(query, 512, L"SELECT UserID, RoleID FROM Users WHERE Username = '%s' AND PasswordHash = '%s'", username, passHash.c_str());

                // Выполняем запрос
                retCode = SQLExecDirectW(hStmt, query, SQL_NTS);

                if (SQL_SUCCEEDED(retCode)) {
                    // Читаем результат (SQLFetch вернет SQL_SUCCESS, если строка найдена)
                    if (SQLFetch(hStmt) == SQL_SUCCESS) {
                        SQLINTEGER userId = 0, roleId = 0;
                        SQLLEN cbUserId = 0, cbRoleId = 0;
                        SQLGetData(hStmt, 1, SQL_C_SLONG, &userId, sizeof(userId), &cbUserId);
                        SQLGetData(hStmt, 2, SQL_C_SLONG, &roleId, sizeof(roleId), &cbRoleId);

                        SQLHSTMT hLogStmt = NULL;
                        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hLogStmt);
                        wchar_t logQuery[512];
                        swprintf_s(logQuery, 512, L"INSERT INTO Sessions (UserID, IPAddress) VALUES (%d, '127.0.0.1')", userId);
                        SQLExecDirectW(hLogStmt, logQuery, SQL_NTS);
                        SQLFreeHandle(SQL_HANDLE_STMT, hLogStmt);

                        MessageBoxW(hWnd, L"Доступ разрешен. Вход записан в системный журнал.", L"Успех", MB_OK);

                        // 2. Скрываем окно авторизации
                        ShowWindow(hWnd, SW_HIDE);

                        // 3. Создаем и показываем форму Дашборда
                        HWND hDash = CreateWindowW(szDashboardClass, L"Главный дашборд - Панель управления",
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInst, nullptr);

                        // Заготовка на будущее:
                        // 1. Сделать запись в SystemLogs (ID сессии и IP) [cite: 20]
                        // 2. Скрыть это окно и открыть форму "Главный дашборд" [cite: 7]
                    }
                    else {
                        MessageBoxW(hWnd, L"Неверный логин или пароль!", L"Отказ в доступе", MB_ICONWARNING);
                    }
                }
                else {
                    MessageBoxW(hWnd, L"Ошибка выполнения SQL-запроса.", L"Ошибка БД", MB_ICONERROR);
                }

                // Освобождаем память запроса и отключаемся
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                SQLDisconnect(hDbc);
            }
            else {
                MessageBoxW(hWnd, L"Не удалось подключиться к серверу БД. Проверь config.ini", L"Ошибка сети", MB_ICONERROR);
            }

            // Убиваем дескрипторы
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
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
// Обработчик окна Главного Дашборда
LRESULT CALLBACK DashboardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // Выведем текст приветствия на новой форме
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
