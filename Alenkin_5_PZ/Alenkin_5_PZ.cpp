#include "framework.h"
#include "Alenkin_5_PZ.h"
#include <wincrypt.h> 
#include <sql.h>      
#include <sqlext.h>   
#include <string>
#include <commctrl.h> // Для таблиц и продвинутых контролов
#include <thread> // Добавил для асинхронности

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
#define IDC_BTN_OPEN_ANALYTICS 107 // ID кнопки для 4-й формы
#define IDC_BTN_GENERATE_REPORT 108 // ID кнопки внутри 4-й формы
#define IDC_LBL_REPORT_RESULT 109 // ID текста результата
#define IDC_EDIT_FILTER 110
#define IDC_BTN_FILTER 111
#define IDC_BTN_OPEN_WIZARD 112
#define IDC_BTN_WIZ_NEXT 113
#define IDC_BTN_WIZ_CANCEL 114
// НОВЫЕ ID:
#define IDC_EDIT_SUPPLIER_NAME 115
#define IDC_BTN_ADD_SUPPLIER 116
#define IDC_BTN_ADD_PRODUCT 117
#define IDC_BTN_MANAGE_MATERIALS 118
#define IDC_BTN_MANAGE_USERS 119
#define IDC_EDIT_PROD_NAME 120
#define IDC_EDIT_PROD_PRICE 121
#define IDC_BTN_SAVE_PRODUCT 122

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
WCHAR szDashboardClass[] = L"DashboardWindow";
WCHAR szEditorClass[] = L"EditorWindow"; // Класс для Формы 3
WCHAR szAnalyticsClass[] = L"AnalyticsWindow"; // Класс для Формы 4
WCHAR szWizardClass[] = L"WizardWindow"; // Класс для Формы Транзакций
WCHAR szProductClass[] = L"ProductWindow"; // Класс для Формы добавления продукта

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DashboardWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    EditorWndProc(HWND, UINT, WPARAM, LPARAM); // Анонс Формы 3
LRESULT CALLBACK    AnalyticsWndProc(HWND, UINT, WPARAM, LPARAM); // Анонс Формы 4
LRESULT CALLBACK    WizardWndProc(HWND, UINT, WPARAM, LPARAM); // Анонс Транзакционного Мастера
LRESULT CALLBACK    ProductWndProc(HWND, UINT, WPARAM, LPARAM); // Анонс Формы Продукции
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

    WNDCLASSEXW wcexEd = { 0 };
    wcexEd.cbSize = sizeof(WNDCLASSEX);
    wcexEd.style = CS_HREDRAW | CS_VREDRAW;
    wcexEd.lpfnWndProc = EditorWndProc;
    wcexEd.hInstance = hInstance;
    wcexEd.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexEd.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexEd.lpszClassName = szEditorClass;
    RegisterClassExW(&wcexEd);

    // Регистрация Формы 4 (Панель аналитики)
    WNDCLASSEXW wcexAn = { 0 };
    wcexAn.cbSize = sizeof(WNDCLASSEX);
    wcexAn.style = CS_HREDRAW | CS_VREDRAW;
    wcexAn.lpfnWndProc = AnalyticsWndProc;
    wcexAn.hInstance = hInstance;
    wcexAn.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexAn.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexAn.lpszClassName = szAnalyticsClass;
    RegisterClassExW(&wcexAn);

    // Регистрация Мастера Транзакций
    WNDCLASSEXW wcexWiz = { 0 };
    wcexWiz.cbSize = sizeof(WNDCLASSEX);
    wcexWiz.style = CS_HREDRAW | CS_VREDRAW;
    wcexWiz.lpfnWndProc = WizardWndProc;
    wcexWiz.hInstance = hInstance;
    wcexWiz.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexWiz.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexWiz.lpszClassName = szWizardClass;
    RegisterClassExW(&wcexWiz);

    // Регистрация Формы добавления продукции
    WNDCLASSEXW wcexProd = { 0 };
    wcexProd.cbSize = sizeof(WNDCLASSEX);
    wcexProd.style = CS_HREDRAW | CS_VREDRAW;
    wcexProd.lpfnWndProc = ProductWndProc;
    wcexProd.hInstance = hInstance;
    wcexProd.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexProd.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexProd.lpszClassName = szProductClass;
    RegisterClassExW(&wcexProd);

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
                        SQLCloseCursor(hStmt);

                        // Увеличили высоту окна дашборда, чтобы влезли новые кнопки
                        HWND hDash = CreateWindowW(szDashboardClass, L"Главный дашборд", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 800, 700, nullptr, nullptr, hInst, nullptr);
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
    static HWND hListView, hBtnEditor, hBtnAnalytics, hEditFilter;
    switch (message)
    {
    case WM_CREATE:
    {
        // ВИЗУАЛЬНЫЙ КОНСТРУКТОР ФИЛЬТРОВ
        CreateWindowW(L"STATIC", L"Фильтр (Цена >):", WS_VISIBLE | WS_CHILD, 330, 20, 120, 20, hWnd, NULL, hInst, NULL);
        hEditFilter = CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 450, 20, 60, 20, hWnd, (HMENU)IDC_EDIT_FILTER, hInst, NULL);
        CreateWindowW(L"BUTTON", L"Поиск", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 520, 18, 60, 24, hWnd, (HMENU)IDC_BTN_FILTER, hInst, NULL);

        hBtnEditor = CreateWindowW(L"BUTTON", L"Добавить поставщика (Редактор)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 480, 250, 40, hWnd, (HMENU)IDC_BTN_OPEN_EDITOR, hInst, NULL);

        hBtnAnalytics = CreateWindowW(L"BUTTON", L"Панель аналитики", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            300, 480, 250, 40, hWnd, (HMENU)IDC_BTN_OPEN_ANALYTICS, hInst, NULL);

        // КНОПКА ТРАНЗАКЦИОННОГО МАСТЕРА (Уменьшили ширину, чтобы влезла новая кнопка)
        CreateWindowW(L"BUTTON", L"Мастер заказов (Транзакции)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 530, 250, 40, hWnd, (HMENU)IDC_BTN_OPEN_WIZARD, hInst, NULL);

        // НОВЫЕ КНОПКИ
        CreateWindowW(L"BUTTON", L"Добавить продукцию", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            300, 530, 250, 40, hWnd, (HMENU)IDC_BTN_ADD_PRODUCT, hInst, NULL);

        CreateWindowW(L"BUTTON", L"Справочник материалов", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 580, 250, 40, hWnd, (HMENU)IDC_BTN_MANAGE_MATERIALS, hInst, NULL);

        CreateWindowW(L"BUTTON", L"Пользователи", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            300, 580, 250, 40, hWnd, (HMENU)IDC_BTN_MANAGE_USERS, hInst, NULL);

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
            HWND hEditor = CreateWindowW(szEditorClass, L"Карточка поставщика", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 150, 150, 450, 400, nullptr, nullptr, hInst, nullptr);
        }
        else if (wmId == IDC_BTN_OPEN_ANALYTICS) {
            HWND hAnalytics = CreateWindowW(szAnalyticsClass, L"Панель аналитики", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 200, 200, 500, 350, nullptr, nullptr, hInst, nullptr);
        }
        else if (wmId == IDC_BTN_OPEN_WIZARD) {
            HWND hWiz = CreateWindowW(szWizardClass, L"Транзакционный мастер (Оформление)", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 250, 250, 400, 200, nullptr, nullptr, hInst, nullptr);
        }
        else if (wmId == IDC_BTN_ADD_PRODUCT) {
            HWND hProd = CreateWindowW(szProductClass, L"Добавление продукции", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 300, 300, 300, 250, nullptr, nullptr, hInst, nullptr);
        }
        else if (wmId == IDC_BTN_MANAGE_MATERIALS || wmId == IDC_BTN_MANAGE_USERS) {
            MessageBoxW(hWnd, L"Этот раздел в разработке.", L"Инфо", MB_OK);
        }
        else if (wmId == IDC_BTN_FILTER) {
            wchar_t filterVal[50];
            GetWindowTextW(hEditFilter, filterVal, 50);
            if (wcslen(filterVal) == 0) wcscpy_s(filterVal, L"0");

            ListView_DeleteAllItems(hListView);

            wchar_t connStr[256] = { 0 };
            GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
            SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
            SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
            if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
                SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                wchar_t query[512];
                swprintf_s(query, 512, L"SELECT ProductID, ProductName, BasePrice FROM Products WHERE BasePrice > %ls", filterVal);
                if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, query, SQL_NTS))) {
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
    // ДОБАВИЛ ПЕРЕМЕННУЮ ДЛЯ ИМЕНИ ПОСТАВЩИКА
    static HWND hComboCountry, hComboCity, hEditSupName;
    switch (message)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Страна:", WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hWnd, NULL, hInst, NULL);
        hComboCountry = CreateWindowW(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL, 20, 40, 200, 150, hWnd, (HMENU)IDC_COMBO_COUNTRY, hInst, NULL);

        CreateWindowW(L"STATIC", L"Город (автозагрузка):", WS_VISIBLE | WS_CHILD, 20, 80, 200, 20, hWnd, NULL, hInst, NULL);
        hComboCity = CreateWindowW(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VISIBLE | WS_CHILD | WS_VSCROLL, 20, 100, 200, 150, hWnd, (HMENU)IDC_COMBO_CITY, hInst, NULL);

        // НОВЫЕ ПОЛЯ ДЛЯ ДОБАВЛЕНИЯ ПОСТАВЩИКА
        CreateWindowW(L"STATIC", L"Название поставщика:", WS_VISIBLE | WS_CHILD, 20, 140, 200, 20, hWnd, NULL, hInst, NULL);
        hEditSupName = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 160, 200, 20, hWnd, (HMENU)IDC_EDIT_SUPPLIER_NAME, hInst, NULL);
        CreateWindowW(L"BUTTON", L"Сохранить в БД", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 190, 200, 30, hWnd, (HMENU)IDC_BTN_ADD_SUPPLIER, hInst, NULL);

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
                    int index = SendMessageW(hComboCountry, CB_ADDSTRING, 0, (LPARAM)name);
                    SendMessageW(hComboCountry, CB_SETITEMDATA, index, id);
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt); SQLDisconnect(hDbc);
        }
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        break;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_COUNTRY)
        {
            int idx = SendMessageW(hComboCountry, CB_GETCURSEL, 0, 0);
            if (idx != CB_ERR) {
                int countryId = SendMessageW(hComboCountry, CB_GETITEMDATA, idx, 0);
                SendMessageW(hComboCity, CB_RESETCONTENT, 0, 0);

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
        // НОВАЯ ЛОГИКА ДОБАВЛЕНИЯ ПОСТАВЩИКА
        else if (LOWORD(wParam) == IDC_BTN_ADD_SUPPLIER)
        {
            wchar_t supName[100];
            GetWindowTextW(hEditSupName, supName, 100);
            int cityIdx = SendMessageW(hComboCity, CB_GETCURSEL, 0, 0);

            if (cityIdx == CB_ERR || wcslen(supName) == 0) {
                MessageBoxW(hWnd, L"Введите название и выберите город!", L"Внимание", MB_ICONWARNING);
            }
            else {
                int cityId = SendMessageW(hComboCity, CB_GETITEMDATA, cityIdx, 0);
                wchar_t connStr[256] = { 0 };
                GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
                SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
                SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
                if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
                    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                    wchar_t query[512];
                    swprintf_s(query, 512, L"INSERT INTO Suppliers (SupplierName, CityID) VALUES ('%ls', %d)", supName, cityId);
                    if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, query, SQL_NTS))) {
                        MessageBoxW(hWnd, L"Поставщик успешно добавден в БД!", L"Успех", MB_OK);
                        SetWindowTextW(hEditSupName, L"");
                    }
                    else {
                        MessageBoxW(hWnd, L"Ошибка выполнения запроса", L"Ошибка", MB_ICONERROR);
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

// ---------------- ФОРМА 4: АНАЛИТИКА (АСИНХРОННОСТЬ) ----------------
LRESULT CALLBACK AnalyticsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateWindowW(L"BUTTON", L"Рассчитать стоимость номенклатуры", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 50, 50, 300, 40, hWnd, (HMENU)IDC_BTN_GENERATE_REPORT, hInst, NULL);
        CreateWindowW(L"STATIC", L"Статус: Ожидание...", WS_VISIBLE | WS_CHILD, 50, 110, 400, 100, hWnd, (HMENU)IDC_LBL_REPORT_RESULT, hInst, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_GENERATE_REPORT) {
            SetDlgItemTextW(hWnd, IDC_LBL_REPORT_RESULT, L"Статус: Выполняю тяжелый запрос в фоне...");

            std::thread([hWnd]() {
                Sleep(2000);
                wchar_t connStr[256] = { 0 };
                GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
                SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
                SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

                std::wstring result = L"Статус: Ошибка запроса";
                if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
                    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                    if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, (SQLWCHAR*)L"SELECT SUM(BasePrice) FROM Products", SQL_NTS))) {
                        if (SQLFetch(hStmt) == SQL_SUCCESS) {
                            SQLWCHAR sum[100]; SQLLEN cb;
                            SQLGetData(hStmt, 1, SQL_C_WCHAR, sum, sizeof(sum), &cb);
                            result = L"ОТЧЕТ ГОТОВ!\nОбщая цена товаров: " + std::wstring(sum) + L" руб.";
                        }
                    }
                    SQLFreeHandle(SQL_HANDLE_STMT, hStmt); SQLDisconnect(hDbc);
                }
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

                SetDlgItemTextW(hWnd, IDC_LBL_REPORT_RESULT, result.c_str());
                }).detach();
        }
        break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------- ФОРМА 5: ТРАНЗАКЦИОННЫЙ МАСТЕР ----------------
LRESULT CALLBACK WizardWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int step = 1;
    static HWND hLblStep, hBtnNext, hBtnCancel;
    static SQLHDBC hDbc = NULL;
    static SQLHENV hEnv = NULL;

    switch (message)
    {
    case WM_CREATE:
        step = 1;
        hLblStep = CreateWindowW(L"STATIC", L"Шаг 1: Подготовка транзакции...\nНажмите 'Далее', чтобы начать.", WS_VISIBLE | WS_CHILD, 20, 20, 340, 40, hWnd, NULL, hInst, NULL);
        hBtnNext = CreateWindowW(L"BUTTON", L"Далее", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 80, 100, 30, hWnd, (HMENU)IDC_BTN_WIZ_NEXT, hInst, NULL);
        hBtnCancel = CreateWindowW(L"BUTTON", L"Отмена", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 130, 80, 100, 30, hWnd, (HMENU)IDC_BTN_WIZ_CANCEL, hInst, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_WIZ_NEXT) {
            if (step == 1) {
                wchar_t connStr[256] = { 0 };
                GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
                SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
                SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
                if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {

                    SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);

                    SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                    SQLExecDirectW(hStmt, (SQLWCHAR*)L"INSERT INTO ProductionOrders (Status) VALUES ('Черновик')", SQL_NTS);
                    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

                    step = 2;
                    SetWindowTextW(hLblStep, L"Шаг 2: Заказ СОЗДАН в БД, но еще не зафиксирован.\nНажмите 'Далее' для COMMIT, или 'Отмена' для ROLLBACK.");
                }
                else {
                    MessageBoxW(hWnd, L"Ошибка подключения", L"Ошибка", MB_OK);
                }
            }
            else if (step == 2) {
                SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_COMMIT);
                SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
                SQLDisconnect(hDbc); SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
                hDbc = NULL; hEnv = NULL;
                MessageBoxW(hWnd, L"Шаг 3: Успех! Транзакция зафиксирована (COMMIT).", L"Успех", MB_OK);
                DestroyWindow(hWnd);
            }
        }
        else if (LOWORD(wParam) == IDC_BTN_WIZ_CANCEL) {
            if (step == 2 && hDbc != NULL) {
                SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
                SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
                SQLDisconnect(hDbc); SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
                hDbc = NULL; hEnv = NULL;
                MessageBoxW(hWnd, L"Действие отменено! Изменения в БД ОТКАТИЛИСЬ (ROLLBACK).", L"Откат", MB_ICONWARNING);
            }
            DestroyWindow(hWnd);
        }
        break;
    case WM_DESTROY:
        if (hDbc != NULL) {
            SQLEndTran(SQL_HANDLE_DBC, hDbc, SQL_ROLLBACK);
            SQLDisconnect(hDbc); SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
            hDbc = NULL;
        }
        break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------- ФОРМА 6: ДОБАВЛЕНИЕ ГОТОВОЙ ПРОДУКЦИИ ----------------
LRESULT CALLBACK ProductWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hEditProdName, hEditProdPrice;
    switch (message)
    {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Название продукта:", WS_VISIBLE | WS_CHILD, 20, 20, 200, 20, hWnd, NULL, hInst, NULL);
        hEditProdName = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 40, 200, 25, hWnd, (HMENU)IDC_EDIT_PROD_NAME, hInst, NULL);

        CreateWindowW(L"STATIC", L"Базовая цена:", WS_VISIBLE | WS_CHILD, 20, 80, 200, 20, hWnd, NULL, hInst, NULL);
        // Не ставим ES_NUMBER, чтобы можно было вводить десятичные точки
        hEditProdPrice = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 100, 200, 25, hWnd, (HMENU)IDC_EDIT_PROD_PRICE, hInst, NULL);

        CreateWindowW(L"BUTTON", L"Добавить в БД", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 150, 200, 35, hWnd, (HMENU)IDC_BTN_SAVE_PRODUCT, hInst, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_SAVE_PRODUCT) {
            wchar_t prodName[100], prodPrice[50];
            GetWindowTextW(hEditProdName, prodName, 100);
            GetWindowTextW(hEditProdPrice, prodPrice, 50);

            if (wcslen(prodName) == 0 || wcslen(prodPrice) == 0) {
                MessageBoxW(hWnd, L"Заполните все поля!", L"Ошибка", MB_ICONWARNING);
                return 0;
            }

            wchar_t connStr[256] = { 0 };
            GetPrivateProfileStringW(L"Database", L"ConnectionString", L"", connStr, 256, L".\\config.ini");
            SQLHENV hEnv = NULL; SQLHDBC hDbc = NULL; SQLHSTMT hStmt = NULL;
            SQLAllocHandle(SQL_HANDLE_ENV, NULL, &hEnv); SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

            if (SQL_SUCCEEDED(SQLDriverConnectW(hDbc, hWnd, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
                SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
                wchar_t query[512];
                swprintf_s(query, 512, L"INSERT INTO Products (ProductName, BasePrice) VALUES ('%ls', %ls)", prodName, prodPrice);
                if (SQL_SUCCEEDED(SQLExecDirectW(hStmt, query, SQL_NTS))) {
                    MessageBoxW(hWnd, L"Готовая продукция добавлена!\nНа Дашборде нажмите 'Поиск', чтобы обновить таблицу.", L"Успех", MB_OK);
                    SetWindowTextW(hEditProdName, L"");
                    SetWindowTextW(hEditProdPrice, L"");
                }
                else {
                    MessageBoxW(hWnd, L"Ошибка БД при добавлении", L"Ошибка", MB_ICONERROR);
                }
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt); SQLDisconnect(hDbc);
            }
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc); SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        }
        break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam)); return (INT_PTR)TRUE;
        }
        break;
    }

    return (INT_PTR)FALSE;
}