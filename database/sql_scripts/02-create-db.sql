CREATE DATABASE photo_center_db
    WITH OWNER = admin_user
    ENCODING = 'UTF8'
    LC_COLLATE = 'en_US.UTF-8'
    LC_CTYPE = 'en_US.UTF-8'
    TABLESPACE = pg_default
    CONNECTION LIMIT = -1;

\connect photo_center_db

ALTER DEFAULT PRIVILEGES FOR ROLE admin_user
    IN SCHEMA public
    GRANT USAGE ON SEQUENCES TO vendor, employee, manager;