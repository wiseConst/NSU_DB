-- 1. outlet_types
COPY outlet_types (id, name)
FROM 'db_data/outlet_types.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 2. outlets
COPY outlets (id, address, num_workers, type_id)
FROM 'db_data/outlets.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 3. branches
-- Note: branches.outlet_id is both PRIMARY KEY and FOREIGN KEY to outlets.id
COPY branches (outlet_id)
FROM 'db_data/branches.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 4. photo_stores
-- Note: photo_stores.outlet_id is PRIMARY KEY and FOREIGN KEY to outlets.id
COPY photo_stores (outlet_id)
FROM 'db_data/photo_stores.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 5. kiosks
COPY kiosks (outlet_id, branch_id)
FROM 'db_data/kiosks.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 6. service_types
COPY service_types (id, name, price)
FROM 'db_data/service_types.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 7. service_types_outlets
COPY service_types_outlets (id, service_type_id, outlet_type_id)
FROM 'db_data/service_types_outlets.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 8. firms
COPY firms (id, name)
FROM 'db_data/firms.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 9. items
-- Note: firm_id is nullable (ON DELETE SET NULL), so empty strings in CSV will be loaded as NULL.
COPY items (id, name, price, firm_id)
FROM 'db_data/items.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 10. clients
COPY clients (id, full_name, is_professional, discount)
FROM 'db_data/clients.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 11. orders
-- For 'accept_time', ensure your CSV has a format like 'YYYY-MM-DD HH:MM:SS'.
-- For 'is_urgent', CSV should contain 'TRUE' or 'FALSE'.
COPY orders (id, accept_time, overall_price, is_urgent, outlet_id, client_id)
FROM 'db_data/orders.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 12. print_discounts
COPY print_discounts (id, photo_amount, discount)
FROM 'db_data/print_discounts.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 13. print_orders
COPY print_orders (id, order_id, print_discount_id)
FROM 'db_data/print_orders.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 14. storages
COPY storages (id, capacity, outlet_id)
FROM 'db_data/storages.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 15. service_orders
COPY service_orders (id, count, order_id, service_type_id)
FROM 'db_data/service_orders.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 16. film_development_orders
COPY film_development_orders (id, service_order_id, code)
FROM 'db_data/film_development_orders.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 17. films
COPY films (id, code, service_order_id)
FROM 'db_data/films.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 18. vendors
COPY vendors (id, name)
FROM 'db_data/vendors.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 19. vendor_items
COPY vendor_items (id, price, quantity, vendor_id, item_id)
FROM 'db_data/vendor_items.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 20. paper_types
COPY paper_types (id, name)
FROM 'db_data/paper_types.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 21. paper_sizes
COPY paper_sizes (id, name)
FROM 'db_data/paper_sizes.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 22. print_prices
COPY print_prices (id, price, paper_size_id, paper_type_id)
FROM 'db_data/print_prices.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 23. frames
COPY frames (id, amount, frame_number, print_order_id, print_price_id)
FROM 'db_data/frames.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 24. deliveries
-- For 'date', ensure your CSV has a format like 'YYYY-MM-DD'.
COPY deliveries (id, date, storage_id, vendor_id)
FROM 'db_data/deliveries.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 25. delivery_items
COPY delivery_items (id, price, delivery_id, item_id)
FROM 'db_data/delivery_items.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 26. service_types_needed_items
COPY service_types_needed_items (item_id, service_type_id, count)
FROM 'db_data/service_types_needed_items.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';

-- 27. storage_items
COPY storage_items (id, quantity, item_id, storage_id)
FROM 'db_data/storage_items.csv'
DELIMITER ','
CSV HEADER
ENCODING 'UTF8';
