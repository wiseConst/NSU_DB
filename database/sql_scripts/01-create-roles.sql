-- Создание ролей
CREATE ROLE vendor NOLOGIN;
CREATE ROLE employee NOLOGIN;
CREATE ROLE manager NOLOGIN;

-- Создание пользователей и присвоение им ролей
CREATE USER admin_user WITH PASSWORD 'admin_password' SUPERUSER;

CREATE USER vendor_user WITH PASSWORD 'vendor_password';

GRANT vendor TO vendor_user;
CREATE USER employee_user WITH PASSWORD 'employee_password';
GRANT employee TO employee_user;

CREATE USER manager_user WITH PASSWORD 'manager_password';
GRANT manager TO manager_user;