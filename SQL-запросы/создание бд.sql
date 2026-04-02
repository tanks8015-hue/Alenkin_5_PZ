-- Создание базы данных
CREATE DATABASE FactoryManagementDB;
GO
USE FactoryManagementDB;
GO

-- ==========================================
-- БЛОК АДМИНИСТРИРОВАНИЯ И БЕЗОПАСНОСТИ
-- ==========================================
CREATE TABLE Roles (
    RoleID INT IDENTITY(1,1) PRIMARY KEY,
    RoleName VARCHAR(50) NOT NULL
);

CREATE TABLE Users (
    UserID INT IDENTITY(1,1) PRIMARY KEY,
    Username VARCHAR(50) NOT NULL UNIQUE,
    PasswordHash VARCHAR(64) NOT NULL, -- Для хранения SHA-256
    RoleID INT NOT NULL FOREIGN KEY REFERENCES Roles(RoleID)
);

CREATE TABLE Sessions (
    SessionID INT IDENTITY(1,1) PRIMARY KEY,
    UserID INT NOT NULL FOREIGN KEY REFERENCES Users(UserID),
    LoginTime DATETIME DEFAULT GETDATE(),
    IPAddress VARCHAR(15) NOT NULL
);

CREATE TABLE SystemLogs (
    LogID INT IDENTITY(1,1) PRIMARY KEY,
    SessionID INT NULL FOREIGN KEY REFERENCES Sessions(SessionID),
    ActionType VARCHAR(50) NOT NULL,
    Description NVARCHAR(255) NOT NULL,
    LogTime DATETIME DEFAULT GETDATE()
);

-- ==========================================
-- БЛОК СПРАВОЧНИКОВ (ДЛЯ СВЯЗАННЫХ СПИСКОВ)
-- ==========================================
CREATE TABLE Countries (
    CountryID INT IDENTITY(1,1) PRIMARY KEY,
    CountryName NVARCHAR(100) NOT NULL
);

CREATE TABLE Cities (
    CityID INT IDENTITY(1,1) PRIMARY KEY,
    CountryID INT NOT NULL FOREIGN KEY REFERENCES Countries(CountryID),
    CityName NVARCHAR(100) NOT NULL
);

CREATE TABLE Suppliers (
    SupplierID INT IDENTITY(1,1) PRIMARY KEY,
    SupplierName NVARCHAR(100) NOT NULL,
    CityID INT NOT NULL FOREIGN KEY REFERENCES Cities(CityID)
);

-- ==========================================
-- БЛОК ПРОИЗВОДСТВА И СКЛАДА
-- ==========================================
CREATE TABLE Materials (
    MaterialID INT IDENTITY(1,1) PRIMARY KEY,
    MaterialName NVARCHAR(100) NOT NULL,
    Price DECIMAL(18,2) NOT NULL -- Цена за единицу сырья
);

CREATE TABLE Products (
    ProductID INT IDENTITY(1,1) PRIMARY KEY,
    ProductName NVARCHAR(100) NOT NULL,
    BasePrice DECIMAL(18,2) NOT NULL -- Базовая цена продукта
);

CREATE TABLE BillOfMaterials (
    BomID INT IDENTITY(1,1) PRIMARY KEY,
    ProductID INT NOT NULL FOREIGN KEY REFERENCES Products(ProductID),
    MaterialID INT NOT NULL FOREIGN KEY REFERENCES Materials(MaterialID),
    Quantity DECIMAL(18,2) NOT NULL -- Сколько сырья нужно на 1 продукт
);

CREATE TABLE Warehouse (
    WarehouseID INT IDENTITY(1,1) PRIMARY KEY,
    MaterialID INT NULL FOREIGN KEY REFERENCES Materials(MaterialID),
    ProductID INT NULL FOREIGN KEY REFERENCES Products(ProductID),
    Quantity DECIMAL(18,2) NOT NULL DEFAULT 0
);

-- ==========================================
-- БЛОК ТРАНЗАКЦИЙ И ЗАКАЗОВ
-- ==========================================
CREATE TABLE ProductionOrders (
    OrderID INT IDENTITY(1,1) PRIMARY KEY,
    OrderDate DATETIME DEFAULT GETDATE(),
    Status NVARCHAR(50) DEFAULT 'Новый',
    TotalCost DECIMAL(18,2) DEFAULT 0 -- Пересчитывается триггером
);

CREATE TABLE OrderItems (
    OrderItemID INT IDENTITY(1,1) PRIMARY KEY,
    OrderID INT NOT NULL FOREIGN KEY REFERENCES ProductionOrders(OrderID),
    ProductID INT NOT NULL FOREIGN KEY REFERENCES Products(ProductID),
    Quantity INT NOT NULL
);
GO

CREATE TRIGGER trg_UpdateOrderTotal
ON OrderItems
AFTER INSERT, UPDATE, DELETE
AS
BEGIN
    SET NOCOUNT ON;

    -- Временная таблица для хранения затронутых ID заказов
    DECLARE @AffectedOrders TABLE (OrderID INT);

    -- Собираем ID из вставленных или удаленных строк
    INSERT INTO @AffectedOrders (OrderID)
    SELECT OrderID FROM inserted
    UNION
    SELECT OrderID FROM deleted;

    -- Обновляем сумму для затронутых заказов (строгий запрет на SELECT * соблюден)
    UPDATE po
    SET TotalCost = ISNULL((
        SELECT SUM(oi.Quantity * p.BasePrice)
        FROM OrderItems oi
        INNER JOIN Products p ON oi.ProductID = p.ProductID
        WHERE oi.OrderID = po.OrderID
    ), 0)
    FROM ProductionOrders po
    INNER JOIN @AffectedOrders ao ON po.OrderID = ao.OrderID;
END;
GO