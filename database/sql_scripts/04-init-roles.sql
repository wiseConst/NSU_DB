-- Функция для назначения прав по CRUD
-- R = SELECT
-- C = INSERT
-- U = UPDATE
-- D = DELETE

-- Права для таблиц и ролей
-- Таблицы, на которые admin имеет полный доступ через SUPERUSER (лишние GRANT можно опустить)
-- Для остальных ролей:

-- branches (R для Vendor, Employee, Manager)
GRANT SELECT ON TABLE branches TO vendor, employee, manager;

-- outlets (R для Vendor, Employee, Manager)
GRANT SELECT ON TABLE outlets TO vendor, employee, manager;

-- outlet_types (R для Vendor, Employee, Manager)
GRANT SELECT ON TABLE outlet_types TO vendor, employee, manager;

-- photo_stores (R для Vendor, Employee, Manager)
GRANT SELECT ON TABLE photo_stores TO vendor, employee, manager;

-- kiosks (R для Vendor, Employee, Manager)
GRANT SELECT ON TABLE kiosks TO vendor, employee, manager;

-- clients (CR для Employee, CRU для Manager)
GRANT SELECT, INSERT ON TABLE clients TO employee;
GRANT SELECT, INSERT, UPDATE ON TABLE clients TO manager;

-- items (R для Vendor, R для Employee, CRUD для Manager)
GRANT SELECT ON TABLE items TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE items TO manager;

-- service_types_outlets (R для Vendor, Employee, CRUD для Manager)
GRANT SELECT ON TABLE service_types_outlets TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE service_types_outlets TO manager;

-- service_types (R для Vendor, Employee, CRUD для Manager)
GRANT SELECT ON TABLE service_types TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE service_types TO manager;

-- orders (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE orders TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE orders TO manager;

-- firms (CRUD для Vendor, R для Employee и Manager)
GRANT SELECT, UPDATE, DELETE, INSERT ON TABLE firms TO vendor;
GRANT SELECT ON TABLE firms TO employee, manager;

-- films (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE films TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE films TO manager;

-- service_orders (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE service_orders TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE service_orders TO manager;

-- film_development_orders (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE film_development_orders TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE film_development_orders TO manager;

-- vendors (CRUD для Vendor, R для Employee и Manager)
GRANT SELECT, UPDATE, DELETE, INSERT ON TABLE vendors TO vendor;
GRANT SELECT ON TABLE vendors TO employee, manager;

-- vendor_items (CRUD для Vendor, R для Employee и Manager)
GRANT SELECT, UPDATE, DELETE, INSERT ON TABLE vendor_items TO vendor;
GRANT SELECT ON TABLE vendor_items TO employee, manager;

-- print_discounts (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE print_discounts TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE print_discounts TO manager;

-- paper_types (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE paper_types TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE paper_types TO manager;

-- paper_sizes (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE paper_sizes TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE paper_sizes TO manager;

-- print_orders (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE print_orders TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE print_orders TO manager;

-- print_prices (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE print_prices TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE print_prices TO manager;

-- storages (R для Vendor, Employee, CRUD для Manager)
GRANT SELECT ON TABLE storages TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE storages TO manager;

-- storage_items (R для Vendor, Employee, CRUD для Manager)
GRANT SELECT ON TABLE storage_items TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE storage_items TO manager;

-- frames (CR для Employee, CRUD для Manager)
GRANT SELECT, INSERT ON TABLE frames TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE frames TO manager;

-- deliveries (R для Vendor, Employee, CRUD для Manager)
GRANT SELECT ON TABLE deliveries TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE deliveries TO manager;

-- delivery_items (R для Vendor, Employee, CRUD для Manager)
GRANT SELECT ON TABLE delivery_items TO vendor, employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE delivery_items TO manager;

-- service_types_needed_items (R для Employee, CRUD для Manager)
GRANT SELECT ON TABLE service_types_needed_items TO employee;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE service_types_needed_items TO manager;
