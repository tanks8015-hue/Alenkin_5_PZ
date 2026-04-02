#include "framework.h"
#include "Alenkin_5_PZ.h"
#include <wincrypt.h> 
#include <sql.h>      
#include <sqlext.h>   
#include <string>
#include <commctrl.h> // Для таблиц и продвинутых контролов

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// ID элементов
#define IDC_USERNAME_EDIT 101
#define IDC_PASSWORD_EDIT 102
#define IDC_LOGIN_BUTTON 103
#define IDC_BTN_OPEN_EDITOR 104 // Кнопка открытия 3-й формы
#define IDC_COMBO_COUNTRY 105   // Выпадающий список "Страна"
#define IDC_COMBO_CITY 106      // Выпадающий список "Город"

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
WCHAR szDashboardClass[] = L"DashboardWindow";
WCHAR szEditorClass[] = L"EditorWindow"; // Класс для Формы 3

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DashboardWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    EditorWndProc(HWND, UINT, WPARAM, LPARAM); // Анонс Формы 3
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

std::wstring GenerateSHA256(const std::wstring& input)
{
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALENKIN5PZ, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    WNDCLASSEXW wcexDash = { 0 };
    wcexDash.cbSize = sizeof(WNDCLASSEX);
    wcexDash.style = CS_HREDRAW | CS_VREDRAW;
    wcexDash.lpfnWndProc = DashboardWndProc;
    wcexDash.hInstance = hInstance;
    wcexDash.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALENKIN5PZ));
    wcexDash.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexDash.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexDash.lpszClassName = szDashboardClass;
    RegisterClassExW(&wcexDash);

    // Регистрация Формы 3 (Карточка редактирования)
    WNDCLASSEXW wcexEd = { 0 };
    wcexEd.cbSize = sizeof(WNDCLASSEX);
    wcexEd.style = CS_HREDRAW | CS_VREDRAW;
    wcexEd.lpfnWndProc = EditorWndProc;
    wcexEd.hInstance = hInstance;
    wcexEd.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexEd.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexEd.lpszClassName = szEditorClass;
    RegisterClassExW(&wcexEd);

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

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
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALENKIN5PZ));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ALENKIN5PZ);
    wcex.lpszClassName = szWindowClass;
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, L"Авторизация", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 350, 250, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

// ---------------- ФОРМА 1: АВТОРИЗАЦИЯ ----------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hUsernameEdit, hPasswordEdit, hLoginButton;
    switch (message)
    {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Логин:", WS_VISIBLE | WS_CHILD, 50, 20, 100, 20, hWnd, NULL, hInst, NULL);
        hUsernameEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 50, 40, 200, 25, hWnd, (HMENU)IDC_USERNAME_EDIT, hInst, NULL);
        CreateWindowW(L"STATIC", L"Пароль:", WS_VISIBLE | WS_CHILD, 50, 70, 100, 20, hWnd, NULL, hInst, NULL);
        hPasswordEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 50, 90, 200, 25, hWnd, (HMENU)IDC_PASSWORD_EDIT, hInst, NULL);
        hLoginButton = CreateWindowW(L"BUTTON", L"Войти", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 50, 130, 200, 30, hWnd, (HMENU)IDC_LOGIN_BUTTON, hInst, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_LOGIN_BUTTON)
        {
            wchar_t username[100] = { 0 }, password[100] = { 0 };
            GetWindowTextW(hUsernameEdit, username, 100);
            GetWindowTextW(hPasswordEdit, password, 100);

            if (wcslen(username) == 0 || wcslen(password) == 0) return 0;
            std::wstring passHash = GenerateSHA256(password);

            wchar_t connStr[256] = { 0 };
            GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");

            SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
            SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv);
            SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
            SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

            if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
                SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                wchar_t query[512];
                swprintf_s(query, 512, L"SELECT UserID FROM Users WHERE Username = '%ls' AND PasswordHash = '%ls'", username, passHash.c_str());

                if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, query, SQL_NTS))) {
                    if (SQLFetch(hStmt) == SQL_SUCCESS) {
                        SQLCloseCursor(hStmt); // Закрываем курсор!

                        HWND hDash = CreateWindowW(szDashboardClass, L"Главный дашборд", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInst, nullptr);
                        if (hDash) {
                            ShowWindow(hDash, SW_SHOW);
                            UpdateWindow(hDash);
                            ShowWindow(hWnd, SW_HIDE);
                        }
                    }
                    else MessageBoxW(hWnd, L"Отказ!", L"Ошибка", MB_OK);
                }
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                SQLDisconnect(hDbc);
            }
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        }
        break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------- ФОРМА 2: ГЛАВНЫЙ ДАШБОРД ----------------
LRESULT CALLBACK DashboardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hListView, hBtnEditor;
    switch (message)
    {
    case WM_CREATE:
    {
        // Кнопка для вызова Формы 3
        hBtnEditor = CreateWindowW(L"BUTTON", L"Добавить поставщика (Редактор)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 480, 250, 40, hWnd, (HMENU)IDC_BTN_OPEN_EDITOR, hInst, NULL);

        hListView = CreateWindowExW(0, WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_SINGLESEL,
            20, 60, 600, 400, hWnd, NULL, hInst, NULL);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW lvc; lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.iSubItem = 0; lvc.cx = 50; lvc.pszText = (LPWSTR)L"ID"; ListView_InsertColumn(hListView, 0, &lvc);
        lvc.iSubItem = 1; lvc.cx = 300; lvc.pszText = (LPWSTR)L"Название"; ListView_InsertColumn(hListView, 1, &lvc);
        lvc.iSubItem = 2; lvc.cx = 150; lvc.pszText = (LPWSTR)L"Цена"; ListView_InsertColumn(hListView, 2, &lvc);

        wchar_t connStr[256] = { 0 };
        GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
        SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
        SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
            SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
            if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, (SQLWCHAR*)L"SELECT ProductID, ProductName, BasePrice FROM Products", SQL_NTS))) {
                SQLINTEGER prodId; SQLWCHAR prodName[100]; SQLWCHAR priceStr[50]; SQLLEN cbId, cbName, cbPrice;
                int rowCount = 0;
                while (SQLFetch(hStmt) == SQL_SUCCESS) {
                    SQLGetData(hStmt, 1, SQL_C_SLONG, &prodId, sizeof(prodId), &cbId);
                    SQLGetData(hStmt, 2, SQL_C_WCHAR, prodName, sizeof(prodName), &cbName);
                    SQLGetData(hStmt, 3, SQL_C_WCHAR, priceStr, sizeof(priceStr), &cbPrice);

                    LVITEMW lvi = { 0 }; lvi.mask = LVIF_TEXT; lvi.iItem = rowCount; lvi.iSubItem = 0;
                    wchar_t idBuf[20]; swprintf_s(idBuf, 20, L"%d", prodId);
                    lvi.pszText = idBuf;
                    ListView_InsertItem(hListView, &lvi);
                    ListView_SetItemText(hListView, rowCount, 1, prodName);
                    ListView_SetItemText(hListView, rowCount, 2, priceStr);
                    rowCount++;
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt); SQLDisconnect(hDbc);
        }
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == IDC_BTN_OPEN_EDITOR) {

            // Жестко задаем координаты (150, 150) и отвязываем от Дашборда (ставим nullptr вместо hWnd)
            HWND hEditor = CreateWindowW(szEditorClass, L"Карточка поставщика",
                WS_OVERLAPPEDWINDOW,
                150, 150, 450, 400,
                nullptr, nullptr, hInst, nullptr);

            if (hEditor) {
                // Железный пинок системе, чтобы она показала окно
                ShowWindow(hEditor, SW_SHOWNORMAL);
                UpdateWindow(hEditor);
                SetForegroundWindow(hEditor); // Тащим поверх всех окон насильно
            }
            else {
                MessageBoxW(hWnd, L"Ошибка создания окна", L"Ошибка", MB_ICONERROR);
            }
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        TextOutW(hdc, 20, 20, L"Справочник готовой продукции (Данные из SQL):", 45);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------- ФОРМА 3: КАРТОЧКА И СВЯЗАННЫЕ СПИСКИ ----------------
LRESULT CALLBACK EditorWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hComboCountry, hComboCity;

    switch (message)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Страна:", WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hWnd, NULL, hInst, NULL);
        hComboCountry = CreateWindowW(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL, 20, 40, 200, 150, hWnd, (HMENU)IDC_COMBO_COUNTRY, hInst, NULL);

        CreateWindowW(L"STATIC", L"Город (автозагрузка):", WS_VISIBLE | WS_CHILD, 20, 80, 200, 20, hWnd, NULL, hInst, NULL);
        hComboCity = CreateWindowW(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL, 20, 100, 200, 150, hWnd, (HMENU)IDC_COMBO_CITY, hInst, NULL);

        // Загружаем список стран из БД при открытии окна
        wchar_t connStr[256] = { 0 };
        GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
        SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
        SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
            SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
            if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, (SQLWCHAR*)L"SELECT CountryID, CountryName FROM Countries", SQL_NTS))) {
                SQLINTEGER id; SQLWCHAR name[100]; SQLLEN cbId, cbName;
                while (SQLFetch(hStmt) == SQL_SUCCESS) {
                    SQLGetData(hStmt, 1, SQL_C_SLONG, &id, sizeof(id), &cbId);
                    SQLGetData(hStmt, 2, SQL_C_WCHAR, name, sizeof(name), &cbName);

                    // Добавляем текст в комбобокс
                    int index = SendMessageW(hComboCountry, CB_ADDSTRING, 0, (LPARAM)name);
                    // Незаметно привязываем ID страны к этой строке
                    SendMessageW(hComboCountry, CB_SETITEMDATA, index, id);
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt); SQLDisconnect(hDbc);
        }
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        break;
    }
    case WM_COMMAND:
        // ЕСЛИ ПОЛЬЗОВАТЕЛЬ ВЫБРАЛ СТРАНУ
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_COUNTRY)
        {
            // Получаем выбранный индекс
            int idx = SendMessageW(hComboCountry, CB_GETCURSEL, 0, 0);
            if (idx != CB_ERR) {
                // Вытаскиваем спрятанный ID страны
                int countryId = SendMessageW(hComboCountry, CB_GETITEMDATA, idx, 0);

                // Очищаем второй список
                SendMessageW(hComboCity, CB_RESETCONTENT, 0, 0);

                // Подключаемся к БД и ищем города ТОЛЬКО для этой страны (СВЯЗАННЫЙ СПИСОК!)
                wchar_t connStr[256] = { 0 };
                GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
                SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
                SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

                if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
                    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

                    wchar_t query[256];
                    swprintf_s(query, 256, L"SELECT CityID, CityName FROM Cities WHERE CountryID = %d", countryId);

                    if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, query, SQL_NTS))) {
                        SQLINTEGER id; SQLWCHAR name[100]; SQLLEN cbId, cbName;
                        while (SQLFetch(hStmt) == SQL_SUCCESS) {
                            SQLGetData(hStmt, 1, SQL_C_SLONG, &id, sizeof(id), &cbId);
                            SQLGetData(hStmt, 2, SQL_C_WCHAR, name, sizeof(name), &cbName);

                            int cIdx = SendMessageW(hComboCity, CB_ADDSTRING, 0, (LPARAM)name);
                            SendMessageW(hComboCity, CB_SETITEMDATA, cIdx, id);
                        }
                    }
                    SQLFreeHandle(SQL_HANDLE_STMT, hStmt); SQLDisconnect(hDbc);
                }
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
            }
        }
        break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}