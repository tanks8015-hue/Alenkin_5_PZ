INSERT INTO Countries (CountryName) VALUES ('Россия'), ('Китай'), ('Беларусь');
-- ID России = 1, Китая = 2, Беларуси = 3
INSERT INTO Cities (CountryID, CityName) VALUES (1, 'Москва'), (1, 'Екатеринбург'), (1, 'Казань');
INSERT INTO Cities (CountryID, CityName) VALUES (2, 'Пекин'), (2, 'Шэньчжэнь');
INSERT INTO Cities (CountryID, CityName) VALUES (3, 'Минск'), (3, 'Брест');