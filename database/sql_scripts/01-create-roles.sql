-- Создание ролей
CREATE ROLE vendor NOLOGIN;
CREATE ROLE employee NOLOGIN;
CREATE ROLE manager NOLOGIN;

-- Создание пользователей и присвоение им ролей
CREATE USER admin_user WITH PASSWORD 'admin' SUPERUSER;

CREATE USER vendor_user WITH PASSWORD 'vendor';
GRANT vendor TO vendor_user;

CREATE USER employee_user WITH PASSWORD 'employee';
GRANT employee TO employee_user;

CREATE USER manager_user WITH PASSWORD 'manager';
GRANT manager TO manager_user;